#pragma once

#include <utility>
#include <cassert>
#include <memory>
#include <concepts>

namespace zet {
    const static std::size_t DEFAULT_POOL_CAPACITY = 1024;
    
    struct PoolHandle {
        std::size_t Index;
        std::size_t Generation;

        constexpr bool operator==(const PoolHandle& other) const noexcept {
            return Index == other.Index && Generation == other.Generation;
        }
        
        constexpr bool operator!=(const PoolHandle& other) const noexcept {
            return !(*this == other);
        }
    };

    static constexpr PoolHandle INVALID_POOL_HANDLE = { static_cast<std::size_t>(-1), 0 };

    template <typename T, std::size_t C = DEFAULT_POOL_CAPACITY> 
    requires (C > 0) && std::destructible<T>
    class Pool {
    public:
        class Iterator {
        public:
            constexpr Iterator(Pool* pool, std::size_t index) : pool(pool), index(index) {
                MoveToNext();
            }

            constexpr T& operator*() const {
                return pool->data[index].value;
            }

            constexpr Iterator& operator++() {
                ++index;
                MoveToNext();
                return *this;
            }

            constexpr bool operator==(const Iterator& other) const {
                return index == other.index;
            }

            constexpr bool operator!=(const Iterator& other) const {
                return !(*this == other);
            }

        private:
            constexpr void MoveToNext() {
                while (index < C && !pool->occupied[index]) {
                    ++index;
                }
            }

            Pool* pool;
            std::size_t index;
        };

        constexpr Pool() : nextFree(0) {
            for (std::size_t i = 0; i < C; ++i) {
                occupied[i] = false;
                generations[i] = 1; // 세대는 1부터 시작
                if (i < C - 1) {
                    data[i].next = i + 1;
                }
            }
            data[C - 1].next = TERMINATOR;
        }

        constexpr ~Pool() {
            Clear();
        }

        Pool(const Pool&) = delete;
        Pool& operator=(const Pool&) = delete;

        constexpr Iterator begin() { return Iterator(this, 0); }
        constexpr Iterator end() { return Iterator(this, C); }

        template <typename... Args> requires std::constructible_from<T, Args...>
        constexpr PoolHandle Create(Args&&... args) {
            assert(nextFree != TERMINATOR && "[zet::Pool] POOL IS FULL");

            std::size_t index = nextFree;
            nextFree = data[index].next;

            std::construct_at(std::addressof(data[index].value), std::forward<Args>(args)...);
            occupied[index] = true;
            
            return PoolHandle{ index, generations[index] };
        }

        constexpr void Destroy(PoolHandle handle) {
            assert(handle.Index < C && "[zet::Pool] INVALID INDEX");

            assert(generations[handle.Index] == handle.Generation && 
                "[zet::Pool] DANGLING HANDLE (ALREADY DESTROYED OR REPLACED)");

            assert(occupied[handle.Index] && "[zet::Pool] DESTROYING NON-EXISTENT OBJECT");

            std::destroy_at(std::addressof(data[handle.Index].value));
            
            occupied[handle.Index] = false;
            generations[handle.Index]++;
            
            data[handle.Index].next = nextFree;
            nextFree = handle.Index;
        }

        constexpr std::size_t GetGeneration(std::size_t index) const noexcept {
            return index < C ? generations[index] : 0;
        }

        constexpr bool IsOccupied(std::size_t index) const noexcept {
            return index < C && occupied[index];
        }

        constexpr T& Get(PoolHandle handle) {
            assert(handle.Index < C && "[zet::Pool] INVALID INDEX");

            assert(generations[handle.Index] == handle.Generation && 
                "[zet::Pool] DANGLING HANDLE DETECTED (USE AFTER FREE)");

            assert(occupied[handle.Index] && "[zet::Pool] ACCESSING NON-EXISTENT OBJECT");
            
            return data[handle.Index].value;
        }

        constexpr const T& Get(PoolHandle handle) const {
            assert(handle.Index < C && "[zet::Pool] INVALID INDEX");

            assert(generations[handle.Index] == handle.Generation && 
                "[zet::Pool] DANGLING HANDLE DETECTED (USE AFTER FREE)");

            assert(occupied[handle.Index] && "[zet::Pool] ACCESSING NON-EXISTENT OBJECT");
            
            return data[handle.Index].value;
        }

        constexpr T& operator[](PoolHandle handle) { 
            return Get(handle); 
        }

        constexpr const T& operator[](PoolHandle handle) const { 
            return Get(handle); 
        }

        constexpr void Clear() {
            for (std::size_t i = 0; i < C; ++i) {
                if (occupied[i]) {
                    std::destroy_at(std::addressof(data[i].value));
                    occupied[i] = false;
                    generations[i]++;
                }
            }
            
            nextFree = 0;
            for (std::size_t i = 0; i < C; ++i) {
                if (i < C - 1) {
                    data[i].next = i + 1;
                }
            }
            data[C - 1].next = TERMINATOR;
        }

    private:
        static constexpr std::size_t TERMINATOR = static_cast<std::size_t>(-1);

        union Node {
            T value;
            std::size_t next;

            constexpr Node() : next(0) {}
            constexpr ~Node() {}
        };

        Node data[C];
        std::size_t generations[C];
        std::size_t nextFree;
        bool occupied[C];
    };
}