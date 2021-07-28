
#pragma once

#include "../expansionPort.h"
#include "../../../tools/pia.h"
#include "../../disk/via/via.h"

namespace LIBC64 {

struct Fastloader : ExpansionPort {
    Fastloader();

    enum Type { PROF_DOS = 0, PROLOGIC_DOS = 1 } type;
    Emulator::Pia pia;
    Via via;
    Emulator::Interface::Media* media = nullptr;
    unsigned romSize = 0;
    uint8_t* rom = nullptr;
    bool kernalJumper; // 1: use expansion kernal with hiram line, 0: c64 kernal

    auto reset(bool softReset = false) -> void;
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    auto readIo1( uint16_t addr ) -> uint8_t;
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto readRomH( uint16_t addr ) -> uint8_t;
    auto clock() -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    auto setJumper( bool state ) -> void;
    auto getJumper( ) -> bool;

    auto hasHiramCableConnected() -> bool;

};

extern Fastloader* fastloader;

}
