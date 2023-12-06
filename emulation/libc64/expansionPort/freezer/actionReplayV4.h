
#pragma once

namespace LIBC64 {      
    
struct ActionReplayV4 : Freezer {

    ActionReplayV4(System* system) : Freezer(system, true, false) {
        
        ram = new uint8_t[ 8 * 1024 ];
    }

    // mark non bootable to use delayed autostart
    //auto isBootable( ) -> bool { return true; }
    
    ~ActionReplayV4() {
        delete[] ram;
    }
    
    bool enable = true;
    bool useRam = false;
    bool bug = false;
    uint8_t* ram = nullptr;
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        
        if (!enable)
            return;
        
        uint8_t bank = (value >> 3) & 3;

        cRomH = cRomL = getChip( bank & 3 );
        
        exRom = (value >> 1) & 1;
        game = (value & 1) ^ 1;

        useRam = (value & 0x20) ? true : false;

        bug = exRom && game && useRam;

        if (bug)
            // it seems the cartridge switches between 'cart off' and '8k mode'. (a jumpy exRom line ?)
            // both modes differ only in address space 0x8000 till 0x9fff (Cart Lo <> Ram)
            // This results in BUS contention, means a Or(ing) between cartridge and C64 memory.
            exRom = false;
        
        system->changeExpansionPortMemoryMode( exRom, game );
        
        if (value & 0x40)
            nmiCall(false);
        
        if (value & 4)
            enable = false;
    }

    auto readIo1( uint16_t addr ) -> uint8_t {
        if (!enable)
            return 0;

        // not handled by cart designer. usage leads to unwanted behaviour.
        uint8_t value = ExpansionPort::readIo1(addr);
        writeIo1( addr, value );
        return value;
    }
    
    auto readIo2( uint16_t addr ) -> uint8_t {
        
        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank
        Chip* chip = cRomL;
                
        if (!enable)
            chip = getChip(3);        
        else if (useRam)
            return ram[ addr ];
        
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

        if (bug)
            return system->readRam( 0x8000 | addr ) | ram[ addr ];

        if (useRam)
            return ram[ addr ];
        
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
    
    auto reset(bool softReset = false) -> void {
        cRomH = cRomL = getChip(0);
        enable = true;
        useRam = false;
        bug = false;
        if (softReset) {
            game = true;
            exRom = false;
        }
        std::memset(ram, 0, 8 * 1024);
    }
        
    auto serializeStep2(Emulator::Serializer& s) -> void {

        FreezeButton::serializeStep2( s );

        s.integer( enable );        
        s.integer( useRam );
        s.integer( bug );
        s.array( ram, 8 * 1024 );
    }
    
};    
    
}
