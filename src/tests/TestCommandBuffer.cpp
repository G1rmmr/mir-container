#include "doctest.h"
#include "CommandBuffer.hpp"

namespace {
    struct Payload { int* Target; int Value; };

    void ApplyPayload(const void* raw) noexcept {
        const auto& payload = *static_cast<const Payload*>(raw);
        *payload.Target = payload.Value;
    }
}

TEST_CASE("CommandBuffer rejects invalid commands without allocation") {
    zet::CommandBuffer<64> commands;
    int value = 0;

    CHECK(commands.Push(&ApplyPayload, Payload{&value, 42}));
    CHECK(commands.SizeBytes() > 0);
    commands.Commit();
    CHECK(value == 42);
    CHECK(commands.SizeBytes() == 0);

    struct OversizedPayload { std::byte Bytes[128]; };
    CHECK_FALSE(commands.Push<OversizedPayload>([](const void*) noexcept {}, {}));
    CHECK_FALSE(commands.Push<Payload>(nullptr, {&value, 7}));
}
