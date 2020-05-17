
#pragma once

#define ID_PORT_1 0
#define ID_PORT_2 1

#include "../../cia/base.h"
#include "keyboard.h"
#include "controlPort/controlPort.h"
#include "../../tools/serializer.h"

namespace LIBC64 {
    
struct Input {
	
    Input();
    
    ControlPort* controlPort1 = nullptr;
    ControlPort* controlPort2 = nullptr;
    Keyboard keyboard;
    CIA::Base::Lines* lines = nullptr;
    uint8_t potMask;
    
    struct Jit {
        bool enable = false;
        bool midscreen = false;
    } jit;
    
    auto connectControlport( Interface::Connector* connector, Interface::Device* device ) -> void;
    auto getConnectedDevice( Interface::Connector* connector ) -> Interface::Device*;
    auto getCursorPosition( Interface::Device* device, int16_t& x, int16_t& y ) -> bool;
    
    auto readCiaPortA( CIA::Base::Lines* lines ) -> uint8_t;
    auto readCiaPortB( CIA::Base::Lines* lines ) -> uint8_t;
    
    auto writeCiaPortA( CIA::Base::Lines* lines ) -> void;
    auto writeCiaPortB( CIA::Base::Lines* lines ) -> void;
    
    auto updateLightpen( uint8_t ioa, uint8_t iob ) -> void;
	auto poll() -> void;
    
    auto moreThanTwoRowsActivated( uint8_t row ) -> bool;
    
    auto restore() -> bool;
    auto reset() -> void;
    
    auto jitPoll() -> void;
    
    auto readPotX() -> uint8_t;
    auto readPotY() -> uint8_t;
    
    auto clock() -> void {
        controlPort1->clock();
        controlPort2->clock();
    }
    auto drawCursor(bool midScreen = false) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
};

}
