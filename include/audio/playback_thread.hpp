#pragma once

// ---------------------------------------------------------------------------
// playback_thread.hpp  —  Dedicated ALSA playback thread
// ---------------------------------------------------------------------------

#include "audio_player.hpp"
#include "ring_buffer.hpp"

#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

static constexpr std::size_t PLAYBACK_RING_CAP = 64;

struct AudioControl;

class PlaybackThread {
public:
    PlaybackThread(
        AudioPlayer& player,
        RingBuffer<std::vector<int16_t>, PLAYBACK_RING_CAP>& ring,
        int channels,
        AudioControl& control
    )
        : player_(player),
          ring_(ring),
          channels_(channels),
          control_(control),
          stop_(false),
          ring_waits_(0),
          max_consecutive_waits_(0),
          alsa_underruns_(0)
    {
        //
    }

    PlaybackThread(const PlaybackThread&)            = delete;
    PlaybackThread& operator=(const PlaybackThread&) = delete;

    void start() {
        stop_.store(false, std::memory_order_relaxed);
        thread_ = std::thread(&PlaybackThread::run, this);
    }

    void stop()  { stop_.store(true, std::memory_order_relaxed); }
    void join()  { if (thread_.joinable()) thread_.join(); }

    // ── Diagnostics ──────────────────────────────────────────────────────────
    // ring_waits    : times pop() found the ring empty (scheduling jitter, not
    //                 an audio glitch — the ALSA hardware buffer absorbs this)
    // alsa_underruns: times snd_pcm_writei returned EPIPE (actual audible gap)
    int ringWaits()          const { return ring_waits_          .load(std::memory_order_relaxed); }
    int maxConsecutiveWaits() const { return max_consecutive_waits_.load(std::memory_order_relaxed); }
    int alsaUnderruns() const { return alsa_underruns_.load(std::memory_order_relaxed); }



private:
    void run() {
        using namespace std::chrono_literals;

        // ── Pre-fill barrier ─────────────────────────────────────────────────
        // Hold playback until capture has buffered MIN_PREFILL periods.
        // 8 x 512 / 48000 = 85 ms covers capture startup + scheduler jitter.
        constexpr std::size_t MIN_PREFILL = 8; // default 8
        while (!stop_.load(std::memory_order_relaxed) &&
               ring_.size() < MIN_PREFILL) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // ────────────────────────────────────────────────────────────────────

        // Sleep duration when ring is empty: slightly under one ALSA period
        // (512 frames / 48000 Hz = 10.67 ms) so we don't spin between periods
        // but wake up before the next one is due.
        constexpr auto PERIOD_SLEEP = std::chrono::milliseconds(8);
        int consecutive_waits = 0;  // local — only this thread writes it

        while (!stop_.load(std::memory_order_relaxed)) {


            auto maybe = ring_.pop();

            if (!maybe.has_value()) {
                // Ring momentarily empty — normal jitter, not an audible gap.
                // The ALSA hardware buffer keeps playing during this window.
                ++ring_waits_;
                ++consecutive_waits;
                // Update the high-water mark if this run is the longest seen.
                // compare_exchange_weak loop: only updates when we beat the record.
                int prev = max_consecutive_waits_.load(std::memory_order_relaxed);
                while (consecutive_waits > prev &&
                       !max_consecutive_waits_.compare_exchange_weak(
                           prev, consecutive_waits,
                           std::memory_order_relaxed)) {}
                std::this_thread::sleep_for(PERIOD_SLEEP);
                continue;
            }

            consecutive_waits = 0;  // reset streak — capture is keeping up
            std::vector<int16_t> chunk = std::move(*maybe);
            const int chunk_frames = static_cast<int>(chunk.size()) / channels_;

            // Apply gain and mute
            if (control_.muted.load(std::memory_order_relaxed)) {
                std::fill(chunk.begin(), chunk.end(), 0);
            } else {
                int gain = control_.gain_percent.load(std::memory_order_relaxed);
                if (gain != 100) {
                    float factor = gain / 100.0f;
                    for (auto& sample : chunk) {
                        float scaled = sample * factor;
                        if (scaled > 32767.0f) scaled = 32767.0f;
                        else if (scaled < -32768.0f) scaled = -32768.0f;
                        sample = static_cast<int16_t>(scaled);
                    }
                }
            }

            // Calculate peak level (0..100)
            int16_t max_val = 0;
            for (int16_t sample : chunk) {
                int16_t abs_val = std::abs(sample);
                if (abs_val > max_val) {
                    max_val = abs_val;
                }
            }
            int level_percent = static_cast<int>((static_cast<double>(max_val) / 32768.0) * 100.0);

            // Apply VU decay filter (at most 5% drop per chunk for smooth visual falloff)
            if (level_percent < last_level_) {
                level_percent = std::max(level_percent, last_level_ - 5);
            }
            last_level_ = level_percent;

            // Store in atomic levels
            control_.level_left.store(level_percent, std::memory_order_relaxed);
            control_.level_right.store(level_percent, std::memory_order_relaxed);

            // playFrames returns false on EPIPE — that is a real ALSA underrun.
            if (!player_.playFrames(chunk, chunk_frames)) {
                ++alsa_underruns_;
            }

            // frames_played_.fetch_add(chunk_frames, std::memory_order_relaxed);
        }
    }

    AudioPlayer&                                           player_;
    RingBuffer<std::vector<int16_t>, PLAYBACK_RING_CAP>&  ring_;
    int                                                    channels_;
    int                                                    total_frames_;
    AudioControl&                                          control_;

    std::atomic<bool>  stop_;
    std::atomic<int>   ring_waits_;          // total ring-empty sleeps
    std::atomic<int>   max_consecutive_waits_; // longest unbroken empty streak
    std::atomic<int>   alsa_underruns_;   // real EPIPE events  (audible glitches)

    std::thread          thread_;
    int                  last_level_ = 0;
};
