
#define SI_IF_M (M ? SAMPLE_INTR : 0)

namespace WDCFAMILY {

// Absolute Indexed Indirect-(a,x)
template<bool JSR> auto W65816::opJmpAbsIndexedIndirect() -> void {
    uint16_t addr = readPC();
    if constexpr (JSR) {
        push<NATIVE>(pc >> 8);
        push<NATIVE>(pc & 0xff);
    }
    addr |= readPCNoInc() << 8;
    readPCIdle();
    addr += x;
    uint16_t newPC = read((pbr << 16) | addr );
    newPC |= read<SAMPLE_INTR>((pbr << 16) | ((addr + 1) & 0xffff) ) << 8;
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
        if constexpr (Index == INDEX_X) readBankIdle((addr & 0xff00) | ((addr + x) & 0xff));
        if constexpr (Index == INDEX_Y) readBankIdle((addr & 0xff00) | ((addr + y) & 0xff));

        if constexpr (Inst == STZ && Index == INDEX_X) writeBank<SI_IF_M>(addr + x, 0);
        if constexpr (Inst == STA && Index == INDEX_X) writeBank<SI_IF_M>(addr + x, a & 0xff);
        if constexpr (Inst == STA && Index == INDEX_Y) writeBank<SI_IF_M>(addr + y, a & 0xff);
        if constexpr (Inst == STY && Index == NONE) writeBank<SI_IF_M>(addr, y & 0xff);
        if constexpr (Inst == STX && Index == NONE) writeBank<SI_IF_M>(addr, x & 0xff);
        if constexpr (Inst == STA && Index == NONE) writeBank<SI_IF_M>(addr, a & 0xff);
        if constexpr (Inst == STZ && Index == NONE) writeBank<SI_IF_M>(addr, 0);

        if constexpr (!M) {
            if constexpr (Inst == STZ && Index == INDEX_X) writeBank<SAMPLE_INTR>(addr + x + 1, 0);
            if constexpr (Inst == STA && Index == INDEX_X) writeBank<SAMPLE_INTR>(addr + x + 1, a >> 8);
            if constexpr (Inst == STA && Index == INDEX_Y) writeBank<SAMPLE_INTR>(addr + y + 1, a >> 8);
            if constexpr (Inst == STY && Index == NONE) writeBank<SAMPLE_INTR>(addr + 1, y >> 8);
            if constexpr (Inst == STX && Index == NONE) writeBank<SAMPLE_INTR>(addr + 1, x >> 8);
            if constexpr (Inst == STA && Index == NONE) writeBank<SAMPLE_INTR>(addr + 1, a >> 8);
            if constexpr (Inst == STZ && Index == NONE) writeBank<SAMPLE_INTR>(addr + 1, 0);
        }
    } else {
        uint16_t data;
        if constexpr (Index == INDEX_X) idle4(addr, addr + x);
        if constexpr (Index == INDEX_Y) idle4(addr, addr + y);

        if constexpr (Index == INDEX_X) data = readBank<SI_IF_M>(addr + x);
        if constexpr (Index == INDEX_Y) data = readBank<SI_IF_M>(addr + y);
        if constexpr (Index == 0)       data = readBank<SI_IF_M>(addr);

        if constexpr (!M) {
            if constexpr (Index == INDEX_X) data |= readBank<SAMPLE_INTR>(addr + x + 1) << 8;
            if constexpr (Index == INDEX_Y) data |= readBank<SAMPLE_INTR>(addr + y + 1) << 8;
            if constexpr (Index == 0)       data |= readBank<SAMPLE_INTR>(addr + 1) << 8;
        }
        arithmetic<M, Inst>(data);
    }
}

