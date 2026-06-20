
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
    uint8_t* stateBuffer = nullptr;

    Emulator::SystemTimer& sysTimer;

    ResidfpHandler(unsigned nr, System* system, SidManager& sidManager, USBSIDPico& usbSIDPico, Type type);

    ~ResidfpHandler();

    auto reset() -> void override;

    auto setType( Type type ) -> void override;

    auto setDigiBoost( bool state ) -> void override;

    auto readIO( uint8_t addr ) -> uint8_t override;

    auto peekIO( uint8_t addr ) -> uint8_t override;

    auto writeIO( uint8_t addr, uint8_t value ) -> void override;

    auto clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int override;

    auto clock() -> void override;

    auto clockSilent() -> void override;

    auto getSample() -> float override;

    auto serialize(Emulator::Serializer& s, bool light) -> void override;

    auto enableFilter( bool state ) -> void override;

    auto adjustFilterCurve6581(int value) -> void override;

    auto adjustFilterCurve8580(int value) -> void override;

    auto adjustFilterRange6581(int value) -> void override;

    auto setWaveformStrength(uint8_t value) -> void override;

    auto updateSnapshot(DebuggerSnapshot& snap) -> void override;

    auto setSampleRate(double sampleRate) -> void override;

    auto clone(Sid* src, bool keepProps) -> void override;

    auto rememberParams(uint8_t addr, uint8_t value) -> void;
};

}
