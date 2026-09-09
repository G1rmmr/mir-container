#pragma once

#include "MemoryAllocator.hpp"
#include "PointerHandle.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

namespace zet::memory {
    class LinearAllocator : public MemoryAllocator {
    private:
        struct Destructor { void (*Func)(void*) noexcept = nullptr; Destructor* Next = nullptr; void* Target = nullptr; };

    public:
        explicit LinearAllocator(std::span<std::byte> storage) noexcept
            : start(storage.data()), end(storage.empty() ? storage.data() : storage.data() + storage.size()), offset(storage.data()) {
            if (storage.size() > std::numeric_limits<std::uint32_t>::max()) start = end = offset = nullptr;
        }
        LinearAllocator(void* storage, std::size_t size) noexcept : LinearAllocator(std::span<std::byte>(static_cast<std::byte*>(storage), size)) {}
        ~LinearAllocator() override { Reset(); }
        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;

        [[nodiscard]] void* TryAllocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) noexcept {
            void* result = Reserve(size, alignment, offset);
            if (!result) return nullptr;
            offset = static_cast<std::byte*>(result) + size;
            return result;
        }
        void* Allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) noexcept { return TryAllocate(size, alignment); }

        template <typename T, bool IsInternal = false, typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] T* TryCreate(Args&&... args) {
            if constexpr (!IsInternal) static_assert(std::is_trivially_destructible_v<T>, "Use IsInternal=true for non-trivial types.");
            std::byte* next = offset;
            void* objectMemory = Reserve(sizeof(T), alignof(T), next);
            if (!objectMemory) return nullptr;
            next = static_cast<std::byte*>(objectMemory) + sizeof(T);
            void* nodeMemory = nullptr;
            if constexpr (!std::is_trivially_destructible_v<T>) {
                nodeMemory = Reserve(sizeof(Destructor), alignof(Destructor), next);
                if (!nodeMemory) return nullptr;
                next = static_cast<std::byte*>(nodeMemory) + sizeof(Destructor);
            }
            T* object = ::new (objectMemory) T(std::forward<Args>(args)...);
            if constexpr (!std::is_trivially_destructible_v<T>) {
                auto* node = ::new (nodeMemory) Destructor{ [](void* value) noexcept { static_cast<T*>(value)->~T(); }, lastDestructor, object };
                lastDestructor = node;
            }
            offset = next;
            return object;
        }
        template <typename T, bool IsInternal = false, typename... Args>
        requires std::constructible_from<T, Args...>
        T* Create(Args&&... args) { return TryCreate<T, IsInternal>(std::forward<Args>(args)...); }

        template <typename T, bool IsInternal = false, typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] PointerHandle<T> CreateHandle(Args&&... args) {
            T* object = TryCreate<T, IsInternal>(std::forward<Args>(args)...);
            if (!object) return {};
            return PointerHandle<T>(this, static_cast<std::uint32_t>(reinterpret_cast<std::byte*>(object) - start));
        }

        [[nodiscard]] std::byte* GetBase() const override { return start; }
        [[nodiscard]] std::uint64_t GetEpoch() const noexcept override { return epoch; }
        [[nodiscard]] std::size_t UsedBytes() const noexcept { return IsValid() ? static_cast<std::size_t>(offset - start) : 0; }
        [[nodiscard]] std::size_t RemainingBytes() const noexcept { return IsValid() ? static_cast<std::size_t>(end - offset) : 0; }
        [[nodiscard]] bool IsValid() const noexcept { return start != nullptr && end != nullptr && offset != nullptr; }
        void Reset() noexcept {
            for (Destructor* node = lastDestructor; node != nullptr; node = node->Next) node->Func(node->Target);
            offset = start;
            lastDestructor = nullptr;
            ++epoch;
        }

    private:
        [[nodiscard]] void* Reserve(std::size_t size, std::size_t alignment, std::byte* current) const noexcept {
            if (!IsValid() || alignment == 0 || (alignment & (alignment - 1)) != 0) return nullptr;
            void* result = current;
            std::size_t space = static_cast<std::size_t>(end - current);
            return std::align(alignment, size, result, space) ? result : nullptr;
        }
        Destructor* lastDestructor = nullptr;
        std::uint64_t epoch = 1;
        std::byte* start = nullptr;
        std::byte* end = nullptr;
        std::byte* offset = nullptr;
    };
}
