
#pragma once

#include <cstdint>
#include <vector>
#include "../../tools/serializer.h"

namespace LIBC64 {
    
struct Mapper {
    virtual auto write( bool io1, uint16_t addr, uint8_t value ) -> void {}
	virtual auto read( bool io1, uint16_t addr ) -> uint8_t;
	virtual auto readRomL( uint16_t addr ) -> void {}
	virtual auto readRomH( uint16_t addr ) -> void {}
    virtual auto serialize(Emulator::Serializer& s) -> void {}
    
    virtual auto init() -> void;    
};
    
struct Cart {        
    Cart();    
	
    struct Header {
        enum Type : uint16_t { Default = 0, Ocean = 5, Funplay = 7, SuperGames = 8, System3 = 15, Zaxxon = 18 } type;
        
        uint8_t data[64];
        
        // pins on startup, some carts change this during runtime
        bool exRom;
        bool game;
        
        uint16_t version;  
    } header;
    
    struct Chip {
        enum Type { Rom = 0, Ram = 1, FlashRom = 2 } type;
        unsigned id;
        uint8_t bank;
        uint16_t size;
        uint16_t addr;
        uint32_t offset;
        uint8_t* ptr;
        uint8_t* ptrHi; //for 16k banks
    };
    std::vector<Chip> chips;
    Chip* cRomL;
    Chip* cRomH;

    auto load( uint8_t* data, unsigned size ) -> bool;
    auto unload() -> void;
    auto inUse() -> bool { return cartFound; }
    auto writeIo(bool io1, uint16_t addr, uint8_t value) -> void;
	auto readIo(bool io1, uint16_t addr) -> uint8_t;
    auto romL(unsigned addr) -> uint8_t;
    auto romH(unsigned addr) -> uint8_t;
	auto init( ) -> unsigned;
    auto serialize(Emulator::Serializer& s) -> void;
    
protected:        
    
    uint8_t* data = nullptr;
    unsigned size;
    bool cartFound;
    Mapper* mapper = nullptr;
    
    auto readHeader() -> bool;
    auto readChips() -> bool;
    auto setMapper() -> void;
    
    auto getDWord( uint8_t* ptr ) -> uint32_t;
    auto getWord( uint8_t* ptr ) -> uint16_t;
};    
    
extern Cart* cart;
}
