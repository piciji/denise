
#pragma once

#include <cstdint>
#include "expansionPort.h"
#include "../drive/hardDrive.h"
#include "hdBase.h"

namespace LIBAMI {

    struct Agnus;

    struct HDController : ExpansionPort {        

        enum class Ident { BuiltIn, BuiltInRDB, MtecAT500 } ident;

        HardDrive& hardDrive;
        HDBase* pcb;

        HDController(Agnus& agnus, HardDrive& hardDrive) : hardDrive(hardDrive), ExpansionPort(agnus) {
            pcb = new HDBase(*this, hardDrive);
        }

        auto pages() const -> unsigned { return 1; }

        auto readAutoConf(uint32_t addr) -> uint8_t;

        auto read(uint32_t addr) -> uint8_t;

        auto readW(uint32_t addr) -> uint16_t;

        auto write(uint32_t addr, uint8_t data) -> void;

        auto writeW(uint32_t addr, uint16_t data) -> void;

        auto reset(bool softReset) -> void;

        auto createPCB(Ident ident) -> void;

        auto serialize(Emulator::Serializer& s, bool light = false) -> void;
    };

}
