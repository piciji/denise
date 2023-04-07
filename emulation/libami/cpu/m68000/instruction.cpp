
#include "m68000.h"

namespace M68FAMILY {

template<uint8_t Inst, uint8_t Size> auto M68000::opImmShift(uint16_t opcode) -> void {
    int reg = opcode & 7;
    uint8_t shift = (opcode >> 9) & 7;
    if (shift == 0) shift = 8;

    prefetch<SampleIPL>();
    uint32_t result = shifter<Inst, Size>(readRegD<Size>( reg ), shift );
    SYNC( (Size == Long ? 4 : 2) + shift * 2);
    writeRegD<Size>(reg, result);
}

template<uint8_t Inst, uint8_t Size> auto M68000::opRegShift(uint16_t opcode) -> void {
    int reg = opcode & 7;
    int shift = readRegD( (opcode >> 9) & 7 ) & 63;

    prefetch<SampleIPL>();
    uint32_t result = shifter<Inst, Size>(readRegD<Size>( reg ), shift );
    SYNC( (Size == Long ? 4 : 2) + shift * 2);
    writeRegD<Size>(reg, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opShift(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Word>(opcode & 7, result, ea))
        return;
    
    prefetch<SampleIPL>();
    writeEA<Mode, Word>(ea, singleShifter<Inst>(result) );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opBit(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    int bits = (Size == Long) ? (regsD[ reg ] & 31) : (regsD[ reg ] & 7);

    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    result = bit<Inst>(result, bits);
    prefetch<SampleIPL>();

    if constexpr(Mode == Immediate) SYNC(2);
    if constexpr(Mode == DataRegisterDirect) cyclesBit<Inst>(bits);
    if constexpr(Inst != Btst) writeEA<Mode, Size>(ea, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opImmBit(uint16_t opcode) -> void {
    uint32_t result, ea;
    int bits = (Size == Long) ? (irc & 31) : (irc & 7);
    readExtensionWord();

    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;
    
    result = bit<Inst>(result, bits);
    prefetch<SampleIPL>();

    if constexpr(Mode == DataRegisterDirect) cyclesBit<Inst>(bits);
    if constexpr(Inst != Btst) writeEA<Mode, Size>(ea, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opClr(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Size>(opcode & 7, result, ea)) // dummy read
        return;
    
    z = true;
    n = v = c = false;
    prefetch<SampleIPL>();

    if constexpr( Mode == DataRegisterDirect && Size == Long) SYNC( 2 );
    writeEA<Mode, Size>(ea, 0);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opNbcd(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Byte>(opcode & 7, result, ea))
        return;

    result = bcd<Sbcd>(result, 0);
    prefetch<SampleIPL>();

    if constexpr( Mode == DataRegisterDirect ) SYNC( 2 );
    writeEA<Mode, Byte>(ea, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opNeg(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    result = arithmetic<Inst, Size>(result, 0);
    prefetch<SampleIPL>();

    if constexpr( Mode == DataRegisterDirect && Size == Long) SYNC( 2 );
    writeEA<Mode, Size>(ea, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opScc(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Byte>(opcode & 7, result, ea))
        return;

    result = testCondition<Inst>() ? 0xff : 0;

    prefetch<SampleIPL>();
    if ( result && Mode == DataRegisterDirect ) SYNC( 2 );
    writeEA<Mode, Byte>(ea, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opTas(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Byte, TasCycle>(opcode & 7, result, ea))
        return;

    // check if MSB changed, which is found out via status flags.
    // second CPU shouldn't get the BUS while AS line is held high.
    // without AS line keeping high, it could be happen, that when two CPU's accessing TAS the same moment,
    // both CPU's detect a changed MSB and assume they own the BUS.
    // TAS make sure, second CPU can not read (if AS line is respected) before first CPU finished "write" cycle.
    defaultFlags<Byte>(result);
    result |= 0x80;

    if ( Mode != DataRegisterDirect )
        SYNC( 2 );

    writeEA<Mode, Byte, TasCycle>(ea, result);

    #ifdef TAS_SPINLOCK
    if constexpr(Mode != DataRegisterDirect)
        TAS_CYCLE_END();
    #endif

    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opTst(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    defaultFlags<Size>(result);
    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opArithmetic(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    prefetch<SampleIPL>();

    if constexpr (Size == Long) {
        if constexpr(isDirectMode<Mode>())  SYNC(4);
        else                                SYNC(2);
    }

    writeRegD<Size>(reg, arithmetic<Inst, Size>( result, readRegD<Size>(reg) ));
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opCmp(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    prefetch<SampleIPL>();
    if constexpr(Size == Long) SYNC( 2 );
    arithmetic<Cmp, Size>( result, readRegD<Size>(reg) );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opArithmeticEA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    result = arithmetic<Inst, Size>(readRegD<Size>(reg), result);
    prefetch<SampleIPL>();

    if constexpr (Size == Long && isRegisterMode<Mode>()) SYNC(4);
    writeEA<Mode, Size>(ea, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opArithmeticA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    result = sign<Size>(result);

    prefetch<SampleIPL>();
    if constexpr( Size == Word || isDirectMode<Mode>() ) SYNC(4);
    else SYNC(2);

    writeRegA(reg, (Inst == Adda) ? (result + readRegA<Long>(reg)) : (readRegA<Long>(reg) - result) );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opCmpa(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    result = sign<Size>(result);
    arithmetic<Cmp, Long>( result, readRegA<Long>(reg) );
    prefetch<SampleIPL>();
    SYNC( 2 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMul(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Word>(opcode & 7, result, ea))
        return;

    prefetch<SampleIPL>();
    cyclesMul<Inst>(result);
    if constexpr(Inst == Mulu)  result = result * readRegD<Word>(reg);
    else                        result = (int16_t)result * (int16_t)readRegD<Word>(reg);

    defaultFlags<Long>(result);
    writeRegD<Long>(reg, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opDiv(uint16_t opcode) -> void {
    uint32_t result, divisor, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Word>(opcode & 7, divisor, ea))
        return;

    uint32_t dividend = readRegD<Long>(reg);

    if (divisor == 0) {
        v = c = false;
        if constexpr(Inst == Divu) {
            n = negative<Long>(dividend);
            z = (dividend & 0xffff0000) == 0;
        } else {
            n = false;
            z = true;
        }
        SYNC(8);
        trapException(5);
        return;
    }

    bool overflow;

    if constexpr(Inst == Divu) {
        uint32_t quotient = dividend / divisor;
        uint16_t remainder = dividend % divisor;
        overflow = quotient > 0xffff;
        result = (quotient & 0xffff) | (remainder << 16);

    } else {
        overflow = dividend == 0x80000000 && (int16_t)divisor == -1;

        if (!overflow) {
            int32_t quotient = (int32_t) dividend / (int16_t) divisor;
            int16_t remainder = (int32_t) dividend % (int16_t) divisor;

            overflow = ((quotient & 0xffff8000) != 0 && (quotient & 0xffff8000) != 0xffff8000);

            if (!overflow) {
                // in C++ -7 % 3 = -1 (ok), 7 % -3 = 1 (but 68k calculates -1)
                if ((remainder < 0) != ((int32_t) dividend < 0))
                    remainder = -remainder;

                result = (quotient & 0xffff) | (remainder << 16);
            }
        }
    }

    if (overflow) {
        if constexpr(Inst == Divu)  SYNC(6);
        else cyclesDiv<Inst>(dividend, divisor);

        v = n = true;
        c = z = false;
    } else {
        cyclesDiv<Inst>(dividend, divisor);

        defaultFlags<Word>(result);
        writeRegD<Long>(reg, result);
    }

    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMove(uint16_t opcode) -> void {
    // Inst is target Mode
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    if constexpr(Inst == DataRegisterDirect)
        writeRegD<Size>(reg, result);
    else {
        uint16_t IR;
        if constexpr(Inst == AbsoluteLong && !isDirectMode<Mode>())
            ea = calcEA<Inst, Size, SkipExtension>(reg);
        else if constexpr(Inst == AddressRegisterIndirectWithPreDecrement)
            ea = calcEA<Inst, Size, ConcurrentAdrCalc>(reg);
        else
            ea = calcEA<Inst, Size>(reg);

        if constexpr (Inst == AddressRegisterIndirectWithPreDecrement) {
            if constexpr (Size != Long)
                writeRegA(reg, ea);

            if constexpr(Size == Long) IR = ird;
            prefetch<SampleIPL>();
        }

        if (misaligned<Size>(ea)) {
            if constexpr(Size == Long && Inst == AddressRegisterIndirectWithPreDecrement) ird = IR;
            setMoveCCWhenAddressError<Mode, Inst, Size>(result);
            addressExceptionMoveEA<Inst, Size>(ea, result);
            return;
        }

        if constexpr (Inst == AddressRegisterIndirectWithPostIncrement)
            writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));

        if constexpr (Inst == AddressRegisterIndirectWithPreDecrement) {
            if constexpr (Size == Long)
                writeRegA(reg, ea);

            write<Size>(ea, result);
        } else if constexpr( (Inst == AddressRegisterIndirectWithPostIncrement)
                || ( Inst == AddressRegisterIndirect && ((Size == Long) || isDirectMode<Mode>()) )
                || ( Inst == AddressRegisterIndirectWithDisplacement && isDirectMode<Mode>() )
                || ( Inst == AddressRegisterIndirectWithIndex && isDirectMode<Mode>() )
                || ( Inst == AbsoluteLong && !isDirectMode<Mode>() )
        ) // check results in "iplLatches"
            write<Size, SampleIPL | Reverse>(ea, result);
        else
            write<Size, Reverse>(ea, result);

        if constexpr(Inst == AbsoluteLong && !isDirectMode<Mode>())
            readExtensionWord();
    }

    defaultFlags<Size>(result);

    if constexpr(Inst == DataRegisterDirect)
        prefetch<SampleIPL>();
    else if constexpr(Inst != AddressRegisterIndirectWithPreDecrement) {
        // analyzed from microcode, a few modes sample IPL at last write and last prefetch (two times)
        if constexpr((Inst == AddressRegisterIndirectWithPostIncrement) ||
                    ( (Size == Long) && isDirectMode<Mode>() &&
                    (Inst == AddressRegisterIndirect || Inst == AddressRegisterIndirectWithDisplacement || Inst == AddressRegisterIndirectWithIndex))) {
            prefetch();
        } else
            prefetch<SampleIPL>();
    }
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMoveA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;
    
    writeRegA(reg, Size == Word ? (int16_t)result : result);
    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opArithmeticI(uint16_t opcode) -> void {
    uint32_t result, ea;
    uint32_t imm = calcEA<Immediate, Size>(0);
    
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;
    
    prefetch<SampleIPL>();
    result = arithmetic<Inst, Size>( imm, result);
    
    if constexpr(Size == Long && isRegisterMode<Mode>()) {
        if constexpr(Inst == Cmp)   SYNC(2);
        else                        SYNC(4);
    }
    
    if constexpr(Inst != Cmp)
        writeEA<Mode, Size>(ea, result);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opArithmeticQ(uint16_t opcode) -> void {
    uint32_t result, ea;
    uint32_t operand = (opcode >> 9) & 7;
    if (operand == 0) operand = 8;

    if (!readEA<Mode, Mode == AddressRegisterDirect ? Long : Size>(opcode & 7, result, ea))
        return;

    prefetch<SampleIPL>();
    if (Mode == AddressRegisterDirect)
        result = Inst == Add ? (result + operand) : (result - operand);
    else
        result = arithmetic<Inst, Size>( operand, result);

    if (Mode == AddressRegisterDirect || (Size == Long && Mode == DataRegisterDirect)) SYNC( 4 );
    writeEA<Mode, Size>(ea, result);
}

template<uint8_t Inst, uint8_t Size> auto M68000::opMoveQ(uint16_t opcode) -> void {
    uint8_t data = opcode & 0xff;

    prefetch<SampleIPL>();
    defaultFlags<Byte>(data);
    writeRegD((opcode >> 9) & 7, (int32_t)(int8_t)data);
}

template<uint8_t Inst, uint8_t Size> auto M68000::opBcc(uint16_t opcode) -> void {
    SYNC(2);

    if (testCondition<Inst>()) {
        uint32_t newPC = pc + (Size == Word ? (int16_t)irc : (int8_t)(opcode & 0xff));

        if (misaligned<Long>(newPC))
            return addressException(newPC, pc, SF_READ | SF_PRG);

        pc = newPC;
        fullPrefetch<SampleIPL>();
    } else {
        SYNC(2);
        if (Size == Word) readExtensionWord();
        prefetch<SampleIPL>();
    }
}

template<uint8_t Inst, uint8_t Size> auto M68000::opBsr(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    SYNC(2);
    sp -= 4;

    if (misaligned<Long>(sp))
        return addressException(sp, pc, SF_DATA, (Size == Word ? (pc + 2) : pc) >> 16 );

    write<Long, Reverse>( sp, Size == Word ? (pc + 2) : pc); // stack PC of next opcode

    pc += (Size == Word) ? (int16_t)irc : (int8_t)(opcode & 0xff);
    if (misaligned<Long>(pc))
        return addressException(pc, pc, SF_READ | SF_PRG); // stack new PC two times

    fullPrefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opDbcc(uint16_t opcode) -> void {
    SYNC(2);
    int reg = opcode & 7;
    uint32_t memPC = pc;
    
    if (!testCondition<Inst>()) {
        pc = memPC + (int16_t)irc;

        if (misaligned<Long>(pc))
            return addressException(pc, pc + 2, SF_READ | SF_PRG);

        firstPrefetch(); // assumes branch is taken, hence prefetch from new PC. (in order to speed up most likely case)
        uint16_t word = readRegD<Word>(reg);
        writeRegD<Word>( reg, word - 1);
        
        if (word) { // while prefetching, CPU finds out if branch is really taken
            prefetch<SampleIPL>();
            return;
        }
    } else
        SYNC(2);
    
    pc = memPC + 2;
    fullPrefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opJmp(uint16_t opcode) -> void {
    uint32_t adr = calcEA<Mode, Long, SkipExtension>(opcode & 7);
    
    if constexpr(isIndexMode<Mode>())                                   SYNC(4);
    if constexpr(Mode == AbsoluteShort || isDisplacementMode<Mode>())   SYNC(2);

    if (misaligned<Long>(adr))
        return addressException(adr, (Mode == AbsoluteLong) ? (pc - 2) : pc, SF_READ | SF_PRG);

    pc = adr;
    fullPrefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opJsr(uint16_t opcode) -> void {
    uint32_t adr = calcEA<Mode, Long, SkipExtension>(opcode & 7);

    if constexpr(isIndexMode<Mode>())                                   SYNC(4);
    if constexpr(Mode == AbsoluteShort || isDisplacementMode<Mode>())   SYNC(2);
    
    if constexpr(isAbsMode<Mode>())
        pc += 2;
    
    if (misaligned<Long>(adr))
        return addressException(adr, pc, SF_READ | SF_PRG);

    uint32_t& sp = regsA[7];
    sp -= 4;
    
    if constexpr(isDisplacementMode<Mode>() || isIndexMode<Mode>())
        pc += 2;

    uint32_t pcNextInstruction = pc;
    pc = adr;
    firstPrefetch(); // confirmed with fx68k, first prefetch happens before stacking

    if (misaligned<Long>(sp))
        return addressException(sp, pcNextInstruction, SF_DATA, pcNextInstruction >> 16); // stack PC of next instruction
    
    write<Long, Reverse>(sp, pcNextInstruction);
    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opLea(uint16_t opcode) -> void {
    writeRegA( (opcode >> 9) & 7, calcEA<Mode, Long>(opcode & 7) );
    if constexpr(isIndexMode<Mode>()) SYNC(2);
    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opPea(uint16_t opcode) -> void {
    uint32_t ea = calcEA<Mode, Long>(opcode & 7);
    uint16_t IR = ird;

    if constexpr(!isAbsMode<Mode>()) {
        sampleInterrupt(); // like all the other IPL sample points, it happens one clock cycle sooner (confirmed with fx68k)
        if constexpr(isIndexMode<Mode>()) SYNC(2);
        prefetch();
    }

    uint32_t& sp = regsA[7];
    sp -= 4;

    if (misaligned<Long>(sp)) {
        ird = IR;
        return addressException( sp, pc, SF_DATA, ea >> 16 );
    }

    write<Long, Reverse>(sp, ea);

    if constexpr(isAbsMode<Mode>())
        prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMovemToReg(uint16_t opcode) -> void {
    uint16_t mask = irc;
    readExtensionWord();
    uint32_t ea = calcEA<Mode, Size>(opcode & 7);

    if (misaligned<Long>(ea))
        return addressException(ea, isIndexMode<Mode>() ? (pc - 2) : (pc + 2), SF_READ | (isPcMode<Mode>() ? SF_PRG : SF_DATA ));

    if constexpr(Mode == AddressRegisterIndirectWithPostIncrement)
        sampleInterrupt();

    if (mask & (1 << 0)) { writeRegD(0, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 1)) { writeRegD(1, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 2)) { writeRegD(2, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 3)) { writeRegD(3, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 4)) { writeRegD(4, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 5)) { writeRegD(5, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 6)) { writeRegD(6, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 7)) { writeRegD(7, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }

    if (mask & (1 <<  8)) { writeRegA(0, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 <<  9)) { writeRegA(1, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 10)) { writeRegA(2, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 11)) { writeRegA(3, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 12)) { writeRegA(4, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 13)) { writeRegA(5, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 14)) { writeRegA(6, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 15)) { writeRegA(7, sign<Size>( read<Size, FC_EA_PRG>(ea) ) ); ea += Size; }

    if constexpr(Mode == AddressRegisterIndirectWithPostIncrement)
        writeRegA( opcode & 7, ea );

    read<Word, FC_EA_PRG>(ea); // dummy read (brackets for Long are wrong in Yacht )

    if constexpr(Mode != AddressRegisterIndirectWithPostIncrement)
        prefetch<SampleIPL>();
    else
        prefetch();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMovemToEa(uint16_t opcode) -> void {
    uint16_t mask = irc;
    readExtensionWord();

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement) {
        uint32_t ea = readRegA(opcode & 7);
        if (mask && misaligned<Long>(ea))
            return addressException(ea - 2, pc + 2, SF_DATA, firstMovemWrite(mask, 0, false) );

        if (mask & (1 << 0)) { ea -= Size; write<Size>(ea, regsA[7] ); }
        if (mask & (1 << 1)) { ea -= Size; write<Size>(ea, regsA[6] ); }
        if (mask & (1 << 2)) { ea -= Size; write<Size>(ea, regsA[5] ); }
        if (mask & (1 << 3)) { ea -= Size; write<Size>(ea, regsA[4] ); }
        if (mask & (1 << 4)) { ea -= Size; write<Size>(ea, regsA[3] ); }
        if (mask & (1 << 5)) { ea -= Size; write<Size>(ea, regsA[2] ); }
        if (mask & (1 << 6)) { ea -= Size; write<Size>(ea, regsA[1] ); }
        if (mask & (1 << 7)) { ea -= Size; write<Size>(ea, regsA[0] ); }

        if (mask & (1 << 8)) { ea -= Size; write<Size>(ea, regsD[7] ); }
        if (mask & (1 << 9)) { ea -= Size; write<Size>(ea, regsD[6] ); }
        if (mask & (1 << 10)) { ea -= Size; write<Size>(ea, regsD[5] ); }
        if (mask & (1 << 11)) { ea -= Size; write<Size>(ea, regsD[4] ); }
        if (mask & (1 << 12)) { ea -= Size; write<Size>(ea, regsD[3] ); }
        if (mask & (1 << 13)) { ea -= Size; write<Size>(ea, regsD[2] ); }
        if (mask & (1 << 14)) { ea -= Size; write<Size>(ea, regsD[1] ); }
        if (mask & (1 << 15)) { ea -= Size; write<Size>(ea, regsD[0] ); }

        writeRegA( opcode & 7, ea );
    } else {
        uint32_t ea = calcEA<Mode, Size>(opcode & 7);
        if (mask && misaligned<Long>(ea))
            return addressException(ea, pc + 2, SF_DATA, firstMovemWrite(mask, Size == Long ? 16 : 0, true) );

        if (mask & (1 << 0)) { write<Size, Reverse>(ea, regsD[0] ); ea += Size; }
        if (mask & (1 << 1)) { write<Size, Reverse>(ea, regsD[1] ); ea += Size; }
        if (mask & (1 << 2)) { write<Size, Reverse>(ea, regsD[2] ); ea += Size; }
        if (mask & (1 << 3)) { write<Size, Reverse>(ea, regsD[3] ); ea += Size; }
        if (mask & (1 << 4)) { write<Size, Reverse>(ea, regsD[4] ); ea += Size; }
        if (mask & (1 << 5)) { write<Size, Reverse>(ea, regsD[5] ); ea += Size; }
        if (mask & (1 << 6)) { write<Size, Reverse>(ea, regsD[6] ); ea += Size; }
        if (mask & (1 << 7)) { write<Size, Reverse>(ea, regsD[7] ); ea += Size; }

        if (mask & (1 << 8)) { write<Size, Reverse>(ea, regsA[0] ); ea += Size; }
        if (mask & (1 << 9)) { write<Size, Reverse>(ea, regsA[1] ); ea += Size; }
        if (mask & (1 << 10)) { write<Size, Reverse>(ea, regsA[2] ); ea += Size; }
        if (mask & (1 << 11)) { write<Size, Reverse>(ea, regsA[3] ); ea += Size; }
        if (mask & (1 << 12)) { write<Size, Reverse>(ea, regsA[4] ); ea += Size; }
        if (mask & (1 << 13)) { write<Size, Reverse>(ea, regsA[5] ); ea += Size; }
        if (mask & (1 << 14)) { write<Size, Reverse>(ea, regsA[6] ); ea += Size; }
        if (mask & (1 << 15)) { write<Size, Reverse>(ea, regsA[7] ); ea += Size; }
    }

    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opArithmeticX(uint16_t opcode) -> void {
    int destReg = (opcode >> 9) & 7;

    prefetch<SampleIPL>();
    if constexpr(Size == Long)  SYNC(4);

    writeRegD<Size>( destReg, arithmetic<Inst, Size>(readRegD<Size>( opcode & 7 ), readRegD<Size>( destReg ) ) );
}

template<uint8_t Inst, uint8_t Size> auto M68000::opArithmeticBCD(uint16_t opcode) -> void {
    int destReg = (opcode >> 9) & 7;

    prefetch<SampleIPL>();
    SYNC(2);

    writeRegD<Size>( destReg, bcd<Inst>(readRegD<Size>( opcode & 7 ), readRegD<Size>( destReg ) ) );
}

template<uint8_t Inst, uint8_t Size> auto M68000::opArithmeticXEa(uint16_t opcode) -> void {
    uint32_t result, dest, ea;
    int srcReg = opcode & 7;
    int destReg = (opcode >> 9) & 7;

    ea = readRegA( srcReg );
    ea -= (Size == Byte && srcReg == 7) ? 2 : Size;
    SYNC(2);

    if (misaligned<Size>(ea)) {
        if constexpr(Size == Word) writeRegA(srcReg, ea);
        return addressException(Size == Long ? (ea + 2) : ea, pc + 2, SF_READ | SF_DATA);
    }

    writeRegA(srcReg, ea);
    result = read<Size, Reverse>(ea);

    ea = readRegA( destReg );
    ea -= (Size == Byte && destReg == 7) ? 2 : Size;

    if (misaligned<Size>(ea)) {
        if constexpr(Size == Word) writeRegA(destReg, ea);
        return addressException(Size == Long ? (ea + 2) : ea, pc + 2, SF_READ | SF_DATA);
    }

    writeRegA(destReg, ea);
    if constexpr(Size != Long)  dest = read<Size, SampleIPL>(ea);
    else                        dest = read<Size, Reverse>(ea);

    if constexpr(Inst == Abcd || Inst == Sbcd)
        result = bcd<Inst>(result, dest );
    else
        result = arithmetic<Inst, Size>(result, dest );

    if constexpr(Size == Long) write<Word, SampleIPL>( ea + 2, result & 0xffff );

    prefetch();

    if constexpr(Size == Long)  write<Word>( ea, (result >> 16) & 0xffff );
    else                        write<Size>( ea, result );
}

template<uint8_t Inst, uint8_t Size> auto M68000::opCmpm(uint16_t opcode) -> void {
    uint32_t result, dest, ea;
    int srcReg = opcode & 7;
    int destReg = (opcode >> 9) & 7;

    ea = readRegA( srcReg );
    if (misaligned<Size>(ea))
        return addressException(ea, pc + 2, SF_READ | SF_DATA);

    result = read<Size>(ea);
    writeRegA(srcReg, ea + ((Size == Byte && srcReg == 7) ? 2 : Size) );

    ea = readRegA( destReg );
    if (misaligned<Size>(ea))
        return addressException(ea, pc + 2, SF_READ | SF_DATA);

    sampleInterrupt();
    dest = read<Size>(ea);
    writeRegA(destReg, ea + ((Size == Byte && destReg == 7) ? 2 : Size) );

    arithmetic<Cmp, Size>(result, dest );
    prefetch();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opCcr(uint16_t opcode) -> void {
    uint8_t data = irc & 0xff;
    readExtensionWord();
    SYNC(8); // confirmed with fx68k
    ccr<Inst>( data );
    fullPrefetch<SampleIPL>(); // first fetch is dummy at same address like extension word
}

template<uint8_t Inst, uint8_t Size> auto M68000::opSr(uint16_t opcode) -> void {
    if (!s)
        return privilegeException();

    uint16_t data = irc;
    readExtensionWord();
    SYNC(8);
    sr<Inst>( data );
    fullPrefetch<SampleIPL>(); // first fetch is dummy at same address like extension word
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opChk(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    uint32_t dataD = readRegD<Word>(reg);

    z = zero<Word>( dataD );
    v = c = n = false;

    SYNC(4);
    if ( (int16_t)dataD > (int16_t)result) {
        SYNC(4);
        n = negative<Word>(dataD); // z, n flag changes happen not here, but later in exception routine
        return trapException( 6 );
    }

    SYNC(2);
    if ((int16_t)dataD < 0) {
        SYNC(4);
        n = true;
        return trapException( 6 );
    }
    
    prefetch<SampleIPL>(); // at the end, Yacht is wrong
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMoveFromSr(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    prefetch<SampleIPL>();
    if constexpr(isRegisterMode<Mode>()) SYNC(2);

    writeEA<Mode, Size>(ea, getSR() );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMoveToSr(uint16_t opcode) -> void {
    if (!s)
        return privilegeException();

    uint32_t result, ea;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    SYNC(4);
    setSR( result );
    fullPrefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68000::opMoveToCcr(uint16_t opcode) -> void {
    uint32_t result, ea;
    if (!readEA<Mode, Size>(opcode & 7, result, ea))
        return;

    SYNC(4);
    setCCR( result & 0xff );
    fullPrefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opExgDxDy(uint16_t opcode) -> void {
    prefetch<SampleIPL>();
    SYNC(2);
    std::swap( regsD[opcode & 7], regsD[(opcode >> 9) & 7 ] );
}

template<uint8_t Inst, uint8_t Size> auto M68000::opExgAxAy(uint16_t opcode) -> void {
    prefetch<SampleIPL>();
    SYNC(2);
    std::swap( regsA[opcode & 7], regsA[(opcode >> 9) & 7 ] );
}

template<uint8_t Inst, uint8_t Size> auto M68000::opExgAxDy(uint16_t opcode) -> void {
    prefetch<SampleIPL>();
    SYNC(2);
    std::swap( regsA[opcode & 7], regsD[(opcode >> 9) & 7 ] );
}

template<uint8_t Inst, uint8_t Size> auto M68000::opExt(uint16_t opcode) -> void {
    uint32_t dn = readRegD<Size>( opcode & 7 );

    if constexpr(Size == Long)  dn = sign<Word>(dn);
    else                        dn = sign<Byte>(dn);

    writeRegD<Size>(opcode & 7, dn);
    defaultFlags<Size>(dn);
    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opMoveUsp(uint16_t opcode) -> void {
    if (!s)
        return privilegeException();

    if (Inst == UspAn)
        writeRegA( opcode & 7, usp );
    else
        usp = readRegA( opcode & 7 );

    prefetch<SampleIPL>();
}

auto M68000::opNop(uint16_t opcode) -> void {
    prefetch<SampleIPL>();
}

auto M68000::opReset(uint16_t opcode) -> void {
    if (!s)
        return privilegeException();

    SYNC(2);
    sampleInterrupt();
    SYNC(2);
    RESET_OUT(); // assert reset line after 4.5 clocks for 124 clocks (128.5)
    SYNC(124);
    prefetch();
}

auto M68000::opRte(uint16_t opcode) -> void {
    if (!s)
        return privilegeException();

    uint32_t& sp = regsA[7];
    if (misaligned<Long>(sp))
        return addressException(sp, pc, SF_READ | SF_DATA); // always leads to halted state after 20 cycles

    uint16_t newSr = read<Word>( sp );
    uint32_t newPc = read<Long>( sp + 2 );

    // SP and status will be updated during second micro cycle of third BUS cycle.
    sp += 6;
    setSR( newSr );

    if (misaligned<Long>(newPc))
        return addressException(newPc, pc, SF_READ | SF_PRG);

    pc = newPc;
    fullPrefetch<SampleIPL>();
}

auto M68000::opRtr(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    if (misaligned<Long>(sp))
        return addressException(sp, pc, SF_READ | SF_DATA); // if in user mode, a misaligned SP doesn't have to lead to halted state

    uint8_t newCcr = read<Word>( sp ) & 0xff;
    uint32_t newPc = read<Long>( sp + 2 );

    sp += 6;
    setCCR( newCcr );

    if (misaligned<Long>(newPc))
        return addressException(newPc, pc, SF_READ | SF_PRG);

    pc = newPc;
    fullPrefetch<SampleIPL>();
}

auto M68000::opRts(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    if (misaligned<Long>(sp))
        return addressException(sp, pc, SF_READ | SF_DATA); // if in user mode, a misaligned SP doesn't have to lead to halted state

    uint32_t newPc = read<Long>( sp );
    sp += 4;

    if (misaligned<Long>(newPc))
        // after stacking current PC, new PC is copied to PC and stacked as fault address.
        // for simplicity, that is not emulated and not needed either, because PC is updated from vector a few cycles later.
        return addressException(newPc, pc, SF_READ | SF_PRG);

    pc = newPc;
    fullPrefetch<SampleIPL>();
}

auto M68000::opStop(uint16_t opcode) -> void {
    if (!s)
        return privilegeException();

    sampleInterrupt(); // if detected, no stop at all. like all the other sampling points (one clock cycle sooner)
    setSR( irc );
    control |= Stop;
    SYNC(4);
    pc += 4; // no extension word, no prefetch -> internal: SYNC(2), pc += 2, SYNC(2) pc += 2
    // e.g. IRQ detected one clock before opcode edge = 4 extra cycles
    // e.g. IRQ detected at opcode edge = 8 extra cycles
}

template<uint8_t Inst, uint8_t Size> auto M68000::opSwap(uint16_t opcode) -> void {
    uint32_t data = readRegD<Long>(opcode & 7);
    data = (data >> 16) | (data & 0xffff) << 16;
    writeRegD<Long>(opcode & 7, data);
    defaultFlags<Long>(data);
    prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opTrap(uint16_t opcode) -> void {
    SYNC(4);
    trapException( (opcode & 0xf) + 32 );
}

auto M68000::opTrapv(uint16_t opcode) -> void {
    if (v) {
        read<Word, FC_PRG>(pc); // doesn't update irc
        trapException( 7 );
    } else
        prefetch<SampleIPL>();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opLink(uint16_t opcode) -> void {
    int16_t displacement = (int16_t)irc;
    readExtensionWord();
    uint32_t _regA = readRegA(opcode & 7);
    uint32_t sp = regsA[7];
    sp -= 4;
    writeRegA(opcode & 7, sp); // happens 1.5 CPU cycles after reading extension word

    if (misaligned<Long>(sp))
        return addressException(sp, pc + 2, SF_DATA, _regA >> 16);

    sampleInterrupt();
    write<Long, Reverse>(sp, _regA);
    regsA[7] = sp + displacement; // happens 1.5 CPU cycles after "long write"
    prefetch();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opUnlink(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    uint32_t newSP = readRegA<Long>(opcode & 7);
    if (misaligned<Long>(newSP))
        return addressException(newSP, pc + 2, SF_READ | SF_DATA);

    uint32_t data = read<Word>( newSP & 0xffffff ) << 16;
    data |= read<Word, SampleIPL>( (newSP + 2) & 0xffffff );

    sp = newSP + 4; // happens after first micro cycle of prefetch. when using register 7, this step happens too (visible in fx68k)
    writeRegA( opcode & 7, data ); // when using register 7, previous step is useless
    prefetch();
}

template<uint8_t Inst, uint8_t Size> auto M68000::opMovep(uint16_t opcode) -> void {
    uint32_t ea = calcEA<AddressRegisterIndirectWithDisplacement, Byte>( opcode & 7 );

    if constexpr(Inst == ToReg) {
        uint32_t result = 0;
        if constexpr(Size == Long) {
            result |= read<Byte>(ea) << 24; ea += 2;
            result |= read<Byte>(ea) << 16; ea += 2;
        }

        result |= read<Byte>(ea) << 8; ea += 2;
        result |= read<Byte, SampleIPL>(ea);

        writeRegD<Size>((opcode >> 9) & 7, result);
        prefetch();

    } else {
        uint32_t result = readRegD<Long>((opcode >> 9) & 7);

        if constexpr(Size == Long) {
            write<Byte>(ea, (result >> 24) & 0xff); ea += 2;
            write<Byte>(ea, (result >> 16) & 0xff); ea += 2;
        }

        write<Byte>(ea, (result >> 8) & 0xff); ea += 2;
        write<Byte>(ea, result & 0xff);

        prefetch<SampleIPL>();
    }
}

}
