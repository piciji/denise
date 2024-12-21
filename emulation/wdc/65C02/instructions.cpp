
namespace WDCFAMILY {

#define PAGE_CROSSED(a1, a2) (((a1) ^ (a2)) & 0xff00)

// Absolute Indexed Indirect-(a,x)
auto W65C02::opJmpAbsIndexedIndirect() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    uint16_t absIndexed = addr + x;
    read( ((addr & 0xff00) | (absIndexed & 0xff)));
    uint16_t newPC = read(absIndexed);
    newPC |= read<true>((absIndexed + 1) & 0xffff) << 8;
    pc = newPC;
}

// Absolute-a
// Absolute Indexed with X-a,x
// Absolute Indexed with X-a,y
template<uint8_t Inst, uint8_t Index> auto W65C02::opAbsolute() -> void {
    uint16_t absIndexed;
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    constexpr bool storeMode = Inst == STA || Inst == STX || Inst == STY || Inst == STZ;

    if constexpr (Index == INDEX_X) absIndexed = addr + x;
    if constexpr (Index == INDEX_Y) absIndexed = addr + y;

    if constexpr (Index != NONE) {
        if (storeMode || PAGE_CROSSED(addr, absIndexed))
            read( ((addr & 0xff00) | (absIndexed & 0xff)));
    }
    if constexpr (storeMode) {
        if constexpr (Index != NONE) {
            if constexpr (Inst == STZ) write<true>(absIndexed, 0);
            if constexpr (Inst == STA) write<true>(absIndexed, a);
        } else {
            if constexpr (Inst == STY) write<true>(addr, y);
            if constexpr (Inst == STX) write<true>(addr, x);
            if constexpr (Inst == STA) write<true>(addr, a);
            if constexpr (Inst == STZ) write<true>(addr, 0);
        }
    } else {
        uint8_t data;
        if constexpr (Index != NONE)    data = read<true>(absIndexed);
        if constexpr (Index == NONE)    data = read<true>(addr);

        arithmetic<Inst>(data);
    }
}

template<uint8_t Inst, uint8_t Index> auto W65C02::opModifyAbsolute() -> void {
    uint16_t absIndexed;
    uint8_t data;
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    if constexpr (Index == INDEX_X) {
        absIndexed = addr + x;
        read( ((addr & 0xff00) | (absIndexed & 0xff)));
        data = read(absIndexed);
        write(absIndexed, data);
    } else {
        data = read(addr);
        write(addr, data);
    }
    arithmeticM<Inst>(data);
    if constexpr (Index == INDEX_X) write<true>(absIndexed, data);
    if constexpr (Index == NONE)    write<true>(addr, data);
}

// Absolute Indirect-(a)
auto W65C02::opJmpIndirect() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    uint16_t newPC = read(addr);
    read(addr); // dummy here ?
    newPC |= read<true>(addr + 1) << 8;
    pc = newPC;
}

// Zero Page zp
// Zero Page Indexed with X zp,x
// Zero Page Indexed with Y zp,y
template<uint8_t Inst, uint8_t Index> auto W65C02::opZeroPage() -> void {
    constexpr bool storeMode = Inst == STA || Inst == STX || Inst == STY || Inst == STZ;
    uint8_t zeroPage = readPC();

    if constexpr (Index != NONE) {
        read(zeroPage);
        if constexpr (Index == INDEX_X) zeroPage += x;
        if constexpr (Index == INDEX_Y) zeroPage += y;
    }

    if constexpr (storeMode) {
        if constexpr (Inst == STA) write<true>(zeroPage, a);
        if constexpr (Inst == STX) write<true>(zeroPage, x);
        if constexpr (Inst == STY) write<true>(zeroPage, y);
        if constexpr (Inst == STZ) write<true>(zeroPage, 0);
    } else {
        uint8_t data = read<true>(zeroPage);
        arithmetic<Inst>(data);
    }
}

