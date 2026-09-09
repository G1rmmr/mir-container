#pragma once

#include <cstddef>
#include <cstdint>

namespace zet {
	class MemoryAllocator {
	public:
		virtual ~MemoryAllocator() = default;
		virtual std::byte* GetBase() const = 0;
		virtual std::uint64_t GetEpoch() const noexcept = 0;
	};
}
