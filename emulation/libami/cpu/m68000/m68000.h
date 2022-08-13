
/*
 *
 * missing features: BUS error
 *
 * external devices can raise BERR (BUS error) as a result of unwanted memory access. opcodes could be interrupted during each memory access.
 * prefetching often takes place at the same time as ALU operations, which leaves status flags with intermediate values.
 * for Amiga, BERR is wired to expansions port only. don't know if there are any expansions using this.
 * Atari can cause BUS errors.
 */

#pragma once

#include <algorithm>

#ifdef _MSC_VER
#define _unreachable    __assume(false);
#else
#define _unreachable    __builtin_unreachable();
#endif

namespace M68FAMILY {

class M68000 {
    typedef void (M68000::*OpTable)(uint16_t);
    OpTable opTable[0x10000] = {0};
    uint8_t* mulCycleLookup = nullptr;

protected:
    M68000();
    ~M68000();

    enum { Byte = 1, Word = 2, Long = 4, BWL = 7, WL = 6, BW = 3 };
    enum { Asl, Asr, Lsl, Lsr, Rol, Ror, Roxl, Roxr };
    enum { Bchg, Bset, Bclr, Btst };
    enum { Sub, Subx, Add, Addx, Not, Cmp, And, Or, Eor };
    enum { Abcd, Sbcd };
    enum { Adda, Suba };
    enum { Mulu, Muls };
    enum { Divu, Divs };
    enum { UspAn, AnUsp };
    enum { ToMem, ToReg };
    enum { None = 0, SampleIPL = 1, SkipExtension = 2, TasCycle = 4,
           PRG = 8, ConcurrentAdrCalc = 16 /* happens while prefetching */, Reverse = 32 };
    enum { SF_DATA = 1, SF_PRG = 2, SF_EXC = 8, SF_READ = 16 };

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
    enum { Normal = 0, IRQ = 1, Trace = 2, Halt = 4, Stop = 8, TraceScheduled = 16, IRQScheduled = 32 };
    enum { USER_VECTOR, AUTO_VECTOR, SPURIOUS };

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
    uint8_t control;

public:
    auto process() -> void;
    auto setInterrupt( uint8_t level ) -> void;
    auto reset() -> void;
    auto power() -> void;
    auto getFC2() -> bool { return s; }
    auto isHalted() -> bool { return !!(control & Halt); }
    auto setHalt() -> void; // I/O - could be halted from external device

    // use this to calculate the needed wait states by terminating a BUS cycle with VPA line
    template<uint8_t phaseShift = 0> auto internalWaitCyclesBasedOnEClock(uint8_t eCyclePos) -> uint8_t;
    template<uint8_t phaseShift = 0> auto internalWaitCyclesBasedOnMainClockCycles(unsigned clockCycles) -> uint8_t;
    
protected:
    // reset line is bi-directional
    auto resetOut() -> void {}
    // sync to external devices in reasonable steps (until possible wait states could happen)
    // don't process any wait states in this method, because simply you have no hint if it is an internal cycle or not.
    // internal cycles can't be prolonged with wait states, only third CPU cycle of BUS cycle.
    auto sync(uint16_t cycles) -> void;
    // methods to handle BUS cycles and wait states
    // each BUS cycle is terminated with DTACK, VPA or BERR line.
    // if it's not terminated until half of third cycle (S4) within BUS cycle, wait states will be added. (full CPU cycles)
    // DTACK: normal operation
    // BERR: BUS error (currently not emulated, except for IACK cycle)
    // VPA: compatibility to slow 6800 periphery, based on E-Clock (1/10 of main clock)
    // In addition to external wait states by not terminating the cycle in time,
    // for VPA there are internal wait states that cannot be avoided. The amount of cycles depends on the state of E-Clock.
    // for performance reasons, there is no extra method to handle the termination. so you need to take extra care for VPA termination.
    // use this helper function "internalWaitCyclesBasedOnEClock" to get amount of wait cycles. The argument for this function
    // is the cumulated count of CPU cycles (should include possible external wait states)
    // Note:
    // On real 68000 hardware the E clock initial phase at power up is non deterministic.
    // The clock is not affected by hardware reset. That means that the phase of the clock after non power up reset depends on
    // what was the phase before the reset and the number of cycles of the reset pulse.
    // after each cold start the phase of E-Clock could be different.

