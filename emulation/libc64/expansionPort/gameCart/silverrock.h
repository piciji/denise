
#pragma once

namespace LIBC64 {

    struct Silverrock : GameCart {

        Silverrock(System* system) : GameCart(system, true, false) {}

        auto peekRomL( uint16_t addr ) -> uint8_t override {
            return readRomL(addr);
        }

        auto readRomL( uint16_t addr ) -> uint8_t override {
            // tested with original Hugo image in Fallborg's package from CSDb
            addr = ((addr & 1) << 10) |
                    ((addr & 2) << 10) |
                    ((addr & 4) << 7) |
                    ((addr & 8) << 5) |
                    ((addr & 0x10) >> 4) |
                    ((addr & 0x20) >> 4) |
                    ((addr & 0x40) >> 4) |
                    ((addr & 0x80) >> 4) |
                    ((addr & 0x100) >> 4) |
                    ((addr & 0x200) >> 4) |
                    ((addr & 0x400) >> 4) |
                    ((addr & 0x800) >> 4) |
                    ((addr & 0x1000) << 0);

            uint8_t data = Cart::readRomL(addr);

            return ((data & 8) >> 3) | ((data & 4) >> 1) | ((data & 2) << 1) | ((data & 1) << 3) | (data & 0xf0);
        }

        auto writeIo1( uint16_t addr, uint8_t data ) -> void override {
            uint8_t bank;
            addr &= 0xf;
            data >>= 4;

            if (addr == 0) { // REV01
                if (data < 8)
                    bank = data << 1;
                else
                    bank = data - (15 - data);
            } else
                bank = addr; // REV02 (found in VICE) is untested

            cRomL = getChip(bank);
        }

        auto reset(bool softReset = false) -> void override {
            cRomL = getChip(0);
        }
    };

}
