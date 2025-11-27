/*
 * missing features: BUS error
 *
 * external devices can raise BERR (BUS error) as a result of unwanted memory access. opcodes could be interrupted during each memory access.
 * prefetching often takes place at the same time as ALU operations, which leaves status flags with intermediate values.
 * for Amiga, BERR is wired to expansions port only. don't know if there are any expansions using this.
 * Atari can cause BUS errors.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include "dasmHandler.h"
#include "watcher.h"

// Explanations are below
// #define FC_SUPPORT
// #define ADR_EXC_BUS_CYCLE
// #define TAS_SPINLOCK

// a reference can be passed as an alternative to virtual methods. Modern compilers "should" be able to devirtualize.
// depending on Compiler, a reference could be faster

#define REF Agnus
#define REF_NS LIBAMI   // optional
#define REF_TYPE struct // or class
#define REF_INCLUDE "../../agnus/agnus.h"

#ifdef REF
    #ifdef REF_NS
        namespace REF_NS { REF_TYPE REF; }
    #else
        REF_TYPE REF;
        #define REF_NS
    #endif
#endif

namespace M68FAMILY {

class M68000 {
    typedef void (M68000::*OpTable)(uint16_t);
    typedef void (M68000::*OpDasm)(uint16_t, uint32_t&, DasmHandler&);

    OpTable opTable[0x10000] = {nullptr};
    OpDasm* dasmTable = nullptr;
    uint8_t* mulCycleLookup = nullptr;

    friend struct WatchPoints;
    friend struct ModifiedCodes;
    friend struct HistoryHandler;

protected:
#ifdef REF
    M68000(REF_NS::REF& ref) : ref(ref) { build(); }
    REF_NS::REF& ref;
#else
    M68000() { build(); }
#endif

    ~M68000();

    enum { Asl, Asr, Lsl, Lsr, Rol, Ror, Roxl, Roxr,
        Bchg, Bset, Bclr, Btst,
        Sub, Subx, Add, Addx, Not, Cmp, And, Or, Eor, Abcd, Sbcd,
        Adda, Suba, Cmpa,
        Mulu, Muls, Divu, Divs,
        Clr, Nbcd, Neg, Negx, Move, Movea,
        St, Sf, Shi, Sls, Scc, Scs, Sne, Seq,
        Svc, Svs, Spl, Smi, Sge, Slt, Sgt, Sle,
        Bra, Bhi, Bls, Bcc, Bcs, Bne, Beq, Bvc,
        Bvs, Bpl, Bmi, Bge, Blt, Bgt, Ble,
        Dbt, Dbf, Dbhi, Dbls, Dbcc, Dbcs, Dbne, Dbeq,
        Dbvc, Dbvs, Dbpl, Dbmi, Dbge, Dblt, Dbgt, Dble,
        Addi, Subi, Cmpi, Andi, Ori, Eori, Cmpm,
        Addq, Subq, Moveq, Movep,
        Bsr, Jmp, Jsr, Link, Unlk, Tas, Tst,
        Lea, Pea, Movem, Chk, Exg, Ext, Nop, Trap, Trapv,
        Reset, Rte, Rtr, Rts, _Stop, Swap,
    };

    enum { None = 0, SampleIPL = 1, SkipExtension = 2, TasCycle = 4,
           PRG = 8, ConcurrentAdrCalc = 16 /* happens while prefetching */, Reverse = 32 };
    enum { SF_DATA = 1, SF_PRG = 2, SF_EXC = 8, SF_READ = 16 };

public:
    enum { Byte = 1, Word = 2, Long = 4, BWL = 7, WL = 6, BW = 3 };

    enum {
        DataRegisterDirect = 0,
        AddressRegisterDirect = 1,
        AddressRegisterIndirect = 2,
        AddressRegisterIndirectWithPostIncrement = 3,
        AddressRegisterIndirectWithPreDecrement = 4,
        AddressRegisterIndirectWithDisplacement = 5,
        AddressRegisterIndirectWithIndex = 6,
        AbsoluteShort = 7 + 0,
        AbsoluteLong = 7 + 1,
        ProgramCounterIndirectWithDisplacement = 7 + 2,
        ProgramCounterIndirectWithIndex = 7 + 3,
        Immediate = 7 + 4,
    };

    enum { Normal = 0, IRQ = 1, Trace = 2, Halt = 4, Stop = 8,
        TraceScheduled = 0x10, IRQScheduled = 0x20, ResetRoutine = 0x40,
        WatchPoint = 0x80, BreakPoint = 0x100, ExceptionPoint = 0x200, SoftStop = 0x400, ModifiedCode = 0x800,
        History = 0x1000
    };

    enum class DebuggerAction { None, Breakpoint, Watchpoint, ExceptionPoint, Softstop, ModifiedCode, History };

