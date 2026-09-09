#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace zet {
    template <std::size_t C>
    requires (C > 0)
    class BitSet {
    public:
        static constexpr std::size_t Capacity() noexcept { return C; }

        constexpr void Set(std::size_t index, bool value = true) noexcept {
            if (index >= C) return;
            const auto [word, bit] = Position(index);
            if (value) words[word] |= (std::uint64_t{1} << bit);
            else words[word] &= ~(std::uint64_t{1} << bit);
        }

        constexpr void Reset(std::size_t index) noexcept { Set(index, false); }
        constexpr void Flip(std::size_t index) noexcept {
            if (index >= C) return;
            const auto [word, bit] = Position(index);
            words[word] ^= (std::uint64_t{1} << bit);
        }

        constexpr bool Test(std::size_t index) const noexcept {
            if (index >= C) return false;
            const auto [word, bit] = Position(index);
            return (words[word] & (std::uint64_t{1} << bit)) != 0;
        }

        constexpr void Clear() noexcept { words.fill(0); }
        constexpr bool Any() const noexcept { for (auto word : words) if (word != 0) return true; return false; }
        constexpr bool None() const noexcept { return !Any(); }
        constexpr std::size_t Count() const noexcept {
            std::size_t count = 0;
            for (auto word : words) count += std::popcount(word);
            return count;
        }

        constexpr std::size_t FindFirstSet() const noexcept { return FindNextSet(0); }
        constexpr std::size_t FindNextSet(std::size_t start) const noexcept {
            if (start >= C) return C;
            auto [wordIndex, bitIndex] = Position(start);
            std::uint64_t word = words[wordIndex] & (~std::uint64_t{0} << bitIndex);
            while (true) {
                if (word != 0) {
                    const std::size_t result = wordIndex * BitsPerWord + std::countr_zero(word);
                    return result < C ? result : C;
                }
                if (++wordIndex == WordCount) return C;
                word = words[wordIndex];
            }
        }

    private:
        static constexpr std::size_t BitsPerWord = 64;
        static constexpr std::size_t WordCount = (C + BitsPerWord - 1) / BitsPerWord;
        static constexpr std::pair<std::size_t, std::size_t> Position(std::size_t index) noexcept {
            return { index / BitsPerWord, index % BitsPerWord };
        }
        std::array<std::uint64_t, WordCount> words{};
    };
}
