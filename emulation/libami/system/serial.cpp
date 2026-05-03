
#include "serial.h"

#include "system.h"

namespace LIBAMI {

SerialPort::SerialPort(System* system)
: system(system), agnus( system->agnus ) {

}

auto SerialPort::reset() -> void {
    port = 0;
    clock = INT64_MAX;
    control = 0;
}

auto SerialPort::setDTR(bool state) -> void {
    if (plugin == Plugin::DongleBat2) {
        if (state) {
            control = 1;
            clock = agnus.clock;
        }
    }

    setPin( DTR, state );
}

auto SerialPort::setPin(int pin, bool state) -> void {
    auto _old = port;

    int m = _m( pin );

    if (plugin == Plugin::Loopback || plugin == Plugin::LoopbackVA) {
        int _m1 = _m(TXD) | _m(RXD);
        int _m2;
        int _m3;

        if (plugin == Plugin::Loopback) { // AmigaTestKit
            _m2 = _m(RTS) | _m(CTS) | _m(DSR);
            _m3 = _m(CD) | _m(DTR) | _m(RI);
        } else { // VATestDisk
            _m2 = _m(RTS) | _m(CTS);
            _m3 = _m(CD) | _m(DTR) | _m(RI) | _m(DSR);
        }

        if (m & _m1) m |= _m1;
        if (m & _m2) m |= _m2;
        if (m & _m3) m |= _m3;
    }

    if (state)
        port |= m;
    else
        port &= ~m;

    if ((_old & _m(RXD)) && ((port & _m(RXD)) == 0))
        agnus.paula.fallingEdgeRXD();
}

template<bool peek> auto SerialPort::getCTS() -> bool {

    if (plugin == Plugin::DongleBat2) {
        if (!control || (agnus.fallBackCycles(clock) > 0xe2) ) {
            if constexpr (!peek) {
                control = 0;
            }
            return true;
        }

        return false;
    }

    return getPin( CTS );
}

auto SerialPort::getPin(int pin) const -> bool {
    return !!(port & _m(pin));
}

constexpr auto SerialPort::_m(int pin) -> int {
    return 1 << pin;
}

auto SerialPort::serialize(Emulator::Serializer& s) -> void {
    s.integer( port );
    s.integer( clock );
    s.integer( control );
    s.integer( (uint8_t&)plugin );
}

auto SerialPort::setPlugin(Plugin plugin) -> void {
    this->plugin = plugin;
}

template auto SerialPort::getCTS<false>() -> bool;
template auto SerialPort::getCTS<true>() -> bool;

}
