#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace zet {
    inline constexpr std::size_t DEFAULT_LIST_CAPACITY = 1024;

    template <typename T, std::size_t C = DEFAULT_LIST_CAPACITY>
    requires (C > 0) && std::destructible<T>
    class List {
    public:
        constexpr List() = default;
        constexpr ~List() { Clear(); }

        constexpr List(const List& other) requires std::copy_constructible<T> {
            for (const auto& value : other) {
                if (!TryPush(value)) std::terminate();
            }
        }

        constexpr List& operator=(const List& other) requires std::copy_constructible<T> {
            if (this == &other) return *this;
            List copy(other);
            Clear();
            for (auto& value : copy) {
                if (!TryPush(std::move(value))) std::terminate();
            }
            return *this;
        }

        constexpr List(List&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires std::move_constructible<T> {
            for (auto& value : other) {
                if (!TryEmplace(std::move(value))) std::terminate();
            }
            other.Clear();
        }

        constexpr List& operator=(List&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires std::move_constructible<T> {
            if (this == &other) return *this;
            Clear();
            for (auto& value : other) {
                if (!TryEmplace(std::move(value))) std::terminate();
            }
            other.Clear();
            return *this;
        }

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] constexpr T* TryEmplace(Args&&... args) {
            if (IsFull()) return nullptr;
            T* result = std::construct_at(std::addressof(data[size].value), std::forward<Args>(args)...);
            ++size;
            return result;
        }

        [[nodiscard]] constexpr bool TryPush(const T& value) requires std::copy_constructible<T> { return TryEmplace(value) != nullptr; }
        [[nodiscard]] constexpr bool TryPush(T&& value) requires std::move_constructible<T> { return TryEmplace(std::move(value)) != nullptr; }
        [[nodiscard]] constexpr bool TryPop() noexcept {
            if (Empty()) return false;
            std::destroy_at(std::addressof(data[--size].value));
            return true;
        }
        [[nodiscard]] constexpr T* TryGet(std::size_t index) noexcept { return index < size ? std::addressof(data[index].value) : nullptr; }
        [[nodiscard]] constexpr const T* TryGet(std::size_t index) const noexcept { return index < size ? std::addressof(data[index].value) : nullptr; }

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        constexpr T& Push(Args&&... args) {
            T* result = TryEmplace(std::forward<Args>(args)...);
            assert(result && "[zet::List] DATA IS FULL");
            if (!result) std::terminate();
            return *result;
        }
        constexpr void Pop() { const bool result = TryPop(); assert(result && "[zet::List] DATA IS EMPTY"); if (!result) std::terminate(); }
        constexpr T& operator[](std::size_t index) { T* result = TryGet(index); assert(result && "[zet::List] INDEX OUT OF BOUNDS"); if (!result) std::terminate(); return *result; }
        constexpr const T& operator[](std::size_t index) const { const T* result = TryGet(index); assert(result && "[zet::List] INDEX OUT OF BOUNDS"); if (!result) std::terminate(); return *result; }

        constexpr void Clear() noexcept { while (TryPop()) {} }
        constexpr T* begin() noexcept { return std::addressof(data[0].value); }
        constexpr const T* begin() const noexcept { return std::addressof(data[0].value); }
        constexpr T* end() noexcept { return std::addressof(data[size].value); }
        constexpr const T* end() const noexcept { return std::addressof(data[size].value); }
        constexpr std::size_t Size() const noexcept { return size; }
        constexpr bool Empty() const noexcept { return size == 0; }
        constexpr bool IsFull() const noexcept { return size == C; }
        static constexpr std::size_t Capacity() noexcept { return C; }

    private:
        union Storage { T value; constexpr Storage() {} constexpr ~Storage() {} };
        Storage data[C];
        std::size_t size = 0;
    };
}
