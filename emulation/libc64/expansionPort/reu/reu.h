
#pragma once

#include "../expansionPort.h"
#include "../uci/uci.h"

namespace LIBC64 {
    
struct Reu : ExpansionPort {   

    Reu(System* system);
    ~Reu() override;

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
    Emulator::Interface::Media* ramMedia = nullptr;
    Uci uci;
        
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

    auto writeIo1( uint16_t addr, uint8_t value ) -> void override;
    auto readIo1( uint16_t addr ) -> uint8_t override;
    auto peekIo1( uint16_t addr ) -> uint8_t override;
    auto writeIo2( uint16_t addr, uint8_t value ) -> void override;
    auto readIo2( uint16_t addr ) -> uint8_t override;
    auto peekIo2( uint16_t addr ) -> uint8_t override;
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void override;
    auto prepareRam(unsigned size) -> void;

    auto setRamSize(int id) -> void override;
    auto getRamSize() -> int override;
    auto injectRam() -> void;
	
	auto isExrom( ) -> bool override;
	auto isGame( ) -> bool override;

    auto clock() -> void override;
    auto clockSCPU() -> void;
    auto reset(bool softReset = false) -> void override;
    auto serialize(Emulator::Serializer& s) -> void override;
    
    auto incrementAddresses() -> void;
    auto decrementTransferLength() -> void;
    auto readReu() -> uint8_t;
    auto writeReu(uint8_t value) -> void;
    auto allowIrq() -> bool;
    auto isBootable( ) -> bool override;
    auto hasFreezeButton() -> bool override;
    auto freeze() -> void override;
    
    template<bool fromSCPU = false> inline auto stash() -> void;
    template<bool fromSCPU = false> inline auto fetch() -> void;
    template<bool fromSCPU = false> inline auto swap() -> void;
    template<bool fromSCPU = false> inline auto verify() -> void;
	
	auto readRomL(uint16_t addr) -> uint8_t override;
    auto peekRomL(uint16_t addr) -> uint8_t override;
	auto writeRomL( uint16_t addr, uint8_t data ) -> void override;
    auto listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void override;
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void override;
    auto readRomH( uint16_t addr ) -> uint8_t override;
    auto peekRomH( uint16_t addr ) -> uint8_t override;
    auto readUltimaxA0( uint16_t addr ) -> uint8_t override;
    auto peekUltimaxA0( uint16_t addr ) -> uint8_t override;
    auto writeRomH( uint16_t addr, uint8_t data ) -> void override;
    auto writeUltimaxA0( uint16_t addr, uint8_t data ) -> void override;

    auto hasRom() -> bool override { return rom ? true : false; }

	auto setExpander( ExpansionPort* expander ) -> void;
    auto getSizeNotConsideredForMemorySerialization() -> unsigned override;

};    

}
