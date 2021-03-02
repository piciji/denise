
#pragma once

#include "../vicII/base.h"
#include "../system/system.h"

namespace LIBC64 {
    
// base Class for all carts connected to expansion port.
// if there is no cart connected, use this class directly.    
    
struct ExpansionPort {
        
    ExpansionPort() {

        setId( Interface::ExpansionIdNone );
    }
    
    virtual ~ExpansionPort() {}
    
    struct {
        uint16_t addr;
    } bus;
    
    // pins on startup, some carts change this during runtime
    bool exRom = true;
    bool game = true;
    bool dma = false;   
	
	ExpansionPort* expander = nullptr;
    
    Interface::ExpansionId id = Interface::ExpansionIdNone; // base type of expansion
    
    std::function<void (bool state)> irqCall;    
    std::function<void (bool state)> nmiCall;
       
    std::function<bool ()> vicBA; // check if Vic needs bus
    std::function<void (bool state)> dmaCall; // change rdy and aec same time
    
    virtual auto hasFreezeButton() -> bool { return false; }
    
    virtual auto freeze() -> void {}

    virtual auto hasCustomButton() -> bool { return false; }

    virtual auto customButton() -> void {}
    
    virtual auto isDma() -> bool { return dma; }
    
    virtual auto isExrom( ) -> bool { return exRom; }
    
    virtual auto isGame( ) -> bool { return game; }
    
    virtual auto isBootable( ) -> bool { return false; }
    
    virtual auto hasRom() -> bool { return false; }
    
    virtual auto addressBus() -> uint16_t { return bus.addr; }
    
    virtual auto readIo1( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto readIo2( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto writeIo1( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto writeIo2( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto readRomL( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto readRomH( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto writeRomL( uint16_t addr, uint8_t data ) -> void {        
        system->writeRam( addr, data );
    }
    
    virtual auto writeRomH( uint16_t addr, uint8_t data ) -> void {
        system->writeRam( addr, data );
    }  
    
    virtual auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void { }
    
    virtual auto writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void { }   

    virtual auto readUltimaxA0( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
        
    virtual auto writeUltimaxA0( uint16_t addr, uint8_t data ) -> void { }
                 
    // a cartridge can listen for all addresses puted on bus, doesn't matter how actual PLA mapping is.
    // some RAM based cartridges accept writes between 0x8000 and 0x9fff to their own ram even if PLA
    // maps C64 RAM in this area
    virtual auto listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void { }

    virtual auto listenToWritesAtA0ToBF(uint16_t addr, uint8_t data ) -> void { }
    
    virtual auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {}
    
    virtual auto prepareRam(unsigned size) -> void {}
    
    virtual auto reset( bool softReset = false ) -> void {
        game = true;
        exRom = true;
        dma = false;
    }
    
    virtual auto setJumper( unsigned jumperId, bool state ) -> void {}
    
    virtual inline auto clock() -> void {}   
    
    virtual auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( game );
        s.integer( exRom );
        s.integer( dma );
        s.integer( bus.addr );
    }
	
	virtual auto hasSecondaryRom() -> bool { return false; }
    
    auto setId(Interface::ExpansionId id) -> void { this->id = id; }           
};   

}