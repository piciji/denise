
// todo: receive data

#define PULSE_WIDTH ((serPer & 0x7fff) + 1)

namespace LIBAMI {

auto Paula::getSerdatR() -> uint16_t {
    uint16_t out = 0;

    if (intreq & 0x800) out |= 0x4000;
    if (!serDat) out |= 0x2000;
    if (!serShifter) out |= 0x1000;

    return out;
}

auto Paula::setSerdat(uint16_t value) -> void {
    serDat = value;

    if (value && !serShifter) {
        prepareTransfer();
        agnus.updateEvent<Agnus::EVENT_SERIAL>(PULSE_WIDTH);
    }
}

auto Paula::setSerper(uint16_t value) -> void {
    serPer = value;
}

auto Paula::serialEvent() -> void {

    serShifter >>= 1;

    if (!serShifter) {
        if (serDat) {
            prepareTransfer();
        } else {
            return agnus.setEventInactive<Agnus::EVENT_SERIAL>();
        }
    }

    agnus.updateEvent<Agnus::EVENT_SERIAL>(PULSE_WIDTH);
}

auto Paula::prepareTransfer() -> void {
    serShifter = serDat;
    serDat = 0;

    serShifter <<= 1; // one start bit is mandatory

    scheduleIntreqTbe();
}

}