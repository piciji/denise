
namespace WDCFAMILY {

#define PAGE_CROSSED(a1, a2) (((a1) ^ (a2)) & 0xff00)

// Absolute Indexed Indirect-(a,x)
auto W65C02::opJmpAbsIndexedIndirect() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    uint16_t absIndexed = addr + x;
    read( ((addr & 0xff00) | (absIndexed & 0xff)));
    uint16_t newPC = read(absIndexed);
    newPC |= read<SAMPLE_INTR>((absIndexed + 1) & 0xffff) << 8;
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
            if constexpr (Inst == STZ) write<SAMPLE_INTR>(absIndexed, 0);
            if constexpr (Inst == STA) write<SAMPLE_INTR>(absIndexed, a);
        } else {
            if constexpr (Inst == STY) write<SAMPLE_INTR>(addr, y);
            if constexpr (Inst == STX) write<SAMPLE_INTR>(addr, x);
            if constexpr (Inst == STA) write<SAMPLE_INTR>(addr, a);
            if constexpr (Inst == STZ) write<SAMPLE_INTR>(addr, 0);
        }
    } else {
        uint8_t data;
        if constexpr (Index != NONE)    data = read<SAMPLE_INTR>(absIndexed);
        if constexpr (Index == NONE)    data = read<SAMPLE_INTR>(addr);

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
        if (PAGE_CROSSED(addr, absIndexed))
            read( ((addr & 0xff00) | (absIndexed & 0xff)));
        SET_MEMORY_LOCK(true);
        data = read(absIndexed);
        write(absIndexed, data);
    } else {
        SET_MEMORY_LOCK(true);
        data = read(addr);
        write(addr, data);
    }
    arithmeticM<Inst>(data);
    if constexpr (Index == INDEX_X) write<SAMPLE_INTR>(absIndexed, data);
    if constexpr (Index == NONE)    write<SAMPLE_INTR>(addr, data);
    SET_MEMORY_LOCK(false);
}

// Absolute Indirect-(a)
auto W65C02::opJmpIndirect() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    uint16_t newPC = read(addr);
    read(addr); // dummy here ?
    newPC |= read<SAMPLE_INTR>(addr + 1) << 8;
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
        if constexpr (Inst == STA) write<SAMPLE_INTR>(zeroPage, a);
        if constexpr (Inst == STX) write<SAMPLE_INTR>(zeroPage, x);
        if constexpr (Inst == STY) write<SAMPLE_INTR>(zeroPage, y);
        if constexpr (Inst == STZ) write<SAMPLE_INTR>(zeroPage, 0);
    } else {
        uint8_t data = read<SAMPLE_INTR>(zeroPage);
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

    SET_MEMORY_LOCK(true);
    uint8_t data = read(zeroPage);
    write(zeroPage, data);
    arithmeticM<Inst>(data);
    write<SAMPLE_INTR>(zeroPage, data);
    SET_MEMORY_LOCK(false);
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
        write<SAMPLE_INTR>( addr, a);
    else {
        uint8_t data = read<SAMPLE_INTR>(addr);
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
        if constexpr (Index == INDEX_Y) write<SAMPLE_INTR>( absIndexed, a);
        if constexpr (Index == NONE)    write<SAMPLE_INTR>( addr, a);
    } else {
        uint8_t data;
        if constexpr (Index == INDEX_Y) data = read<SAMPLE_INTR>(absIndexed);
        if constexpr (Index == NONE)    data = read<SAMPLE_INTR>(addr);
        arithmetic<Inst>(data);
    }
}

// Immediate-#
template<uint8_t Inst> auto W65C02::opImmediate() -> void {
    uint8_t data = readPC<SAMPLE_INTR>();
    arithmetic<Inst>(data);
}

// Program Counter Relative-r
auto W65C02::opBranch(bool take) -> void {
    uint8_t data = readPC<SAMPLE_INTR>();

    if (take) {
        uint16_t addr = pc + int8_t(data);
        idle();
        if (PAGE_CROSSED(addr, pc))
            read<SAMPLE_INTR>( ((pc & 0xff00) | (addr & 0xff)));

        pc = addr;
    }
}

template<uint8_t Inst> auto W65C02::opBB() -> void {
    uint8_t zeroPage = readPC();
    uint8_t offset = read(zeroPage);
    uint8_t data = readPC<SAMPLE_INTR>();

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
            read<SAMPLE_INTR>( ((pc & 0xff00) | (addr & 0xff)));

        pc = addr;
    }
}

