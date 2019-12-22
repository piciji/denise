
#pragma once

#include "../../system/system.h"

namespace LIBC64  {

// interface for all control port connected devices    
    
struct ControlPort {
    
    ControlPort( Interface::Device* device = nullptr );
    
    Interface::Device* device;
    
    static auto create( Interface::Device* device ) -> ControlPort*;
    
    virtual auto read( ) -> uint8_t { return 0xff; }
    virtual auto write( uint8_t value ) -> void {}
    
    virtual auto getPotX() -> uint8_t { return 0xff; }
    virtual auto getPotY() -> uint8_t { return 0xff; }
    
    virtual inline auto tick() -> void {}
    virtual auto reset() -> void {}
    virtual auto poll() -> void {}
    virtual auto draw(bool midScreen = false) -> void {}    
    
    virtual auto getCursorPosition( int16_t& x, int16_t& y ) -> bool { return false; }
    
    virtual auto serialize(Emulator::Serializer& s) -> void;
};

}

