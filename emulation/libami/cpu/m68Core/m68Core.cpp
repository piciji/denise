
#include "m68Core.h"
#include <stdexcept>

#ifdef REF
    #ifdef REF_INCLUDE
    #include REF_INCLUDE
    #endif
    #define REF_CALL ref.
#else
    #define REF_CALL
#endif

#define READ_LONG           REF_CALL readLong
#define READ_WORD           REF_CALL readWord
#define READ_BYTE           REF_CALL readByte
#define WRITE_LONG          REF_CALL writeLong
#define WRITE_WORD          REF_CALL writeWord
#define WRITE_BYTE          REF_CALL writeByte
#define PEEK_WORD           REF_CALL peekWord
#define DEBUG_POINT_REACHED REF_CALL debugPointReached
#define PORT_SIZE           REF_CALL portSize

#ifdef FC_SUPPORT
#define READ_LONG_PRG   REF_CALL readLongPRG
#define READ_WORD_PRG   REF_CALL readWordPRG
#define READ_BYTE_PRG   REF_CALL readBytePRG
#else
#define READ_LONG_PRG   READ_LONG
#define READ_WORD_PRG   READ_WORD
#define READ_BYTE_PRG   READ_BYTE
#endif

#define SYNC            REF_CALL sync
#define IACK_CYCLE      REF_CALL iackCycle
#define RESET_OUT       REF_CALL resetOut
#define TAS_CYCLE_BEGIN REF_CALL tasCycleBegin
#define TAS_CYCLE_END   REF_CALL tasCycleEnd

#include "memory.cpp"
#include "optable.cpp"
#include "instruction.cpp"
#include "alu.cpp"
#include "exception.cpp"
#include "disassembler.cpp"

