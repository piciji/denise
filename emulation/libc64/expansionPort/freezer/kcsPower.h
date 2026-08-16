
/**
 * KCS Power Cartridge
 *
 * Written by
 *  ClausS
 *
 **/

#pragma once

#include "freezer.h"

namespace LIBC64 {

struct KCSPower : Freezer {

    // Configuration modes as used by the original KCS Power Cartridge and VICE
    enum {
        CMODE_8KGAME  = 0,
        CMODE_16KGAME = 1,
        CMODE_RAM     = 2,
        CMODE_ULTIMAX = 3
    };

    KCSPower(System* system)
    : Freezer(system, false, false)
    {
        ram = new uint8_t[128];
        std::memset(ram, 0, 128);
        config = CMODE_16KGAME;

        // don't call "changeExpansionPortMemoryMode" during creation phase
       // applyConfig();
    }

    ~KCSPower() {
        delete[] ram;
    }

    uint8_t* ram   = nullptr;
    uint8_t  config = CMODE_16KGAME;

    // Apply configuration mode to GAME / EXROM lines
    // Mirrors VICE: cart_config_changed_slotmain()
    void applyConfig()
    {
        // EXROM = (config >> 1) & 1
        // GAME  = (config & 1) ^ 1
        bool ex = (config & 0x02) != 0;
        bool ga = ((config & 0x01) == 0);  // 0 -> GAME=1, 1 -> GAME=0

        exRom = ex;
        game  = ga;

        system->changeExpansionPortMemoryMode(exRom, game);
    }

    auto peekIo1(uint16_t addr) -> uint8_t override
    {
        // ROM mirror at $9E00-$9EFF (second-to-last page of ROML)
        uint16_t romAddr = 0x1E00 | (addr & 0x00FF);
        return Cart::readRomL(romAddr);
    }

    // --- IO1: $DE00-$DEFF ---

    auto readIo1(uint16_t addr) -> uint8_t override
    {
        // (addr & 2) selects configuration:
        //   0 = 8K GAME mode
        //   1 = RAM mode
        config = (addr & 0x0002) ? CMODE_RAM : CMODE_8KGAME;
        applyConfig();

        // ROM mirror at $9E00-$9EFF (second-to-last page of ROML)
        uint16_t romAddr = 0x1E00 | (addr & 0x00FF);
        return Cart::readRomL(romAddr);
    }

    auto writeIo1(uint16_t addr, uint8_t /*value*/) -> void override
    {
        // (addr & 2) selects configuration:
        //   0 = 16K GAME mode
        //   1 = ULTIMAX mode (freeze mode)
        config = (addr & 0x0002) ? CMODE_ULTIMAX : CMODE_16KGAME;
        applyConfig();
    }

    auto peekIo2(uint16_t addr) -> uint8_t override
    {
        return readIo2( addr );
    }

    // --- IO2: $DF00-$DFFF ---

    auto readIo2(uint16_t addr) -> uint8_t override
    {
        uint8_t ofs = static_cast<uint8_t>(addr & 0x00FF);

        if (ofs & 0x80) {
            // Status readback:
            // bit 7 = EXROM
            // bit 6 = GAME
            // bit 0-5 = open bus (VIC-II phase 1)
            uint8_t v = 0;
            if (config & 0x02) v |= 0x80;          // EXROM
            if ((config & 0x01) == 0) v |= 0x40;   // GAME

            // Read open bus state (equivalent to vicii_read_phi1())
            uint8_t bus = ExpansionPort::readIo2(addr);
            v |= (bus & 0x3F);

            return v;
        }

        // $DF00-$DF7F: internal 128-byte RAM
        return ram[ofs & 0x7F];
    }

    auto writeIo2(uint16_t addr, uint8_t value) -> void override
    {
        uint8_t ofs = static_cast<uint8_t>(addr & 0x00FF);

        // $DF00-$DF7F: RAM write
        if (!(ofs & 0x80)) {
            ram[ofs & 0x7F] = value;
        }
        // $DF80-$DFFF: open area (ignored)
    }

    // --- Reset & Freeze handling ---

    auto reset(bool softReset = false) -> void override
    {
        // Standard ROM mapping (as in Cart::reset())
        cRomL = getChip(0);
        cRomH = chips.size() > 1 ? getChip(1) : getChip(0);

        // Power-up configuration: 16K GAME mode
        config = CMODE_16KGAME;
        applyConfig();

        if (!softReset && ram) {
            std::memset(ram, 0, 128);
        }
        resetFreeze();
    }

    auto didFreeze() -> void override
    {
        // Enter ULTIMAX mode on freeze (as in kcs_freeze())
        config = CMODE_ULTIMAX;
        applyConfig();

        // Release NMI latch for multi-freeze capability (Denise-specific)
        nmiCall(false);
    }

    auto serializeSwitchedIn(Emulator::Serializer& s) -> void override
    {
        FreezeButton::serialize(s);
        s.integer(config);
        s.array(ram, 128);
    }
};

} // namespace LIBC64
