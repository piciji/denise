
#pragma once

#include "../cart/cart.h"
#include "../../../tools/mx29lv640eb.h"

namespace LIBC64 {
    
struct EasyFlash3 : Cart {   
    
    EasyFlash3();
    ~EasyFlash3();
        
    struct Slot {
        Slot();

        uint8_t* rom = nullptr;
        unsigned romSize = 0;
        Emulator::Interface::Media* media;
        bool writeProtect;
        bool binFormat = false;
        std::vector<Chip> chips;
    } slots[8];

    Emulator::MX29LV640EB flash;
    uint8_t* dataFlash;


    enum class Mode { EF3, Kernal, AR, SS } mode;
    bool disableUlimaxForVICInFirstHalfCycle = false;
    Slot* activeSlot;
    uint8_t bank;
    uint8_t* ram = nullptr;
    static uint8_t eapi[768];
    bool ef3Boot;   // ef3 only (not a jumper)
    bool enableMenu;
    bool LED;

    auto readIo1( uint16_t addr ) -> uint8_t;

    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    
    auto readIo2( uint16_t addr ) -> uint8_t;

    auto control( uint8_t value ) -> void;
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;
    
    auto assign( Cart* cart ) -> void;
    
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto unsetRom(Emulator::Interface::Media* media) -> void;
    auto assumeChips( ) -> void;
    
    auto reset(bool softReset = false) -> void;
    
    auto readRomL( uint16_t addr ) -> uint8_t;
    auto readRomH( uint16_t addr ) -> uint8_t;
    
    auto writeRomL( uint16_t addr, uint8_t data ) -> void;
    auto writeRomH( uint16_t addr, uint8_t data ) -> void;
    
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void;
    
    auto write( Slot* slot ) -> void;
    
    auto createImage(unsigned& imageSize) -> uint8_t*;
    
    auto setWriteProtect(Emulator::Interface::Media* media, bool state) -> void;
    
    auto isWriteProtected(Emulator::Interface::Media* media) -> bool;

    auto serialize(Emulator::Serializer& s) -> void;

    auto serializeSlot(Slot* slot, Emulator::Serializer& s) -> void;
    
    auto isBootable( ) -> bool;
    
    auto updateDeviceState() -> void;
	
	auto protectFromDeletion() -> bool { return true; }

    auto hasCustomButton() -> bool { return ef3Mode; } // menu button of EF3

    auto customButton() -> void;

	auto useEF1Slots() -> bool;
};

extern EasyFlash3* easyFlash3;
}
