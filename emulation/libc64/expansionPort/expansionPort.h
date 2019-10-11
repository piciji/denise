
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
        active = false;
        dma = false;
    }
    
    // pins on startup, some carts change this during runtime
    bool exRom;
    bool game;
    bool active; // something connected
    bool dma;
    
    struct {
        uint16_t addr;
        uint8_t data;
        bool write;
    } bus;
    
    std::function<void (bool state)> irqCall;
    
    std::function<void (bool state)> nmiCall;
   
    
    std::function<bool ()> vicBA; // check if Vic needs bus
    std::function<void (bool state)> dmaCall; // change rdy and aec same time
    
    virtual auto isDma() -> bool { return dma; }
    
    virtual auto isExrom( ) -> bool { return exRom; }
    
    virtual auto isGame( ) -> bool { return game; }
    
    virtual auto isActive( ) -> bool { return active; }
    
    virtual auto addressBus() -> uint16_t { return bus.addr; }
    
    virtual auto dataBus() -> uint8_t { return bus.data; }
    
    virtual auto writeBus() -> bool { return bus.write; }    
    
    virtual auto readIo1( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto readIo2( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto writeIo1( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto writeIo2( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto readRomL( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto readRomH( uint16_t addr ) -> uint8_t { return vicII->lastReadPhase1(); }
    
    virtual auto writeRomL( uint16_t addr, uint8_t data ) -> void {}
    
    virtual auto writeRomH( uint16_t addr, uint8_t data ) -> void {}    
    
    virtual auto reset(bool state) -> void {}
    
    virtual auto cycleLo() -> void {}   
    
    virtual auto cycleHi() -> void {}   
    
    virtual auto serialize(Emulator::Serializer& s) -> void {}
    
};
    
}