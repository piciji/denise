
#pragma once

#include "freezer.h"

namespace LIBC64 {

struct ActionReplayMK2 : Freezer {

    ActionReplayMK2(System* system) : Freezer(system, true, false) {

    }

    uint8_t enableCounter = 0;
    uint8_t disableCounter = 0;
    bool enable = true;

    auto readIo1( uint16_t addr ) -> uint8_t {

        _enable();

        return ExpansionPort::readRomL( addr );
    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void {

        _enable();
    }

    auto peekIo2( uint16_t addr ) -> uint8_t {
        auto chip = getChip(1);

        if (!chip)
            return ExpansionPort::readRomL( addr );

        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank

        return *(chip->ptr + addr);
    }

    auto readIo2( uint16_t addr ) -> uint8_t {
        _disable();

        auto chip = getChip(1);

        if (!chip)
            return ExpansionPort::readRomL( addr );

        addr = (0x1f << 8) | (addr & 0xff); // last page of selected rom bank

        return *(chip->ptr + addr);
    }

    auto writeIo2( uint16_t addr, uint8_t value ) -> void {
        _disable();
    }

    auto _enable() -> void {
        // could be re enabled without a freeze
        if (++enableCounter == 60) {
            cRomH = cRomL = getChip(1);
            enableCounter = 0;
            game = true;
            exRom = false;
            enable = true;
            system->changeExpansionPortMemoryMode(exRom, game);
        }

        disableCounter = 0;
    }

    auto _disable() -> void {
        if (!enable)
            return;

        if (++disableCounter == 125) {
            disableCounter = 0;
            game = true;
            exRom = true;
            enable = false;
            system->changeExpansionPortMemoryMode(exRom, game);
        }
    }

    auto didFreeze() -> void {
        nmiCall(false);
        cRomH = cRomL = getChip(0);
        enableCounter = 0;
        disableCounter = 0;
        enable = true;
    }

    auto reset(bool softReset = false) -> void {
        cRomH = cRomL = getChip(1);
        enableCounter = 0;
        disableCounter = 0;
        enable = true;
        resetFreeze();
    }

    auto serializeSwitchedIn(Emulator::Serializer& s) -> void {

        FreezeButton::serialize( s );

        s.integer(enable);
        s.integer(enableCounter);
        s.integer(disableCounter);
    }

};

}
