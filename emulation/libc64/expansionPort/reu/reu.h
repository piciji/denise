
#pragma once

#include "../expansionPort.h"

namespace LIBC64 {
    
struct Reu : ExpansionPort {   
    
    Reu(Emulator::Events* events);
    ~Reu();
    
    Emulator::Events* events;
    using Callback = std::function<void ()>;
    
    uint8_t status;          
    uint8_t command;
    uint8_t intMask;    
    uint8_t control;
    
    struct {
        uint16_t hostAddr;    
        uint32_t reuAddr;
        uint16_t transferLength;           
    } reg;     
    
    uint16_t hostAddr;    
    uint32_t reuAddr;
    uint16_t transferLength;
    
    unsigned size = 0; // in kb
    uint8_t* data = nullptr; 
    
    unsigned romSize = 0;
    uint8_t* rom = nullptr; 
    
    unsigned dumpSize = 0;
    uint8_t* dump = nullptr;
        
    uint32_t wrapAround;
    uint32_t dramWrapAround;
    
    Callback setIrq;
    Callback unsetIrq;
    Callback setDma;
    Callback finish;
    
    bool waitForStart;
    uint8_t vicBaLow;
    bool steal;
    uint8_t value;
    uint8_t value2;    
    bool swapRead;   
    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;    
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto readRomL(uint16_t addr) -> uint8_t;
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto prepareRam(unsigned size) -> void;
    auto setRam( uint8_t* dump, unsigned dumpSize ) -> void;
    auto unsetRam() -> void;
    auto injectRam() -> void;

    auto cycleHi() -> void;
    
    auto reset() -> void;   
    auto serialize(Emulator::Serializer& s) -> void;
    
    auto incrementAddresses() -> void;
    auto decrementTransferLength() -> void;
    auto readReu() -> uint8_t;
    auto writeReu(uint8_t value) -> void;
    auto allowIrq() -> bool;
    auto isBootable( ) -> bool;
    
    inline auto stash() -> void;
    inline auto fetch() -> void;
    inline auto swap() -> void;
    inline auto verify() -> void;
    
    auto hasRom() -> bool { return rom ? true : false; }

};    
    
extern Reu* reu;
}