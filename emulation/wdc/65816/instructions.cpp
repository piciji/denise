
namespace WDCFAMILY {

// Absolute Indexed Indirect-(a,x)
template<bool JSR> auto W65816::opJmpAbsIndexedIndirect() -> void {
    uint16_t addr = readPC();
    if constexpr (JSR) {
        push<true>(pc >> 8);
        push<true>(pc & 0xff);
    }
    addr |= readPC() << 8;
    IDLE();
    uint16_t newPC = read((pbr << 16) | ((addr + x) & 0xffff) );
    SAMPLE_INTR
    newPC |= read((pbr << 16) | ((addr + x + 1) & 0xffff) ) << 8;
    pc = newPC;
    if constexpr (JSR) { if (modeE) setByteH(s, 1); }
}

// Absolute-a
// Absolute Indexed with X-a,x
// Absolute Indexed with X-a,y
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opAbsolute() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    constexpr bool storeMode = Inst == STA || Inst == STX || Inst == STY || Inst == STZ;

    if constexpr (storeMode) {
        if constexpr (Index != NONE) IDLE();
    } else {
        if constexpr (Index == INDEX_X) idle4(addr, addr + x);
        if constexpr (Index == INDEX_Y) idle4(addr, addr + y);
    }
    if constexpr (M) SAMPLE_INTR

    if constexpr (storeMode) {
        if constexpr (Inst == STZ && Index == INDEX_X) writeBank(addr + x, 0);
        if constexpr (Inst == STA && Index == INDEX_X) writeBank(addr + x, a & 0xff);
        if constexpr (Inst == STA && Index == INDEX_Y) writeBank(addr + y, a & 0xff);
        if constexpr (Inst == STY && Index == NONE) writeBank(addr, y & 0xff);
        if constexpr (Inst == STX && Index == NONE) writeBank(addr, x & 0xff);
        if constexpr (Inst == STA && Index == NONE) writeBank(addr, a & 0xff);
        if constexpr (Inst == STZ && Index == NONE) writeBank(addr, 0);

        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Inst == STZ && Index == INDEX_X) writeBank(addr + x + 1, 0);
            if constexpr (Inst == STA && Index == INDEX_X) writeBank(addr + x + 1, a >> 8);
            if constexpr (Inst == STA && Index == INDEX_Y) writeBank(addr + y + 1, a >> 8);
            if constexpr (Inst == STY && Index == NONE) writeBank(addr + 1, y >> 8);
            if constexpr (Inst == STX && Index == NONE) writeBank(addr + 1, x >> 8);
            if constexpr (Inst == STA && Index == NONE) writeBank(addr + 1, a >> 8);
            if constexpr (Inst == STZ && Index == NONE) writeBank(addr + 1, 0);
        }
    } else {
        uint16_t data;
        if constexpr (Index == INDEX_X) data = readBank(addr + x);
        if constexpr (Index == INDEX_Y) data = readBank(addr + y);
        if constexpr (Index == 0)       data = readBank(addr);

        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_X) data |= readBank(addr + x + 1) << 8;
            if constexpr (Index == INDEX_Y) data |= readBank(addr + y + 1) << 8;
            if constexpr (Index == 0)       data |= readBank(addr + 1) << 8;
        }
        arithmetic<M, Inst>(data);
    }
}

template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opModifyAbsolute() -> void {
    uint16_t data;
    uint16_t addr = readPC();
    addr |= readPC() << 8;
    if constexpr (Index != 0) IDLE();
    if constexpr (Index == INDEX_X) data = readBank(addr + x);
    if constexpr (Index == 0)       data = readBank(addr);

    if constexpr (!M) {
        if constexpr (Index == INDEX_X) data |= readBank(addr + x + 1) << 8;
        if constexpr (Index == 0)       data |= readBank(addr + 1) << 8;
    }
    IDLE();
    arithmeticM<M, Inst>(data);
    if constexpr (!M) {
        if constexpr (Index == INDEX_X) writeBank(addr + x + 1, data >> 8);
        if constexpr (Index == 0)       writeBank(addr + 1, data >> 8);
    }

    SAMPLE_INTR
    if constexpr (Index == INDEX_X) writeBank(addr + x, data & 0xff);
    if constexpr (Index == 0)       writeBank(addr, data & 0xff);
}

