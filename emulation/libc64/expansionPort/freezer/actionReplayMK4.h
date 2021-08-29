
#pragma once

namespace LIBC64 {      
    
struct ActionReplayMK4 : Freezer {

    ActionReplayMK4() : Freezer(true, false) {
        
    }    
    
    bool enable = true;

    auto bootSpeed() -> float { return 0.9; }
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        
        if (!enable)
            return;

        uint8_t bank = (value & 1) | (((value >> 4) & 1) << 1);
        cRomH = cRomL = getChip( bank );
        exRom = ((value >> 3) & 1) ^ 1;
        game = (value >> 1) & 1;
        system->changeExpansionPortMemoryMode(exRom, game);     
        
        if (value & 4)
            enable = false;
    }    
    
    auto readIo2( uint16_t addr ) -> uint8_t {
        
        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank
        Chip* chip = cRomL;
                
        if (!enable)
            chip = getChip(3);        
        
        if (!chip)
            return ExpansionPort::readRomL( addr );
            
        return *(chip->ptr + addr);
    }    
    
    auto didFreeze() -> void {
        nmiCall(false);
        enable = true;
        cRomH = cRomL = getChip(0);
    }
    
    auto reset(bool softReset = false) -> void {
        enable = true;
        cRomH = cRomL = getChip(1);        
    }
        
    auto serializeStep2(Emulator::Serializer& s) -> void {

        FreezeButton::serializeStep2( s );

        s.integer( enable );
    }
    
};    
    
}
