#pragma once

// ---------------------------------------------------------------------------
// ring_buffer.hpp  —  Lock-free Single-Producer / Single-Consumer ring buffer
//
// Designed for the audio pipeline:
//   Producer  = capture thread  (writes chunks)
//   Consumer  = playback thread (reads chunks)
//
// Template parameter T  : element type  (here: std::vector<int16_t>)
// Template parameter CAP: capacity in slots (must be power-of-two for the
//                         fast modulo trick; a static_assert enforces this)
//
// Guarantees
//   • wait-free on the fast path (no CAS loop, no mutex)
//   • only one thread may call push(), only one may call pop() at a time
//   • std::memory_order_acquire / release pairs ensure the payload write is
//     visible to the consumer before the updated write-index is
// ---------------------------------------------------------------------------

#include <atomic>
#include <array>
#include <optional>
#include <type_traits>
#include <cstddef>

template <typename T, std::size_t CAP>
class RingBuffer {
    static_assert((CAP & (CAP - 1)) == 0, "CAP must be a power of two");

public:
    RingBuffer() : write_idx_(0), read_idx_(0) {}

    // -----------------------------------------------------------------------
    // push  — called by the PRODUCER thread only
    //
    // Copies `item` into the next free slot.
    // Returns false (and drops the item) if the buffer is full.
    // -----------------------------------------------------------------------
    bool push(const T& item) {
        const std::size_t w = write_idx_.load(std::memory_order_relaxed);
        const std::size_t next_w = (w + 1) & MASK;

        // Full: the slot the producer wants to write into is still held by
        // the consumer.
        if (next_w == read_idx_.load(std::memory_order_acquire))
            return false;

        slots_[w] = item;  // write payload before advancing the index

        write_idx_.store(next_w, std::memory_order_release);
        return true;
    }

    // Move-push variant to avoid a redundant copy when the caller is done
    // with the source object.
    bool push(T&& item) {
        const std::size_t w = write_idx_.load(std::memory_order_relaxed);
        const std::size_t next_w = (w + 1) & MASK;

        if (next_w == read_idx_.load(std::memory_order_acquire))
            return false;

        slots_[w] = std::move(item);

        write_idx_.store(next_w, std::memory_order_release);
        return true;
    }

    // -----------------------------------------------------------------------
    // pop  — called by the CONSUMER thread only
    //
    // Moves the oldest item out of the buffer.
    // Returns std::nullopt if the buffer is empty.
    // -----------------------------------------------------------------------
    std::optional<T> pop() {
        const std::size_t r = read_idx_.load(std::memory_order_relaxed);

        // Empty: nothing has been written yet (or everything was consumed).
        if (r == write_idx_.load(std::memory_order_acquire))
            return std::nullopt;

        T item = std::move(slots_[r]);

        read_idx_.store((r + 1) & MASK, std::memory_order_release);
        return item;
    }

    // Convenience helpers (approximate — not sequentially consistent).
    bool empty() const {
        return read_idx_.load(std::memory_order_acquire) ==
               write_idx_.load(std::memory_order_acquire);
    }

    bool full() const {
        const std::size_t w    = write_idx_.load(std::memory_order_acquire);
        const std::size_t next = (w + 1) & MASK;
        return next == read_idx_.load(std::memory_order_acquire);
    }

    std::size_t size() const {
        const std::size_t w = write_idx_.load(std::memory_order_acquire);
        const std::size_t r = read_idx_.load(std::memory_order_acquire);
        return (w - r) & MASK;
    }

private:
    static constexpr std::size_t MASK = CAP - 1;

    // Pad the two atomic indices onto separate cache lines (typically 64 bytes)
    // so the producer and consumer never cause false sharing.
    alignas(64) std::atomic<std::size_t> write_idx_;
    alignas(64) std::atomic<std::size_t> read_idx_;

    // The actual storage.  Using a plain array avoids dynamic allocation and
    // keeps all slots in a contiguous region that the CPU prefetcher likes.
    std::array<T, CAP> slots_;
};