// Absolute Indirect-(a)
template<bool JML> auto W65816::opJmpIndirect() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;

    uint16_t newPC = read(addr);
    if constexpr (!JML) SAMPLE_INTR
    newPC |= read( uint16_t(addr + 1) ) << 8;

    if constexpr (JML) {
        SAMPLE_INTR
        pbr = read( uint16_t(addr + 2) );
    }
    pc = newPC;
}

// Absolute Long-al
// Absolute Long Indexed With X-al,x
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opLong() -> void {
    uint32_t addr = readPC();
    addr |= readPC() << 8;
    addr |= readPC() << 16;
    if constexpr (M) SAMPLE_INTR

    if constexpr (Inst == STA) {
        if constexpr (Index == INDEX_X) write((addr + x) & 0xffffff, a & 0xff);
        if constexpr (Index == 0)       write(addr, a & 0xff);
        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_X) write( (addr + x + 1) & 0xffffff, a >> 8);
            if constexpr (Index == 0)       write( (addr + 1) & 0xffffff, a >> 8);
        }
    } else {
        uint16_t data;
        if constexpr (Index == INDEX_X) data = read( (addr + x) & 0xffffff );
        if constexpr (Index == 0)       data = read( addr );
        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_X) data |= read( (addr + x + 1) & 0xffffff ) << 8;
            if constexpr (Index == 0)       data |= read( (addr + 1) & 0xffffff ) << 8;
        }
        arithmetic<M, Inst>(data);
    }
}

// Block Move-xyc
template<bool M, bool Mvn> auto W65816::opMove() -> void {
    dbr = readPC();
    uint8_t data = readPC();
    data = read((data << 16) | x);
    write((dbr << 16) | y, data);
    IDLE();
    if constexpr (Mvn) {
        if constexpr (M) {
            incByteL(x);
            incByteL(y);
        } else {
            x += 1;
            y += 1;
        }
    } else {
        if constexpr (M) {
            decByteL(x);
            decByteL(y);
        } else {
            x += -1;
            y += -1;
        }
    }
    SAMPLE_INTR
    IDLE();
    if(a--)
        pc -= 3;
}

// Direct Indexed Indirect-(d,x)
template<bool M, uint8_t Inst> auto W65816::opIndexedIndirect() -> void {
    uint16_t data = readPC();
    idle2();
    IDLE();
    uint16_t addr = getDirectAddressIndirect(data + x);
    if constexpr (M) SAMPLE_INTR

    if constexpr (Inst == STA) {
        writeBank( addr, a & 0xff);
        if constexpr (!M) {
            SAMPLE_INTR
            writeBank( addr + 1, a >> 8);
        }
    } else {
        data = readBank(addr);
        if constexpr (!M) {
            SAMPLE_INTR
            data |= readBank(addr + 1) << 8;
        }
        arithmetic<M, Inst>(data);
    }
}

