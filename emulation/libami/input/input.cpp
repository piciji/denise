
#include "input.h"
#include "controlPort/controlPort.h"
#include "../system/system.h"
#include "../interface.h"
#include "../agnus/agnus.h"
#include "../../tools/bits.h"

#define portAOutputLo (lines->ddra & ~lines->pra)
#define portAOutputHi (lines->ddra & lines->pra)
#define portBOutputLo (lines->ddrb & ~lines->prb)
#define portBOutputHi (lines->ddrb & lines->prb)

namespace LIBAMI {

Input::Input(System* system, Agnus& agnus, Cia<MOS_8520>& cia1)
: system(system), cia1(cia1), agnus(agnus), keyboard(system->interface, agnus, cia1) {
    this->interface = system->interface;

    controlPort1 = new ControlPort(interface, *this);
    controlPort2 = new ControlPort(interface, *this);
}

auto Input::readParallelportCIA1B(uint8_t& res) -> void {
    if (controlPort2->device->isMultiPlayerAdapter()) {
        jitPoll();
        controlPort2->readParallelFromCIA1B(res);
    } else if (controlPort1->device->isMultiPlayerAdapter()) {
        jitPoll();
        controlPort1->readParallelFromCIA1B(res);
    }
}

auto Input::readParallelportCIA2A(uint8_t& res) -> void {
    if (controlPort2->device->isMultiPlayerAdapter()) {
        jitPoll();
        controlPort2->readParallelFromCIA2A(res);
    } else if (controlPort1->device->isMultiPlayerAdapter()) {
        jitPoll();
        controlPort1->readParallelFromCIA2A(res);
    }
}

auto Input::readCia() -> uint8_t {
    jitPoll();
    uint8_t out = 0xff;
    if (controlPort1->readCia()) out &= ~0x40;
    if (controlPort2->readCia()) out &= ~0x80;
    return out;
}

auto Input::writeCiaPort1(bool state) -> void {
    controlPort1->writeCia(state);
}

auto Input::writeCiaPort2(bool state) -> void {
    controlPort2->writeCia(state);
}

auto Input::peekDenisePortA() -> uint16_t {
    jitPoll();
    return controlPort1->readDirection();
}

auto Input::readDenisePortA() -> uint16_t {
    system->observeInputFetches();
    jitPoll();
    uint16_t out = controlPort1->readDirection();
    if (system->dongle.connected())
        system->dongleJoydat<false>(out);
    return out;
}

auto Input::writeDeniseJoytest(uint16_t data) -> void {
    controlPort1->writeJoytest(data);
    controlPort2->writeJoytest(data);
}

auto Input::peekDenisePortB() -> uint16_t {
    jitPoll();
    return controlPort2->readDirection();
}

auto Input::readDenisePortB() -> uint16_t {
    system->observeInputFetches();
    jitPoll();
    uint16_t out = controlPort2->readDirection();
    if (system->dongle.connected())
        system->dongleJoydat<true>(out);
    return out;
}

auto Input::observePot(uint8_t& x0, uint8_t& y0, uint8_t& x1, uint8_t& y1) -> void {
    jitPoll();
    controlPort1->observePot(x0, y0);
    controlPort2->observePot(x1, y1);
}

auto Input::writePot(uint8_t& x0, uint8_t& y0, uint8_t& x1, uint8_t& y1) -> void {
    controlPort1->writePot(x0, y0);
    controlPort2->writePot(x1, y1);
}

auto Input::observePotPort1(uint8_t& x0, uint8_t& y0) -> void {
    jitPoll();
    controlPort1->observePot(x0, y0);
}

auto Input::observePotPort2(uint8_t& x1, uint8_t& y1) -> void {
    jitPoll();
    controlPort2->observePot(x1, y1);
}

auto Input::emergencyPoll() -> void {
    interface->jitPoll(0);
    sampling.midscreen++;
    controlPort1->poll();
    controlPort2->poll();
    sampling.externalKeyEvent = false;
}

inline auto Input::jitPoll() -> void {

    if (sampling.allow && (sampling.externalKeyEvent || (sampling.mode == Dynamic_Sampling) || (sampling.midscreen < 3))) {
        if (interface->jitPoll(sampling.externalKeyEvent ? 0 : (sampling.mode == Restricted_Dynamic_Sampling ? 5 : -1))) {
            sampling.externalKeyEvent = false;
            sampling.midscreen++;
            controlPort1->poll();
            controlPort2->poll();
            //interface->log(vicII->getVcounter(), false);
        }
    }
}

auto Input::initFrame() -> void {

    bool jitDisable = !sampling.allow || (sampling.midscreen == 0);
    //interface->log("jit ", true);
    //interface->log( !jitDisable ? "on" : "off", false );

    if (jitDisable) {
        controlPort1->poll();
        controlPort2->poll();
    }

    sampling.midscreen = 0;
    sampling.externalKeyEvent = false;
}

auto Input::drawCursor() -> void {
    controlPort1->draw( );
    controlPort2->draw( );
}

auto Input::reset() -> void {
    if (!agnus.resetFromKeyboard)
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

    *controlPort = ControlPort::create( interface, *this, device );

    updateSampling();

    (*controlPort)->reset();
}

auto Input::setSampling(uint8_t mode) -> void {
    sampling.mode = (SamplingMode)mode;
    updateSampling();
}

auto Input::updateSampling() -> void {
    sampling.allow = (sampling.mode != Static_Sampling) && !system->runAheadPreventJit();
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

    keyboard.serialize( s );

    for( auto& connector : interface->connectors ) {

        Interface::Device* device = getConnectedDevice( &connector );

        if (!device)
            device = interface->getUnplugDevice();

        unsigned deviceId = device->id;

        s.integer( deviceId );

        if ( s.mode() == Emulator::Serializer::Mode::Load ) {

            if (deviceId != device->id) {
                // state was generated with another connected device.
                // we need to connect the requested device.
                device = interface->getDevice( deviceId );

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
