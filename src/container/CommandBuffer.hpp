#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace zet {
	template<std::size_t C = 8192>
	class CommandBuffer {
	public:
		struct Header {
			void (*Apply)(const void*);
			std::size_t PayloadSize;
		};

		template<typename T> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] constexpr bool Push(void (*apply)(const void*), const T& data) noexcept {
			if constexpr (sizeof(Header) + sizeof(T) > C) {
				return false;
			} else {
				std::size_t alignSize = alignof(std::max_align_t);
				std::size_t alignedOffset = (writeOffset + alignSize - 1) & ~(alignSize - 1);
				std::size_t headerSize = sizeof(Header);
				std::size_t dataSize = sizeof(T);

				if (apply == nullptr || alignedOffset + headerSize + dataSize > C) {
					return false;
				}

				Header header{apply, dataSize};
				std::memcpy(stream + alignedOffset, &header, headerSize);
				std::memcpy(stream + alignedOffset + headerSize, &data, dataSize);

				writeOffset = alignedOffset + headerSize + dataSize;
				return true;
			}
		}

		constexpr std::size_t SizeBytes() const noexcept { return writeOffset; }
		static constexpr std::size_t CapacityBytes() noexcept { return C; }

		constexpr void Commit() noexcept {
			std::size_t readOffset = 0;
			std::size_t alignSize = alignof(std::max_align_t);
			std::size_t headerSize = sizeof(Header);

			while (readOffset < writeOffset) {
				readOffset = (readOffset + alignSize - 1) & ~(alignSize - 1);

				if (readOffset >= writeOffset) {
					break;
				}

				Header header;
				std::memcpy(&header, &stream[readOffset], headerSize);

				header.Apply(stream + readOffset + headerSize);
				readOffset += header.PayloadSize + headerSize;
			}

			writeOffset = 0;
		}

	private:
		alignas(std::max_align_t) std::byte stream[C];
		std::size_t writeOffset = 0;
	};
}