// Direct-d
// Direct Indexed with X-d,x
// Direct Indexed with Y-d,y
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opDirect() -> void {
    constexpr bool storeMode = Inst == STA || Inst == STX || Inst == STY || Inst == STZ;
    uint8_t data = readPC();
    idle2();
    if constexpr (Index != 0) IDLE();
    if constexpr (M) SAMPLE_INTR

    if constexpr (storeMode) {
        if constexpr (Inst == STZ && Index == 0)        write( directAdr(data), 0 );
        if constexpr (Inst == STY && Index == 0)        write( directAdr(data), y & 0xff );
        if constexpr (Inst == STA && Index == 0)        write( directAdr(data), a & 0xff );
        if constexpr (Inst == STX && Index == 0)        write( directAdr(data), x & 0xff );
        if constexpr (Inst == STZ && Index == INDEX_X)  write( directAdr(data + x), 0 );
        if constexpr (Inst == STY && Index == INDEX_X)  write( directAdr(data + x), y & 0xff );
        if constexpr (Inst == STA && Index == INDEX_X)  write( directAdr(data + x), a & 0xff );
        if constexpr (Inst == STX && Index == INDEX_Y)  write( directAdr(data + y), x & 0xff );

        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Inst == STZ && Index == 0)        write( directAdr(data + 1), 0 );
            if constexpr (Inst == STY && Index == 0)        write( directAdr(data + 1), y >> 8 );
            if constexpr (Inst == STA && Index == 0)        write( directAdr(data + 1), a >> 8 );
            if constexpr (Inst == STX && Index == 0)        write( directAdr(data + 1), x >> 8 );
            if constexpr (Inst == STZ && Index == INDEX_X)  write( directAdr(data + x + 1), 0 );
            if constexpr (Inst == STY && Index == INDEX_X)  write( directAdr(data + x + 1), y >> 8 );
            if constexpr (Inst == STA && Index == INDEX_X)  write( directAdr(data + x + 1), a >> 8 );
            if constexpr (Inst == STX && Index == INDEX_Y)  write( directAdr(data + x + 1), x >> 8 );
        }
    } else {
        uint16_t operand;
        if constexpr (Index == INDEX_X) operand = read(directAdr( x + data));
        if constexpr (Index == INDEX_Y) operand = read(directAdr( y + data));
        if constexpr (Index == 0)       operand = read(directAdr( data));

        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_X) operand |= read(directAdr( x + data + 1)) << 8;
            if constexpr (Index == INDEX_Y) operand |= read(directAdr( y + data + 1)) << 8;
            if constexpr (Index == 0)       operand |= read(directAdr( data + 1)) << 8;
        }
        arithmetic<M, Inst>(operand);
    }
}

template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opModifyDirect() -> void {
    uint16_t addr1, addr2;
    uint16_t data = readPC();
    idle2();
    if constexpr (Index != 0) IDLE();

    if constexpr (Index == INDEX_X) addr1 = directAdr(data + x);
    if constexpr (Index == 0)       addr1 = directAdr(data);
    data = read(addr1);

    if constexpr (!M) {
        if constexpr (Index == INDEX_X) addr2 = directAdr(data + x + 1);
        if constexpr (Index == 0)       addr2 = directAdr(data + 1);
        data |= read(addr2) << 8;
    }
    IDLE();
    arithmeticM<M, Inst>(data);
    if constexpr (!M)
        write(addr2, data >> 8);

    SAMPLE_INTR
    write(addr1, data & 0xff);
}

// Direct Indirect-(d)
// Direct Indirect Indexed-(d),y
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opIndirect() -> void {
    uint16_t data = readPC();
    idle2();
    uint16_t addr = read(directAdr(data));
    addr |= read(directAdr(data + 1)) << 8;

    if constexpr (Inst == STA) {
        if constexpr (Index != 0) IDLE();
        if constexpr (M) SAMPLE_INTR
        if constexpr (Index == INDEX_Y) writeBank( addr + y, a & 0xff);
        if constexpr (Index == 0)       writeBank( addr, a & 0xff);
        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_Y) writeBank(addr + y + 1, a >> 8);
            if constexpr (Index == 0)       writeBank(addr + 1, a >> 8);
        }
    } else {
        uint16_t operand;
        if constexpr (Index != 0) idle4(addr, addr + y);
        if constexpr (M) SAMPLE_INTR

        if constexpr (Index == INDEX_Y) operand = readBank(addr + y);
        if constexpr (Index == 0)       operand = readBank(addr);
        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_Y) operand |= readBank(addr + y + 1) << 8;
            if constexpr (Index == 0)       operand |= readBank(addr + 1) << 8;
        }
        arithmetic<M, Inst>(operand);
    }
}

// Direct Indirect Long-[d]
// Direct Indirect Long Indexed-[d],y
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opIndirectLong() -> void {
    uint8_t data = readPC();
    idle2();
    uint32_t addr = read(uint16_t(d + data) );
    addr |= read(uint16_t(d + data + 1) ) << 8;
    addr |= read(uint16_t(d + data + 2) ) << 16;
    if constexpr (M) SAMPLE_INTR

    if constexpr (Inst == STA) {
        if constexpr (Index == INDEX_Y) write( (addr + y) & 0xffffff, a & 0xff );
        if constexpr (Index == 0)       write( addr, a & 0xff );

        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_Y) write( (addr + y + 1) & 0xffffff, a >> 8 );
            if constexpr (Index == 0)       write( (addr + 1) & 0xffffff, a >> 8 );
        }
    } else {
        uint16_t operand;
        if constexpr (Index == INDEX_Y) operand = read( (addr + y) & 0xffffff );
        if constexpr (Index == 0)       operand = read( addr );

        if constexpr (!M) {
            SAMPLE_INTR
            if constexpr (Index == INDEX_Y) operand |= read( (addr + y + 1) & 0xffffff ) << 8;
            if constexpr (Index == 0)       operand |= read( (addr + 1) & 0xffffff ) << 8;
        }
        arithmetic<M, Inst>(operand);
    }
}