protected:
    uint32_t regsD[8];
    uint32_t regsA[8];
    uint32_t pc;

    uint32_t usp;
    uint32_t ssp;

    uint16_t irc;
    uint16_t ird;

    bool c;
    bool v;
    bool z;
    bool n;
    bool x;

    uint8_t i;
    bool s;

    uint8_t iplPins;
    uint8_t iplSample;
    int control;

    WatchPoints watchPoints = WatchPoints(*this, WatchPoint);
    WatchPoints breakPoints = WatchPoints(*this, BreakPoint);
    WatchPoints exceptionPoints = WatchPoints(*this, ExceptionPoint);
    ModifiedCodes modifiedCode = ModifiedCodes(*this, ModifiedCode);
    HistoryHandler historyHandler = HistoryHandler(*this, History);
    std::optional<uint32_t> softStep = std::nullopt;

public:
    enum { USER_VECTOR = 0, AUTO_VECTOR = 1, UNINITIALIZED = 2,  SPURIOUS = -1};

    auto process() -> void;
    auto setInterrupt( uint8_t level ) -> void;
    auto reset() -> void;
    auto power() -> void;
    auto getFC2() -> bool { return s; }
    auto isHalted() -> bool { return !!(control & Halt); }
    auto setHalt() -> void; // I/O - could be halted from external device
    auto getCCR() -> uint8_t;
    auto getSR() -> uint16_t;

    auto debuggerStepOver() -> void;
    auto debuggerStepInto() -> void;
    auto debuggerAdd(DebuggerAction action, unsigned addr, unsigned addrTo = 0) -> void;
    auto debuggerRemove(DebuggerAction action, unsigned addr) -> void;
    auto debuggerEnable(DebuggerAction action, unsigned addr) -> void;
    auto debuggerDisable(DebuggerAction action, unsigned addr) -> void;
    auto debuggerDisableAll() -> void;

    auto disassemble(uint32_t addr, unsigned& bytes, uint16_t* memSnap = nullptr) -> std::string;
    auto disassembleData(uint32_t addr, unsigned words) -> std::string;
    auto disassembleTrace(unsigned i, uint16_t& flags) -> std::string;
    auto hasModifiedCode() -> bool { return modifiedCode.getAndForget(); }
    auto inDebugMode() -> bool {
        return control & (WatchPoint | BreakPoint | ExceptionPoint | SoftStop | History);
    }
    auto pcEdge() -> uint32_t { return (pc - 2) & 0xffffff; }

    // use this to calculate the needed wait states by terminating a BUS cycle with VPA line
    template<uint8_t phaseShift = 0> auto internalWaitCyclesBasedOnEClock(int eCyclePos) -> uint8_t;
    template<uint8_t phaseShift = 0> auto internalWaitCyclesBasedOnMainClockCycles(int clockCycles) -> uint8_t;
    
