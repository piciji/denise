#pragma once

#include "../../interface.h"
#include "../../../tools/flash040.h"
#include "../cart/freezer.h"

namespace LIBC64 {
    
struct RetroReplay : Freezer {
    
    RetroReplay(Emulator::Events* events);
    
    ~RetroReplay();
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;
    
    auto assign(Cart* cart) -> void;
    
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    auto readIo1( uint16_t addr ) -> uint8_t;
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto updateRamBank() -> void;
    auto setJumper( unsigned jumperId, bool state ) -> void;
    template<bool specialCase = false> auto getFlashAddr( uint32_t addr ) -> uint32_t;
    auto init() -> void;
    
    auto readRomL( uint16_t addr ) -> uint8_t;
    auto readRomH( uint16_t addr ) -> uint8_t;
    auto readUltimaxA0( uint16_t addr ) -> uint8_t;
    
    auto writeRomL( uint16_t addr, uint8_t data ) -> void;
    auto writeRomH( uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxA0( uint16_t addr, uint8_t data ) -> void;
    
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void;
    
    auto didFreeze() -> void;
    auto blockFreeze() -> bool;
    auto reset() -> void;
    auto setWriteProtect(bool state) -> void;
    auto write() -> void;
    auto checkForEmptyBank(uint8_t* ptr) -> bool;
    auto isBootable( ) -> bool; 
    auto serialize(Emulator::Serializer& s) -> void;

    Emulator::Interface::Media* media;
    bool binFormat;
    
    Emulator::Flash040 flash;
    Emulator::Events* events;
    uint8_t* flashData;
    uint8_t* ram = nullptr;
    bool flashJumper;
    bool bankJumper;
    bool enabled;
    uint8_t bank;
    bool frozen;
    bool ramMode;
    
    bool alternateRam;
    bool allowBank;
    bool noFreeze;
    bool reuMapping;
    bool writeOnce;
    uint16_t ramBank;
    bool writeProtect;
};    
    
extern RetroReplay* retroReplay;   

}