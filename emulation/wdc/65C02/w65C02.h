
#pragma once

#include <cstdint>

// SOB is tested in every cycle and this affects performance.
// therefore, it should only be activated when it is actually being used.
//#define SUPPORT_SOB

// use this if you want communication to be about references instead of inheritance.
// put reference in constructor

//#define REF reference
//#define REF_NS namespace of reference // optional
//#define REF_TYPE struct // or class
//#define REF_INCLUDE "include path of reference class"

#ifdef REF
    #ifdef REF_NS
        namespace REF_NS { REF_TYPE REF; }
    #else
        REF_TYPE REF;
        #define REF_NS
    #endif
#endif

namespace WDCFAMILY {

struct W65C02 {

    auto power() -> void;
    auto process() -> void;
    auto setNmiLineLow(bool state) -> void;
    auto setIrqLineLow(bool state) -> void;
    auto setRdyLineLow(bool state) -> void;
    auto setSobLineLow(bool state) -> void;

    enum {  RESET = 1, WAI = 2, STP = 4, IRQ_LINE = 8, NMI_LINE = 0x10, RDY_LINE = 0x20, SOB_LINE = 0x40,
            NMI_TRANSITION = 0x80, IRQ_PENDING = 0x100, NMI_PENDING = 0x200,
            SOB_TRANSITION = 0x400, SOB_BLOCK1 = 0x800, SOB_BLOCK2 = 0x1000};

    enum {  LDA = 1, LDX, LDY, LDD, ORA, AND, EOR, ADC, SBC, CMP, CPX, CPY,
            ROL, ROR, ASL, LSR, DEC, INC, TSB, TRB, BIT, BIT_IM,
            STA, STZ, STX, STY,
            SMB0, SMB1, SMB2, SMB3, SMB4, SMB5, SMB6, SMB7, RMB0, RMB1, RMB2, RMB3, RMB4, RMB5, RMB6, RMB7,
            BBR0, BBR1, BBR2, BBR3, BBR4, BBR5, BBR6, BBR7, BBS0, BBS1, BBS2, BBS3, BBS4, BBS5, BBS6, BBS7 };

    enum {  NONE = 0, INDEX_X = 1, INDEX_Y = 2 };

protected:
#ifdef REF
    W65816(REF_NS::REF& ref) : ref(ref) {}
    REF_NS::REF& ref;
#else
    W65C02() { }
#endif
    struct RegP {
        bool c;
        bool z;
        bool i;
        bool d;
        bool b;
        bool m;
        bool v;
        bool n;

        operator uint8_t() const {
            return c | z << 1 | i << 2 | d << 3 | b << 4 | m << 5 | v << 6 | n << 7;
        }

        auto& operator=(uint8_t data) {
            c = data & 1;
            z = data & 2;
            i = data & 4;
            d = data & 8;
            b = data & 0x10;
            m = data & 0x20;
            v = data & 0x40;
            n = data & 0x80;
            return *this;
        }
    };

    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    RegP p;

    int control;
    int lines;

    template<bool hardware = true> auto interrupt(const uint16_t& vector) -> void;
    auto checkForInterrupt() -> void;
    auto checkForSOB() -> void;

    template<bool sampleInterrupt = false> inline auto read(uint16_t addr) -> uint8_t;
    template<bool sampleInterrupt = false, bool writeStatus = false> inline auto write(uint16_t addr, uint8_t value) -> void;

    template<bool sampleInterrupt = false> inline auto readPC() -> uint8_t;
    template<bool sampleInterrupt = false> auto idle() -> void;
    template<bool sampleInterrupt = false, bool writeStatus = false> auto push(uint8_t data) -> void;
    template<bool sampleInterrupt = false> auto pull() -> uint8_t;

    template<uint8_t Inst> auto arithmetic(uint8_t data) -> void;
    template<uint8_t Inst> auto arithmeticM(uint8_t& reg) -> void;

    // opcodes
    template<uint8_t Inst, uint8_t Index = 0> auto opZeroPage() -> void;
    template<uint8_t Inst, uint8_t Index = 0> auto opModifyZeroPage() -> void;
    template<uint8_t Inst> auto opZeroPageIndexedIndirect() -> void;
    template<uint8_t Inst, uint8_t Index = 0> auto opZeroPageIndirect() -> void;
    template<uint8_t Inst> auto opImmediate() -> void;
    template<uint8_t Inst> auto opBB() -> void;
    auto opNoOp5c() -> void;
    auto opJmpAbsIndexedIndirect() -> void;
    template<uint8_t Inst, uint8_t Index = 0> auto opAbsolute() -> void;
    template<uint8_t Inst, uint8_t Index = 0> auto opModifyAbsolute() -> void;
    auto opJmpIndirect() -> void;
    auto opBranch(bool take) -> void;
    template<uint8_t Inst, uint8_t Index = 0> auto opImplied() -> void;
    auto opTXS() -> void;
    auto opTSX() -> void;
    auto opTYX() -> void;
    auto opTXA() -> void;
    auto opTYA() -> void;
    auto opTXY() -> void;
    auto opTAY() -> void;
    auto opTAX() -> void;
    template<uint8_t Inst> auto opPush() -> void;
    auto opPHP() -> void;
    auto opPLP() -> void;
    template<uint8_t Inst> auto opPull() -> void;
    auto opJSR() -> void;
    auto opRTI() -> void;
    auto opRTS() -> void;
    auto opJmpAbsolute() -> void;
    auto opWait() -> void;
    auto opStop() -> void;
    auto opNOP() -> void;
    auto opInvalid() -> void;
    auto opClearD() -> void;
    auto opClearC() -> void;
    auto opClearV() -> void;
    auto opSetC() -> void;
    auto opSetD() -> void;
    template<bool setI> auto opUpdateI() -> void;
    auto opBRK() -> void;

#ifndef REF
    virtual auto readByte(uint32_t adr) -> uint8_t = 0;
    virtual auto writeByte(uint32_t adr, uint8_t value) -> void = 0;
    virtual auto updateRDY(bool state) -> void {} // RDY is bi-directional
    virtual auto setMemoryLock(bool state) -> void {} // hint to other BUS participants not to interfere RMW
#endif

};

}