template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opModifyAbsolute() -> void {
    uint16_t data;
    uint16_t addr = readPC();
    addr |= readPCNoInc() << 8;
    if constexpr (Index != NONE)  readBankIdle((addr & 0xff00) | ((addr + x) & 0xff) );
    SET_MEMORY_LOCK(true);
    if constexpr (Index == INDEX_X) data = readBank(addr + x);
    if constexpr (Index == NONE)    data = readBank(addr);

    if constexpr (!M) {
        if constexpr (Index == INDEX_X) data |= readBank(addr + x + 1) << 8;
        if constexpr (Index == NONE)    data |= readBank(addr + 1) << 8;
    }

    if (modeE) { // M is always 1 in emulation mode
        if constexpr (Index == INDEX_X) writeBank(addr + x, data & 0xff);
        else                            writeBank(addr, data & 0xff);
    } else {
        if constexpr (!M) {
            if constexpr (Index == INDEX_X) readBankIdle(addr + x + 1 );
            if constexpr (Index == NONE)    readBankIdle(addr + 1);
        } else {
            if constexpr (Index == INDEX_X) readBankIdle(addr + x );
            if constexpr (Index == NONE)    readBankIdle(addr);
        }
    }
    pc++;

    arithmeticM<M, Inst>(data);
    if constexpr (!M) {
        if constexpr (Index == INDEX_X) writeBank(addr + x + 1, data >> 8);
        if constexpr (Index == NONE)    writeBank(addr + 1, data >> 8);
    }

    if constexpr (Index == INDEX_X) writeBank<SAMPLE_INTR>(addr + x, data & 0xff);
    if constexpr (Index == NONE)    writeBank<SAMPLE_INTR>(addr, data & 0xff);
    SET_MEMORY_LOCK(false);
}

// Absolute Indirect-(a)
template<bool JML> auto W65816::opJmpIndirect() -> void {
    uint16_t addr = readPC();
    addr |= readPC() << 8;

    uint16_t newPC = read(addr);
    newPC |= read<JML ? 0 : SAMPLE_INTR>( uint16_t(addr + 1) ) << 8;

    if constexpr (JML) {
        pbr = read<SAMPLE_INTR>( uint16_t(addr + 2) );
    }
    pc = newPC;
}

// Absolute Long-al
// Absolute Long Indexed With X-al,x
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opLong() -> void {
    uint32_t addr = readPC();
    addr |= readPC() << 8;
    addr |= readPC() << 16;

    if constexpr (Inst == STA) {
        if constexpr (Index == INDEX_X) write<SI_IF_M>((addr + x) & 0xffffff, a & 0xff);
        if constexpr (Index == NONE)    write<SI_IF_M>(addr, a & 0xff);
        if constexpr (!M) {
            if constexpr (Index == INDEX_X) write<SAMPLE_INTR>( (addr + x + 1) & 0xffffff, a >> 8);
            if constexpr (Index == NONE)    write<SAMPLE_INTR>( (addr + 1) & 0xffffff, a >> 8);
        }
    } else {
        uint16_t data;
        if constexpr (Index == INDEX_X) data = read<SI_IF_M>( (addr + x) & 0xffffff );
        if constexpr (Index == NONE)    data = read<SI_IF_M>( addr );
        if constexpr (!M) {
            if constexpr (Index == INDEX_X) data |= read<SAMPLE_INTR>( (addr + x + 1) & 0xffffff ) << 8;
            if constexpr (Index == NONE)    data |= read<SAMPLE_INTR>( (addr + 1) & 0xffffff ) << 8;
        }
        arithmetic<M, Inst>(data);
    }
}

// Block Move-xyc
template<bool M, bool Mvn> auto W65816::opMove() -> void {
    dbr = readPC();
    uint8_t data = readPC();
    data = read((data << 16) | x);
    uint32_t addr = (dbr << 16) | y;
    write(addr, data);
    idle(addr);
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

    idle<SAMPLE_INTR>(addr);
    if(a--)
        pc -= 3;
}

