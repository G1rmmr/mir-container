#include "doctest.h"
#include "LinearAllocator.hpp"
#include "StackAllocator.hpp"
#include "PointerHandle.hpp"
#include <array>

struct DestructCounter {
    int* counter;
    DestructCounter(int* c) : counter(c) {}
    ~DestructCounter() {
        if (counter) (*counter)++;
    }
};

TEST_CASE("LinearAllocator and PointerHandle operations") {
	alignas(64) std::array<std::byte, 1024> storage{};
	zet::memory::LinearAllocator allocator(storage);

    SUBCASE("First allocation has offset 0 and is not null") {
        zet::PointerHandle<int> handle = allocator.CreateHandle<int>(42);
        
        int* ptr = handle.Get();
        REQUIRE(ptr != nullptr);
        CHECK(*ptr == 42);
    }

    SUBCASE("Subsequent allocations and values") {
        zet::PointerHandle<int> handle1 = allocator.CreateHandle<int>(10);
        zet::PointerHandle<int> handle2 = allocator.CreateHandle<int>(20);

        REQUIRE(handle1.Get() != nullptr);
        REQUIRE(handle2.Get() != nullptr);
        CHECK(*handle1.Get() == 10);
        CHECK(*handle2.Get() == 20);
        CHECK(handle1.Get() != handle2.Get());
    }

    SUBCASE("Null handle states") {
        zet::PointerHandle<int> nullHandle;
        CHECK(nullHandle.Get() == nullptr);
    }

    SUBCASE("Move semantics for PointerHandle") {
        zet::PointerHandle<int> handle1 = allocator.CreateHandle<int>(99);
        int* originalPtr = handle1.Get();

        zet::PointerHandle<int> handle2 = std::move(handle1);
        CHECK(handle1.Get() == nullptr);
        REQUIRE(handle2.Get() != nullptr);
        CHECK(handle2.Get() == originalPtr);
        CHECK(*handle2.Get() == 99);
    }

    SUBCASE("CreateHandle buffer overflow prevention") {
        // Create an allocator of size 8
		alignas(64) std::array<std::byte, 8> smallStorage{};
		zet::memory::LinearAllocator smallAlloc(smallStorage);
        
        // Creating an object of size 8 fits
        auto h1 = smallAlloc.CreateHandle<double>(1.0);
        CHECK(h1.Get() != nullptr);
        
        // Creating another object of size 8 exceeds the limit (due to alignment/metadata or size)
        auto h2 = smallAlloc.CreateHandle<double>(2.0);
        CHECK(h2.Get() == nullptr);
    }

    SUBCASE("Destructor allocation failure in Create destroys object immediately") {
        int destructCount = 0;
        
        // Allocate space for just one DestructCounter object (8 bytes) + its destructor metadata node (24 bytes)
        // If we size it to 32 bytes:
        // First, DestructCounter is created (8 bytes + alignment).
        // Then, the destructor node (24 bytes) fits.
        // If we limit it to 16 bytes:
        // DestructCounter is created, but the destructor node fails to allocate.
        // In this case, Create should destruct the object immediately and return nullptr to prevent a leak.
        {
			alignas(64) std::array<std::byte, 16> tinyStorage{};
			zet::memory::LinearAllocator tinyAlloc(tinyStorage);
            DestructCounter* ptr = tinyAlloc.Create<DestructCounter, true>(&destructCount);
            CHECK(ptr == nullptr);
            CHECK(destructCount == 1); // Destructed immediately
        }
    }
}

TEST_CASE("StackAllocator operations") {
	alignas(64) std::array<std::byte, 1024> storage{};
	zet::memory::StackAllocator allocator(storage);

    SUBCASE("Basic allocation") {
        zet::PointerHandle<int> handle = allocator.CreateHandle<int>(7);
        REQUIRE(handle.Get() != nullptr);
        CHECK(*handle.Get() == 7);
    }

    SUBCASE("FreeToMarker releases memory correctly") {
        std::byte* marker1 = allocator.GetBase() + 0;
        
        zet::PointerHandle<int> handle1 = allocator.CreateHandle<int>(1);
        
        // Allocate a second item
        zet::PointerHandle<int> handle2 = allocator.CreateHandle<int>(2);
        
        CHECK(*handle1.Get() == 1);
        CHECK(*handle2.Get() == 2);
        
		allocator.FreeToMarker(marker1);
		CHECK(handle1.Get() == nullptr);
		CHECK(handle2.Get() == nullptr);
    }

    SUBCASE("CreateHandle buffer overflow prevention") {
		alignas(64) std::array<std::byte, 8> smallStorage{};
		zet::memory::StackAllocator smallAlloc(smallStorage);
        auto h1 = smallAlloc.CreateHandle<double>(1.0);
        CHECK(h1.Get() != nullptr);
        
        auto h2 = smallAlloc.CreateHandle<double>(2.0);
        CHECK(h2.Get() == nullptr);
    }
}
