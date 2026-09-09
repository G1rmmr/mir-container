#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace zet {
    inline constexpr std::size_t DEFAULT_MAP_CAPACITY = 1024;

    template <typename K, typename V, std::size_t C = DEFAULT_MAP_CAPACITY>
    requires (C > 0) && std::destructible<K> && std::destructible<V>
    class Map {
    public:
        constexpr Map() = default;
        struct Pair { const K& Key; V& Value; };
        struct ConstPair { const K& Key; const V& Value; };
        struct InsertResult { V* Value = nullptr; bool Inserted = false; };

        class Iterator {
        public:
            constexpr Iterator(Map* map, std::size_t index) : map(map), index(index) { MoveToNext(); }
            constexpr Pair operator*() const { return { map->keys[index].value, map->values[index].value }; }
            constexpr Iterator& operator++() { ++index; MoveToNext(); return *this; }
            constexpr bool operator==(const Iterator& other) const { return map == other.map && index == other.index; }
        private:
            constexpr void MoveToNext() { while (index < C && !map->occupied[index]) ++index; }
            Map* map;
            std::size_t index;
        };

        class ConstIterator {
        public:
            constexpr ConstIterator(const Map* map, std::size_t index) : map(map), index(index) { MoveToNext(); }
            constexpr ConstPair operator*() const { return { map->keys[index].value, map->values[index].value }; }
            constexpr ConstIterator& operator++() { ++index; MoveToNext(); return *this; }
            constexpr bool operator==(const ConstIterator& other) const { return map == other.map && index == other.index; }
        private:
            constexpr void MoveToNext() { while (index < C && !map->occupied[index]) ++index; }
            const Map* map;
            std::size_t index;
        };

        constexpr ~Map() { Clear(); }
        Map(const Map&) = delete;
        Map& operator=(const Map&) = delete;

        constexpr Iterator begin() { return Iterator(this, 0); }
        constexpr Iterator end() { return Iterator(this, C); }
        constexpr ConstIterator begin() const { return ConstIterator(this, 0); }
        constexpr ConstIterator end() const { return ConstIterator(this, C); }

        template <typename... Args>
        requires std::constructible_from<K, const K&> && std::constructible_from<V, Args...>
        [[nodiscard]] constexpr InsertResult TryEmplace(const K& key, Args&&... args) {
            const std::size_t start = std::hash<K>{}(key) % C;
            for (std::size_t probe = 0; probe < C; ++probe) {
                const std::size_t index = (start + probe) % C;
                if (!occupied[index]) {
                    try {
                        std::construct_at(std::addressof(keys[index].value), key);
                        try {
                            V* value = std::construct_at(std::addressof(values[index].value), std::forward<Args>(args)...);
                            occupied[index] = true;
                            ++count;
                            return { value, true };
                        } catch (...) {
                            std::destroy_at(std::addressof(keys[index].value));
                            throw;
                        }
                    } catch (...) {
                        throw;
                    }
                }
                if (keys[index].value == key) return { std::addressof(values[index].value), false };
            }
            return {};
        }

        template <typename... Args>
        requires std::constructible_from<V, Args...> && std::is_nothrow_swappable_v<V>
        [[nodiscard]] constexpr V* TryInsertOrAssign(const K& key, Args&&... args) {
            InsertResult result = TryEmplace(key, std::forward<Args>(args)...);
            if (result.Value == nullptr || result.Inserted) return result.Value;
            V replacement(std::forward<Args>(args)...);
            using std::swap;
            swap(*result.Value, replacement);
            return result.Value;
        }

        template <typename... Args>
        requires std::constructible_from<K, const K&> && std::constructible_from<V, Args...> && std::is_nothrow_swappable_v<V>
        constexpr V& Insert(const K& key, Args&&... args) {
            V* value = TryInsertOrAssign(key, std::forward<Args>(args)...);
            assert(value && "[zet::Map] HASHMAP IS FULL");
            if (!value) std::terminate();
            return *value;
        }

        [[nodiscard]] constexpr V* Find(const K& key) {
            const std::size_t start = std::hash<K>{}(key) % C;
            for (std::size_t probe = 0; probe < C; ++probe) {
                const std::size_t index = (start + probe) % C;
                if (!occupied[index]) return nullptr;
                if (keys[index].value == key) return std::addressof(values[index].value);
            }
            return nullptr;
        }
        [[nodiscard]] constexpr const V* Find(const K& key) const { return const_cast<Map*>(this)->Find(key); }
        [[nodiscard]] constexpr bool Contains(const K& key) const { return Find(key) != nullptr; }
        constexpr void Clear() noexcept {
            for (std::size_t i = 0; i < C; ++i) {
                if (!occupied[i]) continue;
                std::destroy_at(std::addressof(values[i].value));
                std::destroy_at(std::addressof(keys[i].value));
                occupied[i] = false;
            }
            count = 0;
        }
        constexpr std::size_t Size() const noexcept { return count; }
        constexpr bool Empty() const noexcept { return count == 0; }
        constexpr bool IsFull() const noexcept { return count == C; }
        static constexpr std::size_t Capacity() noexcept { return C; }

    private:
        template <typename U> union Storage { U value; constexpr Storage() {} constexpr ~Storage() {} };
        Storage<K> keys[C];
        Storage<V> values[C];
        bool occupied[C]{};
        std::size_t count = 0;
    };
}
