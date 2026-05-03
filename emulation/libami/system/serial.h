
#pragma once

#include <cstdint>
#include "../../tools/serializer.h"

namespace LIBAMI {

struct System;
struct Agnus;

struct SerialPort {
    explicit SerialPort(System* system);

    enum class Plugin { None, Loopback, LoopbackVA, DongleBat2 } plugin = Plugin::None;

    static constexpr int TXD = 2;
    static constexpr int RXD = 3;
    static constexpr int RTS = 4;
    static constexpr int CTS = 5;
    static constexpr int DSR = 6;
    static constexpr int CD = 8;
    static constexpr int DTR = 20;
    static constexpr int RI = 22;

    System* system;
    Agnus& agnus;
    uint32_t port = 0;
    int64_t clock = INT64_MAX;
    uint8_t control = 0;

    auto setRTS(bool state) -> void { setPin( RTS, state ); }
    auto setDTR(bool state) -> void;
    auto setTXD(bool state) -> void { setPin( TXD, state ); }
    auto setPin(int pin, bool state) -> void;

    auto getRXD() const -> bool { return getPin(RXD); }
    auto getRI() const -> bool { return getPin(RI); }
    auto getDSR() const -> bool { return getPin(DSR); }
    template<bool peek = false> auto getCTS() -> bool;
    auto getCD() const -> bool { return getPin(CD); }
    auto getRTS() const -> bool { return getPin(RTS); }
    auto getDTR() const -> bool { return getPin(DTR); }
    auto getPin(int pin) const -> bool;

    auto setPlugin(Plugin plugin) -> void;

    auto reset() -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    static constexpr auto _m(int pin) -> int;
};

}