// Direct Indexed Indirect-(d,x)
template<bool M, uint8_t Inst> auto W65816::opIndexedIndirect() -> void {
    uint16_t data = readPC();
    idle2();
    readPCIdle();
    uint16_t addr = getDirectAddressIndirect(data + x);

    if constexpr (Inst == STA) {
        writeBank<SI_IF_M>( addr, a & 0xff);
        if constexpr (!M)
            writeBank<SAMPLE_INTR>( addr + 1, a >> 8);
    } else {
        data = readBank<SI_IF_M>(addr);
        if constexpr (!M)
            data |= readBank<SAMPLE_INTR>(addr + 1) << 8;

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
    if constexpr (Index != 0) readPCIdle();

    if constexpr (storeMode) {
        if constexpr (Inst == STZ && Index == 0)        write<SI_IF_M>( directAdr(data), 0 );
        if constexpr (Inst == STY && Index == 0)        write<SI_IF_M>( directAdr(data), y & 0xff );
        if constexpr (Inst == STA && Index == 0)        write<SI_IF_M>( directAdr(data), a & 0xff );
        if constexpr (Inst == STX && Index == 0)        write<SI_IF_M>( directAdr(data), x & 0xff );
        if constexpr (Inst == STZ && Index == INDEX_X)  write<SI_IF_M>( directAdr(data + x), 0 );
        if constexpr (Inst == STY && Index == INDEX_X)  write<SI_IF_M>( directAdr(data + x), y & 0xff );
        if constexpr (Inst == STA && Index == INDEX_X)  write<SI_IF_M>( directAdr(data + x), a & 0xff );
        if constexpr (Inst == STX && Index == INDEX_Y)  write<SI_IF_M>( directAdr(data + y), x & 0xff );

        if constexpr (!M) {
            if constexpr (Inst == STZ && Index == 0)        write<SAMPLE_INTR>( directAdr(data + 1), 0 );
            if constexpr (Inst == STY && Index == 0)        write<SAMPLE_INTR>( directAdr(data + 1), y >> 8 );
            if constexpr (Inst == STA && Index == 0)        write<SAMPLE_INTR>( directAdr(data + 1), a >> 8 );
            if constexpr (Inst == STX && Index == 0)        write<SAMPLE_INTR>( directAdr(data + 1), x >> 8 );
            if constexpr (Inst == STZ && Index == INDEX_X)  write<SAMPLE_INTR>( directAdr(data + x + 1), 0 );
            if constexpr (Inst == STY && Index == INDEX_X)  write<SAMPLE_INTR>( directAdr(data + x + 1), y >> 8 );
            if constexpr (Inst == STA && Index == INDEX_X)  write<SAMPLE_INTR>( directAdr(data + x + 1), a >> 8 );
            if constexpr (Inst == STX && Index == INDEX_Y)  write<SAMPLE_INTR>( directAdr(data + y + 1), x >> 8 );
        }
    } else {
        uint16_t operand;
        if constexpr (Index == INDEX_X) operand = read<SI_IF_M>(directAdr( x + data));
        if constexpr (Index == INDEX_Y) operand = read<SI_IF_M>(directAdr( y + data));
        if constexpr (Index == 0)       operand = read<SI_IF_M>(directAdr( data));

        if constexpr (!M) {
            if constexpr (Index == INDEX_X) operand |= read<SAMPLE_INTR>(directAdr( x + data + 1)) << 8;
            if constexpr (Index == INDEX_Y) operand |= read<SAMPLE_INTR>(directAdr( y + data + 1)) << 8;
            if constexpr (Index == 0)       operand |= read<SAMPLE_INTR>(directAdr( data + 1)) << 8;
        }
        arithmetic<M, Inst>(operand);
    }
}

template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opModifyDirect() -> void {
      uint16_t addr1, addr2;
    uint16_t data;
    uint8_t offset = readPC();
    idle2();
    if constexpr (Index != 0) readPCIdle();

    SET_MEMORY_LOCK(true);
    if constexpr (Index == INDEX_X) addr1 = directAdr(offset + x);
    if constexpr (Index == 0)       addr1 = directAdr(offset);
    data = read(addr1);

    if constexpr (!M) {
        if constexpr (Index == INDEX_X) addr2 = directAdr(offset + x + 1);
        if constexpr (Index == 0)       addr2 = directAdr(offset + 1);
        data |= read(addr2) << 8;
    }

    if (modeE) {
        write(addr1, data & 0xff);
    } else {
        if constexpr (!M)   idle(addr2);
        else                idle(addr1);
    }

    arithmeticM<M, Inst>(data);
    if constexpr (!M)
        write(addr2, data >> 8);

    write<SAMPLE_INTR>(addr1, data & 0xff);
    SET_MEMORY_LOCK(false);
}

// Direct Indirect-(d)
// Direct Indirect Indexed-(d),y
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opIndirect() -> void {
    uint16_t data = readPC();
    idle2();
    uint16_t addr = read(directAdr(data));
    addr |= read(directAdr(data + 1)) << 8;

    if constexpr (Inst == STA) {
        if constexpr (Index == INDEX_Y) readBankIdle((addr & 0xff00) | ((addr + y) & 0xff) );
        if constexpr (Index == INDEX_Y) writeBank<SI_IF_M>( addr + y, a & 0xff);
        if constexpr (Index == 0)       writeBank<SI_IF_M>( addr, a & 0xff);
        if constexpr (!M) {
            if constexpr (Index == INDEX_Y) writeBank<SAMPLE_INTR>(addr + y + 1, a >> 8);
            if constexpr (Index == 0)       writeBank<SAMPLE_INTR>(addr + 1, a >> 8);
        }
    } else {
        uint16_t operand;
        if constexpr (Index != 0) idle4(addr, addr + y);

        if constexpr (Index == INDEX_Y) operand = readBank<SI_IF_M>(addr + y);
        if constexpr (Index == 0)       operand = readBank<SI_IF_M>(addr);
        if constexpr (!M) {
            if constexpr (Index == INDEX_Y) operand |= readBank<SAMPLE_INTR>(addr + y + 1) << 8;
            if constexpr (Index == 0)       operand |= readBank<SAMPLE_INTR>(addr + 1) << 8;
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

    if constexpr (Inst == STA) {
        if constexpr (Index == INDEX_Y) write<SI_IF_M>( (addr + y) & 0xffffff, a & 0xff );
        if constexpr (Index == 0)       write<SI_IF_M>( addr, a & 0xff );

        if constexpr (!M) {
            if constexpr (Index == INDEX_Y) write<SAMPLE_INTR>( (addr + y + 1) & 0xffffff, a >> 8 );
            if constexpr (Index == 0)       write<SAMPLE_INTR>( (addr + 1) & 0xffffff, a >> 8 );
        }
    } else {
        uint16_t operand;
        if constexpr (Index == INDEX_Y) operand = read<SI_IF_M>( (addr + y) & 0xffffff );
        if constexpr (Index == 0)       operand = read<SI_IF_M>( addr );

        if constexpr (!M) {
            if constexpr (Index == INDEX_Y) operand |= read<SAMPLE_INTR>( (addr + y + 1) & 0xffffff ) << 8;
            if constexpr (Index == 0)       operand |= read<SAMPLE_INTR>( (addr + 1) & 0xffffff ) << 8;
        }
        arithmetic<M, Inst>(operand);
    }
}

// Immediate-#
template<bool M, uint8_t Inst> auto W65816::opImmediate() -> void {
    uint16_t operand = readPC<SI_IF_M>();
    if constexpr (!M)
        operand |= readPC<SAMPLE_INTR>() << 8;

    arithmetic<M, Inst>(operand);
}

// Program Counter Relative-r
auto W65816::opBranch(bool take) -> void {
    uint8_t data = readPC<SAMPLE_INTR>();

    if (take) {
        readPCIdle();
        uint16_t addr = pc + int8_t(data);
        if(modeE && PAGE_CROSSED(pc, addr))
            idle<SAMPLE_INTR>((pbr << 16) | (pc & 0xff00) | (addr & 0xff) );
        pc = addr;
    }
}

// Program Counter Relative Long-rl
auto W65816::opBranchLong() -> void {
    uint16_t data = readPC();
    data |= readPC() << 8;
    readPCIdle<SAMPLE_INTR>();
    pc += int16_t(data);
}

// Stack Relative-d,s
template<bool M, uint8_t Inst> auto W65816::opStackRelative() -> void {
    uint8_t data = readPC();
    readPCIdle();
    if constexpr (Inst == STA) {
        writeStack<SI_IF_M>(data, a & 0xff);
        if constexpr (!M)
            writeStack<SAMPLE_INTR>(data + 1, a >> 8);

    } else {
        uint16_t operand = readStack<SI_IF_M>(data);
        if constexpr (!M)
            operand |= readStack<SAMPLE_INTR>(data + 1) << 8;

        arithmetic<M, Inst>(operand);
    }
}

// Stack Relative Indirect Indexed-(d,s),y
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opStackRelativeIndirect() -> void {
    uint8_t data = readPC();
    readPCIdle();
    uint16_t addr = readStack(data);
    addr |= readStack(data + 1) << 8;
    idle((s + data + 1) & 0xffff);

    if constexpr (Inst == STA) {
        writeBank<SI_IF_M>(addr + y, a & 0xff);
        if constexpr (!M)
            writeBank<SAMPLE_INTR>(addr + y + 1, a >> 8);
    } else {
        uint16_t operand = readBank<SI_IF_M>(addr + y);
        if constexpr (!M)
            operand |= readBank<SAMPLE_INTR>(addr + y + 1) << 8;

        arithmetic<M, Inst>(operand);
    }
}

// Accumulator-A
// Implied-i
template<bool M, uint8_t Inst, uint8_t Index> auto W65816::opImplied() -> void {
    idleIrq();
    if constexpr (Index == 0) arithmeticM<M, Inst>(a);
    if constexpr (Index == INDEX_X) arithmeticM<M, Inst>(x);
    if constexpr (Index == INDEX_Y) arithmeticM<M, Inst>(y);
}

auto W65816::opTXS() -> void {
    idleIrq();
    if (modeE)
        setByteL(s, x & 0xff);
    else
        s = x;
}

#define _TRANSFER(M, From, To) \
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
    readPCIdle();
    if constexpr (!M) {
        if constexpr (Inst == STX) push( (x >> 8) & 0xff );
        if constexpr (Inst == STY) push( (y >> 8) & 0xff );
        if constexpr (Inst == STA) push( (a >> 8) & 0xff );
    }

    if constexpr (Inst == STX) push<SAMPLE_INTR>( x & 0xff );
    if constexpr (Inst == STY) push<SAMPLE_INTR>( y & 0xff );
    if constexpr (Inst == STA) push<SAMPLE_INTR>( a & 0xff );
}

auto W65816::opPHD() -> void {
    readPCIdle();
    push<NATIVE>( (d >> 8) & 0xff );
    push<NATIVE | SAMPLE_INTR>( d & 0xff );
    if (modeE) setByteH(s, 1);
}

auto W65816::opPHP() -> void {
    readPCIdle<SAMPLE_INTR>();
    push( p );
}

auto W65816::opPHK() -> void {
    readPCIdle();
    push<SAMPLE_INTR>( pbr );
}

auto W65816::opPHB() -> void {
    readPCIdle();
    push<SAMPLE_INTR>( dbr );
}

auto W65816::opPLP() -> void {
    readPCIdle();
    readPCIdle();
    p = pull<SAMPLE_INTR>();
    if (modeE) p.x = p.m = true;
    if (p.x) {
        x &= 0xff;
        y &= 0xff;
    }
}

auto W65816::opPLD() -> void {
    readPCIdle();
    readPCIdle();
    d = pull<NATIVE>();
    d |= pull<NATIVE | SAMPLE_INTR>() << 8;
    p.z = d == 0;
    p.n = d & 0x8000;
    if (modeE) setByteH(s, 1);
}

auto W65816::opPLB() -> void {
    readPCIdle();
    readPCIdle();
    dbr = pull<NATIVE | SAMPLE_INTR>();
    p.z = dbr == 0;
    p.n = dbr & 0x80;
    if (modeE) setByteH(s, 1);
}

#define _PULL8(reg) \
    setByteL(reg, pull<SAMPLE_INTR>());  \
    p.n = reg & 0x80;   \
    p.z = (reg & 0xff) == 0;

#define _PULL(reg) \
    reg = pull();   \
    reg |= pull<SAMPLE_INTR>() << 8; \
    p.n = reg & 0x8000; \
    p.z = reg == 0;

template<bool M, uint8_t Inst> auto W65816::opPull() -> void {
    readPCIdle();
    readPCIdle();
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
    newPC |= readPCNoInc() << 8;
    readPCIdle();
    push(pc >> 8);
    push<SAMPLE_INTR>(pc & 0xff);
    pc = newPC;
}

auto W65816::opJSL() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC() << 8;
    push<NATIVE>(pbr);
    idle((s + 1) & 0xffff );
    uint8_t newPbr = readPCNoInc();
    push<NATIVE>(pc >> 8);
    push<NATIVE | SAMPLE_INTR>(pc & 0xff);
    pc = newPC;
    pbr = newPbr;
    if (modeE) setByteH(s, 1);
}

auto W65816::opRTI() -> void {
    readPCIdle();
    readPCIdle();
    p = pull();
    if (modeE)
        p.x = p.m = true;

    if (p.x) {
        x &= 0xff;
        y &= 0xff;
    }

    pc = pull();
    if(modeE) {
        pc |= pull<SAMPLE_INTR>() << 8;
    } else {
        pc |= pull() << 8;
        pbr = pull<SAMPLE_INTR>();
    }
}

auto W65816::opRTS() -> void {
    readPCIdle();
    readPCIdle();
    uint16_t newPC = pull();
    newPC |= pull() << 8;
    idle<SAMPLE_INTR>(s);
    pc = newPC;
    pc++;
}

auto W65816::opRTL() -> void {
    readPCIdle();
    readPCIdle();
    uint16_t newPC = pull<NATIVE>();
    newPC |= pull<NATIVE>() << 8;
    pbr = pull<NATIVE | SAMPLE_INTR>();
    pc = newPC;
    pc++;
    if (modeE) setByteH(s, 1);
}

auto W65816::opJmpAbsolute() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC<SAMPLE_INTR>() << 8;
    pc = newPC;
}

