
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
agnus( cpu, cia1, cia2 ),
input(agnus, cia1, interface) {

    this->interface = interface;

    cia1.serialOut = [this](bool spLine, bool cntLine) {
        // Keyboard computer is not interested in CNT line changes, triggered by CIA
        input.keyboard.handshake(spLine);
    };


    cia1.readPort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( port == Cia::PORTA )
            return (uint8_t)(input.readCiaPortA( ) & lines->ioa);

        return (uint8_t)0xff;
    };

    cia1.writePort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( port == Cia::PORTA ) {
         //   if (lines->ioa != lines->ioaOld)

        } else {
            //if (lines->iob != lines->iobOld)

        }
    };

    cia2.writePort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( port == Cia::PORTA ) {
            cia2.setCNTAndSP( lines->ioa & 2, lines->ioa & 1 );
        }
    };
}

auto System::power(bool softReset, bool resetInstruction) -> void {

    agnus.reset();

    if (!resetInstruction) {
        if (!softReset) {
            cpu.power();

        } else {
            cpu.reset();
        }
    }

    if (!softReset)
        resetFromKeyboard = 0;

    cia1.reset();
    cia2.reset();
    agnus.reset();
    input.reset();

    powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
}

auto System::run() -> void {
    leaveEmulation = false;

    input.poll();

    if (resetFromKeyboard) {
        if ((resetFromKeyboard & 0x80) == 0) {
            power(true);
            resetFromKeyboard |= 0x80;
        }

        while (resetFromKeyboard) { // CPU and most chips on hold, Denise hasn't a reset line
            input.checkForEmergencyPoll(); // wait for releasing reset key combination
            agnus.processEvents();
            // todo leave each frame
        }
    }

    while( !leaveEmulation ) {
        cpu.process();
    }
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
