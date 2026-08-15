
#pragma once

#include "../cart/cart.h"
#include "../../../tools/flash040.h"
#include "../../../tools/m93c86.h"

namespace LIBC64 {

struct Gmod2 : Cart {

    Gmod2(System* system);
    ~Gmod2();

	Emulator::Interface::Media* mediaSecondary;
    uint8_t* romSecondary;

    Emulator::Flash040 flash;
    Emulator::M93C86 eeprom;

    uint8_t* flashData;
    uint8_t* eepromData;

    uint8_t bank;

    bool writeEnable;

    bool writeProtect;
    bool writeProtectEeprom;

    auto init() -> void;
    auto readIo1( uint16_t addr ) -> uint8_t;
    auto peekIo1( uint16_t addr ) -> uint8_t;
    auto writeIo1( uint16_t addr, uint8_t value ) -> void;

    auto clock() -> void;

    auto readRomL( uint16_t addr ) -> uint8_t;
    auto peekRomL( uint16_t addr ) -> uint8_t;
    auto writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void;

    auto reset(bool softReset = false) -> void;
    auto prepare() -> void;
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto setEeprom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto write() -> void;
    auto writeEeprom() -> void;

    static auto createImage(unsigned& imageSize, uint8_t id) -> uint8_t*;

    auto serialize(Emulator::Serializer& s) -> void;

    auto setWriteProtect(bool state) -> void;
    auto isWriteProtected() -> bool;

    auto setSecondaryWriteProtect(bool state) -> void;
    auto isSecondaryWriteProtected() -> bool;
	
	auto hasSecondaryRom() -> bool { return true; }

    auto getSizeNotConsideredForMemorySerialization() -> unsigned;

    auto isBootable( ) -> bool { return true; }
};

}