auto W65816::opJmpAbsoluteLong() -> void {
    uint16_t newPC = readPC();
    newPC |= readPC() << 8;
    pbr = readPC<SAMPLE_INTR>();
    pc = newPC;
}

auto W65816::opWDM() -> void {
    readPC<SAMPLE_INTR>();
}

inline auto W65816::opNOP() -> void {
    idleIrq();
}

auto W65816::opClear(bool& flag) -> void {
    idleIrq();
    flag = false;
}

auto W65816::opSet(bool& flag) -> void {
    idleIrq();
    flag = true;
}

auto W65816::opPer() -> void {
    uint16_t data = readPC();
    data |= readPC() << 8;
    readPCIdle();
    data = pc + (int16_t)data;
    push<NATIVE>(data >> 8);
    push<NATIVE | SAMPLE_INTR>(data & 0xff);
    if (modeE) setByteH(s, 1);
}

auto W65816::opResetP() -> void {
    uint8_t data = readPCNoInc();
    readPCIdle<SAMPLE_INTR>();
    pc++;
    p = p & ~data;
    if (modeE) p.x = p.m = true;
    if (p.x) {
        setByteH(x, 0);
        setByteH(y, 0);
    }
}

auto W65816::opSetP() -> void {
    uint8_t data = readPCNoInc();
    readPCIdle<SAMPLE_INTR>();
    pc++;
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
    uint16_t operand = read(uint16_t(d + data));
    operand |= read(uint16_t(d + data + 1)) << 8;
    push<NATIVE>(operand >> 8);
    push<NATIVE | SAMPLE_INTR>(operand & 0xff);
    if (modeE) setByteH(s, 1);
}

