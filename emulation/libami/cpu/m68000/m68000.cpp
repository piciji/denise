
#include "m68000.h"

#ifdef REF
    #ifdef REF_INCLUDE
    #include REF_INCLUDE
    #endif
    #define REF_CALL ref.
#else
    #define REF_CALL
#endif

#define READ_WORD       REF_CALL readWord
#define READ_BYTE       REF_CALL readByte
#define WRITE_WORD      REF_CALL writeWord
#define WRITE_BYTE      REF_CALL writeByte

#ifdef FC_SUPPORT
#define READ_WORD_PRG   REF_CALL readWordPRG
#define READ_BYTE_PRG   REF_CALL readBytePRG
#else
#define READ_WORD_PRG   READ_WORD
#define READ_BYTE_PRG   READ_BYTE
#endif

#define SYNC            REF_CALL sync
#define IACK_CYCLE      REF_CALL iackCycle
#define RESET_OUT       REF_CALL resetOut
#define ADR_EXC_ACCESS  REF_CALL adrExcAccess
#define TAS_CYCLE_BEGIN REF_CALL tasCycleBegin
#define TAS_CYCLE_END   REF_CALL tasCycleEnd

#include "memory.cpp"
#include "optable.cpp"
#include "instruction.cpp"
#include "alu.cpp"
#include "exception.cpp"

