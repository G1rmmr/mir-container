#pragma once

#include <cstdint>

namespace zet {
    enum class Status : std::uint8_t {
        Success,
        Full,
        Empty,
        InvalidIndex,
        InvalidHandle,
        NotFound,
        AlreadyExists,
        CycleDetected,
        Truncated,
        InsufficientScratch
    };
}