protected:
#ifndef REF
    // reset line is bi-directional
    virtual auto resetOut() -> void {}
    // sync to external devices in reasonable steps (until possible wait states could happen)
    // don't process any wait states in this method, because simply you have no hint if it is an internal cycle or not.
    // internal cycles can't be prolonged with wait states, only third CPU cycle of BUS cycle.
    virtual auto sync(unsigned cycles) -> void {}
    // methods to handle BUS cycles and wait states
    // each BUS cycle is terminated with DTACK, VPA or BERR line.
    // if it's not terminated until half of third cycle (S4) within BUS cycle, wait states will be added. (full CPU cycles)
    // DTACK: normal operation
    // BERR: BUS error (currently not emulated, except for IACK cycle)
    // VPA: compatibility to slow 6800 periphery, based on E-Clock (1/10 of main clock)
    // In addition to external wait states by not terminating the cycle in time,
    // for VPA there are internal wait states that cannot be avoided. The amount of cycles depends on the state of E-Clock.
    // for performance reasons, there is no extra method to handle the termination. so you need to take extra care for VPA termination.
    // use this helper function "internalWaitCyclesBasedOnEClock" to get amount of wait cycles. keep track of E-Clock position in derived class by yourself.
    // Note:
    // On real 68000 hardware the E-Clock initial phase at power up is non deterministic.
    // The clock is not affected by hardware reset. That means that the phase of the clock after non power up reset depends on
    // what was the phase before the reset and the number of cycles of the reset pulse.
    // after each cold start the phase of E-Clock could be different.
    virtual auto readByte(uint32_t adr) -> uint8_t = 0;
    virtual auto readWord(uint32_t adr) -> uint16_t = 0;
    virtual auto peekWord(uint32_t adr) -> uint16_t = 0;
    virtual auto writeByte(uint32_t adr, uint8_t data) -> void = 0;
    virtual auto writeWord(uint32_t adr, uint16_t data) -> void = 0;

    // BUS cycle to fetch the Interrupt vector.
    // in contrast to the BUS cycle operations above, this one returns the termination type.

    // for slower 6800 peripheral devices (like CIA, PIA) respond with VPA, which uses auto-vectored interrupts.
    // is synchronized to E-Clock and generates wait states internally. use helper "internalWaitCyclesBasedOnEClock"
    // BUS cycle needs between 10 - 19 clocks, depends on timing, when peripheral device raises VPA and the internal state of E-Clock.
    // if the BUS cycle is terminated too late, externally caused waiting cycles are added.
    // vector is fixed by CPU.
    // return AUTO_VECTOR;

    // respond with DTACK (like a normal 68000 cycle), which uses user-vectored interrupts
    // external device puts vector on data BUS after analyzing interrupt level from A1 - A3.
    // vector = 24 + level; // e.g. Amiga (often confused with auto-vectored interrupts because the same vectors are used.)
    // return USER_VECTOR;

    // uninitialized vector (technically a user-vectored interrupt)
    // 15 is a Motorola convention, a recommendation to the interrupt controllers if the peripheral has not been initialized with an appropriate vector.
    // external device puts vector on Data BUS.
    virtual auto iackCycle(uint8_t level, uint8_t& vector) -> int {
        vector = 15;
        return USER_VECTOR;
    }

    // respond with BERR (BUS error)
    // vector is fixed by CPU.
    // return SPURIOUS;

    // function codes: optional, when periphery needs to distinguish between data (FC0) and program (FC1) access.
    // like BERR line, function code lines are only wired to expansions port for Amiga.
    // IACK cycle (FC0, FC1 raised at same time) is detectable by overriding "IackCycle"
    // FC2 is always high during IACK cycle (because CPU have to switch to supervisor before)
    // during a "Write" cycle FC1 (program) should never(?) be active, because there is no write for PC addressing modes.
    // function codes will be updated during first half clock cycle (1/8 of BUS cycle)
    // works like "readByte" / "readWord" but hints periphery that FC1 is asserted
    virtual auto readBytePRG(uint32_t adr) -> uint8_t { return 0; }
    virtual auto readWordPRG(uint32_t adr) -> uint16_t { return 0; }

    // some devices observe AS line only and treat BUS cycles which cause an address exception as valid read/write cycles.
    // don't know for sure ... there are some Amiga expansions doing this, so override it when needed.
    // wait states are possible too. That alone is reason enough to implement this method.
    // Another reason could be that external devices monitors the AS line and triggers actions in combination with certain addresses.
    virtual auto adrExcAccess(bool readMode, uint32_t adr, uint8_t FC10, uint16_t value = 0) -> void {}

    // address strobe (AS) line remains asserted after read cycle of TAS, so read and following write are indivisible,
    // just like one single BUS cycle. when using multiple CPU's at the same BUS override following methods to
    // set up a spinlock. simply it tells you the state of AS line.
    // note: don't rely on TAS for Amiga, because DMA accesses don't respect AS line of CPU
    virtual auto tasCycleBegin() -> void {}
    virtual auto tasCycleEnd() -> void {}

    virtual auto debugPointReached(DebuggerAction action, unsigned addr) -> void {}
