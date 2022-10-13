
#include "system.h"
#include "../input/input.h"
#include "../../tools/sanitizer.h"
#include  "../../tools/macros.h"

namespace LIBAMI {

System* system = nullptr;

System::System(Interface* interface) :
cia1(1),
cia2(2),
cpu(agnus),
blitter(agnus),
copper(agnus),
agnus( cpu, blitter, copper, cia1, cia2, input ),
input(agnus, cia1, interface) {

    this->interface = interface;

    cia1.serialOut = [this](bool spLine, bool cntLine) {
        // Keyboard computer is not interested in CNT line changes, triggered by CIA
        input.keyboard.handshake(spLine);
    };


    cia1.readPort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA )
            return (uint8_t)(input.readCiaPortA( ) & lines->ioa);

        return (uint8_t)0xff;
    };

    cia1.writePort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA ) {
         //   if (lines->ioa != lines->ioaOld)
            if ((lines->ioa ^ lines->ioaOld) & 1)
                agnus.setOVL(lines->ioa & 1);

        } else {
            //if (lines->iob != lines->iobOld)

        }
    };

    cia2.writePort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA ) {
            cia2.setCNTAndSP( lines->ioa & 2, lines->ioa & 1 );
        }
    };
}

auto System::power(bool softReset, bool resetInstruction) -> void {

    agnus.reset(softReset);

    if (!resetInstruction) {
        if (!softReset) {
            cpu.power();

        } else {
            cpu.reset();
        }
    }

    cia1.reset();
    cia2.reset();
    input.reset();

    powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
}

auto System::run() -> void {
    leaveEmulation = false;

    input.poll();

    if (agnus.resetFromKeyboard)
        agnus.waitKeyboardReset();

    while( !leaveEmulation ) {
        cpu.process();
    }

    agnus.setEventInactive<Agnus::EVENT_LEAVE_EMULATION>();
}

auto System::informAboutKeyUpdate() -> void {
    input.sampling.emergencyPolling = true; // call from another thread
}

auto System::setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {
    if (size >= (512 * 1024))
        size = 512 * 1024;
    else
        size = Emulator::powerOfTwo( size );

    if (!data || !size) {
        data = nullptr;
        size = 1;
    }

    switch (typeId) {
        case 0:
        default:
            agnus.kickRom = data;
            agnus.kickRomMask = size - 1;
            break;
        case 1:
            agnus.extRom = data;
            agnus.extRomMask = size - 1;
            break;
    }
}



}