auto W65816::opPushEffectiveAddress() -> void {
    uint16_t data = readPC();
    data |= readPC() << 8;
    push<NATIVE>(data >> 8);
    push<NATIVE | SAMPLE_INTR>(data & 0xff);
    if (modeE) setByteH(s, 1);
}

inline auto W65816::opXBA() -> void {
    readPCIdle();
    readPCIdle<SAMPLE_INTR>();
    a = (a >> 8) | (a << 8);
    p.z = (a & 0xff) == 0;
    p.n = a & 0x80;
}

auto W65816::opXCE() -> void {
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
    if (!TRAP_HANDLER())
        interrupt<false>(modeE ? 0xfff4 : 0xffe4);
}

auto W65816::opWait() -> void {
    control |= WAI;
    readPCIdle<SAMPLE_INTR>();
}

auto W65816::opStop() -> void {
    control |= STP;
    readPCIdle();
}

template<bool setI> auto W65816::opUpdateI() -> void {
    if (control & (IRQ_PENDING | NMI_PENDING))
        readPCNoInc<SAMPLE_INTR | (setI ? SET_FLAG_I : CLEAR_FLAG_I)>();
    else
        readPCIdle<SAMPLE_INTR | (setI ? SET_FLAG_I : CLEAR_FLAG_I)>();

    p.i = setI;
}

}
