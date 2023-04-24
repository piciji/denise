
#include "controlPort.h"

#include "joypad.cpp"
#include "mouse1351.cpp"
#include "mouseNeos.cpp"
#include "paddles.cpp"
#include "StackLightRifle.cpp"
#include "MagnumLightPhaser.cpp"
#include "GunStick.cpp"
#include "StackLightpen.cpp"
#include "InkwellLightpen.cpp"

namespace LIBC64  {
  
ControlPort::ControlPort( System* system, Interface::Device* device ) : system(system), sysTimer(system->sysTimer) {
    this->interface = system->interface;
    this->device = device;       
}   

auto ControlPort::create( System* system, Interface::Device* device ) -> ControlPort* {
    
    if (!device)
        return new ControlPort( system, nullptr );
    
    if (device->isJoypad())        
        return new Joypad( system, device );
    
    if ( device->isMouse() && device->name.find( "1351" ) != std::string::npos )
        return new Mouse1351( system, device );
    
    if ( device->isMouse() && device->name.find( "Neos" ) != std::string::npos )
        return new MouseNeos( system, device );

    if ( device->isPaddles() )
        return new Paddles( system, device );
    
    if ( device->isLightGun() && device->name.find( "Stack" ) != std::string::npos )
        return new StackLightRifle( system, device );
    
    if ( device->isLightGun() && device->name.find( "Magnum" ) != std::string::npos )
        return new MagnumLightPhaser( system, device );
    
    if ( device->isLightGun() && device->name.find( "Gun Stick" ) != std::string::npos )
        return new GunStick( system, device );
    
    if ( device->isLightPen() && device->name.find( "Stack" ) != std::string::npos )
        return new StackLightpen( system, device );
    
    if ( device->isLightPen() && device->name.find( "Inkwell" ) != std::string::npos )
        return new InkwellLightpen( system, device );
    
    return new ControlPort( system, device );
}

auto ControlPort::serialize(Emulator::Serializer& s) -> void {
    
}
    
}

