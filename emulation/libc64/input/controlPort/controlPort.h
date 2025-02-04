
#pragma once

#include "../../system/system.h"

namespace Emulator {
    struct SystemTimer;
}

namespace LIBC64  {

struct System;
struct SystemTimer;
struct VicIIBase;
// interface for all control port connected devices    
    
struct ControlPort {

    ControlPort( System* system, Interface::Device* device = nullptr );

    System* system;
    Emulator::Interface* interface;
    Emulator::SystemTimer& sysTimer;
    Interface::Device* device;

    VicIIBase* vicII;
    
    static auto create( System* system, Interface::Device* device ) -> ControlPort*;
    
    virtual auto read( ) -> uint8_t { return 0xff; }
    virtual auto write( uint8_t value ) -> void {}
    
    virtual auto getPotX() -> uint8_t { return 0xff; }
    virtual auto getPotY() -> uint8_t { return 0xff; }
    
    virtual auto reset() -> void {}
    virtual auto poll() -> void {}
    virtual auto draw(bool midScreen = false) -> void {}    
    
    virtual auto getCursorPosition( int16_t& x, int16_t& y ) -> bool { return false; }
    
    virtual auto serialize(Emulator::Serializer& s) -> void;

    virtual auto allowJit() -> bool { return true; }

    virtual auto connectUserPort() -> bool { return false; }

    virtual auto readUserPort() -> uint8_t { return 0xff; }
};

}

