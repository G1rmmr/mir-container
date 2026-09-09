#pragma once

#include <utility>
#include <cassert>
#include <memory>
#include <concepts>
#include <algorithm>

#ifndef ZET_NAMESPACE
#define ZET_NAMESPACE zet
#endif

namespace ZET_NAMESPACE {
    const static std::size_t DEFAULT_SPARSE_SET_CAPACITY = 1024;

    template <typename T, std::size_t C = DEFAULT_SPARSE_SET_CAPACITY> 
    requires (C > 0) && std::destructible<T>
    class SparseSet {
    public:
        constexpr SparseSet() {
            std::fill(sparse, sparse + C, INVALID);
        }

        constexpr ~SparseSet() {
            Clear();
        }

        template <typename... Args> requires std::constructible_from<T, Args...>
        constexpr T& Assign(std::size_t id, Args&&... args) {
            assert(id < C && "[SparseSet] ID OUT OF RANGE");
            
            if (sparse[id] != INVALID) {
                std::size_t denseIndex = sparse[id];
                std::destroy_at(std::addressof(data[denseIndex].value));
                T* ptr = std::construct_at(std::addressof(data[denseIndex].value), std::forward<Args>(args)...);
                return *ptr;
            }

            std::size_t denseIndex = count;
            sparse[id] = denseIndex;
            ids[denseIndex] = id;
            
            T* ptr = std::construct_at(std::addressof(data[count++].value), std::forward<Args>(args)...);
            return *ptr;
        }

        constexpr void Remove(std::size_t id) {
            if (!Contains(id)) {
                return;
            }

            std::size_t toRemove = sparse[id];
            std::size_t lastIndex = count - 1;
            std::size_t lastId = ids[lastIndex];

            if (toRemove != lastIndex) {
                data[toRemove].value = std::move(data[lastIndex].value);
                ids[toRemove] = lastId;
                
                sparse[lastId] = toRemove;
            }

            std::destroy_at(std::addressof(data[lastIndex].value));
            sparse[id] = INVALID;
            count--;
        }

        constexpr bool Contains(std::size_t id) const noexcept {
            if (id >= C) {
                return false;
            }

            if (sparse[id] == INVALID) {
                return false;
            }

            std::size_t index = sparse[id];
            return index < count && ids[index] == id;
        }

        constexpr T& Get(std::size_t id) {
            assert(Contains(id) && "[SparseSet] ID NOT FOUND");
            return data[sparse[id]].value;
        }

        constexpr const T& Get(std::size_t id) const {
            assert(Contains(id) && "[SparseSet] ID NOT FOUND");
            return data[sparse[id]].value;
        }

        constexpr std::size_t Size() const noexcept { 
            return count; 
        }
        
        constexpr T& GetAt(std::size_t index) {
            assert(index < count && "[SparseSet] INDEX OUT OF BOUNDS");
            return data[index].value;
        }

        constexpr T* begin() noexcept { 
            return std::addressof(data[0].value); 
        }

        constexpr const T* begin() const noexcept { 
            return std::addressof(data[0].value); 
        }

        constexpr T* end() noexcept { 
            return std::addressof(data[count].value); 
        }

        constexpr const T* end() const noexcept { 
            return std::addressof(data[count].value); 
        }

        constexpr void Clear() {
            for (std::size_t i = 0; i < count; ++i) {
                std::destroy_at(std::addressof(data[i].value));
                std::size_t id = ids[i];
                sparse[id] = INVALID;
            }
            count = 0;
        }

    private:
        static constexpr std::size_t INVALID = static_cast<std::size_t>(-1);

        union Storage {
            T value;
            constexpr Storage() {}
            constexpr ~Storage() {}
        };

        Storage data[C];
        std::size_t ids[C];
        std::size_t sparse[C];
        std::size_t count = 0;
    };
}
