
#include "m68Core.h"

namespace M68FAMILY {

template<M68Core::Engine E> auto M68Core::trapException(uint8_t vector) -> void { // group 2 exceptions
    group = 2;
    uint16_t SR = getSR();
    setSuperVisor();
    bool formatExc = vector == 14;

    control &= ~(Stop | Trace); // a scheduled trace exception still happens directly afterwards
    uint32_t& sp = regsA[7];

    if (E >= Engine::M68020) {
        t0 = false;
        if (vector < 32 && !formatExc) {
            sp -= 12;
            write<E, Long>(sp + 8, pcEdge);
            write<E, Word>(sp + 6, 0x2000 | vector << 2);
        } else { // Trap / Format
            sp -= 8;
            write<E, Word>(sp + 6, vector << 2);
        }
        if (formatExc)
            pc -= 2;

        write<E, Word>(sp + 0, SR);
        write<E, Long>(sp + 2, pc);

    } else {
        if (misaligned(sp))
            throw AddressError(sp - 2, pc, SF_DATA);

        write<E, Word>(sp - 2, pc & 0xffff);
        sp -= 6;
        write<E, Word>(sp + 0, SR);
        write<E, Word>(sp + 2, (pc >> 16) & 0xffff);
    }
    appendStepOut( pc );
    executeAt<E>( vector << 2 );
}

template<M68Core::Engine E> auto M68Core::illegal(uint16_t opcode) -> void {
    group1Exception<E>(4);
}

template<M68Core::Engine E> auto M68Core::lineA(uint16_t opcode) -> void {
    group1Exception<E>(10);
}

template<M68Core::Engine E> auto M68Core::lineF(uint16_t opcode) -> void {
    group1Exception<E>(11);
}

auto M68Core::privilegeException() -> void {
    try {
        if (engine == Engine::M68000)
            group1Exception<Engine::M68000>( 8 );
        else if (engine == Engine::M68020)
            group1Exception<Engine::M68020>( 8 );
    } catch (const std::exception& exc) {
        group0Exception( exc );
    }
}

template<M68Core::Engine E> auto M68Core::privilegeException() -> void {
    group1Exception<E>( 8 );
}

auto M68Core::traceException() -> void {
    try {
        if (engine == Engine::M68000)
            group1Exception<Engine::M68000>( 9 );
        else if (engine == Engine::M68020)
            group1Exception<Engine::M68020>( 9 );
    } catch (const std::exception& exc) {
        group0Exception( exc );
    }
}

template<M68Core::Engine E> auto M68Core::group1Exception(uint8_t vector) -> void {
    group = 1;
    uint16_t SR = getSR();
    setSuperVisor();
    pc -= 2;

    control &= ~(Stop | Trace | TraceScheduled);
    SYNC(4);
    uint32_t& sp = regsA[7];
    
    if (E >= Engine::M68020) {
        t0 = false;
        if (vector == 9) { // trace
            sp -= 12;
            write<E, Long>(sp + 8, pcEdge);
            write<E, Word>(sp + 6, 0x2000 | vector << 2);
        } else {
            sp -= 8;
            write<E, Word>(sp + 6, vector << 2);
        }
        write<E, Word>(sp + 4, pc & 0xffff);
        write<E, Word>(sp + 0, SR);
        write<E, Word>(sp + 2, (pc >> 16) & 0xffff);

    } else {
        if (misaligned(sp))
            throw AddressError(sp - 2, pc, SF_DATA | SF_EXC);

        write<E, Word>(sp - 2, pc & 0xffff);
        sp -= 6;
        write<E, Word>(sp + 0, SR);
        write<E, Word>(sp + 2, (pc >> 16) & 0xffff);
    }

    appendStepOut( pc );
    executeAt<E>( vector << 2 );
}

auto M68Core::IRQException() -> void {
    try {
        if (engine == Engine::M68000)
            IRQException<Engine::M68000>();
        else if (engine == Engine::M68020)
            IRQException<Engine::M68020>();
    } catch (const std::exception& exc) {
        group0Exception( exc );
    }
}

template<M68Core::Engine E> auto M68Core::IRQException() -> void { // a group 1 exception
    group = 1;
    uint16_t SR = getSR();
    setSuperVisor();
    i = iplSample;

    // group 1 exceptions load PC from internal "AU" register, which contains the old value before prefetching.
    // for performance reasons we don't emulate the "AU" register.
    pc -= 2;

    control &= ~(IRQ | Stop | Trace | TraceScheduled);
    SYNC(6);
    uint32_t& sp = regsA[7];
    uint16_t vectorAdr;

    if (E >= Engine::M68020) {
        t0 = false;
        ird = (uint16_t)getInterruptVector( iplSample & 7 );
        vectorAdr = ird << 2;

        // 68020 supports unaligned stack writes

        int rounds = m ? 2 : 1;
        while (rounds--) {
            sp -= 8;
            write<E, Word>(sp + 6, vectorAdr);
            write<E, Word>(sp + 4, pc & 0xffff);
            write<E, Word>(sp + 0, SR);
            write<E, Word>(sp + 2, (pc >> 16) & 0xffff);
            if (m) {
                setMasterSP( false );
                SR |= 0x2000;
                vectorAdr |= 0x1000;
            }
        }

    } else {
        if (misaligned(sp))
            throw AddressError(sp - 2, pc, SF_DATA | SF_EXC);

        write<E, Word>(sp - 2, pc & 0xffff);
        ird = (uint16_t)getInterruptVector( iplSample & 7 );
        vectorAdr = ird << 2;
        SYNC(4); // justify vector (2 shift operations)
        sp -= 6;
        write<E, Word>(sp + 0, SR);
        write<E, Word>(sp + 2, (pc >> 16) & 0xffff);
    }

    appendStepOut( pc );
    executeAt<E>( vectorAdr & 0xfff );

    // when IPL sample state changes, another IRQ service routine could be processed without a single instruction in between.
    // this also applies for a possible address exception which not causing a double fault.
}

auto M68Core::getInterruptVector(uint8_t level) -> uint8_t {
    // IACK cycle is a special (4 cycle minimum) read operation which is finished by a handshake.
    // IACK cycle is recognized by external devices, when all function codes FC0-2 bits set.
    // the interrupt level is placed on A1 - A3, A4 - A23 are all set.
    // Amiga seems to detect this cycle, when A4 - A23 are all set, because function codes are routed directly to Expansion Port
    // like any other BUS cycle, it's terminated by one of the following methods.
    // user vector: by raising DTACK line, after placing vector on data BUS.
    // auto vector: by raising VPA line, IACK cycle will be terminated and an auto vectored interrupt is generated. data BUS is ignored.
    // spurious vector: by raising BERR line, IACK cycle will be terminated and a spurious interrupt is generated. data BUS is ignored.
    uint8_t vector;

    SYNC(2);
    int terminate = IACK_CYCLE( level, vector );

    if (terminate == USER_VECTOR) {
        // user vector presented on data BUS (64 - 255), but CPU allows user vectors < 64 too (e.g. Amiga)
    } else if (terminate == AUTO_VECTOR) {
        vector = 24 + level;
    } else if (terminate == SPURIOUS) {
        // external device responds with BUS error, to inform that noise triggered IPL.
        vector = 24;
    } else // uninitialized
        vector = 15;

    SYNC(2);
    return vector;
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size> auto M68Core::addressExceptionEA(uint32_t ea) -> void {
    uint32_t _pc = pc;

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement && Size != Long)
        _pc += 2;
    else if constexpr(isIndexMode<Mode>() || isDisplacementMode<Mode>())
        _pc -= 2;

    throw AddressError(ea, _pc, SF_READ | (isPcMode<Mode>() ? SF_PRG : SF_DATA));
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size> auto M68Core::addressExceptionMoveEA(uint32_t ea) -> void {
    uint32_t _pc = pc;
    unsigned flags = SF_DATA;

    if constexpr(Mode < AddressRegisterIndirectWithPreDecrement)
        _pc += 2;

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement && Size == Long)
        ea += 2;

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement && Size != Long) {
        // In this case, the "prefetch" happened before and TVN has already been updated.
        // A possible address error will now stack the exception flag.
        if (nextIsGroup1Exception<E>())
            flags |= SF_EXC;
    }

