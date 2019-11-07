
#pragma once

namespace LIBC64 {      
    
struct ActionReplayV4 : ActionReplay {

    ActionReplayV4() : ActionReplay(true, false) {
        
        ram = new uint8_t[ 8 * 1024 ];
    }
    
    ~ActionReplayV4() {
        delete[] ram;
    }
    
    bool enable = true;
    bool useRam = false;
    uint8_t* ram = nullptr;
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        
        if (!enable)
            return;
        
        uint8_t bank = (value >> 3) & 3;

        cRomL = getChip( bank & 3 );
        cRomH = getChip( bank & 3 );
        
        system->changeExpansionPortMemoryMode((value >> 1) & 1, (value & 1) ^ 1 );     
        
        useRam = (value & 0x20) ? true : false;
        
        if (value & 0x40)
            nmiCall(false);
        
        if (value & 4)
            enable = false;
    }
    
    auto readIo2( uint16_t addr ) -> uint8_t {
        
        if (!enable || !cRomL)
            return ExpansionPort::readRomL( addr );
            
        if (useRam)
            return ram[ (0x1f << 8) | (addr & 0xff) ];
        
        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank
        return *(cRomL->ptr + addr);
    }    
    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void {
        if (!enable)
            return;
        
        if (useRam)
            ram[ (0x1f << 8) | (addr & 0xff) ] = value;
    }
    
    auto readRomL( uint16_t addr ) -> uint8_t {
        
        if (useRam)
            return ram[ addr & 0x1fff ];
        
        return Cart::readRomL( addr );
    }
    
    auto writeRomL( uint16_t addr, uint8_t data ) -> void {
        
        if (useRam)
            ram[ addr & 0x1fff ] = data;
        
        ExpansionPort::writeRomL( addr, data );
    }
        
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {
        
        if (useRam)
            ram[ addr & 0x1fff ] = data;
        
        ExpansionPort::writeUltimaxRomL( addr, data );
    }
    
    auto didFreeze() -> void {
        enable = true;
    }
    
    auto reset() -> void {
        cRomL = getChip(1);
        cRomH = getChip(1);
        enable = true;
        useRam = false;
        std::memset(ram, 0, 8 * 1024);
    }
        
    auto serializeStep2(Emulator::Serializer& s) -> void {
    
        ActionReplay::serializeStep2( s );

        s.integer( enable );        
        s.integer( useRam );        
        s.array( ram, 8 * 1024 );
    }
    
};    
    
}
