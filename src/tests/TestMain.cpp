#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <atomic>
#include <cstdlib>
#include <new>

namespace {
    std::atomic_size_t allocationCount{0};
}

std::size_t AllocationCount() noexcept {
    return allocationCount.load(std::memory_order_relaxed);
}

void* operator new(std::size_t size) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* ptr = std::malloc(size)) return ptr;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* ptr = std::malloc(size)) return ptr;
    throw std::bad_alloc();
}

void operator delete(void* ptr) noexcept { std::free(ptr); }
void operator delete[](void* ptr) noexcept { std::free(ptr); }
void operator delete(void* ptr, std::size_t) noexcept { std::free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { std::free(ptr); }
