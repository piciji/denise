
#pragma once

#include "sid.h"
#include "../../tools/systimer.h"

namespace reSIDfp {
    class SID;
}

namespace LIBC64 {

struct USBSIDPico;

struct ResidfpHandler : Sid {
    reSIDfp::SID& residfp;

    short* buffer = nullptr;

    Emulator::SystemTimer& sysTimer;

    ResidfpHandler(unsigned nr, System* system, SidManager& sidManager, USBSIDPico& usbSIDPico, Type type);

    ~ResidfpHandler();

    auto reset() -> void;

    auto setType( Type type ) -> void;

    auto setDigiBoost( bool state ) -> void;

    auto readIO( uint8_t addr ) -> uint8_t;

    auto peekIO( uint8_t addr ) -> uint8_t;

    auto writeIO( uint8_t addr, uint8_t value ) -> void;

    auto clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int;

    auto clock() -> void;

    auto clockSilent() -> void;

    auto getSample() -> float;

    auto serialize(Emulator::Serializer& s, bool light) -> void;

    auto enableFilter( bool state ) -> void;

    auto adjustFilterCurve6581(int value) -> void;

    auto adjustFilterCurve8580(int value) -> void;

    auto adjustFilterRange6581(int value) -> void;

    auto setWaveformStrength(uint8_t value) -> void;

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;

    auto setSampleRate(double sampleRate) -> void;

    auto clone(Sid* src, bool keepProps) -> void;

    auto rememberParams(uint8_t addr, uint8_t value) -> void;
};

}