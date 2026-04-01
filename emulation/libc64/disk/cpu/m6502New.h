
#pragma once

/**
 * special emulation of the 6502 core
 *
 * gives back control to the caller before a possible read/write to VIA
 * this is usefully when emulating more than one 6502 CPU same time.
 * alternatively you could run each CPU in a thread but that is costly.
 * furthermore generating savestates is difficult because you can not restore
 * the stackframe, means you are forced to restart at clean opcode edge. that could
 * result that both 6502 are not correctly synced when resuming emulation.
 *
 * this approach has it's flaws too. code is more complex because you need to
 * programmatically resume execution at a later cycle. by jumping out to the caller
 * and jumping back in to next cycle, execution speed is slower of course.
 *
 * NOTE: this approach is customized for the needs of 1541 emulation. there is no
 * need to step out each cycle. It's enough to step out before a possible VIA read/write
 * and before the step which samples interrupts (last one in most cases).
 * address generation will not be interrupted. (no IEC bus dependency)
 * interrupt processing will not be interrupted. (no IEC bus dependency)
 * RDY is not used by 1541 and therefore not reworked in this approach.
 * so we have the chance to give back control to C64 before a read/write to time
 * VIA accesses between C64 and drives in a cycle accurate way.
 *
 */

#include <cstdint>
#include "../../../tools/serializer.h"

namespace LIBC64 {

struct Drive;
struct System;

struct M6502New {
    M6502New(System* system, Drive* drive) : system(system), drive(drive) {}

    System* system;
    Drive* drive;

    enum { Normal = 0, IRQ = 1, Halt = 2, ResetRoutine = 4,
        WatchPoint = 8, WatchPointWrite = 0x10, BreakPoint = 0x20, ExceptionPoint = 0x40, SoftStop = 0x80, ModifiedCode = 0x100,
        History = 0x200
    };

    uint16_t pc;

    uint8_t regX;

    uint8_t regY;

    uint8_t regA;

    uint8_t regS;

    uint8_t regP;

    uint8_t flagZ;

    uint8_t flagN;

    bool readNext;

    uint8_t zeroPage;
    uint16_t absolute;
    uint16_t absIndexed;
    uint8_t dataBus;
    uint8_t _value;

    uint8_t soBlock;

    uint8_t soSample = 0;

    int control;
    bool irqPending;
    bool nmiPending;

    int operation;

    auto process() -> void;

    auto isReadNext() const -> bool { return readNext; }

    template<bool software = false> auto interrupt() -> void;

    auto power() -> void;

    auto reset() -> void;

    auto setStatus(uint8_t val) -> void;

    auto resetRoutine() -> void;

    auto setIrq(bool state) -> void;

    auto setNmi() -> void;

    auto triggerSO(uint8_t delay = 1) -> void;

    auto handleSo() -> void;

    auto serialize(Emulator::Serializer& s) -> void;
};

}
