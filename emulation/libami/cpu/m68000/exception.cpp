
#include "m68000.h"

namespace M68FAMILY {

auto M68000::trapException(uint8_t vector) -> void { // group 2 exceptions
    uint16_t SR = getSR();
    setSuperVisor();

    control &= ~(Stop | Trace); // a scheduled trace exception still happens directly afterwards
    uint32_t& sp = regsA[7];
    
    if (misaligned(sp))
        return addressException(sp - 2, pc, SF_DATA, pc & 0xffff);

    write<Word>(sp - 2, pc & 0xffff);
    sp -= 6;
    write<Word>(sp + 0, SR);
    write<Word>(sp + 2, (pc >> 16) & 0xffff);
    executeAt( vector << 2, 2 );
}

auto M68000::illegal(uint16_t opcode) -> void {
    group1Exception(4);
}

auto M68000::lineA(uint16_t opcode) -> void {
    group1Exception(10);
}

auto M68000::lineF(uint16_t opcode) -> void {
    group1Exception(11);
}

auto M68000::privilegeException() -> void {
    group1Exception( 8 );
}

auto M68000::traceException() -> void {
    group1Exception( 9 );
}

auto M68000::group1Exception(uint8_t vector) -> void {
    uint16_t SR = getSR();
    setSuperVisor();
    pc -= 2;

    control &= ~(Stop | Trace | TraceScheduled);
    SYNC(4);
    uint32_t& sp = regsA[7];

    if (misaligned(sp))
        return addressException(sp - 2, pc, SF_DATA | SF_EXC, pc & 0xffff);

    write<Word>(sp - 2, pc & 0xffff);
    sp -= 6;
    write<Word>(sp + 0, SR);
    write<Word>(sp + 2, (pc >> 16) & 0xffff);
    executeAt( vector << 2, 1 );
}

auto M68000::IRQException() -> void { // a group 1 exception
    uint16_t SR = getSR();
    setSuperVisor();
    i = iplSample;

    // group 1 exceptions load PC from internal "AU" register, which contains the old value before prefetching.
    // for performance reasons we don't emulate the "AU" register.
    pc -= 2;

    control &= ~(IRQ | Stop | Trace | TraceScheduled);
    SYNC(6);
    uint32_t& sp = regsA[7];

    if (misaligned(sp))
        return addressException(sp - 2, pc, SF_DATA | SF_EXC, pc & 0xffff);

    write<Word>(sp - 2, pc & 0xffff);
    uint16_t vectorAdr = (uint16_t)getInterruptVector( iplSample & 7 ) << 2;
    SYNC(4); // justify vector (2 shift operations)
    sp -= 6;
    write<Word>(sp + 0, SR);
    write<Word>(sp + 2, (pc >> 16) & 0xffff);
    executeAt( vectorAdr, 1 );

    // when IPL sample state changes, another IRQ service routine could be processed without a single instruction in between.
    // this also applies for a possible address exception which not causing a double fault.
}

auto M68000::getInterruptVector(uint8_t level) -> uint8_t {
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
    uint8_t terminate = IACK_CYCLE( level, vector );

    if (terminate & USER_VECTOR) {
        // user vector presented on data BUS (64 - 255), but CPU allows user vectors < 64 too (e.g. Amiga)
    } else if (terminate & AUTO_VECTOR) {
        vector = 24 + level;
    } else if (terminate & SPURIOUS) {
        // external device responds with BUS error, to inform that noise triggered IPL.
        vector = 24;
    } else // uninitialized
        vector = 15;

    SYNC(2);
    return vector;
}

template<uint8_t Mode, uint8_t Size> auto M68000::addressExceptionEA(uint32_t ea) -> void {
    uint32_t _pc = pc;

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement && Size != Long)
        _pc += 2;
    else if constexpr(isIndexMode<Mode>() || isDisplacementMode<Mode>())
        _pc -= 2;

    addressException(ea, _pc, SF_READ | (isPcMode<Mode>() ? SF_PRG : SF_DATA) );
}