namespace M68FAMILY {

M68000::~M68000() {
    if (mulCycleLookup)
        delete[] mulCycleLookup;
}

auto M68000::process()->void {

    if (control) { // for performance only
        if (control & Halt)
            return SYNC(4);

        if (control & TraceScheduled) // highest prioritized group 1 exception
            return traceException();

        if ((control & Trace) && !(control & Stop))
            // trace happens if it was enabled at beginning of an instruction.
            // hence instruction which enabled tracing will not be traced.
            // stop instruction can not schedule trace mode, but an already scheduled trace prevent "stop" at all.
            control |= TraceScheduled;

        if (control & IRQ)
            return IRQException();

        if (control & Stop) {
            sampleInterrupt(); // if not detected, 4 extra cycles.
            SYNC(4);

            if (!s)
                // when stop instruction switch to user mode and no trace was active at beginning of instruction
                // and no IRQ is pending a privilege exception happens.
                privilegeException();
            return;
        }

        if (control & ResetRoutine)
            resetRoutine();
    }
    
    (this->*opTable[ird])(ird);
}

auto M68000::power() -> void {
    // E-Clock phase is non deterministic on power up.
    usp = 0;
    for(uint8_t i = 0; i < 8; i++) {
        // unchanged on soft reset, uninitialized on hard reset.
        regsD[i] = 0;
        regsA[i] = 0; // 0 - 6
    }
    
    reset();
}

auto M68000::reset() -> void { // highest prioritized group 0 routine
    // E-Clock phase is unchanged on soft reset, but depends on reset pulse duration.
    c = v = z = n = x = false;
    s = true;
    i = 7;
    iplPins = iplSample = 0;
    control = ResetRoutine; // emulation begins, when reset line is de-asserted
}

auto M68000::getCCR() -> uint8_t {
    return c << 0 | v << 1 | z << 2 | n << 3 | x << 4;
}

auto M68000::setCCR(uint8_t data) -> void {
    c = data & 1;
    v = (data >> 1) & 1;
    z = (data >> 2) & 1;
    n = (data >> 3) & 1;
    x = (data >> 4) & 1;
}

auto M68000::getSR() -> uint16_t {
    return getCCR() | (i & 7) << 8 | s << 13 | !!((control & Trace) != 0) << 15;
}

auto M68000::setSR(uint16_t data) -> void {
    setCCR( data & 0xff );

    i = (data >> 8) & 7;
    // (repeated) "stop" instruction samples at beginning, so it takes 4 more cycles until a detected interrupt begins.

    if (i != 7)
        control |= IRQScheduled;

    if ((data >> 15) & 1)
        control |= Trace;
    else
        control &= ~Trace;

    setSuperVisor( (data >> 13) & 1 );
}

inline auto M68000::sampleInterrupt() -> void {
    if (control & IRQScheduled) {
        control &= ~IRQScheduled;
        iplSample = iplPins;

        if ( (iplSample > i) || (iplSample == 7) )
            control |= IRQ;
        else
        // some move instructions have two sampling points.
        // if first sampling is accepted but second sampling is not, IRQ doesn't happening.
            control &= ~IRQ;
    }
}

auto M68000::setInterrupt( uint8_t level ) -> void {
    // 1 - 6 are level sensitive
    // 7 is edge sensitive, but a mask change can re-trigger it without having to change the PIN state (wtf)
    level &= 7;
    if (iplPins == level)
        return;

    iplPins = level;
    control |= IRQScheduled;
}

auto M68000::setHalt() -> void {
    control |= Halt;
}

template<uint8_t Inst> auto M68000::cyclesBit(uint8_t bit) -> void {
    switch(Inst) {
        case Btst: SYNC(2); break;
        case Bclr: SYNC( bit > 15 ? 6 : 4); break;
        case Bset:
        case Bchg: SYNC( bit > 15 ? 4 : 2 ); break;
    }
}

template<uint8_t Inst> auto M68000::cyclesMul(uint16_t data) -> void {
    switch (Inst) {
        case Muls:
            data = ((data << 1) ^ data) & 0xffff;
            /* fallthrough */
        case Mulu:
            SYNC( mulCycleLookup[data] );
            break;
    }
}

template<uint8_t Inst> auto M68000::cyclesDiv(uint32_t dividend, uint16_t divisor) -> void {
    if constexpr(Inst == Divu) {
        uint32_t hdivisor = divisor << 16;
        unsigned cycles = 36 << 1; // minimum: 3 + (15 * 2) + 2 + 1

        for (int i = 0; i < 15; i++) {

            if ((int32_t) dividend < 0) {
                dividend <<= 1;
                dividend -= hdivisor;
            } else {
                dividend <<= 1;
                if (dividend >= hdivisor) {
                    dividend -= hdivisor;
                    cycles += 2;
                } else
                    cycles += 4;
            }
        }
        SYNC( cycles );

    } else if constexpr(Inst == Divs) {
        int32_t _dividend = (int32_t)dividend;
        int16_t _divisor = (int16_t)divisor;
        unsigned mcycles = _dividend < 0 ? 7 : 6;

        mcycles += 53; // 3 * 15 + 4 + 4 (divisor < 0)

        if (_divisor >= 0) {
            if (_dividend >= 0) mcycles--;
            else mcycles++;
        }

        uint32_t aquot = std::abs(_dividend) / std::abs(_divisor);
        for (int i = 0; i < 15; i++) {
            if ( (int16_t)aquot >= 0)
                mcycles++;

            aquot <<= 1;
        }

        SYNC( mcycles << 1 );
    }
}

template<uint8_t Mode, uint8_t destMode, uint8_t Size> auto M68000::setMoveCCWhenAddressError(uint32_t data) -> void {
    if constexpr(Size == Word) {
        if (isRegisterMode<Mode>() || (destMode != DataRegisterDirect))
            defaultFlags<Word>(data);

    } else { // Long
        if constexpr(isDirectMode<Mode>()) {
            if constexpr(destMode == AddressRegisterIndirectWithPreDecrement || destMode == AbsoluteShort || destMode == AbsoluteLong)
                defaultFlags<Long>(data);
            else if constexpr(destMode == AddressRegisterIndirectWithDisplacement || destMode == AddressRegisterIndirectWithIndex) {
                int16_t hi = (int16_t)(data >> 16);
                if (hi < 0)     n = true, z = false;
                else if (hi)    n = z = false;
                else            n = false;
            }
        } else if constexpr(Mode == AddressRegisterIndirect || Mode == AddressRegisterIndirectWithPostIncrement || Mode == AddressRegisterIndirectWithPreDecrement) {
            if constexpr(destMode == AddressRegisterIndirect || destMode == AddressRegisterIndirectWithPostIncrement || destMode == AbsoluteLong)
                defaultFlags<Word>(data);
            else
                defaultFlags<Long>(data);
        } else {
            if constexpr(destMode == AddressRegisterIndirect || destMode == AddressRegisterIndirectWithPostIncrement || destMode == AbsoluteLong)
                defaultFlags<Word>(data);
            else if constexpr(destMode == AddressRegisterIndirectWithPreDecrement || destMode == AddressRegisterIndirectWithDisplacement
                                || destMode == AddressRegisterIndirectWithIndex || destMode == AbsoluteShort)
                defaultFlags<Long>(data);
            else {
                v = c = z = false;
                n = data & 0x8000;
            }
        }
    }
}

auto M68000::firstMovemWrite( uint16_t mask, unsigned shift, bool reverseOrder) -> uint16_t {
    for(unsigned i = 0; i < 16; i++) {
        if ((mask >> i) & 1) {
            if (reverseOrder)
                return (((i > 7) ? regsA[i - 8] : regsD[i]) >> shift) & 0xffff;

            return (((i > 7) ? regsD[7 - (i - 8)] : regsA[7 - i]) >> shift) & 0xffff;
        }
    }
    return ~0;
}

template<uint8_t phaseShift> auto M68000::internalWaitCyclesBasedOnMainClockCycles(unsigned clockCycles) -> uint8_t {
    // get E-Clock cycle position from continous main clock cycle counter.
    // keep in mind, that a wrap around of cycle counter distort E-Clock phase. handle the wrap around manually!
    return internalWaitCyclesBasedOnEClock<phaseShift>( clockCycles % 10 );
}

// initial phase shift of E-Clock at power up (cold start) is non deterministic.
// keep phase shift after reset
template<> auto M68000::internalWaitCyclesBasedOnEClock<0>(uint8_t eCyclePos) -> uint8_t {
    if (eCyclePos == 1) return 7;
    if (eCyclePos == 2) return 6;
    if (eCyclePos == 3) return 15;
    if (eCyclePos == 4) return 14;
    if (eCyclePos == 5) return 13;
    if (eCyclePos == 6) return 12;
    if (eCyclePos == 7) return 11;
    if (eCyclePos == 8) return 10;
    if (eCyclePos == 9) return 9;
    /*if (eCyclePos == 0)*/ return 8;
}

template<> auto M68000::internalWaitCyclesBasedOnEClock<2>(uint8_t eCyclePos) -> uint8_t {
    if (eCyclePos == 1) return 9;
    if (eCyclePos == 2) return 8;
    if (eCyclePos == 3) return 7;
    if (eCyclePos == 4) return 6;
    if (eCyclePos == 5) return 15;
    if (eCyclePos == 6) return 14;
    if (eCyclePos == 7) return 13;
    if (eCyclePos == 8) return 12;
    if (eCyclePos == 9) return 11;
    /*if (eCyclePos == 0)*/ return 10;
}

template<> auto M68000::internalWaitCyclesBasedOnEClock<4>(uint8_t eCyclePos) -> uint8_t {
    if (eCyclePos == 1) return 11;
    if (eCyclePos == 2) return 10;
    if (eCyclePos == 3) return 9;
    if (eCyclePos == 4) return 8;
    if (eCyclePos == 5) return 7;
    if (eCyclePos == 6) return 6;
    if (eCyclePos == 7) return 15;
    if (eCyclePos == 8) return 14;
    if (eCyclePos == 9) return 13;
    /*if (eCyclePos == 0)*/ return 12;
}

template<> auto M68000::internalWaitCyclesBasedOnEClock<6>(uint8_t eCyclePos) -> uint8_t {
    if (eCyclePos == 1) return 13;
    if (eCyclePos == 2) return 12;
    if (eCyclePos == 3) return 11;
    if (eCyclePos == 4) return 10;
    if (eCyclePos == 5) return 9;
    if (eCyclePos == 6) return 8;
    if (eCyclePos == 7) return 7;
    if (eCyclePos == 8) return 6;
    if (eCyclePos == 9) return 15;
    /*if (eCyclePos == 0)*/ return 14;
}

template<> auto M68000::internalWaitCyclesBasedOnEClock<8>(uint8_t eCyclePos) -> uint8_t {
    if (eCyclePos == 1) return 15;
    if (eCyclePos == 2) return 14;
    if (eCyclePos == 3) return 13;
    if (eCyclePos == 4) return 12;
    if (eCyclePos == 5) return 11;
    if (eCyclePos == 6) return 10;
    if (eCyclePos == 7) return 9;
    if (eCyclePos == 8) return 8;
    if (eCyclePos == 9) return 7;
    /*if (eCyclePos == 0)*/ return 6;
}

}
