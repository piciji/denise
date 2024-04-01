
#pragma once

#include "controlPort.h"

namespace LIBAMI {

struct AnalogControl : ControlPort {

    AnalogControl( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device ) : ControlPort( interface, input, device ) {}

    int16_t posX;
    int16_t posY;

    virtual auto poll( ) -> void {
        // driver reported deltas will be added one time after each single global input polling in vsync
        posX += interface->inputPoll( device->id, 0);
        posY -= interface->inputPoll( device->id, 1);
    }

    virtual auto reset() -> void {
        posX = 0;
        posY = 0;
    }

    virtual auto serialize(Emulator::Serializer& s) -> void {

        s.integer( posX );
        s.integer( posY );
    }

    auto useJitPolling() -> bool {
        return true;
    }
};

}