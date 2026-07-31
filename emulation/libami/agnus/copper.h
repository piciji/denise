
#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include "../../tools/watcher.h"
#include "../../interface.h"
#include "../system/debuggerSnapshot.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;
struct Blitter;
typedef Emulator::Interface::DebuggerAction DebuggerAction;

struct Copper {

    Copper(Agnus& agnus);

    // 0x80 allocate Copper if BUS is free
    // 0x40 allocate Copper if BUS is free and no long gap
    enum State {
        Off                             = 0,

        Strobe_VBL_1                    = 1,
        Strobe_VBL_2                    = 2 | 0x80,
        Strobe_VBL_3                    = 3,
        Strobe_VBL_4                    = 4 | 0x80,
        Strobe_VBL_5                    = 5,
        Strobe_VBL_6                    = 6 | 0x80,
        Strobe_VBL_7                    = 7 | 0x80,

        Strobe_CPU_1                    = 8,
        Strobe_CPU_2                    = 9 | 0x80,
        Strobe_CPU_3                    = 10,
        Strobe_CPU_4                    = 11,
        Strobe_CPU_5                    = 12 | 0x80,
        Strobe_CPU_6                    = 13,

        Read1                           = 14 | 0x80,
        Read2                           = 15 | 0x80,
        Skip1                           = 16,
        Skip2                           = 17,
        Wait1                           = 18,
        Wait2                           = 19,
        Wait3                           = 20,
        Wait4                           = 21,

        WARMUP                          = 22,
        WARMUP2                         = 23,
        Read1FromSkip                   = 24 | 0x80,
        Read1Buggy                      = 25 | 0x80,
    } state, prevState;

    Agnus& agnus;
    Blitter& blitter;

    uint16_t cdang;
    uint8_t strobeCop;
    uint32_t cop1lc;
    uint32_t cop2lc;
    uint32_t copPtr;
    uint32_t copPtrEdge;
    uint32_t copPtrBefore;

    uint16_t ir1;
    uint16_t ir2;

    struct {
        uint8_t hPos;
        uint16_t vPos;
        uint8_t hMask;
        uint16_t vMask;
    } comp;

    bool skipped;

    Emulator::WatchPoints watchPoints = Emulator::WatchPoints();
    Emulator::WatchPoints breakPoints = Emulator::WatchPoints();
    std::unordered_map<uint32_t, uint32_t> lists;
    std::pair<const uint32_t, uint32_t>* list1 = nullptr;
    std::pair<const uint32_t, uint32_t>* list2 = nullptr;
    int listUse = 0;

    enum DebuggerState {None = 0, WatchPoint = 1, BreakPoint = 2, SoftStep = 4, LogList = 8};
    int debuggerState;

    auto debuggerAdd(DebuggerAction action, unsigned ident, unsigned addr, unsigned endAddr) -> void;
    auto debuggerRemove(DebuggerAction action, unsigned ident) -> void;
    auto debuggerRemove(DebuggerAction action) -> void;
    auto updateDmaSnapshot(DebuggerSnapshot& snap) -> void;
    auto checkBreakpoints() -> void;

    auto setCopCon( uint16_t value ) -> void;

    auto process() -> void;
    template<bool wait = true> auto compare() -> bool;

    auto setCOP1LCH(uint16_t value) -> void;
    auto setCOP1LCL(uint16_t value) -> void;
    auto setCOP2LCH(uint16_t value) -> void;
    auto setCOP2LCL(uint16_t value) -> void;

    auto strobeCOPJMP(uint8_t pos, uint8_t triggeredBy ) -> void;
    auto assignCopPtr() -> void;
    auto cycle1() -> void;

    auto blitterBusyUpdate() -> void;
    auto reset() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto flagDebugAction(int action, bool state) -> void;

};

}