auto W65C02::opJSR() -> void {
    uint16_t newPC = readPC();
    idle();
    push(pc >> 8);
    push(pc & 0xff);
    newPC |= read<SAMPLE_INTR>(pc) << 8;
    pc = newPC;
}

auto W65C02::opRTS() -> void {
    readPC();
    read(0x100 | s);
    uint16_t newPC = pull();
    newPC |= pull() << 8;
    pc = newPC;
    readPC<SAMPLE_INTR>();
}

auto W65C02::opRTI() -> void {
    readPC();
    read(0x100 | s);
    p = pull();
    pc = pull();
    pc |= pull<SAMPLE_INTR>() << 8;
}

auto W65C02::opPHP() -> void {
    read( pc );
    push<SAMPLE_INTR | WRITE_LATE_P_REG>( p | 0x10 | 0x20 );
}

auto W65C02::opPLP() -> void {
    idle();
    read(0x100 | s);
    p = pull<SAMPLE_INTR>();
}

auto W65C02::opClearD() -> void {
    idle<SAMPLE_INTR>();
    p.d = false;
}

auto W65C02::opClearC() -> void {
    idle<SAMPLE_INTR>();
    p.c = false;
}

auto W65C02::opClearV() -> void {
    idle<SAMPLE_INTR>();
    p.v = false;
    lines |= SOB_BLOCK2;
}

auto W65C02::opSetC() -> void {
    idle<SAMPLE_INTR>();
    p.c = true;
}

auto W65C02::opSetD() -> void {
    idle<SAMPLE_INTR>();
    p.d = true;
}

template<bool setI> auto W65C02::opUpdateI() -> void {
    setI ? idle<SET_FLAG_I | SAMPLE_INTR>() : idle<CLEAR_FLAG_I | SAMPLE_INTR>();
    p.i = setI;
}

auto W65C02::opJmpAbsolute() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC<SAMPLE_INTR>() << 8;
    pc = newPC;
}

auto W65C02::opNoOp5c() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    read(0xff00 | addr);
    read(0xffff);
    read(0xffff);
    read(0xffff);
    read<SAMPLE_INTR>(0xffff);
}

// Accumulator-A
// Implied-i
template<uint8_t Inst, uint8_t Index> auto W65C02::opImplied() -> void {
    idle<SAMPLE_INTR>();
    if constexpr (Index == 0) arithmeticM<Inst>(a);
    if constexpr (Index == INDEX_X) arithmeticM<Inst>(x);
    if constexpr (Index == INDEX_Y) arithmeticM<Inst>(y);
}

// Stack-s
template<uint8_t Inst> auto W65C02::opPush() -> void {
    idle();
    if constexpr (Inst == STX) push<SAMPLE_INTR>( x );
    if constexpr (Inst == STY) push<SAMPLE_INTR>( y );
    if constexpr (Inst == STA) push<SAMPLE_INTR>( a );
}

#define _PULL(reg) \
    reg = pull<SAMPLE_INTR>();  \
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
    idle<SAMPLE_INTR>();
    s = x;
}

#define _TRANSFER(From, To) \
    idle<SAMPLE_INTR>();  \
    To = From;  \
    p.z = To == 0;   \
    p.n = To & 0x80;

auto W65C02::opTSX() -> void { _TRANSFER(s, x) }
auto W65C02::opTXA() -> void { _TRANSFER(x, a) }
auto W65C02::opTYA() -> void { _TRANSFER(y, a) }
auto W65C02::opTAY() -> void { _TRANSFER(a, y) }
auto W65C02::opTAX() -> void { _TRANSFER(a, x) }

inline auto W65C02::opInvalid() -> void {
    readPC<SAMPLE_INTR>();
}

inline auto W65C02::opNOP() -> void {
    idle<SAMPLE_INTR>();
}

inline auto W65C02::opBRK() -> void {
    interrupt<false>(0xfffe);
}

auto W65C02::opWait() -> void {
    idle();
    control |= WAI;
    // don't change internal RDY state because this line can be forced from external.
    // if forced "hi" then WAI doesn't halt the CPU.
    // if forced "lo" then IRQ/NMI can't resume CPU.
    // external device can detect a WAI instruction by observing RDY line
    OUTPUT_RDY_LOW(); // when set RDY to "hi" in this callback, WAI will not happen.
    idle<SAMPLE_INTR>();
}

auto W65C02::opStop() -> void {
    control |= STP;
    idle();
}

}
