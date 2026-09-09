#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <atomic>
#include <cstdlib>
#include <new>
#include <malloc.h>

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

void* operator new(std::size_t size, std::align_val_t alignment) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* ptr = _aligned_malloc(size, static_cast<std::size_t>(alignment))) return ptr;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return operator new(size, alignment);
}

void operator delete(void* ptr) noexcept { std::free(ptr); }
void operator delete[](void* ptr) noexcept { std::free(ptr); }
void operator delete(void* ptr, std::size_t) noexcept { std::free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { std::free(ptr); }
void operator delete(void* ptr, std::align_val_t) noexcept { _aligned_free(ptr); }
void operator delete[](void* ptr, std::align_val_t) noexcept { _aligned_free(ptr); }
void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept { _aligned_free(ptr); }
void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept { _aligned_free(ptr); }
