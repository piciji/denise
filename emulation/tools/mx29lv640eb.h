
#pragma once

#include "systimer.h"

#define MX29LV_SECTORS 135

namespace Emulator {

    struct MX29LV640EB {

        MX29LV640EB();

        enum class State {  Read, unlock1, unlock2, EraseUnlock1, EraseUnlock2, AutoSelect, Program, ProgramError,
            EraseSelect, ChipErase, SectorEraseTimeout, SectorEraseSuspend, SectorErase } state;

        using Callback = std::function<void ()>;
        Callback erase;
        Callback written;

        SystemTimer* events;
        uint8_t* data = nullptr;
        uint8_t byteToProgram;
        uint8_t eraseMask[MX29LV_SECTORS];

        uint32_t size;
        uint8_t manufacturerId;
        uint8_t deviceId;
        uint32_t unlock1Addr;
        uint32_t unlock2Addr;
        uint32_t unlock1Mask;
        uint32_t unlock2Mask;
        unsigned eraseSectorTimeoutCycles;
        unsigned eraseSectorCycles;
        unsigned eraseChipCycles;
        bool autoSelect;

        auto setData( uint8_t* data ) -> void;

        auto setEvents( SystemTimer* events ) -> void;

        auto reset() -> void;

        auto unlock1(uint32_t addr) -> bool;

        auto unlock2(uint32_t addr) -> bool;

        auto programByte(uint32_t addr, uint8_t value) -> bool;

        auto addSectorForErase(uint32_t addr) -> void;

        auto getSectorByAddr(uint32_t addr) -> uint8_t;

        auto clearEraseMask() -> void;

        auto read( uint32_t addr ) -> uint8_t;

        auto write(uint32_t addr, uint8_t value) -> void;

        auto endCommand() -> void;

        auto serialize(Emulator::Serializer& s) -> void;
    };

}