template<uint8_t Inst, uint8_t Index> auto W65C02::opModifyZeroPage() -> void {
    uint8_t zeroPage = readPC();

    if constexpr (Index != NONE) {
        read(zeroPage);
        if constexpr (Index == INDEX_X) zeroPage += x;
        if constexpr (Index == INDEX_Y) zeroPage += y;
    }

    uint8_t data = read(zeroPage);
    write(zeroPage, data);
    arithmeticM<Inst>(data);
    write<true>(zeroPage, data);
}

// Zero Page Indexed Indirect (zp,x)
template<uint8_t Inst> auto W65C02::opZeroPageIndexedIndirect() -> void {
    uint8_t zeroPage = readPC();
    read(zeroPage);
    zeroPage += x;
    uint16_t addr = read(zeroPage);
    zeroPage += 1;
    addr |= read(zeroPage) << 8;

    if constexpr (Inst == STA)
        write<true>( addr, a);
    else {
        uint8_t data = read<true>(addr);
        arithmetic<Inst>(data);
    }
}

// Zero Page Indirect-(d)
// Zero Page Indirect Indexed-(d),y
template<uint8_t Inst, uint8_t Index> auto W65C02::opZeroPageIndirect() -> void {
    uint16_t absIndexed;
    uint8_t zeroPage = readPC();
    uint16_t addr = read(zeroPage);
    zeroPage += 1;
    addr |= read(zeroPage) << 8;

    if constexpr (Index == INDEX_Y) {
        absIndexed += y;

        if ((Inst == STA) || PAGE_CROSSED(addr, absIndexed))
            read( ((addr & 0xff00) | (absIndexed & 0xff)));
    }

    if constexpr (Inst == STA) {
        if constexpr (Index == INDEX_Y) write<true>( absIndexed, a);
        if constexpr (Index == NONE)    write<true>( addr, a);
    } else {
        uint8_t data;
        if constexpr (Index == INDEX_Y) data = read<true>(absIndexed);
        if constexpr (Index == NONE)    data = read<true>(addr);
        arithmetic<Inst>(data);
    }
}

// Immediate-#
template<uint8_t Inst> auto W65C02::opImmediate() -> void {
    uint8_t data = readPC<true>();
    arithmetic<Inst>(data);
}

// Program Counter Relative-r
auto W65C02::opBranch(bool take) -> void {
    uint8_t data = readPC<true>();

    if (take) {
        uint16_t addr = pc + int8_t(data);
        idle();
        if (PAGE_CROSSED(addr, pc))
            read<true>( ((pc & 0xff00) | (addr & 0xff)));

        pc = addr;
    }
}

template<uint8_t Inst> auto W65C02::opBB() -> void {
    uint8_t zeroPage = readPC();
    uint8_t offset = read(zeroPage);
    uint8_t data = readPC<true>();

    bool take;
    if constexpr (Inst == BBR0) take = (offset & 1) == 0;
    if constexpr (Inst == BBR1) take = (offset & 2) == 0;
    if constexpr (Inst == BBR2) take = (offset & 4) == 0;
    if constexpr (Inst == BBR3) take = (offset & 8) == 0;
    if constexpr (Inst == BBR4) take = (offset & 0x10) == 0;
    if constexpr (Inst == BBR5) take = (offset & 0x20) == 0;
    if constexpr (Inst == BBR6) take = (offset & 0x40) == 0;
    if constexpr (Inst == BBR7) take = (offset & 0x80) == 0;

    if constexpr (Inst == BBS0) take = offset & 1;
    if constexpr (Inst == BBS1) take = offset & 2;
    if constexpr (Inst == BBS2) take = offset & 4;
    if constexpr (Inst == BBS3) take = offset & 8;
    if constexpr (Inst == BBS4) take = offset & 0x10;
    if constexpr (Inst == BBS5) take = offset & 0x20;
    if constexpr (Inst == BBS6) take = offset & 0x40;
    if constexpr (Inst == BBS7) take = offset & 0x80;

    if (take) {
        uint16_t addr = pc + int8_t(data);
        idle();
        if (PAGE_CROSSED(addr, pc))
            read<true>( ((pc & 0xff00) | (addr & 0xff)));

        pc = addr;
    }
}

