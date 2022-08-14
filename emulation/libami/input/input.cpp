
#include "input.h"
#include "controlPort/controlPort.h"
#include "../system/system.h"
#include "../../tools/bits.h"

#define portAOutputLo (lines->ddra & ~lines->pra)
#define portAOutputHi (lines->ddra & lines->pra)
#define portBOutputLo (lines->ddrb & ~lines->prb)
#define portBOutputHi (lines->ddrb & lines->prb)

namespace LIBAMI {

Input::Input(Emulator::Interface* interface) {
    this->interface = interface;
    controlPort1 = new ControlPort(interface);
    controlPort2 = new ControlPort(interface);

    for( auto& device : interface->devices ) {
        if (device.isKeyboard()) {
            keyboard.setDevice( &device );
            break;
        }
    }
}

auto Input::readCiaPortA( ) -> uint8_t {
 //   this->lines = lines;

    jitPoll();

    uint8_t out = controlPort1->readFire() << 6;
    out |= controlPort2->readFire() << 7;

    return out;
}


inline auto Input::jitPoll() -> void {
    if (jit.allow && interface->jitPoll()) {
        keyboard.poll();

        jit.midscreen = true;
        //system->interface->log("update", true);
    } else {
        //system->interface->log("too soon", true);
    }


}

auto Input::poll() -> void {

    bool jitDisable = !jit.allow || !jit.midscreen;

    //system->interface->log("jit ", true);
    //system->interface->log( !jitDisable ? "on" : "off", false );

    if ( jitDisable )
        keyboard.poll();

    controlPort1->poll();
    controlPort2->poll();



    jit.midscreen = false;
}

auto Input::drawCursor(bool midScreen) -> void {
    controlPort1->draw( midScreen );
    controlPort2->draw( midScreen );
}



auto Input::readPotX() -> uint8_t {

    switch(potMask) {
        case 1:
            return controlPort1->getPotX();
        case 2:
            return controlPort2->getPotX();
        case 3:
            return controlPort1->getPotX() & controlPort2->getPotX();
        default:
            return 0xff;
    }
}

auto Input::readPotY() -> uint8_t {

    switch(potMask) {
        case 1:
            return controlPort1->getPotY();
        case 2:
            return controlPort2->getPotY();
        case 3:
            return controlPort1->getPotY() & controlPort2->getPotY();
        default:
            return 0xff;
    }
}

auto Input::reset() -> void {
    //lines = nullptr;
    potMask = 1;
    keyboard.reset();
    controlPort1->reset();
    controlPort2->reset();
    jit.midscreen = false;
}

auto Input::connectControlport( Emulator::Interface::Connector* connector, Emulator::Interface::Device* device ) -> void {

    if (!connector)
        return;

    ControlPort** controlPort = connector->isPort1() ? &controlPort1 : &controlPort2;

    if ((*controlPort)->device == device)
        return;

    if (*controlPort)
        delete *controlPort;

    *controlPort = ControlPort::create( interface, device );

    allowJit();

    (*controlPort)->reset();
}

auto Input::enableJit(bool state) -> void {
    jit.enable = state;
    allowJit();
}

auto Input::allowJit() -> void {
    if (system->runAhead.preventJit && system->runAhead.frames)
        jit.allow = false;
    else
        jit.allow = jit.enable && controlPort1->useJitPolling() && controlPort2->useJitPolling();
}

auto Input::getConnectedDevice( Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device* {

    ControlPort* controlPort = connector->isPort1() ? controlPort1 : controlPort2;

    return controlPort->device;
}

auto Input::getCursorPosition( Emulator::Interface::Device* device, int16_t& x, int16_t& y ) -> bool {

    if (controlPort1->device == device)
        return controlPort1->getCursorPosition( x, y );

    if (controlPort2->device == device)
        return controlPort2->getCursorPosition( x, y );

    return false;
}

auto Input::serialize(Emulator::Serializer& s) -> void {

    s.integer( potMask );

   // this->lines = &cia1.lines;

    keyboard.serialize( s );

    for( auto& connector : system->interface->connectors ) {

        Interface::Device* device = getConnectedDevice( &connector );

        if (!device)
            device = system->interface->getUnplugDevice();

        unsigned deviceId = device->id;

        s.integer( deviceId );

        if ( s.mode() == Emulator::Serializer::Mode::Load ) {

            if (deviceId != device->id) {
                // state was generated with another connected device.
                // we need to connect the requested device.
                device = system->interface->getDevice( deviceId );

                connectControlport( &connector, device );
            }
        }

        ControlPort* controlPort = connector.isPort1() ? controlPort1 : controlPort2;

        controlPort->serialize( s );
    }

    if ( s.mode() == Emulator::Serializer::Mode::Load )
        jit.midscreen = false;
}

}
