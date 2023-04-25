
#pragma once

#include "../../interface.h"
#include "../expansionPort.h"

namespace LIBC64 {

struct Cart : ExpansionPort {
    
    Cart(System* system, bool game = true, bool exrom = false);
        
    uint16_t version;
    uint8_t cartName[32];
    
    struct Chip {
        enum Type { Rom = 0, Ram = 1, FlashRom = 2 } type;
        unsigned id;
        uint16_t bank;
        uint16_t size;
        uint16_t addr;
        uint32_t offset;
        uint8_t* ptr;
        uint8_t* ptrHi; //for 16k banks
    };
    
    std::vector<Chip> chips;
    Chip* cRomL;
    Chip* cRomH;
    
    Interface::CartridgeId cartridgeId;
    uint8_t* rom = nullptr; 
    unsigned romSize = 0;
    
    uint8_t* data = nullptr;
    unsigned size = 0;    
    bool binFormat;
	
	Emulator::Interface::Media* media;
    
    auto readHeader() -> bool;
    virtual auto readChips() -> bool;
    virtual auto assumeChips() -> void;
    virtual auto assumeChips( std::vector<unsigned> sizes ) -> void;
    virtual auto reset(bool softReset = false) -> void;
    virtual auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    
    virtual auto readRomL(uint16_t addr) -> uint8_t;
    virtual auto readRomH(uint16_t addr) -> uint8_t;   
    virtual auto serialize(Emulator::Serializer& s) -> void;
    virtual auto serializeStep2(Emulator::Serializer& s) -> void;	
        
    auto rebuild(Interface::CartridgeId cartridgeId, uint8_t* _rom, unsigned _romSize) -> Cart*;
    virtual auto create( Interface::CartridgeId cartridgeId ) -> Cart* { return nullptr; }
    virtual auto assign(Cart* cart) -> void {}
	virtual auto write() -> void {}
	virtual auto prepare() -> void {}
	virtual auto protectFromDeletion() -> bool { return false; }
    
    auto getChip( unsigned index ) -> Chip* {
        return chips.size() > index ? &chips[index] : nullptr;
    }
    
    auto hasRom() -> bool { return rom ? true : false; }
    
    static auto buildHeader(uint8_t* header, uint16_t _type, bool _game, bool _exrom, std::string _name, uint16_t _version = 0x100 ) -> void;
    static auto buildChipHeader(uint8_t* header, Chip& chip) -> void;
    auto checkForEmptyFlashBank(uint8_t* ptr) -> bool;
};    
   

}