#endif
private:
    auto controlBreaks() -> void;
    auto setCCR(uint8_t data) -> void;
    auto setSR(uint16_t data) -> void;
    auto build() -> void;
    auto sampleInterrupt() -> void;
    auto setSuperVisor(bool state = true) -> void;
    template<uint8_t Size = Word> auto misaligned(uint32_t adr) -> bool;

    template<uint8_t Size = Long> inline auto readRegD(int reg) -> uint32_t;
    template<uint8_t Size = Long> inline auto writeRegD(int reg, uint32_t data) -> void;
    
    template<uint8_t Size = Long> inline auto readRegA(int reg) -> uint32_t;
    inline auto writeRegA(int reg, uint32_t data) -> void;

    template<uint8_t Size, uint8_t Flags = 0> auto read(uint32_t adr) -> uint32_t;
    template<uint8_t Size, uint8_t Flags = 0> auto write(uint32_t adr, uint32_t data) -> void;
    inline auto firstPrefetch() -> void;
    template<uint8_t Flags = None> inline auto prefetch() -> void;
    template<uint8_t Flags = None> auto fullPrefetch() -> void;
    template<uint8_t Flags = None> inline auto readExtensionWord() -> void;

    auto peek(uint32_t adr) -> uint16_t;
    template<uint8_t Size> auto peekInc(uint32_t& adr, DasmHandler& d) -> uint32_t;
    
    template<uint8_t Mode, uint8_t Size, uint8_t Flags = None> auto calcEA(int reg) -> uint32_t;
    template<uint8_t Mode, uint8_t Size, uint8_t Flags = None> auto readEA(int reg, uint32_t& result, uint32_t& ea) -> bool;
    template<uint8_t Mode, uint8_t Size, uint8_t Flags = None> auto writeEA(uint32_t ea, uint32_t data) -> void;
    template<uint8_t Mode, uint8_t Size> auto prepareEaForDasm(uint32_t& adr, uint8_t reg, DasmHandler& d) -> void;

    auto parse(const char* s, uint16_t sum = 0) -> uint16_t;
    template<uint8_t Size> inline auto clip(uint32_t data) -> uint32_t;
    template<uint8_t Size> inline constexpr auto msb() -> uint32_t;
    template<uint8_t Size> inline auto sign(uint32_t data) -> int32_t;
    template<uint8_t Size> inline auto negative(uint32_t data) -> bool;
    template<uint8_t Size> inline auto carry(uint64_t data) -> bool;
    template<uint8_t Size> inline constexpr auto mask() -> uint32_t;
    template<uint8_t Size> inline constexpr auto inverseMask() -> uint32_t;
    template<uint8_t Size> inline constexpr auto bits() -> uint8_t;

    template<uint8_t Size> auto zero(uint32_t result) -> bool {
        return clip<Size>(result) == 0;
    }

    #define OP_DECLARE(ident) \
        template<uint8_t Inst, uint8_t Size> auto op##ident(uint16_t) -> void; \
        template<uint8_t Inst, uint8_t Size> auto dasm##ident(uint16_t, uint32_t&, DasmHandler&) -> void;

    #define OP_DECLARE_EA(ident) \
        template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto op##ident(uint16_t) -> void; \
        template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto dasm##ident(uint16_t, uint32_t&, DasmHandler&) -> void;

    OP_DECLARE(ImmShift)
    OP_DECLARE(RegShift)
    OP_DECLARE_EA(Shift)
    OP_DECLARE_EA(Bit)
    OP_DECLARE_EA(ImmBit)
    OP_DECLARE_EA(Clr)
    OP_DECLARE_EA(Nbcd)
    OP_DECLARE_EA(Neg)
    OP_DECLARE_EA(Scc)
    OP_DECLARE_EA(Tas)
    OP_DECLARE_EA(Tst)
    OP_DECLARE_EA(Arithmetic)
    OP_DECLARE_EA(Cmp)
    OP_DECLARE_EA(ArithmeticEA)
    OP_DECLARE_EA(ArithmeticA)
    OP_DECLARE_EA(Cmpa)
    OP_DECLARE_EA(Mul)
    OP_DECLARE_EA(Div)
    OP_DECLARE_EA(Move)
    OP_DECLARE_EA(MoveA)
    OP_DECLARE_EA(ArithmeticI)
    OP_DECLARE_EA(ArithmeticQ)
    OP_DECLARE(MoveQ)
    OP_DECLARE(Bcc)
    OP_DECLARE(Bsr)
    OP_DECLARE(Dbcc)
    OP_DECLARE_EA(Jmp)
    OP_DECLARE_EA(Jsr)
    OP_DECLARE_EA(Lea)
    OP_DECLARE_EA(Pea)
    OP_DECLARE_EA(MovemToReg)
    OP_DECLARE_EA(MovemToEa)
    OP_DECLARE(ArithmeticX)
    OP_DECLARE(ArithmeticXEa)
    OP_DECLARE(ArithmeticBCD)
    OP_DECLARE(Cmpm)
    OP_DECLARE(Ccr)
    OP_DECLARE(Sr)
    OP_DECLARE_EA(Chk)
    OP_DECLARE_EA(MoveFromSr)
    OP_DECLARE_EA(MoveToSr)
    OP_DECLARE_EA(MoveToCcr)
    OP_DECLARE(ExgDxDy)
    OP_DECLARE(ExgAxAy)
    OP_DECLARE(ExgAxDy)
    OP_DECLARE(Ext)
    OP_DECLARE(Link)
    OP_DECLARE(MoveUspAn)
    OP_DECLARE(MoveAnUsp)
    OP_DECLARE(Swap)
    OP_DECLARE(Trap)
    OP_DECLARE(Unlink)
    OP_DECLARE(MovepReg)
    OP_DECLARE(MovepEa)

    #undef OP_DECLARE_EA
    #undef OP_DECLARE

    auto opTrapv(uint16_t opcode) -> void;
    auto opNop(uint16_t opcode) -> void;
    auto opReset(uint16_t opcode) -> void;
    auto opRte(uint16_t opcode) -> void;
    auto opRtr(uint16_t opcode) -> void;
    auto opRts(uint16_t opcode) -> void;
    auto opStop(uint16_t opcode) -> void;

    auto dasmTrapv(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmNop(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmReset(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmRte(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmRtr(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmRts(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmStop(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmIllegal(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmLineA(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;
    auto dasmLineF(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void;

    template<uint8_t Inst> auto bit(uint32_t data, int bit) -> uint32_t;
    template<uint8_t Inst, uint8_t Size, bool SingleShift = false> auto shifter(uint32_t data, int shift) -> uint32_t;
    
    template<uint8_t Inst> auto singleShifter(uint32_t data) -> uint32_t {
        return shifter<Inst, Word, true>( data, 1 );
    }

    template<uint8_t Inst, uint8_t Size> auto arithmetic(uint32_t src, uint32_t dest) -> uint32_t;
    template<typename T, typename TSign, uint8_t Inst, uint8_t Size> auto arithmeticT(uint32_t src, uint32_t dest) -> uint32_t;
    template<uint8_t Inst> auto bcd(uint32_t src, uint32_t dest) -> uint8_t;
    template<uint8_t Inst> auto testCondition() -> bool;
    
    template<uint8_t Size> auto applyRoxRange(int& shift) -> void;
    template<uint8_t Size> auto defaultFlags(uint32_t data) -> void;
    template<uint8_t Mode, uint8_t destMode, uint8_t Size> auto setMoveCCWhenAddressError(uint32_t data) -> void;

    auto executeAt(uint16_t adr, uint8_t group) -> void;
    template<uint8_t Mode, uint8_t Size> auto addressExceptionEA(uint32_t ea) -> void;
    template<uint8_t Mode, uint8_t Size> auto addressExceptionMoveEA(uint32_t ea, uint32_t data) -> void;
    auto addressException(uint32_t adr, uint32_t _pc, uint8_t flags, uint16_t value = 0) -> void;

    auto resetRoutine() -> void;
    auto group1Exception(uint8_t vector) -> void;
    auto IRQException() -> void;
    auto traceException() -> void;
    auto trapException(uint8_t vector) -> void;
    auto privilegeException() -> void;
    auto illegal(uint16_t opcode) -> void;
    auto lineF(uint16_t opcode) -> void;
    auto lineA(uint16_t opcode) -> void;
    auto getInterruptVector(uint8_t level) -> uint8_t;

    template<uint8_t Inst> auto ccr(uint8_t data) -> void;
    template<uint8_t Inst> auto sr(uint16_t data) -> void;
    
    template<uint8_t Inst> auto cyclesBit(int bit) -> void;
    template<uint8_t Inst> auto cyclesMul(uint16_t data) -> void;
    template<uint8_t Inst> auto cyclesDiv(uint32_t dividend, uint16_t divisor) -> int;

    template<uint8_t Mode> constexpr static auto isRegisterMode() -> bool { return Mode == AddressRegisterDirect || Mode == DataRegisterDirect; }
    template<uint8_t Mode> constexpr static auto isDirectMode() -> bool { return isRegisterMode<Mode>() || Mode == Immediate; }
    template<uint8_t Mode> constexpr static auto isAbsMode() -> bool { return Mode == AbsoluteShort || Mode == AbsoluteLong; }
    template<uint8_t Mode> constexpr static auto isIndexMode() -> bool { return Mode == AddressRegisterIndirectWithIndex || Mode == ProgramCounterIndirectWithIndex; }
    template<uint8_t Mode> constexpr static auto isDisplacementMode() -> bool { return Mode == AddressRegisterIndirectWithDisplacement || Mode == ProgramCounterIndirectWithDisplacement; }
    template<uint8_t Mode> constexpr static auto isPcMode() -> bool { return Mode == ProgramCounterIndirectWithIndex || Mode == ProgramCounterIndirectWithDisplacement; }

    auto firstMovemWrite( uint16_t mask, unsigned shift, bool reverseOrder ) -> uint16_t;
    auto nextIsGroup1Exception() -> bool;

    auto checkSoftStop(uint32_t addr) -> bool;
    auto flagDebugAction(int action, bool state) -> void;
};

}
