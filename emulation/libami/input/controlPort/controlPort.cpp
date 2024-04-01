
#include "../../../tools/serializer.h"
#include "controlPort.h"
#include "../input.h"

#include "joypad.cpp"
#include "mouse.cpp"

namespace LIBAMI  {

ControlPort::ControlPort( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device ) : input(input) {
    this->interface = interface;
    this->device = device;
}

auto ControlPort::create( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device ) -> ControlPort* {

    if (!device)
        return new ControlPort( interface, input, nullptr );

    if (device->isJoypad())
        return new Joypad( interface, input, device );

    if ( device->isMouse())
        return new Mouse( interface, input, device );

    return new ControlPort( interface, input, device );
}

auto ControlPort::serialize(Emulator::Serializer& s) -> void {

}

}