namespace M68FAMILY {

M68Core::~M68Core() {
    delete[] mulCycleLookup;
    delete[] dasmTable;
    delete[] mnemonics;
}

auto M68Core::create() -> void {
    dasmTable = new OpDasm[0x10000];
    mulCycleLookup = new uint8_t[0x10000];
    mnemonics = new const char*[0x10000];

    mulCycleLookup[0] = 34;
    for( int i = 0; i < 0x10000; i++ )
        mulCycleLookup[i] = ((i & 1) << 1) + mulCycleLookup[i / 2];
}

auto M68Core::process() -> void {

    if (control) {
        if (control & Halt)
            return SYNC(4);

        if (control & TraceScheduled) { // highest prioritized group 1 exception
            traceException();
            if (control & (BreakPoint | SoftStop))
                controlBreaks();
            return;
        }

        if ((control & Trace) && !(control & Stop))
            // trace happens if it was enabled at beginning of an instruction.
            // hence instruction which enabled tracing will not be traced.
            // stop instruction can not schedule trace mode, but an already scheduled trace prevent "stop" at all.
            control |= TraceScheduled;

        if (control & IRQ) {
            IRQException();
            if (control & (BreakPoint | SoftStop))
                controlBreaks();
            return;
        }

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

        if (control & History)
            loadTrace(historyHandler.getNext());

        try {
            (this->*opTable[ird])(ird);
        } catch (const std::exception& exc) {
            group0Exception( exc );
        }

        if (control & (BreakPoint | SoftStop))
            controlBreaks();
    } else {
        try {
            (this->*opTable[ird])(ird);
        } catch (const std::exception& exc) {
            group0Exception( exc );
        }
    }
}

auto M68Core::controlBreaks() -> void {
    if (control & (TraceScheduled | Stop) )
        return;

    int source = 0;

    if ((control & SoftStop) && checkSoftStop(pcEdge))
        source = SoftStop;

    if ((control & BreakPoint) && breakPoints.check(pcEdge, true))
        source = BreakPoint;

    if (source != 0)
        DEBUG_POINT_REACHED(source, breakPoints, pcEdge);
}

auto M68Core::loadTrace(Emulator::HistoryEntry<uint16_t>& entry) -> void {
    uint32_t addr = pcEdge;
    entry.addr = pcEdge;
    entry.flags = getSR();

    for (uint16_t& m : entry.mem) {
        m = peek( addr );
        addr += 2;
    }
}

auto M68Core::power() -> void {
    // E-Clock phase is non deterministic on power up.
    usp = 0;
    for(uint8_t i = 0; i < 8; i++) {
        // unchanged on soft reset, uninitialized on hard reset.
        regsD[i] = 0;
        regsA[i] = 0; // 0 - 6
    }
    
    reset();
}

auto M68Core::reset() -> void { // highest prioritized group 0 routine
    // E-Clock phase is unchanged on soft reset, but depends on reset pulse duration.
    c = v = z = n = x = false;
    s = true;
    m = false;
    t0 = false;
    i = 7;
    iplPins = iplSample = 0;
    cacr = 0;
    caar = 0;
    cahr = 0;
    overlap = 0;
    group = 2;
    invalidateCache();

    control = ResetRoutine; // emulation begins, when reset line is de-asserted
    stepOuts.clear();
    watchPoints.reset();
    watchPointsWrite.reset();
    breakPoints.reset();
    exceptionPoints.reset();
    modifiedCode.disable();
    historyHandler.flagWhenNeeded();

    if (!opTable[0])
        throw std::runtime_error("68k not initialized");
}

auto M68Core::getCCR() -> uint8_t {
    return c << 0 | v << 1 | z << 2 | n << 3 | x << 4;
}

auto M68Core::setCCR(uint8_t data) -> void {
    c = data & 1;
    v = (data >> 1) & 1;
    z = (data >> 2) & 1;
    n = (data >> 3) & 1;
    x = (data >> 4) & 1;
}

auto M68Core::getSR() -> uint16_t {
    return getCCR() | (i & 7) << 8 | t0 << 14 | s << 13 | m << 12 | !!((control & Trace) != 0) << 15;
}

auto M68Core::setSR(uint16_t data) -> void {
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

    if (engine >= Engine::M68020) {
        setMasterSP( (data >> 12) & 1 );
        t0 = (data >> 14) & 1;
    }
}

inline auto M68Core::sampleInterrupt() -> void {
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

auto M68Core::setInterrupt( uint8_t level ) -> void {
    // 1 - 6 are level sensitive
    // 7 is edge sensitive, but a mask change can re-trigger it without having to change the PIN state (wtf)
    level &= 7;
    if (iplPins == level)
        return;

    iplPins = level;
    control |= IRQScheduled;
}

auto M68Core::setHalt() -> void {
    control |= Halt;
}

template<uint8_t Inst> auto M68Core::cyclesBit(int bit) -> void {
    switch(Inst) {
        case Btst: SYNC(2); break;
        case Bclr: SYNC( bit > 15 ? 6 : 4); break;
        case Bset:
        case Bchg: SYNC( bit > 15 ? 4 : 2 ); break;
    }
}

template<uint8_t Inst> auto M68Core::cyclesMul(uint16_t data) -> void {
    switch (Inst) {
        case Muls:
            data = ((data << 1) ^ data) & 0xffff;
            /* fallthrough */
        case Mulu:
            SYNC( mulCycleLookup[data] );
            break;
    }
}

template<uint8_t Inst> auto M68Core::cyclesDiv(uint32_t dividend, uint16_t divisor) -> int {
    if constexpr(Inst == Divu) {
        uint32_t hdivisor = divisor << 16;
        int cycles = 36 << 1; // minimum: 3 + (15 * 2) + 2 + 1

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
        int mcycles = _dividend < 0 ? 7 : 6;

        if ((std::abs(_dividend) >> 16) >= std::abs(_divisor)) {
            SYNC( mcycles << 1 );
            return 0;
        }

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

        bool sampleEarly = _divisor < 0 && _dividend >= 0; // confirmed with fx68k

        if (sampleEarly)
            SYNC((mcycles - 1) << 1);
        else
            SYNC(mcycles << 1);

        return sampleEarly ? 2 : 0;
    }

    return 0;
}

template<uint8_t Mode, uint8_t destMode, uint8_t Size> auto M68Core::setMoveCCWhenAddressError(uint32_t data) -> void {
    if constexpr(Size == Word) {
        if (isRegisterMode<Mode>() || (destMode != DataRegisterDirect))
            defaultFlags<Word>(data);

    } else { // Long
        if constexpr(isDirectMode<Mode>()) {
            if constexpr(destMode == AddressRegisterIndirectWithPreDecrement || destMode == AbsoluteShort || destMode == AbsoluteLong)
                defaultFlags<Long>(data);
            else if constexpr(destMode == AddressRegisterIndirectWithDisplacement || destMode == AddressRegisterIndirectWithIndex) {
                int16_t hi = (int16_t)(data >> 16);
                if (hi < 0)     { n = true; z = false; }
                else if (hi)    { n = z = false; }
                else            { n = false; }
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

template<M68Core::Engine E> auto M68Core::nextIsGroup1Exception() -> bool {
    auto opMethod = opTable[ird];

    if ((opMethod == &M68Core::lineA<E>) || (opMethod == &M68Core::lineF<E>) || (opMethod == &M68Core::illegal<E>))
        return true;

    if (!s) {
        if ((opMethod == &M68Core::opReset<E>) || (opMethod == &M68Core::opRte<E>) || (opMethod == &M68Core::opStop<E>) )
            return true;

        if (ird == 0x27c || ird == 0x7c || ird == 0xa7c) // opSr
            return true;

        if ((ird & 0xffc0) == 0x46c0) // opMoveToSr
            return true;

        if ((ird & 0xfff0) == 0x4e60) // opMoveUsp
            return true;
    }

    if (control & IRQ) // todo: need to check IRQ, a pending Trace don't stack "Exception" flag
        return true;

    return false;
}

template<uint8_t phaseShift> auto M68Core::internalWaitCyclesBasedOnMainClockCycles(int clockCycles) -> uint8_t {
    // get E-Clock cycle position from continous main clock cycle counter.
    // keep in mind, that a wrap around of cycle counter distort E-Clock phase. handle the wrap around manually!
    return internalWaitCyclesBasedOnEClock<phaseShift>( clockCycles % 10 );
}

// initial phase shift of E-Clock at power up (cold start) is non-deterministic.
// keep phase shift after reset
template<> auto M68Core::internalWaitCyclesBasedOnEClock<0>(int eCyclePos) -> uint8_t {
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

template<> auto M68Core::internalWaitCyclesBasedOnEClock<2>(int eCyclePos) -> uint8_t {
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

template<> auto M68Core::internalWaitCyclesBasedOnEClock<4>(int eCyclePos) -> uint8_t {
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

template<> auto M68Core::internalWaitCyclesBasedOnEClock<6>(int eCyclePos) -> uint8_t {
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

template<> auto M68Core::internalWaitCyclesBasedOnEClock<8>(int eCyclePos) -> uint8_t {
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

inline auto M68Core::appendStepOut(uint32_t addr) -> void {
    if (stepOuts.size() == stepOuts.capacity())
        stepOuts.clear();
    stepOuts.push_back( addr );
}

auto M68Core::flagDebugAction(int action, bool state) -> void {
    if (state)
        control |= action;
    else
        control &= ~action;
}

auto M68Core::checkSoftStop(uint32_t addr) -> bool {
    if (!softStep.has_value() || softStep.value_or(0) == addr) {
        control &= ~SoftStop;
        return true;
    }
    return false;
}

auto M68Core::debuggerStepOver(bool subroutineOnly) -> void {
    bool _over = true;

    if (subroutineOnly) {
        if (( (ird & 0xffc0) != 0x4e80) && ( (ird & 0xff00) != 0x6100) ) // JSR / BSR
            _over = false;
    } else if (( ird == 0x4e73) || ( ird == 0x4e77) || ( ird == 0x4e75) ) // RTE / RTR / RTS
        _over = false;

    if (_over) {
        unsigned iSize;
        disassemble( pcEdge, iSize );
        softStep = pcEdge + iSize;
    } else {
        softStep = std::nullopt;
    }

    control |= SoftStop;
}

auto M68Core::debuggerStepInto() -> void {
    softStep = std::nullopt;
    control |= SoftStop;
}

auto M68Core::debuggerStepOut() -> bool {
    if (stepOuts.empty())
        return false;
    softStep = stepOuts.back();
    control |= SoftStop;
    return true;
}

auto M68Core::setEngine( Engine engine ) -> void {
    if (!opTable[0] || (engine != this->engine)) {
        this->engine = engine;
        if (engine == Engine::M68000) build<Engine::M68000>();
        else if (engine == Engine::M68020) build<Engine::M68020>();
    }
}

auto M68Core::syncSequencer(unsigned cycles) -> void {
    if (overlap) {
        if (cycles > overlap)
            SYNC( cycles - overlap );
        overlap = 0;
    } else
        SYNC( cycles );
}

auto M68Core::invalidateCache() -> void {
    for (auto& cache : cacheEntries) {
        cache.valid = false;
    }
}

}
