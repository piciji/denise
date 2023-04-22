
#pragma once

#include <ctime>
#include <cstdint>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;

// MSM6242B
struct RTC {

    RTC(Agnus& agnus) : agnus(agnus) {}

    Agnus& agnus;
    uint8_t regs[16];

    time_t lastTime = 0;
    int64_t lastClock = 0;
    time_t diff = 0;

    auto reset(bool softReset) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto write(uint8_t adr, uint8_t val) -> void;
    auto read(uint8_t adr, bool update = true) -> uint8_t;

    auto chipTime() -> time_t;

    auto updateRegs() -> void;
    auto calcDiff() -> void;
};

}
