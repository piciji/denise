
#include "../../interface.h"
#include "../expansionPort.h"

namespace LIBC64 {
    
struct GameCart : ExpansionPort {
    
    GameCart(bool game = true, bool exrom = false);
    
    uint8_t header[64];
    uint16_t version;
    
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
    
    Interface::CartridgeId cartridgeId;        
    uint8_t* rom = nullptr; 
    unsigned romSize = 0;
    
    uint8_t* data = nullptr;
    unsigned size = 0;
    bool writeProtect = false;
    
    auto readHeader() -> bool;
    auto readChips() -> bool;
    virtual auto assumeChips() -> void;
    auto assumeChips( std::vector<unsigned> sizes ) -> void;
    virtual auto reset() -> void;
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    static auto create( Interface::CartridgeId cartridgeId ) -> GameCart*;
    
    virtual auto readRomL(uint16_t addr) -> uint8_t;
    virtual auto readRomH(uint16_t addr) -> uint8_t;   
    auto serialize(Emulator::Serializer& s) -> void;
    auto serializeStep2(Emulator::Serializer& s) -> void;
    
    auto getDWord( uint8_t* ptr ) -> uint32_t;
    auto getWord( uint8_t* ptr ) -> uint16_t;
    auto isBootable( ) -> bool;
    
    static auto getInstance(Interface::CartridgeId cartridgeId, uint8_t* rom, unsigned romSize) -> GameCart*;
};    
    
extern GameCart* gameCart;   

}