auto W65C02::opJSR() -> void {
    uint16_t newPC = readPC();
    idle();
    push(pc >> 8);
    push(pc & 0xff);
    newPC |= read<true>(pc) << 8;
    pc = newPC;
}

auto W65C02::opRTS() -> void {
    readPC();
    read(0x100 | s);
    uint16_t newPC = pull();
    newPC |= pull() << 8;
    pc = newPC;
    readPC<true>();
}

auto W65C02::opRTI() -> void {
    readPC();
    read(0x100 | s);
    p = pull();
    pc = pull();
    pc |= pull<true>() << 8;
}

auto W65C02::opPHP() -> void {
    read( pc );
    push<true>( p | 0x10 | 0x20 );
}

auto W65C02::opPLP() -> void {
    idle();
    read(0x100 | s);
    p = pull<true>();
}

auto W65C02::opClearD() -> void {
    idle<true>();
    p.d = false;
}

auto W65C02::opClearC() -> void {
    idle<true>();
    p.c = false;
}

auto W65C02::opClearV() -> void {
    idle<true>();
    p.v = false;
}

auto W65C02::opSetC() -> void {
    idle<true>();
    p.c = true;
}

auto W65C02::opSetD() -> void {
    idle<true>();
    p.d = true;
}

template<bool setI> auto W65C02::opUpdateI() -> void {
    SAMPLE_INTR
    while (lines & RDY_LINE) {
        SAMPLE_INTR
        REF_CALL READ_BYTE(pc);
        p.i = setI;
    }
    REF_CALL READ_BYTE(pc);
    p.i = setI;
}

auto W65C02::opJmpAbsolute() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC<true>() << 8;
    pc = newPC;
}

auto W65C02::opNoOp5c() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    read(0xff00 | addr);
    read(0xffff);
    read(0xffff);
    read(0xffff);
    read<true>(0xffff);
}

// Accumulator-A
// Implied-i
template<uint8_t Inst, uint8_t Index> auto W65C02::opImplied() -> void {
    idle<true>();
    if constexpr (Index == 0) arithmeticM<Inst>(a);
    if constexpr (Index == INDEX_X) arithmeticM<Inst>(x);
    if constexpr (Index == INDEX_Y) arithmeticM<Inst>(y);
}

// Stack-s
template<uint8_t Inst> auto W65C02::opPush() -> void {
    idle();
    if constexpr (Inst == STX) push<true>( x );
    if constexpr (Inst == STY) push<true>( y );
    if constexpr (Inst == STA) push<true>( a );
}

#define _PULL(reg) \
    reg = pull<true>();  \
    p.n = reg & 0x80;   \
    p.z = reg == 0;

template<uint8_t Inst> auto W65C02::opPull() -> void {
    idle();
    read( 0x100 | s );
    if constexpr (Inst == STX) { _PULL(x) }
    if constexpr (Inst == STY) { _PULL(y) }
    if constexpr (Inst == STA) { _PULL(a) }
}

auto W65C02::opTXS() -> void {
    idle<true>();
    s = x;
}

#define _TRANSFER(From, To) \
    idle<true>();  \
    To = From;  \
    p.z = To == 0;   \
    p.n = To & 0x80;

auto W65C02::opTSX() -> void { _TRANSFER(s, x) }
auto W65C02::opTXA() -> void { _TRANSFER(x, a) }
auto W65C02::opTYA() -> void { _TRANSFER(y, a) }
auto W65C02::opTAY() -> void { _TRANSFER(a, y) }
auto W65C02::opTAX() -> void { _TRANSFER(a, x) }

inline auto W65C02::opInvalid() -> void {
    readPC<true>();
}

inline auto W65C02::opNOP() -> void {
    idle<true>();
}

inline auto W65C02::opBRK() -> void {
    interrupt<false>(0xfffe);
}

auto W65C02::opWait() -> void {
    control |= WAI;
    UPDATE_RDY(true);
    idle<true>();
}

auto W65C02::opStop() -> void {
    control |= STP;
    idle();
}

}
