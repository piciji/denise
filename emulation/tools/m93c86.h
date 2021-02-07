
#pragma once

#include "systimer.h"

#define M93C86_SIZE (2 * 1024)

namespace Emulator {

struct M93C86 {

    M93C86();
    ~M93C86();

    enum class Command : uint8_t { None, Read, Write, Erase, WriteEnable, WriteDisable, WriteAll, EraseAll } command;
    // status for self timed cycles
    enum class Status : uint8_t { Init, Pending, Busy, Ready } status;

    using Callback = std::function<void ()>;
    Callback writeCycle;
    Callback written;

    bool dirty;

    bool cs;
    bool org;
    bool clockState;
    bool dataIn;
    bool dataOut;

    uint8_t* data;
    uint8_t clockCount;
    unsigned commandReg;
    bool enableWrite;
    uint16_t addr;
    uint16_t value;
    uint8_t outShiftReg;

    SystemTimer* events;

    auto reset() -> void;
    auto orgSelect(bool state) -> void;
    auto chipSelect(bool state) -> void;
    auto setData( uint8_t* data ) -> void;

    auto clock(bool state) -> void;
    auto clock() -> void;

    auto read() -> bool;
    auto write(bool state) -> void;

    auto resetCommandReg() -> void;
    auto setEvents( SystemTimer* events ) -> void;

    auto serialize(Emulator::Serializer& s) -> void;
};

}
