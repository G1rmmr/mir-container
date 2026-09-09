#pragma once

#include <cstddef>
#include <concepts>
#include <functional>
#include <memory>
#include <utility>
#include <type_traits>

namespace zet {
    template <typename K, typename V, std::size_t C, typename Compare = std::less<K>>
    requires (C > 0) && std::destructible<K> && std::destructible<V>
    class FixedFlatMap {
    public:
        constexpr FixedFlatMap() = default;
        struct Pair { const K& Key; V& Value; };
        constexpr ~FixedFlatMap() { Clear(); }
        FixedFlatMap(const FixedFlatMap&) = delete;
        FixedFlatMap& operator=(const FixedFlatMap&) = delete;
        template <typename... Args>
        requires std::constructible_from<K, const K&> && std::constructible_from<V, Args...> && std::is_nothrow_move_constructible_v<K> && std::is_nothrow_move_constructible_v<V>
        [[nodiscard]] constexpr V* TryInsert(const K& key, Args&&... args) {
            const std::size_t index = LowerBoundIndex(key);
            if (index < size && Equal(keys[index].value, key)) return std::addressof(values[index].value);
            if (size == C) return nullptr;
            for (std::size_t i = size; i > index; --i) {
                std::construct_at(std::addressof(keys[i].value), std::move(keys[i - 1].value));
                std::construct_at(std::addressof(values[i].value), std::move(values[i - 1].value));
                std::destroy_at(std::addressof(keys[i - 1].value));
                std::destroy_at(std::addressof(values[i - 1].value));
            }
            std::construct_at(std::addressof(keys[index].value), key);
            V* result = std::construct_at(std::addressof(values[index].value), std::forward<Args>(args)...);
            ++size;
            return result;
        }
        [[nodiscard]] constexpr V* Find(const K& key) { const auto index = LowerBoundIndex(key); return index < size && Equal(keys[index].value, key) ? std::addressof(values[index].value) : nullptr; }
        [[nodiscard]] constexpr const V* Find(const K& key) const { return const_cast<FixedFlatMap*>(this)->Find(key); }
        [[nodiscard]] constexpr bool TryErase(const K& key) requires std::is_nothrow_move_constructible_v<K> && std::is_nothrow_move_constructible_v<V> {
            const std::size_t index = LowerBoundIndex(key);
            if (index == size || !Equal(keys[index].value, key)) return false;
            std::destroy_at(std::addressof(keys[index].value)); std::destroy_at(std::addressof(values[index].value));
            for (std::size_t i = index; i + 1 < size; ++i) {
                std::construct_at(std::addressof(keys[i].value), std::move(keys[i + 1].value));
                std::construct_at(std::addressof(values[i].value), std::move(values[i + 1].value));
                std::destroy_at(std::addressof(keys[i + 1].value)); std::destroy_at(std::addressof(values[i + 1].value));
            }
            --size; return true;
        }
        constexpr void Clear() noexcept { while (size != 0) { --size; std::destroy_at(std::addressof(keys[size].value)); std::destroy_at(std::addressof(values[size].value)); } }
        constexpr std::size_t Size() const noexcept { return size; }
        constexpr bool Empty() const noexcept { return size == 0; }
        static constexpr std::size_t Capacity() noexcept { return C; }
    private:
        template <typename U> union Storage { U value; constexpr Storage() {} constexpr ~Storage() {} };
        constexpr std::size_t LowerBoundIndex(const K& key) const { std::size_t first = 0, last = size; while (first < last) { const auto mid = first + (last - first) / 2; if (compare(keys[mid].value, key)) first = mid + 1; else last = mid; } return first; }
        constexpr bool Equal(const K& lhs, const K& rhs) const { return !compare(lhs, rhs) && !compare(rhs, lhs); }
        Storage<K> keys[C]; Storage<V> values[C]; std::size_t size = 0; [[no_unique_address]] Compare compare{};
    };
    template <typename K, std::size_t C, typename Compare = std::less<K>>
    class FixedSet {
        struct Unit {};
        FixedFlatMap<K, Unit, C, Compare> values;
    public:
        [[nodiscard]] constexpr bool TryInsert(const K& key) { return values.TryInsert(key) != nullptr; }
        [[nodiscard]] constexpr bool Contains(const K& key) const { return values.Find(key) != nullptr; }
        [[nodiscard]] constexpr bool TryErase(const K& key) { return values.TryErase(key); }
        constexpr std::size_t Size() const noexcept { return values.Size(); }
    };
}
