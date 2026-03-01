
#pragma once

#include "../cart/freezeButton.h"
#include "../../../tools/mx29lv640eb.h"

namespace LIBC64 {
    
struct EasyFlash3 : FreezeButton {   
    
    EasyFlash3(System* system);
    ~EasyFlash3();
        
    struct Slot {
        uint8_t* rom = nullptr;
        unsigned romSize = 0;
        Emulator::Interface::Media* media = nullptr;
        bool writeProtect = false;
        bool binFormat = false;
        std::vector<Chip> chips;
        bool dirty = false;
    } slots[8];

    Emulator::MX29LV640EB flash;
    uint8_t* dataFlash;
    bool loadSplitted;
    uint32_t flashBaseAdr;

    enum class Mode : uint8_t { Disable, EF3, Kernal, AR, SS5, FC3 } mode;
    uint8_t activeSlot;
    uint8_t bank;
    uint8_t* ram = nullptr;
    static uint8_t eapi[768];
    bool ef3Boot;   // ef3 only (not a jumper)
    bool enableMenu;
    bool LED = false;
    bool cartKill;
    
    // AR/NR
    bool useRam;
    bool writeOnce;
    bool reuMapping;
    bool npMode;
    
    bool romHLine = false;
    bool portUpdated = false;
    bool kernalHack = true; // only here switchable

    auto readIo1( uint16_t addr ) -> uint8_t;
    auto peekIo1( uint16_t addr ) -> uint8_t;

    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;

    auto readIo2( uint16_t addr ) -> uint8_t;
    auto peekIo2( uint16_t addr ) -> uint8_t;
    
    auto create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart*;
    
    auto assign( Cart* cart ) -> void;
    
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto unsetRom(Emulator::Interface::Media* media) -> void;
    auto assumeChips( ) -> void;
    
    auto reset(bool softReset = false) -> void;
    
    auto resetButton() -> bool;
    
    auto readRomL( uint16_t addr ) -> uint8_t;
    auto peekRomL( uint16_t addr ) -> uint8_t;
    auto readRomH( uint16_t addr ) -> uint8_t;
    auto peekRomH( uint16_t addr ) -> uint8_t;
    
    auto writeRomL( uint16_t addr, uint8_t data ) -> void;
    auto writeRomH( uint16_t addr, uint8_t data ) -> void;
    
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void;
    
    auto listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void;
    auto listenToWritesAtA0ToBF(uint16_t addr, uint8_t data ) -> void;
    
    auto write( Slot* slot, bool splitted ) -> void;    
    
    auto setWriteProtect(Emulator::Interface::Media* media, bool state) -> void;
    
    auto isWriteProtected(Emulator::Interface::Media* media) -> bool;

    auto serialize(Emulator::Serializer& s) -> void;
    
    auto isBootable( ) -> bool;
    
    auto updateDeviceState() -> void;
	
	auto protectFromDeletion() -> bool { return true; }

    auto hasCustomButton() -> bool { return true; } // menu button of EF3

    auto customButton() -> void;

    auto buildFlashBaseAdr() -> void;

    auto freeze() -> void;
    
    auto didFreeze() -> void;

    auto clock() -> void;
    
    auto memoryMapUpdated() -> void;
    
    auto updateSlotDisplayName(Slot* slot) -> void;
    auto updateSlotHeaderName(Slot* slot, uint8_t* header) -> void;

    auto getSizeNotConsideredForMemorySerialization() -> unsigned;
};

}
