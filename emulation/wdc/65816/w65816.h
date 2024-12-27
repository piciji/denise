
#pragma once

#include <cstdint>

// RDY input line is tested in every cycle, and this affects performance.
// therefore, it should only be activated when it is actually being used.
#define SUPPORT_RDY

//#define W65816_REF SuperCPU
//#define W65816_REF_NS LIBC64   // optional
//#define W65816_REF_TYPE struct // or class
//#define W65816_REF_INCLUDE "../expansionPort/SuperCPU/SuperCPU.h"

#ifdef W65816_REF
    #ifdef W65816_REF_NS
        namespace W65816_REF_NS { W65816_REF_TYPE W65816_REF; }
    #else
        W65816_REF_TYPE W65816_REF;
        #define W65816_REF_NS
    #endif
#endif

namespace WDCFAMILY {

struct W65816 {

    auto power() -> void;
    auto process() -> void;
    auto setNmiLineLow(bool state) -> void;
    auto setIrqLineLow(bool state) -> void;
    auto setRdyLineLow(bool state) -> void;

    enum {  RESET = 1, WAI = 2, STP = 4, IRQ_LINE = 8, NMI_LINE = 0x10, RDY_LINE = 0x20,
            NMI_TRANSITION = 0x40, IRQ_PENDING = 0x80, NMI_PENDING = 0x100 };

    enum {  LDA = 1, LDX, LDY, ORA, AND, EOR, ADC, SBC, CMP, CPX, CPY,
            ROL, ROR, ASL, LSR, DEC, INC, TSB, TRB, BIT, BIT_IM,
            STA, STZ, STX, STY };

    enum {  NONE = 0, INDEX_X = 1, INDEX_Y = 2 };

    enum { SAMPLE_INTR = 1, SET_FLAG_I = 2, CLEAR_FLAG_I = 4, NATIVE = 8 };

protected:
#ifdef W65816_REF
    W65816(W65816_REF_NS::W65816_REF& ref) : ref(ref) { }
    W65816_REF_NS::W65816_REF& ref;
#else
    W65816() { }
#endif
    struct RegP {
        bool c;
        bool z;
        bool i;
        bool d;
        bool x;
        bool m;
        bool v;
        bool n;

        operator uint8_t() const {
            return c | z << 1 | i << 2 | d << 3 | x << 4 | m << 5 | v << 6 | n << 7;
        }

        auto& operator=(uint8_t data) {
            c = data & 1;
            z = data & 2;
            i = data & 4;
            d = data & 8;
            x = data & 0x10;
            m = data & 0x20;
            v = data & 0x40;
            n = data & 0x80;
            return *this;
        }
    };

    uint16_t pc;
    uint16_t a;
    uint16_t x;
    uint16_t y;
    uint16_t s;
    uint16_t d;

    uint8_t pbr;
    uint8_t dbr;
    RegP p;

    bool modeE;
    int control;
    int lines;

    template<bool hardware = true> auto interrupt(const uint16_t& vector) -> void;

    auto checkForInterrupt() -> void;

    template<uint8_t actions = 0> inline auto idle() -> void;
    template<uint8_t actions = 0> inline auto read(uint32_t addr) -> uint8_t;
    template<uint8_t actions = 0> inline auto write(uint32_t addr, uint8_t value) -> void;
    template<uint8_t actions = 0> inline auto readBank(uint32_t addr) -> uint8_t;
    template<uint8_t actions = 0> inline auto writeBank(uint32_t addr, uint8_t data) -> void;
    template<uint8_t actions = 0> inline auto readPC() -> uint8_t;
    template<uint8_t actions = 0> auto readStack(uint32_t addr) -> uint8_t;
    template<uint8_t actions = 0> auto writeStack(uint32_t addr, uint8_t data) -> void;
    template<uint8_t actions = 0> auto push(uint8_t data) -> void;
    template<uint8_t actions = 0> auto pull() -> uint8_t;