template<uint8_t Mode, uint8_t Size> auto M68000::addressExceptionMoveEA(uint32_t ea, uint32_t data) -> void {
    uint32_t _pc = pc;

    if constexpr(Mode < AddressRegisterIndirectWithDisplacement)
        _pc += 2;

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement && Size == Long)
        ea += 2;

    if constexpr(Size == Long && (Mode != AddressRegisterIndirectWithPreDecrement))
        data >>= 16;

    addressException(ea, _pc, SF_DATA /* there is no writing to PC rel. addresses */, data);
}

auto M68000::addressException(uint32_t adr, uint32_t _pc, uint8_t flags, uint16_t value) -> void {
    // misalignment is recognized during BUS cycle, UDS and LDS are never asserted.
    // AS line is asserted and in case of a "Write", data is put on data BUS.
    // periphery typically treat a BUS cycle as valid when DS is asserted.

    SYNC(2);
    // some periphery observe AS line only (which is asserted during address exception) and treat this as a valid BUS cycle.
    // periphery can not see A0 (simply there is no A0 line), A0 is determined by LDS (which is not asserted)
    // override following function for such periphery.
    #ifdef ADR_EXC_BUS_CYCLE
    ADR_EXC_ACCESS(!!(flags & 16), adr & ~1, flags & 3, value);
    #endif

    uint16_t SR = getSR();
    setSuperVisor();
    control &= ~(Trace | TraceScheduled);
    uint16_t code = (ird & 0xffe0) | flags | ((SR & 0x2000) ? 4 : 0);
    SYNC( 2 + 8); // finish the BUS cycle, put address (and data) on BUS, but without asserting UDS or LDS.

    uint32_t& sp = regsA[7];
    if (misaligned<Long>(sp)) {
        SYNC(4 + 4); // finish the BUS cycle
        return setHalt();
    }

    write<Word>(sp - 2, _pc & 0xffff);
    write<Word>(sp - 6, SR);
    write<Word>(sp - 4, (_pc >> 16) & 0xffff);
    write<Word>(sp - 8, ird);
    write<Word>(sp - 10, adr & 0xffff);
    // not quite right, happens after first micro cycle of following bus cycle. For the sake of simplicity, we do it now.
    // Because external processes cannot observe this directly, this decision does not lead to a possible accuracy issue.
    sp -= 14;
    write<Word>(sp + 0, code );
    write<Word>(sp + 2, (adr >> 16) & 0xffff);

    executeAt(3 << 2, 0);
}

auto M68000::executeAt(uint16_t adr, uint8_t group) -> void { // 18 cycles
    pc = read<Long>(adr);
    
    if (misaligned<Long>(pc)) {
        if (group == 0) { // bus/address error during group 0 service routine halts CPU, Interrupt to vector 2 or 3 doesn't
            SYNC(4 + 4); // finish the BUS cycle
            return setHalt();
        }
        return addressException(pc, adr/* not PC here, always vector adr */, SF_READ | SF_PRG | (group != 2 ? SF_EXC : 0) );
    }

    firstPrefetch();
    SYNC(2);
    prefetch<SampleIPL>();
}

auto M68000::resetRoutine() -> void { // highest prioritized group 0 routine
    SYNC(12); // confirmed with fx68k
    control &= ~ResetRoutine;

    regsA[7] = ssp = read<Long, FC_PRG>(0); // sets FC1
    pc = read<Long, FC_PRG>(4); // sets FC1 too

    if (misaligned<Long>(pc)) { // bus/address error during group 0 service routine halts CPU.
        SYNC(4 + 4);
        return setHalt();
    }

    firstPrefetch();
    SYNC(2);
    prefetch<SampleIPL>();
}

auto M68000::setSuperVisor(bool state) -> void {
    if (s == state)
        return;

    if (state) {
        s = true;
        usp = regsA[7];
        regsA[7] = ssp;
    } else {
        s = false;
        ssp = regsA[7];
        regsA[7] = usp;
    }
}

template<uint8_t Size> auto M68000::misaligned(uint32_t adr) -> bool {
    return (adr & 1) && Size != Byte;
}

}
