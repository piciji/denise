
#pragma once

namespace LIBC64 {

struct FinalCartridgePlus : Freezer {

    bool enableRomL;
    bool enableRomH;
    bool bitCell;
    bool enable;

    FinalCartridgePlus(System* system) : Freezer(system, true, true) {

    }

    // auto isBootable( ) -> bool { return true; }

    virtual auto bootSpeed() -> float { return 4.0; }

    auto arm() -> bool { return true; }

    auto writeIo2( uint16_t addr, uint8_t value ) -> void {

        bitCell = (value >> 7) & 1;
        enableRomL = ((value >> 6) & 1) ^ 1;
        enableRomH = ((value >> 5) & 1);
        // "enable" not understood:
        // fc mode      c64 mode
        // 0x60         0x60
        // 0x20         0x20
        // 0x30         0x30 (disabled both, but how is it re enabled without a freeze ?)
        // 0x33         0x33 (disabled both, 0x03 ?)
        // 0x33         0x33
        // 0x60         0x50 (disabled in c64 boot only, seems plausible)
        enable = ((value >> 4) & 1) ^ 1;

        //system->interface->log( value, true, true );
    }

    auto peekIo2( uint16_t addr ) -> uint8_t {
        return readIo2( addr );
    }

    auto readIo2( uint16_t addr ) -> uint8_t {

        return (bitCell << 7) | (enableRomL << 6) | (enableRomH << 5) | (enable << 4);
    }

    auto clock() -> void {
        // freezer is activated with restore key, but we will allow the typical way also (no freezer button on real cart)
        FreezeButton::clock();

        // cartridge ultimax is enabled only in read mode for following address spaces (if enabled).
        // cartridge listen on BA line to find out if VIC has the BUS. if so, ultimax is inactive also.
        // todo: frequent remapping is slow.
        uint16_t _addr = system->cpu.addressBus();
        bool _write = system->cpu.isWriteCycle();

        if (_write)
            return system->changeExpansionPortMemoryMode( exRom = true, game = true, true );

        if(!enableRomL && !enableRomH)
            return system->changeExpansionPortMemoryMode( exRom = true, game = true, true );

        if (_addr >= 0x8000 && _addr <= 0xbfff) {
            if ( enableRomL)
                return system->changeExpansionPortMemoryMode(exRom = true, game = false, true, true);

     //   } else if (_addr >= 0xa000 && _addr <= 0xbfff) {
       //     if ( enableRomL)
         //       return system->changeExpansionPortMemoryMode(exRom = true, game = false, true, true);

        } else if ( _addr >= 0xe000 && _addr <= 0xffff ) {
            if ( enableRomH)
                return system->changeExpansionPortMemoryMode(exRom = true, game = false, true, true);
        }

        system->changeExpansionPortMemoryMode( exRom = true, game = true, true );
    }

    auto peekUltimaxA0( uint16_t addr ) -> uint8_t {

        return readUltimaxA0( addr );
    }

    auto readUltimaxA0( uint16_t addr ) -> uint8_t {

        if (!cRomL)
            return ExpansionPort::readRomL( addr );

        return *( cRomL->ptrHi + addr);
    }

    auto didFreeze() -> void {
        enableRomL = enableRomH = true;
        enable = true;
        vicII->setUltimax( false );
        nmiCall(false);
    }

    auto readChips() -> bool {
        chips.clear();

        if (!data || (size == 0) || (size < (32 * 1024 + 16)) )
            return false;

        if (std::memcmp(data, "CHIP", 4))
            return false;

        uint8_t* ptr = data;
        ptr += 16;
        ptr += 0x2000;

        Chip chip;
        chip.id = 0;
        chip.type = Chip::Type::Rom;
        chip.bank = 0;
        chip.size = 0x2000;
        chip.ptrHi = nullptr;
        chip.ptr = ptr;
        chips.push_back( chip );

        ptr += chip.size;
        chip.id = 1;
        chip.type = Chip::Type::Rom;
        chip.bank = 1;
        chip.size = 0x4000;
        chip.ptr = ptr;
        chip.ptrHi = chip.ptr + 8192;
        chips.push_back( chip );

        return true;
    }

    auto assumeChips( ) -> void {
        chips.clear();

        if (!data || (size == 0) || (size < (32 * 1024)) )
            return;

        uint8_t* ptr = data;
        ptr += 0x2000;

        Chip chip;
        chip.id = 0;
        chip.type = Chip::Type::Rom;
        chip.bank = 0;
        chip.size = 0x2000;
        chip.ptrHi = nullptr;
        chip.ptr = ptr;
        chips.push_back( chip );

        ptr += chip.size;
        chip.id = 1;
        chip.type = Chip::Type::Rom;
        chip.bank = 1;
        chip.size = 0x4000;
        chip.ptr = ptr;
        chip.ptrHi = chip.ptr + 8192;
        chips.push_back( chip );
    }

    auto serializeSwitchedIn(Emulator::Serializer& s) -> void {

        FreezeButton::serialize( s );

        s.integer( enableRomL );
        s.integer( enableRomH );
        s.integer( bitCell );
        s.integer( enable );
    }

    auto reset(bool softReset = false) -> void {
        enableRomL = true;
        enableRomH = true;
        enable = true;
        freezeArmed = true;
        cyclesTillFreeze = 0;
        bitCell = true;
        game = false;
        exRom = true;
        cRomL = getChip(1);
        cRomH = getChip(0);
        resetFreeze();
    }
};

}