    throw AddressError(ea, _pc, flags);
}

auto M68Core::setBusError(uint32_t addr, bool read, bool prg) -> void {
    unsigned flags = prg ? SF_PRG : SF_DATA;
    if (read)
        flags |= SF_READ;
    if (group != 2)
        flags |= SF_EXC;

    throw BusError(addr, pc, flags);
}

auto M68Core::group0Exception(const std::exception& exc) -> void {
    if (engine == Engine::M68000)
        group0Exception<Engine::M68000>( exc );
    else if (engine == Engine::M68020)
        group0Exception<Engine::M68020>( exc );
}

template<M68Core::Engine E> auto M68Core::group0Exception(const std::exception& exc) -> void {
    try {
        if (const auto addressErr = dynamic_cast<const AddressError*>(&exc); addressErr != nullptr)
            group0Exception<E>(3, addressErr->ea, addressErr->pc, addressErr->flags);

        else if (const auto busErr = dynamic_cast<const BusError*>(&exc); busErr)
            group0Exception<E>(2, busErr->ea, busErr->pc, busErr->flags);

    } catch (const std::exception& doubleFault) {
        SYNC(4);
        setHalt();
    }
}

template<M68Core::Engine E> auto M68Core::group0Exception(uint32_t vector, uint32_t adr, uint32_t _pc, unsigned flags) -> void {
    group = 0;
    // misalignment is recognized during BUS cycle, UDS and LDS are never asserted.
    // AS line is asserted and in case of a "Write", data is put on data BUS.
    // periphery typically treat a BUS cycle as valid when DS is asserted.
    SYNC(2);
    uint16_t SR = getSR();
    setSuperVisor();
    control &= ~(Trace | TraceScheduled);
    t0 = false;
    uint32_t& sp = regsA[7];

    if (E >= Engine::M68020) {
        if (flags & 8)
            flags |= 0x40;

        // todo: a lot
        uint16_t ssw = (flags & 0x43) | ((SR & 0x2000) ? 4 : 0);

        sp -= 4; write<Engine::M68020, Long>(sp, 0);
        sp -= 4; write<Engine::M68020, Long>(sp, 0);
        sp -= 4; write<Engine::M68020, Long>(sp, 0);

        sp -= 4; write<Engine::M68020, Long>(sp, adr);

        sp -= 2; write<Engine::M68020, Word>(sp, 0);
        sp -= 2; write<Engine::M68020, Word>(sp, 0);

        sp -= 2; write<Engine::M68020, Word>(sp, ssw);

        sp -= 2; write<Engine::M68020, Word>(sp, 0);

        sp -= 2; write<Engine::M68020, Word>(sp, 0xa000 | vector << 2);

        sp -= 4; write<Engine::M68020, Long>(sp, _pc);
        sp -= 2; write<Engine::M68020, Word>(sp, SR);
    } else {
        uint16_t code = (ird & 0xffe0) | flags | ((SR & 0x2000) ? 4 : 0);
        SYNC( 2 + 8); // finish the BUS cycle, put address (and data) on BUS, but without asserting UDS or LDS.

        if (misaligned<Long>(sp)) {
            SYNC(4 ); // finish the BUS cycle ... then halt
            throw AddressError(adr, pc, flags);
        }

        write<E, Word>(sp - 2, _pc & 0xffff);
        write<E, Word>(sp - 6, SR);
        write<E, Word>(sp - 4, (_pc >> 16) & 0xffff);
        write<E, Word>(sp - 8, ird);
        write<E, Word>(sp - 10, adr & 0xffff);
        // not quite right, happens after first micro cycle of following bus cycle. For the sake of simplicity, we do it now.
        // Because external processes cannot observe this directly, this decision does not lead to a possible accuracy issue.
        sp -= 14;
        write<E, Word>(sp + 0, code );
        write<E, Word>(sp + 2, (adr >> 16) & 0xffff);
    }

    appendStepOut( pc );
    executeAt<E>(vector << 2);
}