    auto readByte(uint32_t adr) -> uint8_t;
    auto readWord(uint32_t adr) -> uint16_t;
    auto writeByte(uint32_t adr, uint8_t data) -> void;
    auto writeWord(uint32_t adr, uint16_t data) -> void;

    // BUS cycle to fetch the Interrupt vector.
    // in contrast to the BUS cycle operations above this one returns the termination type.
    // read the notes within this method.
    auto IackCycle(uint8_t level, uint8_t& vector) -> uint8_t;

    // function codes: optional, when periphery needs to distinguish between data (FC0) and program (FC1) access.
    // like BERR line, function code lines are only wired to expansions port for Amiga.
    // IACK cycle (FC0, FC1 raised at same time) is detectable by overriding "IackCycle"
    // FC2 is always high during IACK cycle (because CPU have to switch to supervisor before)
    // during a "Write" cycle FC1 (program) should never(?) be active, because there is no write for PC addressing modes.
    // function codes will be updated during first half clock cycle (1/8 of BUS cycle)
    
    // #define FC_SUPPORT
    // works like "readByte" / "readWord", the difference is that FC1 is active instead of FC0.
    auto readBytePRG(uint32_t adr) -> uint8_t;
    auto readWordPRG(uint32_t adr) -> uint16_t;

    // some devices observe AS line only and treat BUS cycles which cause an address exception as valid read/write cycles.
    // don't know for sure ... there are some Amiga expansions doing this, so override it when needed.
    // wait states are possible too. That alone is reason enough to implement this method.
    // Another reason could be that external devices monitors the AS line and triggers actions in combination with certain addresses.
    auto adrExcAccess(bool readMode, uint32_t adr, uint8_t FC10, uint16_t value = 0) -> void {}

    // address strobe (AS) line remains asserted after read cycle of TAS, so read and following write are indivisible,
    // just like one single BUS cycle. when using multiple CPU's at the same BUS override following methods to
    // set up a spinlock. simply it tells you the state of AS line.
    // note: don't rely on TAS for Amiga, because DMA accesses don't respect AS line of CPU
    auto tasCycleBegin() -> void {}
    auto tasCycleEnd() -> void {}

    auto getCCR() -> uint8_t;
    auto getSR() -> uint16_t;

private:
    auto setCCR(uint8_t data) -> void;
    auto setSR(uint16_t data) -> void;
    auto build() -> void;
    auto sampleInterrupt() -> void;
    auto setSuperVisor(bool state = true) -> void;
    template<uint8_t Size = Word> auto misaligned(uint32_t adr) -> bool;

    template<uint8_t Size = Long> inline auto readRegD(uint8_t reg) -> uint32_t;
    template<uint8_t Size = Long> inline auto writeRegD(uint8_t reg, uint32_t data) -> void;
    
    template<uint8_t Size = Long> inline auto readRegA(uint8_t reg) -> uint32_t;
    inline auto writeRegA(uint8_t reg, uint32_t data) -> void;

    template<uint8_t Size, uint8_t Flags = 0> auto read(uint32_t adr) -> uint32_t;
    template<uint8_t Size, uint8_t Flags = 0> auto write(uint32_t adr, uint32_t data) -> void;
    inline auto firstPrefetch() -> void;
    template<uint8_t Flags = None> inline auto prefetch() -> void;
    template<uint8_t Flags = None> auto fullPrefetch() -> void;
    template<uint8_t Flags = None> inline auto readExtensionWord() -> void;
    
    template<uint8_t Mode, uint8_t Size, uint8_t Flags = None> auto calcEA(uint8_t reg) -> uint32_t;
    template<uint8_t Mode, uint8_t Size, uint8_t Flags = None> auto readEA(uint8_t reg, uint32_t& result, uint32_t& ea) -> bool;
    template<uint8_t Mode, uint8_t Size, uint8_t Flags = None> auto writeEA(uint32_t ea, uint32_t data) -> void;

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

