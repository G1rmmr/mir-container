#pragma once

#include "MemoryAllocator.hpp"
#include "PointerHandle.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

namespace zet::memory {
    class StackAllocator : public MemoryAllocator {
    public:
        struct Marker {
            const StackAllocator* Owner = nullptr;
            std::uint32_t Offset = 0;
            std::uint64_t Epoch = 0;
        };

    private:
        struct Destructor { void (*Func)(void*) noexcept = nullptr; Destructor* Next = nullptr; void* Target = nullptr; };

    public:
        explicit StackAllocator(std::span<std::byte> storage) noexcept
            : start(storage.data()), end(storage.empty() ? storage.data() : storage.data() + storage.size()), offset(storage.data()) {
            if (storage.size() > std::numeric_limits<std::uint32_t>::max()) start = end = offset = nullptr;
        }
        StackAllocator(void* storage, std::size_t size) noexcept : StackAllocator(std::span<std::byte>(static_cast<std::byte*>(storage), size)) {}
        ~StackAllocator() override { Reset(); }
        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;

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

        [[nodiscard]] Marker GetMarker() const noexcept { return IsValid() ? Marker{ this, static_cast<std::uint32_t>(offset - start), epoch } : Marker{}; }
        [[nodiscard]] bool TryFreeToMarker(Marker marker) noexcept {
            if (!IsValid() || marker.Owner != this || marker.Epoch != epoch || marker.Offset > UsedBytes()) return false;
            std::byte* target = start + marker.Offset;
            const auto targetAddress = reinterpret_cast<std::uintptr_t>(target);
            while (lastDestructor && reinterpret_cast<std::uintptr_t>(lastDestructor) >= targetAddress) {
                lastDestructor->Func(lastDestructor->Target);
                lastDestructor = lastDestructor->Next;
            }
            offset = target;
            ++epoch;
            return true;
        }
        void FreeToMarker(Marker marker) { const bool result = TryFreeToMarker(marker); assert(result && "[zet::StackAllocator] INVALID MARKER"); }
        void FreeToMarker(std::byte* marker) {
            if (!IsValid()) return;
            const auto address = reinterpret_cast<std::uintptr_t>(marker);
            const auto first = reinterpret_cast<std::uintptr_t>(start);
            const auto current = reinterpret_cast<std::uintptr_t>(offset);
            const bool valid = address >= first && address <= current;
            assert(valid && "[zet::StackAllocator] INVALID MARKER");
            if (valid) (void)TryFreeToMarker(Marker{ this, static_cast<std::uint32_t>(address - first), epoch });
        }

        [[nodiscard]] std::byte* GetBase() const override { return start; }
        [[nodiscard]] std::uint64_t GetEpoch() const noexcept override { return epoch; }
        [[nodiscard]] bool IsValid() const noexcept { return start != nullptr && end != nullptr && offset != nullptr; }
        [[nodiscard]] std::size_t UsedBytes() const noexcept { return IsValid() ? static_cast<std::size_t>(offset - start) : 0; }
        [[nodiscard]] std::size_t RemainingBytes() const noexcept { return IsValid() ? static_cast<std::size_t>(end - offset) : 0; }
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
