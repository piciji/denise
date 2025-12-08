
#pragma once

#include "../cart/cart.h"
#include "../../../tools/pia.h"
#include "../../disk/via/via.h"

#define FASTLOADER_VIA 1
#define FASTLOADER_PIA 2
#define FASTLOADER_IO1 4
#define FASTLOADER_IO2 8
#define FASTLOADER_OUTPINA 0x10
#define FASTLOADER_OUTPINB 0x20
#define FASTLOADER_PORTA 0x40
#define FASTLOADER_PORTB 0x80
#define FASTLOADER_CHARGE 0x100

#define FASTLOADER_PIA_PORT_A (FASTLOADER_PIA | FASTLOADER_OUTPINA | FASTLOADER_PORTA)
#define FASTLOADER_VIA_PORT_B (FASTLOADER_VIA | FASTLOADER_OUTPINB | FASTLOADER_PORTB)
#define FASTLOADER_VIA_PORT_A (FASTLOADER_VIA | FASTLOADER_OUTPINA | FASTLOADER_PORTA)

#define FASTLOADER_PIA_IO1 (FASTLOADER_PIA | FASTLOADER_IO1)
#define FASTLOADER_VIA_IO2 (FASTLOADER_VIA | FASTLOADER_IO2)

#define FASTLOADER_CHARGE_IO1 (FASTLOADER_CHARGE | FASTLOADER_IO1)
#define FASTLOADER_CHARGE_IO2 (FASTLOADER_CHARGE | FASTLOADER_IO2)

namespace LIBC64 {

struct Fastloader : Cart {
    Fastloader(System* system);

    unsigned mode;
    Emulator::Pia pia;
    Via via;
    unsigned voltage = 0;
    unsigned chargeClock = 0;
    bool kernalJumper; // 1: use expansion kernal with hiram line, 0: c64 kernal

    auto reset(bool softReset = false) -> void;
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    auto peekIo1( uint16_t addr ) -> uint8_t;
    auto readIo1( uint16_t addr ) -> uint8_t;
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    auto peekIo2( uint16_t addr ) -> uint8_t;
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto readRomL( uint16_t addr ) -> uint8_t;
    auto peekRomL( uint16_t addr ) -> uint8_t;
    auto readRomH( uint16_t addr ) -> uint8_t;
    auto peekRomH( uint16_t addr ) -> uint8_t;
    auto clock() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart*;
    auto assign(Cart* cart) -> void {}
    auto protectFromDeletion() -> bool { return true; }

    auto setJumper( unsigned jumperId, bool state ) -> void;
    auto getJumper( unsigned jumperId ) -> bool;

    auto hasHiramCableConnected() -> bool;

    auto hasCustomButton() -> bool { return true; } // NMI of Turbo Trans to swap ram disks
    auto customButton() -> void;

    auto autoCharge() -> void;
    auto charge() -> void;
    auto discharge() -> void;

};

}
