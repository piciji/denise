
#pragma once

#include <cstdint>
#include "hdController.h"
#include "hdBase.h"
#include "../drive/hardDrive.h"

namespace LIBAMI {

struct Agnus;

struct MtecAT500 : HDBase {
    
    MtecAT500(HDController& board, HardDrive& hardDrive) : HDBase(board, hardDrive) {}

    ~MtecAT500();

    auto readAutoConf(uint32_t addr) -> uint8_t;

    auto read(uint32_t addr) -> uint8_t;

    auto readW(uint32_t addr) -> uint16_t;

    auto write(uint32_t addr, uint8_t data) -> void;

    auto writeW(uint32_t addr, uint16_t data) -> void;

    auto reset() -> void;

};

}
