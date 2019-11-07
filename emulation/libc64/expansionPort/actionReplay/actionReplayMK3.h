
#pragma once

namespace LIBC64 {      
    
struct ActionReplayMK3 : ActionReplay {

    ActionReplayMK3() : ActionReplay(true, false) {
        
    }
    
    bool enable = true;
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        
        if (!enable)
            return;

        cRomH = cRomL = getChip( value & 1 );
        exRom = ((value >> 3) & 1) ^ 1;
        game = true;
        system->changeExpansionPortMemoryMode(exRom, game);     
        
        if (value & 4)
            enable = false;
    }
    
    auto readIo2( uint16_t addr ) -> uint8_t {
        
        if (!enable)
            return 0;
        
        if (!cRomL)
            return ExpansionPort::readRomL( addr );
            
        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank
        return *(cRomL->ptr + addr);
    }    
    
    auto didFreeze() -> void {
        enable = true;
        cRomH = cRomL = getChip(0);
    }
    
    auto reset() -> void {
        enable = true;
        cRomH = cRomL = getChip(1);        
    }
        
    auto serializeStep2(Emulator::Serializer& s) -> void {
    
        ActionReplay::serializeStep2( s );

        s.integer( enable );
    }
    
};    
    
}
