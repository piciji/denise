
#pragma once

#include "../cart/cart.h"
#include "../../../tools/flash040.h"

namespace LIBC64 {
    
struct EasyFlash : Cart {   
    
    EasyFlash(Emulator::Events* events);
    ~EasyFlash();
    
    Emulator::Events* events;
    
    Emulator::Flash040 flashLo;
    Emulator::Flash040 flashHi;
    
    uint8_t* dataLo;
    uint8_t* dataHi;
    uint8_t bank;
    bool binFormat;
    bool writeProtect;
    uint8_t ram[256];
    Emulator::Interface::Media* media;
    static uint8_t eapi[768];
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    
    auto readIo2( uint16_t addr ) -> uint8_t;
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;
    
    auto assign( Cart* cart ) -> void;
    
    auto init() -> void;
    
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto assumeChips( ) -> void;
    
    auto reset() -> void;
    
    auto readRomL( uint16_t addr ) -> uint8_t;
    auto readRomH( uint16_t addr ) -> uint8_t;
    
    auto writeRomL( uint16_t addr, uint8_t data ) -> void;
    auto writeRomH( uint16_t addr, uint8_t data ) -> void;
    
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void;
    
    auto write() -> void;   
    
    auto checkForEmptyBank(uint8_t* ptr) -> bool;
    
    auto createFlash(unsigned& imageSize) -> uint8_t*;
    
    auto setWriteProtect(bool state) -> void;

    auto serialize(Emulator::Serializer& s) -> void;
    
    auto isBootable( ) -> bool; 
};

extern EasyFlash* easyFlash;
}
