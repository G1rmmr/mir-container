#pragma once

#include <cassert>
#include <cstddef>
#include <string_view>

namespace zet {
    inline constexpr std::size_t DEFAULT_STRING_CAPACITY = 1024;

    template <std::size_t C = DEFAULT_STRING_CAPACITY>
    requires (C > 0)
    class String {
    public:
        struct AppendResult { std::size_t Written = 0; bool Truncated = false; };

        constexpr String() noexcept { buffer[0] = '\0'; }
        constexpr String(const char* value) noexcept { AssignTruncated(value ? std::string_view(value) : std::string_view{}); }
        constexpr String(std::string_view value) noexcept { AssignTruncated(value); }
        constexpr String(std::size_t count, char value) noexcept {
            const std::size_t written = count < Capacity() ? count : Capacity();
            for (std::size_t i = 0; i < written; ++i) buffer[i] = value;
            length = written;
            buffer[length] = '\0';
        }

        [[nodiscard]] constexpr bool TryAssign(std::string_view value) noexcept {
            if (value.size() > Capacity()) return false;
            Copy(value, buffer);
            length = value.size();
            buffer[length] = '\0';
            return true;
        }
        constexpr AppendResult AssignTruncated(std::string_view value) noexcept {
            const std::size_t written = value.size() < Capacity() ? value.size() : Capacity();
            Copy(value.substr(0, written), buffer);
            length = written;
            buffer[length] = '\0';
            return { written, written != value.size() };
        }
        [[nodiscard]] constexpr bool TryAppend(std::string_view value) noexcept {
            if (value.size() > Capacity() - length) return false;
            Copy(value, buffer + length);
            length += value.size();
            buffer[length] = '\0';
            return true;
        }
        constexpr AppendResult Append(std::string_view value) noexcept {
            const std::size_t written = value.size() < Capacity() - length ? value.size() : Capacity() - length;
            Copy(value.substr(0, written), buffer + length);
            length += written;
            buffer[length] = '\0';
            return { written, written != value.size() };
        }
        constexpr AppendResult Append(const char* value) noexcept {
            if (!value) return {};
            std::size_t valueLength = 0;
            while (value[valueLength] != '\0') ++valueLength;
            return Append(std::string_view(value, valueLength));
        }

        constexpr String& operator+=(const char* value) noexcept { (void)Append(value); return *this; }
        constexpr String& operator+=(std::string_view value) noexcept { (void)Append(value); return *this; }
        constexpr String operator+(const String& other) const noexcept { String result(*this); result += other.View(); return result; }
        constexpr String operator+(const char* other) const noexcept { String result(*this); result += other; return result; }

        [[nodiscard]] constexpr std::size_t size() const noexcept { return length; }
        [[nodiscard]] static constexpr std::size_t capacity() noexcept { return C - 1; }
        [[nodiscard]] constexpr bool empty() const noexcept { return length == 0; }
        [[nodiscard]] constexpr const char* c_str() const noexcept { return buffer; }
        [[nodiscard]] constexpr char* data() noexcept { return buffer; }
        [[nodiscard]] constexpr const char* data() const noexcept { return buffer; }
        constexpr void RecalculateLength() noexcept { length = 0; while (length < Capacity() && buffer[length] != '\0') ++length; buffer[length] = '\0'; }
        [[nodiscard]] constexpr std::size_t Size() const noexcept { return size(); }
        [[nodiscard]] static constexpr std::size_t Capacity() noexcept { return capacity(); }
        [[nodiscard]] constexpr std::size_t RemainingCapacity() const noexcept { return Capacity() - length; }
        [[nodiscard]] constexpr bool Empty() const noexcept { return empty(); }
        [[nodiscard]] constexpr const char* CStr() const noexcept { return c_str(); }
        [[nodiscard]] constexpr std::string_view View() const noexcept { return { buffer, length }; }
        constexpr char& operator[](std::size_t index) { assert(index < length && "[zet::String] INDEX OUT OF BOUNDS"); return buffer[index]; }
        constexpr const char& operator[](std::size_t index) const { assert(index < length && "[zet::String] INDEX OUT OF BOUNDS"); return buffer[index]; }
        constexpr bool operator==(const String& other) const noexcept { return View() == other.View(); }
        constexpr bool operator==(std::string_view other) const noexcept { return View() == other; }
        constexpr bool operator==(const char* other) const noexcept { return other ? View() == std::string_view(other) : Empty(); }
        constexpr operator std::string_view() const noexcept { return View(); }

    private:
        static constexpr void Copy(std::string_view source, char* destination) noexcept {
            for (std::size_t i = 0; i < source.size(); ++i) destination[i] = source[i];
        }
        char buffer[C]{};
        std::size_t length = 0;
    };
}
