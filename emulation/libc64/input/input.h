
#pragma once

#define ID_PORT_1 0
#define ID_PORT_2 1

#include <cstdint>
#include "../../interface.h"
#include "../../cia/base.h"
#include "keyboard.h"

namespace Emulator {
    struct Serializer;
}

namespace CIA {
    struct M6526;
}

namespace LIBC64 {

struct System;
struct ControlPort;
struct VicIIBase;

struct Input {
	
    Input(System* system, Emulator::Interface* interface, CIA::M6526& cia1);

    System* system;
    Emulator::Interface* interface = nullptr;
    CIA::M6526& cia1;

    VicIIBase* vicII;
    ControlPort* controlPort1 = nullptr;
    ControlPort* controlPort2 = nullptr;
    Keyboard keyboard;
    CIA::Base::Lines* lines = nullptr;
    uint8_t potMask;

    enum SamplingMode { Static_Sampling = 0, Restricted_Dynamic_Sampling = 1, Dynamic_Sampling = 2 };

    struct Sampling {
        SamplingMode mode = Dynamic_Sampling;
        bool allow = false;
        uint8_t midscreen = 0;
    } sampling;

    auto setVic(VicIIBase* vicII) -> void;

    auto connectControlport( Emulator::Interface::Connector* connector, Emulator::Interface::Device* device ) -> void;
    auto getConnectedDevice( Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device*;
    auto getCursorPosition( Emulator::Interface::Device* device, int16_t& x, int16_t& y ) -> bool;
    
    auto readCiaPortA( CIA::Base::Lines* lines ) -> uint8_t;
    auto readCiaPortB( CIA::Base::Lines* lines ) -> uint8_t;
    
    auto writeCiaPortA( CIA::Base::Lines* lines ) -> void;
    auto writeCiaPortB( CIA::Base::Lines* lines ) -> void;
    
    auto updateLightpen( uint8_t ioa, uint8_t iob ) -> void;
	auto poll() -> void;

    auto restore() -> bool;
    auto reset() -> void;
    
    auto jitPoll() -> void;
    
    auto readPotX() -> uint8_t;
    auto readPotY() -> uint8_t;
    
    auto drawCursor() -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    auto setSampling(uint8_t mode) -> void;
    auto updateSampling() -> void;

    auto setKeycode(uint8_t row, uint8_t col) -> void;

    auto connectUserPort() -> bool;

    auto readUserPort() -> uint8_t;
};

}
