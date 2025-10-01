
#pragma once

#include "../expansionPort.h"

namespace LIBC64 {
    
struct Reu : ExpansionPort {   
    
    Reu(System* system);
    ~Reu();	
    
    using Callback = std::function<void ()>;

    Emulator::SystemTimer& sysTimer;
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
    uint8_t busValue;
    uint8_t busValue2;    
	uint8_t busFloating;
    bool swapRead;

    auto writeIo1( uint16_t addr, uint8_t value ) -> void;    
    auto readIo1( uint16_t addr ) -> uint8_t;    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;    
    auto readIo2( uint16_t addr ) -> uint8_t;    
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto prepareRam(unsigned size) -> void;

    auto setRamSize(int id) -> void;
    auto getRamSize() -> int;

    auto setRam( uint8_t* dump, unsigned dumpSize ) -> void;
    auto unsetRam() -> void;
    auto injectRam() -> void;
	
	auto isExrom( ) -> bool;
	auto isGame( ) -> bool;

    auto clock() -> void;
    auto clockSCPU() -> void;
    auto reset(bool softReset = false) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    
    auto incrementAddresses() -> void;
    auto decrementTransferLength() -> void;
    auto readReu() -> uint8_t;
    auto writeReu(uint8_t value) -> void;
    auto allowIrq() -> bool;
    auto isBootable( ) -> bool;
    auto hasFreezeButton() -> bool;
    auto freeze() -> void;
    
    template<bool fromSCPU = false> inline auto stash() -> void;
    template<bool fromSCPU = false> inline auto fetch() -> void;
    template<bool fromSCPU = false> inline auto swap() -> void;
    template<bool fromSCPU = false> inline auto verify() -> void;
	
	auto readRomL(uint16_t addr) -> uint8_t;
	auto writeRomL( uint16_t addr, uint8_t data ) -> void;
    auto listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void;    
    auto readRomH( uint16_t addr ) -> uint8_t;
    auto readUltimaxA0( uint16_t addr ) -> uint8_t;        
    auto writeRomH( uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxA0( uint16_t addr, uint8_t data ) -> void;

    auto hasRom() -> bool { return rom ? true : false; }
	
	auto hasSecondaryRom() -> bool { return true; }
	
	auto setExpander( ExpansionPort* expander ) -> void;
    auto getSizeNotConsideredForMemorySerialization() -> unsigned;

};    

}
