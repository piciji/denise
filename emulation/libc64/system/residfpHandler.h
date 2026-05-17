
#pragma once

#include "sid.h"

namespace LIBC64 {

struct ResidfpHandler : Sid {
    ResidfpHandler(unsigned nr, System* system, SidManager& sidManager, Type type);

    auto reset() -> void;

    auto setType( Type type ) -> void;

    auto setDigiBoost( bool state ) -> void;

    auto hasDigiBoost() -> bool;

    auto readIO( uint8_t addr ) -> uint8_t;

    auto peekIO( uint8_t addr ) -> uint8_t;

    auto writeIO( uint8_t addr, uint8_t value ) -> void;

    auto clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int;

    auto clock(unsigned options) -> void;

    auto serialize(Emulator::Serializer& s, bool light) -> void;

    auto setIoMask(uint8_t pos) -> void;

    auto useLeftChannel(bool state) -> void;

    auto useRightChannel(bool state) -> void;

    auto volumeCorrection(bool state) -> void;

    auto enableFilter( bool state ) -> void;

    auto filterEnabled() -> bool;

    auto adjustFilterBias6581(int value) -> void;

    auto getFilterBias6581() -> int;

    auto adjustFilterBias8580(int value) -> void;

    auto getFilterBias8580() -> int;
};

}