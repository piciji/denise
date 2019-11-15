
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
  
ControlPort::ControlPort( Interface::Device* device ) {
    
    this->device = device;       
}   

auto ControlPort::create( Interface::Device* device ) -> ControlPort* {
    
    if (!device)
        return new ControlPort( nullptr );
    
    if (device->isJoypad())        
        return new Joypad( device );
    
    if ( device->isMouse() && device->name.find( "1351" ) != std::string::npos )
        return new Mouse1351( device );
    
    if ( device->isMouse() && device->name.find( "Neos" ) != std::string::npos )
        return new MouseNeos( device );

    if ( device->isPaddles() )
        return new Paddles( device );
    
    if ( device->isLightGun() && device->name.find( "Stack" ) != std::string::npos )
        return new StackLightRifle( device );
    
    if ( device->isLightGun() && device->name.find( "Magnum" ) != std::string::npos )
        return new MagnumLightPhaser( device );
    
    if ( device->isLightGun() && device->name.find( "Gun Stick" ) != std::string::npos )
        return new GunStick( device );
    
    if ( device->isLightPen() && device->name.find( "Stack" ) != std::string::npos )
        return new StackLightpen( device );
    
    if ( device->isLightPen() && device->name.find( "Inkwell" ) != std::string::npos )
        return new InkwellLightpen( device );
    
    return new ControlPort( device );
}

auto ControlPort::serialize(Emulator::Serializer& s) -> void {
    
}
    
}
