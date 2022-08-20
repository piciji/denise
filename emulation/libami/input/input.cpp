
#include "input.h"
#include "controlPort/controlPort.h"
#include "../system/system.h"
#include "../agnus/agnus.h"
#include "../../tools/bits.h"

#define portAOutputLo (lines->ddra & ~lines->pra)
#define portAOutputHi (lines->ddra & lines->pra)
#define portBOutputLo (lines->ddrb & ~lines->prb)
#define portBOutputHi (lines->ddrb & lines->prb)

namespace LIBAMI {

Input::Input(Agnus& agnus, Cia& cia1, Emulator::Interface* interface) : cia1(cia1), agnus(agnus), keyboard(interface, agnus, cia1) {
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

auto Input::checkForEmergencyPoll() -> void {
    if (sampling.emergencyPolling) {
        system->interface->jitPoll(0);
        sampling.emergencyPolling = false;
    }
}

inline auto Input::jitPoll() -> void {

    if (sampling.allow && (sampling.emergencyPolling || (sampling.mode == Dynamic_Sampling) || (sampling.midscreen < 2))) {
        if (system->interface->jitPoll(sampling.emergencyPolling ? 0 : (sampling.mode == Restricted_Dynamic_Sampling ? 5 : -1))) {
            sampling.emergencyPolling = false;
            sampling.midscreen++;
            //system->interface->log(vicII->getVcounter(), false);
        }
    }
}

auto Input::poll() -> void {

    bool jitDisable = !sampling.allow || (sampling.midscreen == 0);

    //system->interface->log("jit ", true);
    //system->interface->log( !jitDisable ? "on" : "off", false );

    controlPort1->poll();
    controlPort2->poll();

    sampling.midscreen = 0;
    sampling.emergencyPolling = false;
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

    if (!system->resetFromKeyboard)
        keyboard.reset();

    controlPort1->reset();
    controlPort2->reset();
    sampling.midscreen = 0;
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

    updateSampling();

    (*controlPort)->reset();
}

auto Input::setSampling(uint8_t mode) -> void {
    sampling.mode = (SamplingMode)mode;
    updateSampling();
}

auto Input::updateSampling() -> void {
    if (system->runAhead.preventJit && system->runAhead.frames)
        sampling.allow = false;
    else
        sampling.allow = (sampling.mode != 0) && controlPort1->useJitPolling() && controlPort2->useJitPolling();
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
        sampling.midscreen = 0;
}

}
