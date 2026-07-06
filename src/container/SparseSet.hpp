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
        static constexpr std::size_t PAGE_SIZE = 4096;
        static constexpr std::size_t MAX_PAGES = (C + PAGE_SIZE - 1) / PAGE_SIZE;

        constexpr SparseSet() {
            std::fill(sparse_pages, sparse_pages + MAX_PAGES, nullptr);
        }

        constexpr ~SparseSet() {
            Clear();
            for (std::size_t i = 0; i < MAX_PAGES; ++i) {
                if (sparse_pages[i]) {
                    delete[] sparse_pages[i];
                    sparse_pages[i] = nullptr;
                }
            }
        }

        template <typename... Args> requires std::constructible_from<T, Args...>
        constexpr T& Assign(std::size_t id, Args&&... args) {
            assert(id < C && "[SparseSet] ID OUT OF RANGE");
            
            std::size_t page = id / PAGE_SIZE;
            std::size_t offset = id % PAGE_SIZE;
            
            if (!sparse_pages[page]) {
                sparse_pages[page] = new std::size_t[PAGE_SIZE];
                std::fill(sparse_pages[page], sparse_pages[page] + PAGE_SIZE, INVALID);
            }

            if (sparse_pages[page][offset] != INVALID) {
                std::size_t denseIndex = sparse_pages[page][offset];
                std::destroy_at(std::addressof(data[denseIndex].value));
                T* ptr = std::construct_at(std::addressof(data[denseIndex].value), std::forward<Args>(args)...);
                return *ptr;
            }

            std::size_t denseIndex = count;
            sparse_pages[page][offset] = denseIndex;
            ids[denseIndex] = id;
            
            T* ptr = std::construct_at(std::addressof(data[count++].value), std::forward<Args>(args)...);
            return *ptr;
        }

        constexpr void Remove(std::size_t id) {
            if (!Contains(id)) {
                return;
            }

            std::size_t page = id / PAGE_SIZE;
            std::size_t offset = id % PAGE_SIZE;

            std::size_t toRemove = sparse_pages[page][offset];
            std::size_t lastIndex = count - 1;
            std::size_t lastId = ids[lastIndex];

            if (toRemove != lastIndex) {
                data[toRemove].value = std::move(data[lastIndex].value);
                ids[toRemove] = lastId;
                
                std::size_t lastPage = lastId / PAGE_SIZE;
                std::size_t lastOffset = lastId % PAGE_SIZE;
                sparse_pages[lastPage][lastOffset] = toRemove;
            }

            std::destroy_at(std::addressof(data[lastIndex].value));
            sparse_pages[page][offset] = INVALID;
            count--;
        }

        constexpr bool Contains(std::size_t id) const noexcept {
            if (id >= C) {
                return false;
            }

            std::size_t page = id / PAGE_SIZE;
            std::size_t offset = id % PAGE_SIZE;

            if (!sparse_pages[page] || sparse_pages[page][offset] == INVALID) {
                return false;
            }

            std::size_t index = sparse_pages[page][offset];
            return index < count && ids[index] == id;
        }

        constexpr T& Get(std::size_t id) {
            assert(Contains(id) && "[SparseSet] ID NOT FOUND");
            return data[sparse_pages[id / PAGE_SIZE][id % PAGE_SIZE]].value;
        }

        constexpr const T& Get(std::size_t id) const {
            assert(Contains(id) && "[SparseSet] ID NOT FOUND");
            return data[sparse_pages[id / PAGE_SIZE][id % PAGE_SIZE]].value;
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
                sparse_pages[id / PAGE_SIZE][id % PAGE_SIZE] = INVALID;
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
        std::size_t* sparse_pages[MAX_PAGES]{};
        std::size_t count = 0;
    };
}
