// NoteRain Native — small lock-free queue for cross-thread events
#pragma once

#include <atomic>

namespace nr {

/** Single-producer / single-consumer ring buffer (no locks, POD payloads). */
template <typename T, unsigned Capacity>
class SpscQueue {
public:
  bool push(const T& value) {
    const unsigned head = head_.load(std::memory_order_relaxed);
    const unsigned next = (head + 1) % Capacity;
    if (next == tail_.load(std::memory_order_acquire)) return false; // full
    buffer_[head] = value;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& out) {
    const unsigned tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) return false; // empty
    out = buffer_[tail];
    tail_.store((tail + 1) % Capacity, std::memory_order_release);
    return true;
  }

private:
  T buffer_[Capacity]{};
  std::atomic<unsigned> head_{0};
  std::atomic<unsigned> tail_{0};
};

} // namespace nr
