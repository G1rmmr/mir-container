#include "zet.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

int main() {
    constexpr std::size_t iterations = 1'000'000;
    zet::RingBuffer<std::uint32_t, 1024> queue;
    const auto started = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (std::size_t index = 0; index < iterations; ++index) {
        if (!queue.TryPushBack(static_cast<std::uint32_t>(index))) {
            checksum += *queue.TryFront();
            (void)queue.TryPopFront();
            (void)queue.TryPushBack(static_cast<std::uint32_t>(index));
        }
    }
    while (!queue.Empty()) {
        checksum += *queue.TryFront();
        (void)queue.TryPopFront();
    }
    const auto elapsed = std::chrono::steady_clock::now() - started;
    std::cout << "RingBuffer: "
              << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()
              << " us, checksum=" << checksum << '\n';
}
