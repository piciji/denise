
#define PULSE_WIDTH ((serPer & 0x7fff) + 1)

namespace LIBAMI {

auto Paula::getSerdatR() -> uint16_t {
    uint16_t out = serdatR & 0x3ff;

    if (intreq & 0x800) out |= 0x4000;
    else overrun = false;

    if (overrun) out |= 0x8000;
    if (!serDat) out |= 0x2000;
    if (!serShifter) out |= 0x1000;
    if (rxd) out |= 0x800;

    return out;
}

auto Paula::setSerdat(uint16_t value) -> void {
    serDat = value;

    if (value && !serShifter) {
        prepareTransfer();
        updateTxd();
        serialTransferEvent = agnus.clock + PULSE_WIDTH;
        updateSerialEvent();
    }
}

auto Paula::setSerper(uint16_t value) -> void {
    serPer = value;
}

auto Paula::serialEvent() -> void {
    if (agnus.clock == serialTransferEvent) {
        serShifter >>= 1;

        if (!serShifter) {
            if (serDat) {
                prepareTransfer();
            } else {
                serialTransferEvent = INT64_MAX;
                goto Next;
            }
        }

        updateTxd();

        serialTransferEvent = agnus.clock + PULSE_WIDTH;
    }
Next:
    if (agnus.clock == serialReceiveEvent) {
        receiveShifter <<= 1;
        receiveShifter |= rxd;

        if (++receiveCounter >= ((serPer & 0x8000) ? 11 : 10)) {
            serdatR = receiveShifter;
            receiveShifter = 0;
            receiveCounter = 0;
            overrun = !!(intreq & 0x800);
            scheduleIntreqRbf();

            if (rxd) {
                serialReceiveEvent = INT64_MAX;
                goto Next1;
            }
        }
        serialReceiveEvent = agnus.clock + PULSE_WIDTH;
    }
Next1:
    updateSerialEvent();
}

auto Paula::prepareTransfer() -> void {
    serShifter = serDat;
    serDat = 0;

    serShifter <<= 1; // one start bit is mandatory

    scheduleIntreqTbe();
}

auto Paula::updateTxd() -> void {
    txd = serShifter & 1;
    if (txd && (adkcon & 0x800)) // UARTBRK
        txd = false;

    if (loopBack) {
        if (txd != rxd) {
            rxd = txd;

            if (!rxd && (serialReceiveEvent == INT64_MAX) ) {
                receiveCounter = 0;
                serialReceiveEvent = agnus.clock + (PULSE_WIDTH * 3 / 2);
                updateSerialEvent();
            }
        }
    }
}

auto Paula::updateSerialEvent() -> void {
    int64_t nextClock = serialTransferEvent;
    if (serialReceiveEvent < nextClock)
        nextClock = serialReceiveEvent;

    agnus.updateEventAbs<Agnus::EVENT_SERIAL>(nextClock);
}

}