// Immediate-#
template<bool M, uint8_t Inst> auto W65816::opImmediate() -> void {
    if constexpr (M) SAMPLE_INTR
    uint16_t operand = readPC();
    if constexpr (!M) {
        SAMPLE_INTR
        operand |= readPC() << 8;
    }
    arithmetic<M, Inst>(operand);
}

// Program Counter Relative-r
auto W65816::opBranch(bool take) -> void {
    if (take) {
        uint8_t data = readPC();
        uint16_t addr = pc + int8_t(data);
        idle6(addr);
        SAMPLE_INTR
        IDLE();
        pc = addr;
    } else {
        SAMPLE_INTR
        readPC();
    }
}

// Program Counter Relative Long-rl
auto W65816::opBranchLong() -> void {
    uint16_t data = readPC();
    data |= readPC() << 8;
    uint16_t addr = pc + int16_t(data);
    SAMPLE_INTR
    IDLE();
    pc = addr;
}

// Stack Relative-d,s
template<bool M, uint8_t Inst> auto W65816::opStackRelative() -> void {
    uint8_t data = readPC();
    IDLE();
    if constexpr (M) SAMPLE_INTR

    if constexpr (Inst == STA) {
        writeStack(data, a & 0xff);
        if constexpr (!M) {
            SAMPLE_INTR
            writeStack(data + 1, a >> 8);
        }
    } else {
        uint16_t operand = readStack(data);
        if constexpr (!M) {
            SAMPLE_INTR
            operand |= readStack(data + 1) << 8;
        }
        arithmetic<M, Inst>(operand);
    }
}

// Stack Relative Indirect Indexed-(d,s),y
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opStackRelativeIndirect() -> void {
    uint8_t data = readPC();
    IDLE();
    uint16_t addr = readStack(data);
    addr |= readStack(data + 1) << 8;
    IDLE();
    if constexpr (M) SAMPLE_INTR

    if constexpr (Inst == STA) {
        writeBank(addr + y, a & 0xff);
        if constexpr (!M) {
            SAMPLE_INTR
            writeBank(addr + y + 1, a >> 8);
        }
    } else {
        uint16_t operand = readBank(addr + y);
        if constexpr (!M) {
            SAMPLE_INTR
            operand |= readBank(addr + y + 1) << 8;
        }
        arithmetic<M, Inst>(operand);
    }
}

// Accumulator-A
// Implied-i
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opImplied() -> void {
    SAMPLE_INTR
    idleIrq();
    if constexpr (Index == 0) arithmeticM<M, Inst>(a);
    if constexpr (Index == INDEX_X) arithmeticM<M, Inst>(x);
    if constexpr (Index == INDEX_Y) arithmeticM<M, Inst>(y);
}

auto W65816::opTXS() -> void {
    SAMPLE_INTR
    idleIrq();
    if (modeE)
        setByteL(s, x & 0xff);
    else
        s = x;
}

#define _TRANSFER(M, From, To) \
    SAMPLE_INTR \
    idleIrq();  \
    if (M) {  \
        setByteL(To, From & 0xff);  \
        p.z = (To & 0xff) == 0;  \
        p.n = To & 0x80;     \
    } else {    \
        To = From;  \
        p.z = To == 0;   \
        p.n = To & 0x8000;   \
    }

// C = 16 Bit A
auto W65816::opTSC() -> void { _TRANSFER(false, s, a) }
auto W65816::opTCD() -> void { _TRANSFER(false, a, d) }
auto W65816::opTDC() -> void { _TRANSFER(false, d, a) }
auto W65816::opTCS() -> void {
    SAMPLE_INTR
    idleIrq();
    s = a;
    if (modeE) setByteH(s, 1);
}