    inline auto directAdr(uint32_t addr) -> uint32_t;
    auto getDirectAddressIndirect(uint32_t offset) -> uint16_t;

    inline auto decByteL(uint16_t& reg) -> void;
    inline auto setByteL(uint16_t& reg, uint8_t byte) -> void;
    inline auto setByteH(uint16_t& reg, const uint8_t& byte) -> void;
    inline auto incByteL(uint16_t& reg) -> void;

    inline auto idle2() -> void;
    inline auto idle4(const uint16_t a1, const uint16_t a2) -> void;
    inline auto idle6(uint16_t address) -> void;
    inline auto idleIrq() -> void;

    template<bool M, uint8_t Inst> auto arithmetic(uint16_t data) -> void;
    template<bool M, uint8_t Inst> auto arithmeticM(uint16_t& reg) -> void;

    // opcodes
    template<bool JSR> auto opJmpAbsIndexedIndirect() -> void;
    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opAbsolute() -> void;
    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opModifyAbsolute() -> void;
    template<bool JML> auto opJmpIndirect() -> void;
    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opLong() -> void;
    template<bool M, bool Mvn> auto opMove() -> void;
    template<bool M, uint8_t Inst> auto opIndexedIndirect() -> void;
    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opDirect() -> void;
    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opModifyDirect() -> void;
    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opIndirect() -> void;
    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opIndirectLong() -> void;
    template<bool M, uint8_t Inst> auto opImmediate() -> void;
    auto opBranch(bool take) -> void;
    auto opBranchLong() -> void;
    template<bool M, uint8_t Inst> auto opStackRelative() -> void;
    template<bool M, uint8_t Inst, uint8_t Index> auto opStackRelativeIndirect() -> void;

    template<bool M, uint8_t Inst, uint8_t Index = 0> auto opImplied() -> void;
    auto opTXS() -> void;
    auto opTSX() -> void;
    auto opTYX() -> void;
    auto opTSC() -> void;
    auto opTCS() -> void;
    auto opTCD() -> void;
    auto opTDC() -> void;
    auto opTXA() -> void;
    auto opTYA() -> void;
    auto opTXY() -> void;
    auto opTAY() -> void;
    auto opTAX() -> void;
    template<bool M, uint8_t Inst> auto opPush() -> void;
    auto opPHD() -> void;
    auto opPHP() -> void;
    auto opPHK() -> void;
    auto opPHB() -> void;

    auto opPLP() -> void;
    auto opPLD() -> void;
    auto opPLB() -> void;
    template<bool M, uint8_t Inst> auto opPull() -> void;

    auto opJSR() -> void;
    auto opJSL() -> void;
    auto opRTI() -> void;
    auto opRTS() -> void;
    auto opRTL() -> void;

    auto opJmpAbsolute() -> void;
    auto opJmpAbsoluteLong() -> void;

    auto opWait() -> void;
    auto opStop() -> void;
    auto opWDM() -> void;
    auto opNOP() -> void;

    auto opClear(bool& flag) -> void;
    auto opSet(bool& flag) -> void;
    auto opPer() -> void;
    auto opResetP() -> void;
    auto opSetP() -> void;
    auto opPushEffectiveIndirectAddress() -> void;
    auto opPushEffectiveAddress() -> void;

    auto opXBA() -> void;
    auto opXCE() -> void;

    auto opBRK() -> void;
    auto opCOP() -> void;
    template<bool setI> auto opUpdateI() -> void;

#ifndef W65816_REF
    virtual auto readByte(uint32_t addr) -> uint8_t = 0;
    virtual auto writeByte(uint32_t addr, uint8_t value) -> void = 0;
    virtual auto sync() -> void = 0;

    // optional
    virtual auto outputRDYLineLow() -> void {} // RDY is bi-directional
    virtual auto setMemoryLock(bool state) -> void {} // MLB hints other BUS participants not to interfere RMW
#endif

};

}
