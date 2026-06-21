#pragma once

// ---------------------------------------------------------------------------
// capture_thread.hpp  —  Dedicated ALSA capture thread
//
// Owns a std::thread that loops on AudioRecorder::captureFrames() and pushes
// each period into a RingBuffer<std::vector<int16_t>>.
//
// Usage
//   CaptureThread ct(recorder, ring, channels);
//   ct.start();
//   ...
//   ct.stop();   // sets stop flag; thread exits after the current period
//   ct.join();
// ---------------------------------------------------------------------------

#include "audio/audio_recorder.hpp"
#include "audio/ring_buffer.hpp"

#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

// CAP slots: 64 × 512-frame periods @48 kHz ≈ 680 ms of headroom —
// plenty of slack to absorb scheduling jitter without dropping audio.
// Must be a power of two (ring_buffer.hpp enforces this with static_assert).
static constexpr std::size_t CAPTURE_RING_CAP = 64;

class CaptureThread {
public:
    // `recorder`  : already initialize()'d AudioRecorder
    // `ring`      : shared ring buffer written by this thread, read by playback
    // `channels`  : channel count (needed to size the per-period chunk)
    CaptureThread(
        AudioRecorder& recorder,
        RingBuffer<std::vector<int16_t>, CAPTURE_RING_CAP>& ring,
        int channels
    )
        : recorder_(recorder),
          ring_(ring),
          channels_(channels),
          stop_(false),
          overruns_(0) {}

    // Not copyable / movable — owns a live thread handle.
    CaptureThread(const CaptureThread&)            = delete;
    CaptureThread& operator=(const CaptureThread&) = delete;

    void start() {
        stop_.store(false, std::memory_order_relaxed);
        thread_ = std::thread(&CaptureThread::run, this);
    }

    // Signal the thread to exit cleanly after its current period completes.
    void stop() {
        stop_.store(true, std::memory_order_relaxed);
    }

    void join() {
        if (thread_.joinable()) thread_.join();
    }

    // How many ring-full drops occurred (diagnostic only).
    int overruns() const { return overruns_.load(std::memory_order_relaxed); }

private:
    void run() {
        // The period size may have been negotiated by ALSA to something other
        // than what was requested.  AudioRecorder::captureFrames() resizes the
        // buffer to the actual period on every call, so start with a reasonable
        // initial size — it will be corrected on the first call.
        std::vector<int16_t> chunk(512 * channels_);

        while (!stop_.load(std::memory_order_relaxed)) {
            int frames_captured = 0;

            if (!recorder_.captureFrames(chunk, frames_captured)) {
                // captureFrames already printed the ALSA error; keep trying.
                continue;
            }

            if (frames_captured <= 0) continue;

            // Trim to what was actually captured (handles short reads).
            chunk.resize(static_cast<std::size_t>(frames_captured) * channels_);

            // Move into the ring; if the ring is full the playback thread is
            // lagging — drop this period and count it.
            if (!ring_.push(chunk)) {
                ++overruns_;
                std::cerr << "[capture] ring full — dropped period ("
                          << overruns_.load() << " total)\n";
            }
        }
    }

    AudioRecorder&                                          recorder_;
    RingBuffer<std::vector<int16_t>, CAPTURE_RING_CAP>&    ring_;
    int                                                     channels_;
    std::atomic<bool>                                       stop_;
    std::atomic<int>                                        overruns_;
    std::thread                                             thread_;
};
