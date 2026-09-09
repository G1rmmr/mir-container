#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace zet {
    template<std::size_t C = 8192>
    requires (C > 0)
    class CommandBuffer {
    public:
        struct Header {
            void (*Apply)(const void*) noexcept = nullptr;
            std::size_t PayloadOffset = 0;
            std::size_t NextOffset = 0;
        };

        template<typename T>
        requires std::is_trivially_copyable_v<T> && (alignof(T) <= alignof(std::max_align_t))
        [[nodiscard]] constexpr bool TryPush(void (*apply)(const void*) noexcept, const T& value) noexcept {
            if (!apply) return false;
            const std::size_t record = AlignUp(writeOffset, alignof(std::max_align_t));
            const std::size_t payload = AlignUp(record + sizeof(Header), alignof(T));
            const std::size_t next = payload + sizeof(T);
            if (record > C || payload > C || next > C) return false;
            const Header header{ apply, payload, next };
            std::memcpy(stream + record, &header, sizeof(header));
            std::memcpy(stream + payload, &value, sizeof(T));
            writeOffset = next;
            return true;
        }

        template<typename T>
        requires std::is_trivially_copyable_v<T> && (alignof(T) <= alignof(std::max_align_t))
        [[nodiscard]] constexpr bool Push(void (*apply)(const void*) noexcept, const T& value) noexcept { return TryPush(apply, value); }

        constexpr void Commit() noexcept {
            std::size_t offset = 0;
            while (offset < writeOffset) {
                offset = AlignUp(offset, alignof(std::max_align_t));
                Header header{};
                std::memcpy(&header, stream + offset, sizeof(header));
                header.Apply(stream + header.PayloadOffset);
                offset = header.NextOffset;
            }
            writeOffset = 0;
        }
        constexpr void Clear() noexcept { writeOffset = 0; }
        [[nodiscard]] constexpr std::size_t SizeBytes() const noexcept { return writeOffset; }
        [[nodiscard]] constexpr std::size_t RemainingBytes() const noexcept { return C - writeOffset; }
        [[nodiscard]] constexpr bool Empty() const noexcept { return writeOffset == 0; }
        static constexpr std::size_t CapacityBytes() noexcept { return C; }

    private:
        static constexpr std::size_t AlignUp(std::size_t value, std::size_t alignment) noexcept {
            return (value + alignment - 1) & ~(alignment - 1);
        }
        alignas(std::max_align_t) std::byte stream[C]{};
        std::size_t writeOffset = 0;
    };
}
