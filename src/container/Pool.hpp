#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <exception>
#include <memory>
#include <utility>

namespace zet {
    inline constexpr std::size_t DEFAULT_POOL_CAPACITY = 1024;

    struct PoolHandle {
        const void* Owner = nullptr;
        std::size_t Index = static_cast<std::size_t>(-1);
        std::size_t Generation = 0;
        constexpr bool operator==(const PoolHandle& other) const noexcept { return Owner == other.Owner && Index == other.Index && Generation == other.Generation; }
        constexpr bool operator!=(const PoolHandle& other) const noexcept { return !(*this == other); }
    };
    inline constexpr PoolHandle INVALID_POOL_HANDLE{};

    template <typename T, std::size_t C = DEFAULT_POOL_CAPACITY>
    requires (C > 0) && std::destructible<T>
    class Pool {
    public:
        using Handle = PoolHandle;
        class Iterator {
        public:
            constexpr Iterator(Pool* pool, std::size_t index) : pool(pool), index(index) { Advance(); }
            constexpr T& operator*() const { return pool->data[index].value; }
            constexpr Iterator& operator++() { ++index; Advance(); return *this; }
            constexpr bool operator==(const Iterator& other) const { return pool == other.pool && index == other.index; }
        private:
            constexpr void Advance() { while (index < C && !pool->occupied[index]) ++index; }
            Pool* pool;
            std::size_t index;
        };
        class ConstIterator {
        public:
            constexpr ConstIterator(const Pool* pool, std::size_t index) : pool(pool), index(index) { Advance(); }
            constexpr const T& operator*() const { return pool->data[index].value; }
            constexpr ConstIterator& operator++() { ++index; Advance(); return *this; }
            constexpr bool operator==(const ConstIterator& other) const { return pool == other.pool && index == other.index; }
        private:
            constexpr void Advance() { while (index < C && !pool->occupied[index]) ++index; }
            const Pool* pool;
            std::size_t index;
        };

        constexpr Pool() {
            for (std::size_t i = 0; i < C; ++i) {
                generations[i] = 1;
                data[i].next = i + 1 < C ? i + 1 : TERMINATOR;
            }
        }
        constexpr ~Pool() { Clear(); }
        Pool(const Pool&) = delete;
        Pool& operator=(const Pool&) = delete;

        constexpr Iterator begin() { return Iterator(this, 0); }
        constexpr Iterator end() { return Iterator(this, C); }
        constexpr ConstIterator begin() const { return ConstIterator(this, 0); }
        constexpr ConstIterator end() const { return ConstIterator(this, C); }

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] constexpr Handle TryCreate(Args&&... args) {
            if (nextFree == TERMINATOR) return INVALID_POOL_HANDLE;
            const std::size_t index = nextFree;
            const std::size_t following = data[index].next;
            std::construct_at(std::addressof(data[index].value), std::forward<Args>(args)...);
            nextFree = following;
            occupied[index] = true;
            ++count;
            return { this, index, generations[index] };
        }
        template <typename... Args>
        requires std::constructible_from<T, Args...>
        constexpr Handle Create(Args&&... args) { return TryCreate(std::forward<Args>(args)...); }

        [[nodiscard]] constexpr bool IsValid(Handle handle) const noexcept {
            return handle.Owner == this && handle.Index < C && occupied[handle.Index] && generations[handle.Index] == handle.Generation;
        }
        [[nodiscard]] constexpr T* TryGet(Handle handle) noexcept { return IsValid(handle) ? std::addressof(data[handle.Index].value) : nullptr; }
        [[nodiscard]] constexpr const T* TryGet(Handle handle) const noexcept { return IsValid(handle) ? std::addressof(data[handle.Index].value) : nullptr; }
        [[nodiscard]] constexpr Handle TryHandleAt(std::size_t index) const noexcept {
            return index < C && occupied[index] ? Handle{ this, index, generations[index] } : INVALID_POOL_HANDLE;
        }
        [[nodiscard]] constexpr bool TryDestroy(Handle handle) noexcept {
            if (!IsValid(handle)) return false;
            std::destroy_at(std::addressof(data[handle.Index].value));
            occupied[handle.Index] = false;
            ++generations[handle.Index];
            if (generations[handle.Index] == 0) ++generations[handle.Index];
            data[handle.Index].next = nextFree;
            nextFree = handle.Index;
            --count;
            return true;
        }
        constexpr void Destroy(Handle handle) {
            const bool result = TryDestroy(handle);
            assert(result && "[zet::Pool] INVALID OR STALE HANDLE");
            if (!result) std::terminate();
        }
        constexpr T& Get(Handle handle) { T* result = TryGet(handle); assert(result && "[zet::Pool] INVALID OR STALE HANDLE"); if (!result) std::terminate(); return *result; }
        constexpr const T& Get(Handle handle) const { const T* result = TryGet(handle); assert(result && "[zet::Pool] INVALID OR STALE HANDLE"); if (!result) std::terminate(); return *result; }
        constexpr T& operator[](Handle handle) { return Get(handle); }
        constexpr const T& operator[](Handle handle) const { return Get(handle); }
        constexpr std::size_t GetGeneration(std::size_t index) const noexcept { return index < C ? generations[index] : 0; }
        constexpr bool IsOccupied(std::size_t index) const noexcept { return index < C && occupied[index]; }
        constexpr std::size_t Size() const noexcept { return count; }
        constexpr bool Empty() const noexcept { return count == 0; }
        constexpr bool IsFull() const noexcept { return count == C; }
        static constexpr std::size_t Capacity() noexcept { return C; }
        constexpr void Clear() noexcept {
            for (std::size_t i = 0; i < C; ++i) {
                if (occupied[i]) {
                    std::destroy_at(std::addressof(data[i].value));
                    occupied[i] = false;
                    ++generations[i];
                    if (generations[i] == 0) ++generations[i];
                }
                data[i].next = i + 1 < C ? i + 1 : TERMINATOR;
            }
            nextFree = 0;
            count = 0;
        }

    private:
        static constexpr std::size_t TERMINATOR = static_cast<std::size_t>(-1);
        union Node { T value; std::size_t next; constexpr Node() : next(0) {} constexpr ~Node() {} };
        Node data[C];
        std::size_t generations[C]{};
        bool occupied[C]{};
        std::size_t nextFree = 0;
        std::size_t count = 0;
    };
}
