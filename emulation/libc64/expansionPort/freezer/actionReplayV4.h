
#pragma once

namespace LIBC64 {      
    
struct ActionReplayV4 : Freezer {

    ActionReplayV4() : Freezer(true, false) {
        
        ram = new uint8_t[ 8 * 1024 ];
    }
    
    auto isBootable( ) -> bool { return true; }    
    
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

        cRomH = cRomL = getChip( bank & 3 );
        
        exRom = (value >> 1) & 1;
        game = (value & 1) ^ 1;
        
        system->changeExpansionPortMemoryMode( exRom, game );     
        
        useRam = (value & 0x20) ? true : false;
        
        if (value & 0x40)
            nmiCall(false);
        
        if (value & 4)
            enable = false;
    }
    
    auto readIo2( uint16_t addr ) -> uint8_t {
        
        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank
        Chip* chip = cRomL;
                
        if (!enable)
            chip = getChip(3);        
        else if (useRam)
            return ram[ (0x1f << 8) | (addr & 0xff) ];
        
        if (!chip)
            return ExpansionPort::readRomL( addr );
            
        return *(chip->ptr + addr);        
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
    
    auto listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void {
        if (useRam)
            ram[ addr & 0x1fff ] = data;
    }
    
    auto didFreeze() -> void {
        cRomH = cRomL = getChip(0);
        enable = true;
    }
    
    auto reset() -> void {
        cRomH = cRomL = getChip(0);
        enable = true;
        useRam = false;
        std::memset(ram, 0, 8 * 1024);
    }
        
    auto serializeStep2(Emulator::Serializer& s) -> void {

        FreezeButton::serializeStep2( s );

        s.integer( enable );        
        s.integer( useRam );        
        s.array( ram, 8 * 1024 );
    }
    
};    
    
}
