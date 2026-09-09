#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace zet {
    inline constexpr std::size_t DEFAULT_SPARSE_SET_CAPACITY = 1024;

    template <typename T, std::size_t C = DEFAULT_SPARSE_SET_CAPACITY>
    requires (C > 0) && std::destructible<T>
    class SparseSet {
    public:
        constexpr SparseSet() { ClearSparse(); }
        constexpr ~SparseSet() { Clear(); }
        SparseSet(const SparseSet&) = delete;
        SparseSet& operator=(const SparseSet&) = delete;

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] constexpr T* TryAssign(std::size_t id, Args&&... args) {
            if (id >= C) return nullptr;
            if (Contains(id)) {
                if constexpr (!std::is_nothrow_swappable_v<T>) {
                    return nullptr;
                } else {
                    T replacement(std::forward<Args>(args)...);
                    using std::swap;
                    swap(data[sparse[id]].value, replacement);
                    return std::addressof(data[sparse[id]].value);
                }
            }
            T* result = std::construct_at(std::addressof(data[count].value), std::forward<Args>(args)...);
            ids[count] = id;
            sparse[id] = count;
            ++count;
            return result;
        }
        template <typename... Args>
        requires std::constructible_from<T, Args...>
        constexpr T& Assign(std::size_t id, Args&&... args) {
            T* result = TryAssign(id, std::forward<Args>(args)...);
            assert(result && "[zet::SparseSet] INVALID ID OR UNSUPPORTED REPLACEMENT");
            if (!result) std::terminate();
            return *result;
        }
        [[nodiscard]] constexpr bool TryRemove(std::size_t id) requires std::is_nothrow_swappable_v<T> {
            if (!Contains(id)) return false;
            const std::size_t removed = sparse[id];
            const std::size_t last = count - 1;
            if (removed != last) {
                using std::swap;
                swap(data[removed].value, data[last].value);
                ids[removed] = ids[last];
                sparse[ids[removed]] = removed;
            }
            std::destroy_at(std::addressof(data[last].value));
            sparse[id] = INVALID;
            --count;
            return true;
        }
        constexpr void Remove(std::size_t id) requires std::is_nothrow_swappable_v<T> { (void)TryRemove(id); }
        [[nodiscard]] constexpr bool Contains(std::size_t id) const noexcept { return id < C && sparse[id] != INVALID && sparse[id] < count && ids[sparse[id]] == id; }
        [[nodiscard]] constexpr T* TryGet(std::size_t id) noexcept { return Contains(id) ? std::addressof(data[sparse[id]].value) : nullptr; }
        [[nodiscard]] constexpr const T* TryGet(std::size_t id) const noexcept { return Contains(id) ? std::addressof(data[sparse[id]].value) : nullptr; }
        constexpr T& Get(std::size_t id) { T* result = TryGet(id); assert(result && "[zet::SparseSet] ID NOT FOUND"); if (!result) std::terminate(); return *result; }
        constexpr const T& Get(std::size_t id) const { const T* result = TryGet(id); assert(result && "[zet::SparseSet] ID NOT FOUND"); if (!result) std::terminate(); return *result; }
        constexpr T& GetAt(std::size_t index) { T* result = index < count ? std::addressof(data[index].value) : nullptr; assert(result && "[zet::SparseSet] INDEX OUT OF BOUNDS"); if (!result) std::terminate(); return *result; }
        constexpr const T& GetAt(std::size_t index) const { const T* result = index < count ? std::addressof(data[index].value) : nullptr; assert(result && "[zet::SparseSet] INDEX OUT OF BOUNDS"); if (!result) std::terminate(); return *result; }
        constexpr T* begin() noexcept { return std::addressof(data[0].value); }
        constexpr const T* begin() const noexcept { return std::addressof(data[0].value); }
        constexpr T* end() noexcept { return std::addressof(data[count].value); }
        constexpr const T* end() const noexcept { return std::addressof(data[count].value); }
        constexpr std::size_t Size() const noexcept { return count; }
        constexpr bool Empty() const noexcept { return count == 0; }
        static constexpr std::size_t Capacity() noexcept { return C; }
        constexpr void Clear() noexcept {
            while (count != 0) {
                const std::size_t index = --count;
                sparse[ids[index]] = INVALID;
                std::destroy_at(std::addressof(data[index].value));
            }
        }

    private:
        static constexpr std::size_t INVALID = static_cast<std::size_t>(-1);
        union Storage { T value; constexpr Storage() {} constexpr ~Storage() {} };
        constexpr void ClearSparse() noexcept { for (std::size_t& value : sparse) value = INVALID; }
        Storage data[C];
        std::size_t ids[C]{};
        std::size_t sparse[C]{};
        std::size_t count = 0;
    };
}
