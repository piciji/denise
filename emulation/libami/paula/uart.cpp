
#define PULSE_WIDTH ((serPer & 0x7fff) + 1)
#define MAX_EXPORT_SIZE (1024 * 10)
#include "../../tools/error.h"

namespace LIBAMI {

auto Paula::peekSerdatR() -> uint16_t {
    uint16_t out = serdatR & 0x3ff;

    if (intreq & 0x800) {
        out |= 0x4000;
        if (overrun)
            out |= 0x8000;
    }

    if (!serDat) out |= 0x2000;
    if (!serShifter) out |= 0x1000;
    if (serial.getRXD()) out |= 0x800;

    return out;
}

auto Paula::getSerdatR() -> uint16_t {
    uint16_t out = serdatR & 0x3ff;

    if (intreq & 0x800) out |= 0x4000;
    else overrun = false;

    if (overrun) out |= 0x8000;
    if (!serDat) out |= 0x2000;
    if (!serShifter) out |= 0x1000;
    if (serial.getRXD()) out |= 0x800;

    // inform("r serdat %x", out);
    return out;
}

auto Paula::setSerdat(uint16_t value) -> void {
    serDat = value;
    // inform("serdat %x", serDat);

    if (value && !serShifter) {
        prepareTransfer();
        updateTxd();
        serialTransferEvent = agnus.clock + PULSE_WIDTH;
        updateSerialEvent();
    }
}

auto Paula::setSerper(uint16_t value) -> void {
    // inform("SERPER %x", value);
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
        bool rxd = serial.getRXD();
        receiveShifter |= rxd;
        // inform("RXD %i", rxd);
        unsigned bits = (serPer & 0x8000) ? 11 : 10;

        if (++receiveCounter >= bits) {
            serdatR = 0;
            bits--;
            do {
                serdatR <<= 1;
                serdatR |= (receiveShifter & 1);
                receiveShifter >>= 1;
            } while (--bits);

            receiveShifter = 0;
            receiveCounter = 0;

            if (system->debuggerSnapshot.themes & (unsigned)DebuggerTheme::Serial)
                addIncomingByte(uint8_t(serdatR));

            overrun = !!(intreq & 0x800);
            scheduleIntreqRbf();

            serialReceiveEvent = INT64_MAX;
            goto Final;
        }
        serialReceiveEvent = agnus.clock + PULSE_WIDTH;
    }
Final:
    updateSerialEvent();
}

auto Paula::prepareTransfer() -> void {
    if (system->debuggerSnapshot.themes & (unsigned)DebuggerTheme::Serial)
        addOutgoingByte(uint8_t(serDat));

    serShifter = serDat;
    serDat = 0;

    serShifter <<= 1; // one start bit is mandatory

    scheduleIntreqTbe();
}

auto Paula::updateTxd() -> void {
    txd = serShifter & 1;
    bool uartBrk = (adkcon & 0x800) != 0;
    // inform("TXD %i", txd && !uartBrk);
    serial.setTXD(txd && !uartBrk);
}

auto Paula::fallingEdgeRXD() -> void {
    if (serialReceiveEvent == INT64_MAX) {
        receiveCounter = 0;
        serialReceiveEvent = agnus.clock + (PULSE_WIDTH / 2); // wait half bit time
        updateSerialEvent();
    }
}

auto Paula::updateSerialEvent() -> void {
    int64_t nextClock = serialTransferEvent;
    if (serialReceiveEvent < nextClock)
        nextClock = serialReceiveEvent;

    agnus.updateEventAbs<Agnus::EVENT_SERIAL>(nextClock);
}

auto Paula::addIncomingByte(uint8_t byte) -> void {
    if (isprint( byte ) || byte == '\n') {
        if (incoming.size() >= MAX_EXPORT_SIZE)
            incoming.erase(incoming.begin());

        incoming += byte;
    }
}

auto Paula::addOutgoingByte(uint8_t byte) -> void {
    if (isprint( byte ) || byte == '\n') {
        if (outgoing.size() >= MAX_EXPORT_SIZE)
            outgoing.erase(outgoing.begin());

        outgoing += byte;
    }
}

}