auto W65816::opTSX() -> void { _TRANSFER(p.x, s, x) }
auto W65816::opTYX() -> void { _TRANSFER(p.x, y, x) }
auto W65816::opTXA() -> void { _TRANSFER(p.m, x, a) }
auto W65816::opTYA() -> void { _TRANSFER(p.m, y, a) }
auto W65816::opTXY() -> void { _TRANSFER(p.x, x, y) }
auto W65816::opTAY() -> void { _TRANSFER(p.x, a, y) }
auto W65816::opTAX() -> void { _TRANSFER(p.x, a, x) }

// Stack-s
template<bool M, uint8_t Inst> auto W65816::opPush() -> void {
    IDLE();
    if constexpr (!M) {
        if constexpr (Inst == STX) push( (x >> 8) & 0xff );
        if constexpr (Inst == STY) push( (y >> 8) & 0xff );
        if constexpr (Inst == STA) push( (a >> 8) & 0xff );
    }

    SAMPLE_INTR
    if constexpr (Inst == STX) push( x & 0xff );
    if constexpr (Inst == STY) push( y & 0xff );
    if constexpr (Inst == STA) push( a & 0xff );
}

auto W65816::opPHD() -> void {
    IDLE();
    push<true>( (d >> 8) & 0xff );
    SAMPLE_INTR
    push<true>( d & 0xff );
    if (modeE) setByteH(s, 1);
}

auto W65816::opPHP() -> void {
    IDLE();
    SAMPLE_INTR
    push( p );
}

auto W65816::opPHK() -> void {
    IDLE();
    SAMPLE_INTR
    push( pbr );
}

auto W65816::opPHB() -> void {
    IDLE();
    SAMPLE_INTR
    push( dbr );
}

auto W65816::opPLP() -> void {
    IDLE();
    IDLE();
    SAMPLE_INTR
    p = pull();
    if (modeE) p.x = p.m = true;
    if (p.x) {
        x &= 0xff;
        y &= 0xff;
    }
}

auto W65816::opPLD() -> void {
    IDLE();
    IDLE();
    d = pull<true>();
    SAMPLE_INTR
    d |= pull<true>() << 8;
    p.z = d == 0;
    p.n = d & 0x8000;
    if (modeE) setByteH(s, 1);
}

auto W65816::opPLB() -> void {
    IDLE();
    IDLE();
    SAMPLE_INTR
    pbr = pull<true>();
    p.z = pbr == 0;
    p.n = pbr & 0x80;
    if (modeE) setByteH(s, 1);
}

#define _PULL8(reg) \
    SAMPLE_INTR \
    setByteL(reg, pull());  \
    p.n = reg & 0x80;   \
    p.z = (reg & 0xff) == 0;

#define _PULL(reg) \
    reg = pull();   \
    SAMPLE_INTR \
    reg |= pull() << 8; \
    p.n = reg & 0x8000; \
    p.z = reg == 0;

template<bool M, uint8_t Inst> auto W65816::opPull() -> void {
    IDLE();
    IDLE();
    if constexpr (M) {
        if constexpr (Inst == STX) { _PULL8(x) }
        if constexpr (Inst == STY) { _PULL8(y) }
        if constexpr (Inst == STA) { _PULL8(a) }
    } else {
        if constexpr (Inst == STX) { _PULL(x) }
        if constexpr (Inst == STY) { _PULL(y) }
        if constexpr (Inst == STA) { _PULL(a) }
    }
}

auto W65816::opJSR() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC() << 8;
    IDLE();
    pc--;
    push(pc >> 8);
    SAMPLE_INTR
    push(pc & 0xff);
    pc = newPC;
}

auto W65816::opJSL() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC() << 8;
    push<true>(pbr);
    IDLE();
    uint8_t newPbr = readPC();
    pc--;
    push<true>(pc >> 8);
    SAMPLE_INTR
    push(pc & 0xff);
    pc = newPC;
    pbr = newPbr;
    if (modeE) setByteH(s, 1);
}

