#pragma once

#include <cstdint>
#include "MemoryAllocator.hpp"

namespace zet {
	template <typename T>
	class PointerHandle {
	public:
		PointerHandle() : allocator(nullptr), offset(0), epoch(0) {}
		PointerHandle(MemoryAllocator* allocator, const std::uint32_t offset)
			: allocator(allocator)
			, offset(offset)
			, epoch(allocator ? allocator->GetEpoch() : 0) {}

		~PointerHandle() {
			Reset();
		}

		PointerHandle(const PointerHandle&) = delete;
		PointerHandle& operator=(const PointerHandle&) = delete;

		PointerHandle(PointerHandle&& other) noexcept
			: allocator(other.allocator)
			, offset(other.offset)
			, epoch(other.epoch) {
			other.allocator = nullptr;
			other.offset = 0;
			other.epoch = 0;
		}

		PointerHandle& operator=(PointerHandle&& other) noexcept {
			if(this != &other) {
				allocator = other.allocator;
				offset = other.offset;
				epoch = other.epoch;
				other.allocator = nullptr;
				other.offset = 0;
				other.epoch = 0;
			}

			return *this;
		}

		T* Get() const {
			if(allocator == nullptr || allocator->GetEpoch() != epoch) {
				return nullptr;
			}

			return reinterpret_cast<T*>(allocator->GetBase() + offset);
		}

		void Reset() {
			allocator = nullptr;
			offset = 0;
			epoch = 0;
		}

	private:
		MemoryAllocator* allocator;
		std::uint32_t offset;
		std::uint64_t epoch;
	};
}
