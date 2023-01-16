
#include "../../../tools/serializer.h"
#include "controlPort.h"

#include "joypad.cpp"
#include "mouse.cpp"


namespace LIBAMI  {

ControlPort::ControlPort( Emulator::Interface* interface, Emulator::Interface::Device* device ) {

    this->interface = interface;
    this->device = device;
}

auto ControlPort::create( Emulator::Interface* interface, Emulator::Interface::Device* device ) -> ControlPort* {

    if (!device)
        return new ControlPort( interface, nullptr );

    if (device->isJoypad())
        return new Joypad( interface, device );

    //if ( device->isMouse() && device->name.find( "1351" ) != std::string::npos )
      //  return new Mouse1351( device );

    return new ControlPort( interface, device );
}

auto ControlPort::serialize(Emulator::Serializer& s) -> void {

}

}

