
/**
 * Epyx FastLoad
 *
 * Written by
 *  aaciii
 *
 **/

#pragma once

#include "gameCart.h"
#include "../../system/system.h"

namespace LIBC64 {

struct EpyxFastLoad : GameCart {
    // The RC network is approximated by a 512-cycle retriggerable timer.
    static constexpr unsigned RomCycles = 512;
    unsigned cyclesRemaining = RomCycles;

    EpyxFastLoad(System* system) : GameCart(system, true, false) {}

    auto triggerAccess() -> void {
        cyclesRemaining = RomCycles;
        if (exRom || !game)
            system->changeExpansionPortMemoryMode(exRom = false, game = true);
    }

    auto reset(bool softReset = false) -> void override {
        Cart::reset(softReset);
        triggerAccess();
    }

    auto clock() -> void override {
        if (cyclesRemaining && --cyclesRemaining == 0)
            system->changeExpansionPortMemoryMode(exRom = true, game = true);
    }

    auto readRomL(uint16_t addr) -> uint8_t override {
        triggerAccess();
        return Cart::readRomL(addr & 0x1fff);
    }

    auto peekRomL(uint16_t addr) -> uint8_t override {
        return Cart::readRomL(addr & 0x1fff);
    }

    auto readIo1(uint16_t addr) -> uint8_t override {
        triggerAccess();
        // IO1 selects the RC circuit but does not drive the data bus.
        return ExpansionPort::readIo1(addr);
    }

    auto peekIo1(uint16_t addr) -> uint8_t override {
        return ExpansionPort::peekIo1(addr);
    }

    auto readIo2(uint16_t addr) -> uint8_t override {
        return Cart::readRomL(0x1f00 | (addr & 0xff));
    }

    auto peekIo2(uint16_t addr) -> uint8_t override {
        return Cart::readRomL(0x1f00 | (addr & 0xff));
    }

    auto serializeSwitchedIn(Emulator::Serializer& s) -> void override {
        GameCart::serializeSwitchedIn(s);
        s.integer(cyclesRemaining);
    }

    auto isBootable( ) -> bool override {
        return false;
    }

    auto bootSpeed() -> float override {
        return 0.5;
    }
};

}