template<M68Core::Engine E> auto M68Core::executeAt(uint32_t adr) -> void { // 18 cycles
    if ((control & ExceptionPoint) && exceptionPoints.check( adr >> 2, true )) {
        DEBUG_POINT_REACHED(ExceptionPoint, exceptionPoints, adr >> 2);
    }

    uint32_t _addr = adr;
    if constexpr (E == Engine::M68020)
        _addr = (vbr & ~0x1) + adr;

    uint32_t oldPc = pc;
    pc = read<E, Long>(_addr);
    
    if (misaligned<Long>(pc)) {
        if (group == 0) // bus/address error during group 0 service routine halts CPU, Interrupt to vector 2 or 3 doesn't
            SYNC(4 ); // finish the BUS cycle ... then halt

        if constexpr (E == Engine::M68020)
            throw AddressError(pc, oldPc, SF_READ | SF_PRG | (group != 2 ? SF_EXC : 0));
        else
            throw AddressError(pc, _addr, SF_READ | SF_PRG | (group != 2 ? SF_EXC : 0));
    }

    firstPrefetch<E>();
    SYNC(2);
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        SYNC(2);
    }
    group = 2; // default
}

auto M68Core::resetRoutine() -> void {
    if (engine == Engine::M68000)
        resetRoutine<Engine::M68000>();
    else if (engine == Engine::M68020)
        resetRoutine<Engine::M68020>();
}

template<M68Core::Engine E> auto M68Core::resetRoutine() -> void { // highest prioritized group 0 routine
    SYNC(12); // confirmed with fx68k
    control &= ~ResetRoutine;

    regsA[7] = ssp = read<E, Long, FC_PRG>(0); // sets FC1
    pc = pcEdge = read<E, Long, FC_PRG>(4); // sets FC1 too

    if (misaligned<Long>(pc)) { // bus/address error during group 0 service routine halts CPU.
        SYNC(4 + 4);
        return setHalt();
    }

    firstPrefetch<E>();
    SYNC(2);
    prefetch<E, SampleIPL>();
}

auto M68Core::setSuperVisor(bool state) -> void {
    if (s == state)
        return;

    s = state;

    if (state) {
        usp = regsA[7];
        regsA[7] = m ? msp : ssp;
    } else {
        (m ? msp : ssp) = regsA[7];
        regsA[7] = usp;
    }
}

auto M68Core::setMasterSP(bool state) -> void {
    if (m == state)
        return;

    m = state;

    if (!s)
        return;

    if (state) {
        ssp = regsA[7];
        regsA[7] = msp;
    } else {
        msp = regsA[7];
        regsA[7] = ssp;
    }
}

template<uint8_t Size> auto M68Core::misaligned(uint32_t adr) -> bool {
    return (adr & 1) && Size != Byte;
}

}