    template<uint8_t Inst, uint8_t Size> auto opImmShift(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opRegShift(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opShift(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opBit(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opImmBit(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opClr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opNbcd(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opNeg(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opScc(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opTas(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opTst(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opArithmetic(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opCmp(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opArithmeticEA(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opArithmeticA(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opCmpa(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMul(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opDiv(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMove(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMoveA(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opArithmeticI(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opArithmeticQ(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opMoveQ(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opBcc(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opBsr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opDbcc(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opJmp(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opJsr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opLea(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opPea(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMovemToReg(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMovemToEa(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opArithmeticX(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opArithmeticXEa(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opCmpm(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opCcr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opSr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opChk(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMoveFromSr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMoveToSr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto opMoveToCcr(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opExgDxDy(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opExgAxAy(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opExgAxDy(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opExt(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opLink(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opMoveUsp(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opSwap(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opTrap(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opUnlink(uint16_t opcode) -> void;
    template<uint8_t Inst, uint8_t Size> auto opMovep(uint16_t opcode) -> void;

    auto opTrapv(uint16_t opcode) -> void;
    auto opNop(uint16_t opcode) -> void;
    auto opReset(uint16_t opcode) -> void;
    auto opRte(uint16_t opcode) -> void;
    auto opRtr(uint16_t opcode) -> void;
    auto opRts(uint16_t opcode) -> void;
    auto opStop(uint16_t opcode) -> void;

    template<uint8_t Inst> auto bit(uint32_t data, uint8_t bit) -> uint32_t;
    template<uint8_t Inst, uint8_t Size, bool SingleShift = false> auto shifter(uint32_t data, uint8_t shift) -> uint32_t;
    
    template<uint8_t Inst> auto singleShifter(uint32_t data) -> uint32_t {
        return shifter<Inst, Word, true>( data, 1 );
    }

    template<uint8_t Inst, uint8_t Size> auto arithmetic(uint32_t src, uint32_t dest) -> uint32_t;
    template<uint8_t Inst, uint8_t Size> auto bcd(uint32_t src, uint32_t dest) -> uint8_t;
    template<uint8_t Inst> auto testCondition() -> bool;
    
    template<uint8_t Size> auto applyRoxRange(uint8_t& shift) -> void;
    template<uint8_t Size> auto defaultFlags(uint32_t data) -> void;
    template<uint8_t Mode, uint8_t destMode, uint8_t Size> auto setMoveCCWhenAddressError(uint32_t data) -> void;

    auto executeAt(uint16_t adr, uint8_t group) -> void;
    template<uint8_t Mode, uint8_t Size> auto addressExceptionEA(uint32_t ea) -> void;
    template<uint8_t Mode, uint8_t Size> auto addressExceptionMoveEA(uint32_t ea, uint32_t data) -> void;
    auto addressException(uint32_t adr, uint32_t _pc, uint8_t flags, uint16_t value = 0) -> void;

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
    
    template<uint8_t Inst> auto cyclesBit(uint8_t bit) -> void;
    template<uint8_t Inst> auto cyclesMul(uint16_t data) -> void;
    template<uint8_t Inst> auto cyclesDiv(uint32_t dividend, uint16_t divisor) -> void;

    template<uint8_t Mode> constexpr static auto isRegisterMode() -> bool { return Mode == AddressRegisterDirect || Mode == DataRegisterDirect; }
    template<uint8_t Mode> constexpr static auto isDirectMode() -> bool { return isRegisterMode<Mode>() || Mode == Immediate; }
    template<uint8_t Mode> constexpr static auto isAbsMode() -> bool { return Mode == AbsoluteShort || Mode == AbsoluteLong; }
    template<uint8_t Mode> constexpr static auto isIndexMode() -> bool { return Mode == AddressRegisterIndirectWithIndex || Mode == ProgramCounterIndirectWithIndex; }
    template<uint8_t Mode> constexpr static auto isDisplacementMode() -> bool { return Mode == AddressRegisterIndirectWithDisplacement || Mode == ProgramCounterIndirectWithDisplacement; }
    template<uint8_t Mode> constexpr static auto isPcMode() -> bool { return Mode == ProgramCounterIndirectWithIndex || Mode == ProgramCounterIndirectWithDisplacement; }

    auto firstMovemWrite( uint16_t mask, unsigned shift, bool reverseOrder ) -> uint16_t;
};

}
