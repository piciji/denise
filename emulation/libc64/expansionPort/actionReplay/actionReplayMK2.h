
#pragma once

namespace LIBC64 {      
    
struct ActionReplayMK2 : ActionReplay {

    ActionReplayMK2() : ActionReplay(true, false) {
        
    }
    
    unsigned disableCounter = 0;
    unsigned enableCounter = 0;

    auto readIo1( uint16_t addr ) -> uint8_t {
        enable();
        
        return ExpansionPort::readRomL( addr );
    }
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        enable();
    }
    
    auto readIo2( uint16_t addr ) -> uint8_t {
        disable();
        
        auto chip = getChip(1);
        
        if (!chip)
            return ExpansionPort::readRomL( addr );
            
        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank        
        
        return *(chip->ptr + addr);
    }
    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void {
        disable();
    }
    
    auto enable() -> void {

        if (++enableCounter == 65) {
            cRomH = cRomL = getChip(1);
            game = true;
            exRom = false;
            system->changeExpansionPortMemoryMode(exRom, game);            
        }
        
        disableCounter = 0;
    }
    
    auto disable() -> void {
        
        if (++disableCounter == (50 + (7 * 16))) {   
            enableCounter = 0;
            game = true;
            exRom = true;
            system->changeExpansionPortMemoryMode(exRom, game);            
        }                
    }
    
    auto didFreeze() -> void {
        nmiCall(false);        
        cRomH = cRomL = getChip(0);
        disableCounter = 0;
        enableCounter = 0;        
    }
    
    auto reset() -> void {
        cRomH = cRomL = getChip(1);
        disableCounter = 0;
        enableCounter = 0;
    }
        
    auto serializeStep2(Emulator::Serializer& s) -> void {
    
        ActionReplay::serializeStep2( s );

        s.integer(disableCounter);   
        s.integer(enableCounter);   
    }
    
};    
    
}
