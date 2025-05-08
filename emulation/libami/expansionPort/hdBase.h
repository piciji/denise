
#pragma once

#include <cstdint>

namespace LIBAMI {

struct HardDrive;

struct HDController;

struct HDBase {
    HDBase(HDController& board, HardDrive& hardDrive) : board(board), hardDrive(hardDrive) { rom = nullptr; }

    uint8_t* rom = nullptr;        

    HDController& board;

    HardDrive& hardDrive;

    virtual auto readAutoConf(uint32_t addr) -> uint8_t { return 0xff; }

    virtual auto read(uint32_t addr) -> uint8_t { return 0xff; }

    virtual auto readW(uint32_t addr) -> uint16_t { return 0xffff; }

    virtual auto write(uint32_t addr, uint8_t data) -> void {}

    virtual auto writeW(uint32_t addr, uint16_t data) -> void {}

    virtual auto reset() -> void {}

    virtual auto serialize(Emulator::Serializer& s, bool light = false) -> void {}
};

}
