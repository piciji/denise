
#pragma once

#include "../cart/cart.h"
#include "../../../wdc/65C02/w65C02.h"

namespace LIBC64 {

struct FinalChessCard : Cart, WDCFAMILY::W65C02 {
    FinalChessCard(System* system);
    ~FinalChessCard();

    Emulator::Interface::Media* mediaWrite;
    bool writeProtectRAM;
    unsigned cycles;
    uint8_t latch;
    uint8_t latch2;
    bool writeProtectIO;
    unsigned MHz;
    uint8_t jumpers;
    unsigned frequency;

    uint8_t* ram = nullptr;
    uint8_t* romFCC = nullptr;

    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto setWriteProtect( bool state ) -> void;
    auto isWriteProtected() -> bool;

    auto readByte(uint16_t addr) -> uint8_t;
    auto writeByte(uint16_t addr, uint8_t value) -> void;
    auto idleCycle() -> void;

    auto reset(bool softReset = false) -> void;

    auto clock() -> void;

    auto writeIo2( uint16_t addr, uint8_t data ) -> void;
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto writeIo1( uint16_t addr, uint8_t data ) -> void;

    auto assumeChips( ) -> void;
    auto create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart*;
    auto assign(Cart* cart) -> void {}

    auto setJumper(unsigned jumperId, bool state) -> void;
    auto getJumper(unsigned jumperId) -> bool;

    auto serializeStep2(Emulator::Serializer& s) -> void;
    auto createImage(unsigned& imageSize) -> uint8_t*;
    auto hasSecondaryRom() -> bool { return true; }
    auto isBootable( ) -> bool { return true; }
};

}