auto W65816::opRTI() -> void {
    IDLE();
    IDLE();
    p = pull();
    if (modeE)
        p.x = p.m = true;

    if (p.x) {
        x &= 0xff;
        y &= 0xff;
    }

    pc = pull();
    if(modeE) {
        SAMPLE_INTR
        pc |= pull() << 8;
    } else {
        pc |= pull() << 8;
        SAMPLE_INTR
        pbr = pull();
    }
}

auto W65816::opRTS() -> void {
    IDLE();
    IDLE();
    uint16_t newPC = pull();
    newPC |= pull() << 8;
    SAMPLE_INTR
    IDLE();
    pc = newPC;
    pc++;
}

auto W65816::opRTL() -> void {
    IDLE();
    IDLE();
    uint16_t newPC = pull<true>();
    newPC |= pull<true>() << 8;
    SAMPLE_INTR
    pbr = pull<true>();
    pc = newPC;
    pc++;
    if (modeE) setByteH(s, 1);
}

auto W65816::opJmpAbsolute() -> void {
    uint16_t newPC = readPC();
    SAMPLE_INTR
    newPC |= readPC() << 8;
    pc = newPC;
}

auto W65816::opJmpAbsoluteLong() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC() << 8;
    SAMPLE_INTR
    pbr = readPC();
    pc = newPC;
}

auto W65816::opWDM() -> void {
    SAMPLE_INTR
    readPC();
}

inline auto W65816::opNOP() -> void {
    SAMPLE_INTR
    idleIrq();
}

auto W65816::opClear(bool& flag) -> void {
    SAMPLE_INTR
    idleIrq();
    flag = false;
}

auto W65816::opSet(bool& flag) -> void {
    SAMPLE_INTR
    idleIrq();
    flag = true;
}

auto W65816::opPer() -> void {
    uint16_t data = readPC();
    data |= readPC() << 8;
    IDLE();
    data = pc + (int16_t)data;
    push<true>(data >> 8);
    SAMPLE_INTR
    push<true>(data & 0xff);
    if (modeE) setByteH(s, 1);
}

auto W65816::opResetP() -> void {
    uint8_t data = readPC();
    SAMPLE_INTR
    IDLE();
    p = p & ~data;
    if (modeE) p.x = p.m = true;
    if (p.x) {
        setByteH(x, 0);
        setByteH(y, 0);
    }
}

auto W65816::opSetP() -> void {
    uint8_t data = readPC();
    SAMPLE_INTR
    IDLE();
    p = p | data;
    if (modeE) p.x = p.m = true;
    if (p.x) {
        setByteH(x, 0);
        setByteH(y, 0);
    }
}

auto W65816::opPushEffectiveIndirectAddress() -> void {
    uint8_t data = readPC();
    idle2();
    uint16_t addr = read(directAdr(data));
    addr |= read(directAdr(data + 1)) << 8;
    push<true>(addr >> 8);
    SAMPLE_INTR
    push<true>(addr & 0xff);
    if (modeE) setByteH(s, 1);
}

auto W65816::opPushEffectiveAddress() -> void {
    uint16_t data = readPC();
    data |= readPC() << 8;
    push<true>(data >> 8);
    SAMPLE_INTR
    push<true>(data & 0xff);
    if (modeE) setByteH(s, 1);
}

inline auto W65816::opXBA() -> void {
    IDLE();
    SAMPLE_INTR
    IDLE();
    a = (a >> 8) | (a << 8);
    p.z = (a & 0xff) == 0;
    p.n = a & 0x80;
}

auto W65816::opXCE() -> void {
    SAMPLE_INTR
    idleIrq();
    bool _c = p.c;
    p.c = modeE;
    modeE = _c;

    if(modeE) {
        p.x = p.m = true;
        setByteH(s, 1);
        x &= 0xff;
        y &= 0xff;
    }
}

inline auto W65816::opBRK() -> void {
    interrupt<false>(modeE ? 0xfffe : 0xffe6);
}

inline auto W65816::opCOP() -> void {
    interrupt<false>(modeE ? 0xfff4 : 0xffe4);
}

auto W65816::opWait() -> void {
    control |= WAI;
    SAMPLE_INTR
    IDLE();
}

auto W65816::opStop() -> void {
    control |= STP;
    IDLE();
}

}
