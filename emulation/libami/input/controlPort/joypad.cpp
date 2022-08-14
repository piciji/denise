
#include "controlPort.h"

namespace LIBAMI {

struct Joypad : ControlPort {

    Joypad( Emulator::Interface* interface, Emulator::Interface::Device* device ) : ControlPort( interface, device ) {}

    auto readFire( ) -> uint8_t {

        return interface->inputPoll( device->id, 4 );
    }

    auto useJitPolling() -> bool {
        return true;
    }
};

}