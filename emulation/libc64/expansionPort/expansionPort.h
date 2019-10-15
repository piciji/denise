
#pragma once

#include "../vic/vicII.h"

namespace LIBC64 {
    
// base Class for all carts connected to expansion port.
// if there is no cart connected, use this class directly.    
    
struct ExpansionPort {
        
    ExpansionPort() {
        // if no cart was connected in expansion port
        game = true;
        exRom = true;
        dma = false;
        
        setId( Interface::ExpansionIdNone );
    }
    
    struct {
        uint16_t addr;
    } bus;
    
    // pins on startup, some carts change this during runtime
    bool exRom;
    bool game;
    bool dma;
    
    uint8_t* rom = nullptr;
    unsigned romSize = 0;
    
    Interface::ExpansionId id = Interface::ExpansionIdNone; // base type of expansion
    Interface::CartridgeId cartridgeId = Interface::CartridgeId::Default; // header id
    
    std::function<void (bool state)> irqCall;    
    std::function<void (bool state)> nmiCall;
       
    std::function<bool ()> vicBA; // check if Vic needs bus
    std::function<void (bool state)> dmaCall; // change rdy and aec same time
    
    virtual auto isDma() -> bool { return dma; }
    
    virtual auto isExrom( ) -> bool { return exRom; }
    
    virtual auto isGame( ) -> bool { return game; }
    
    virtual auto isBootable( ) -> bool { return false; }
    
    virtual auto addressBus() -> uint16_t { return bus.addr; }
    
    virtual auto readIo1( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto readIo2( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto writeIo1( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto writeIo2( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto readRomL( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto readRomH( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto writeRomL( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto writeRomH( uint16_t addr, uint8_t data ) -> void {}        
                    
    virtual auto setRom(uint8_t* rom, unsigned romSize) -> void {
        this->rom = rom;
        this->romSize = romSize;
    }
    
    virtual auto setRam(unsigned size) -> void {}
    
    virtual auto reset(bool state) -> void {}
    
    virtual auto cycleLo() -> void {}   
    
    virtual auto cycleHi() -> void {}   
    
    virtual auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( game );
        s.integer( exRom );
        s.integer( dma );
        s.integer( bus.addr );
    }
    
    auto setId(Interface::ExpansionId id) -> void { this->id = id; }
    auto setCartridgeId(Interface::CartridgeId cartridgeId) -> void { this->cartridgeId = cartridgeId; }
};
    
}