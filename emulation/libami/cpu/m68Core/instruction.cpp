
#include "m68Core.h"

namespace M68FAMILY {

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opImmShift(uint16_t opcode) -> void {
    int reg = opcode & 7;
    uint8_t shift = (opcode >> 9) & 7;
    if (shift == 0) shift = 8;

    prefetch<E, SampleIPL>();
    uint32_t result = shifter<Inst, Size>(readRegD<Size>( reg ), shift );

    if constexpr (E == Engine::M68020) {
        switch (Inst) {
            case Asl:
            case Rol:
            case Ror:
                syncSequencer( 8);
                break;

            case Asr:
                syncSequencer( 6);
                break;

            case Lsl:
            case Lsr:
                syncSequencer( 4);
                break;

            case Roxl:
            case Roxr:
                syncSequencer( 12);
                break;
        }
    } else
        SYNC( (Size == Long ? 4 : 2) + shift * 2);

    writeRegD<Size>(reg, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opRegShift(uint16_t opcode) -> void {
    int reg = opcode & 7;
    int shift = readRegD( (opcode >> 9) & 7 ) & 63;

    prefetch<E, SampleIPL>();
    uint32_t result = shifter<Inst, Size>(readRegD<Size>( reg ), shift );

    if constexpr (E == Engine::M68020) {
        switch (Inst) {
            case Asl:
            case Rol:
            case Ror:
                syncSequencer( 8);
                break;

            case Asr:
            case Lsl:
            case Lsr:
                syncSequencer( 6);
                break;

            case Roxl:
            case Roxr:
                syncSequencer( 12);
                break;
        }
    } else
        SYNC( (Size == Long ? 4 : 2) + shift * 2);

    writeRegD<Size>(reg, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opShift(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Word>(opcode & 7, ea);
    
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        switch (Inst) {
            case Asl:
            case Rol:
            case Ror:
                syncSequencer( 4);
                break;

            case Asr:
            case Lsl:
            case Lsr:
            case Roxl:
            case Roxr:
                if (!prefetchBusCycles) SYNC(2);
                break;
        }
    }

    writeEA<E, Mode, Word>(ea, singleShifter<Inst>(result) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opBit(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    int bits = (Size == Long) ? (regsD[ reg ] & 31) : (regsD[ reg ] & 7);

    result = readEA<E, Mode, Size>(opcode & 7, ea);

    result = bit<Inst>(result, bits);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (Size == Byte) {
            if (!prefetchBusCycles) SYNC(2);
            if constexpr(Inst == Btst) SYNC(2);
        } else {
            if (!prefetchBusCycles) SYNC(4);
            else syncSequencer( 5 );
        }
    } else {
        if constexpr(Mode == Immediate) SYNC(2);
        if constexpr(Mode == DataRegisterDirect) cyclesBit<Inst>(bits);
    }

    if constexpr(Inst != Btst) writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opImmBit(uint16_t opcode) -> void {
    uint32_t result, ea;
    int bits = (Size == Long) ? (irc & 31) : (irc & 7);
    readExtensionWord<E>();

    result = readEA<E, Mode, Size>(opcode & 7, ea);
    
    result = bit<Inst>(result, bits);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (Size == Byte) {
            if (!prefetchBusCycles) SYNC(2);
            if constexpr(Inst == Btst) SYNC(2);
        } else {
            if (!prefetchBusCycles) SYNC(4);
            else syncSequencer( 5 );
        }
    } else if constexpr(Mode == DataRegisterDirect) cyclesBit<Inst>(bits);

    if constexpr(Inst != Btst) writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opBitFieldEA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int ofField = (irc >> 6) & 0x1f;
    unsigned wiField = irc & 0x1f;

    unsigned width = (irc & 0x20) ? regsD[wiField & 7] : wiField;
    width = width == 0 ? 32 : (width & 0x1f);

    unsigned _offset = (irc & 0x800) ? regsD[ofField & 7] : ofField;

    int offset = static_cast<int>(_offset);

    int reg = (irc >> 12) & 7;

    readExtensionWord<E>();
    if (!prefetchBusCycles)
        SYNC(2);

    ea = calcEA<E, Mode, Size>(opcode & 7);

    ea += offset >> 3; // negative offset e.g. -18 corresponds to -3 bytes

    offset &= 7; // negative offset e.g. -18 corresponds to bit 6

    unsigned shift = width + static_cast<unsigned>(offset);

    result = read<E, Size, FC_EA_PRG>(ea);

    prefetch<E, SampleIPL>();

    switch (Inst) {
        case Bfchg:
        case Bfclr:
        case Bfset:
            result = bitField<Inst>(result, offset, bfMask<false>( width, offset ));
            syncSequencer( 10 );
            writeEA<E, Inst, Size>( ea, result );

            if (shift > 32) {
                result = read<E, Byte, FC_EA_PRG>(ea + 4);
                SYNC( 2 );
                auto mask = static_cast<uint8_t>(0xff << (40 - shift));
                if constexpr (Inst == Bfchg) writeEA<E, Inst, Byte>(ea + 4, result ^ mask);
                if constexpr (Inst == Bfset) writeEA<E, Inst, Byte>(ea + 4, result | mask);
                if constexpr (Inst == Bfclr) writeEA<E, Inst, Byte>(ea + 4, result & ~mask);
                z &= zero<Byte>(result & mask);
            }

            break;
        default:
        case Bftst:
            (void)bitField<Inst>(result, offset, bfMask<false>( width, offset ));
            syncSequencer( 9 );
            if (shift > 32) {
                SYNC( 1 );
                result = read<E, Byte, FC_EA_PRG>(ea + 4);
                auto mask = static_cast<uint8_t>(0xff << (40 - shift));
                z &= zero<Byte>(result & mask);
            }
            break;
        case Bfextu:
        case Bfexts:
            result = bfShiftLeft<false>( result, offset );
            syncSequencer( 10 );
            if (shift > 32) {
                SYNC( 2 );
                result |= (read<E, Byte, FC_EA_PRG>(ea + 4) << offset) >> 8;
            }
            result = bitFieldExtract<Inst>(result, width);
            writeRegD<Size>(reg, result);
            break;
        case Bfffo:
            result = bfShiftLeft<false>( result, offset );
            if (shift > 32) {
                result |= (read<E, Byte, FC_EA_PRG>(ea + 4) << offset) >> 8;
                syncSequencer( 5 );
            }

            syncSequencer( 21 );
            result = bitFieldFind(result, _offset, width);
            writeRegD<Size>(reg, result);
            break;
        case Bfins:
            result = bitFieldInsert<false>(result, readRegD<Size>( reg ), offset, width);
            syncSequencer( 9 );
            writeEA<E, Inst, Size>( ea, result );

            if (shift > 32) {
                result = read<E, Byte, FC_EA_PRG>(ea + 4);
                uint8_t insert = readRegD<Byte>( reg ) << (40 - shift);
                auto mask = static_cast<uint8_t>(0xff << (40 - shift));
                writeEA<E, Inst, Byte>(ea + 4, (result & ~mask) | insert);
                z &= zero<Byte>(insert);
            }
            break;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opBitField(uint16_t opcode) -> void {
    int ofField = (irc >> 6) & 0x1f;
    unsigned wiField = irc & 0x1f;

    unsigned width = (irc & 0x20) ? regsD[wiField & 7] : wiField;
    width = width == 0 ? 32 : (width & 0x1f);

    unsigned _offset = (irc & 0x800) ? regsD[ofField & 7] : ofField;
    unsigned offset = _offset & 0x1f;

    int reg = (irc >> 12) & 7;
    int regEA = opcode & 7;

    readExtensionWord<E>();

    uint32_t result = readRegD<Size>( regEA );

    prefetch<E, SampleIPL>();

    switch (Inst) {
        case Bfchg:
        case Bfclr:
        case Bfset:
            result = bitField<Inst>(result, offset, bfMask( width, offset ));
            writeRegD<Size>(regEA, result);
            syncSequencer( 12 );
            break;
        default:
        case Bftst:
            (void)bitField<Inst>(result, offset, bfMask( width, offset ));
            syncSequencer(6);
            break;
        case Bfextu:
        case Bfexts:
            result = bitFieldExtract<Inst>( bfShiftLeft( result, offset ), width);
            writeRegD<Size>(regEA, result);
            syncSequencer(8);
            break;
        case Bfffo:
            result = bitFieldFind( bfShiftLeft( result, offset ), _offset, width);
            writeRegD<Size>(regEA, result);
            syncSequencer(18);
            break;
        case Bfins:
            result = bitFieldInsert(result, readRegD<Size>( reg ), offset, width);
            writeRegD<Size>(regEA, result);
            syncSequencer(10);
            break;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opClr(uint16_t opcode) -> void {
    uint32_t ea;
    readEA<E, Mode, Size>(opcode & 7, ea); // dummy read
    
    z = true;
    n = v = c = false;
    prefetch<E, SampleIPL>();

    if constexpr( Mode == DataRegisterDirect && Size == Long) SYNC( 2 );
    writeEA<E, Mode, Size>(ea, 0);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opNbcd(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Byte>(opcode & 7, ea);

    result = bcd<Sbcd>(result, 0);
    prefetch<E, SampleIPL>();

    if constexpr( Mode == DataRegisterDirect ) SYNC( 2 );
    writeEA<E, Mode, Byte>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opNeg(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    result = arithmetic<Inst, Size>(result, 0);
    prefetch<E, SampleIPL>();

    if constexpr( Mode == DataRegisterDirect && Size == Long) SYNC( 2 );
    writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opScc(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Byte>(opcode & 7, ea);

    result = testCondition<Inst>() ? 0xff : 0;

    prefetch<E, SampleIPL>();
    if ( result && Mode == DataRegisterDirect ) SYNC( 2 );
    writeEA<E, Mode, Byte>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opTas(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Byte, TasCycle>(opcode & 7, ea);

    // check if MSB changed, which is found out via status flags.
    // second CPU shouldn't get the BUS while AS line is held high.
    // without AS line keeping high, it could be happen, that when two CPU's accessing TAS the same moment,
    // both CPU's detect a changed MSB and assume they own the BUS.
    // TAS make sure, second CPU can not read (if AS line is respected) before first CPU finished "write" cycle.
    defaultFlags<Byte>(result);
    result |= 0x80;

    if ( Mode != DataRegisterDirect )
        SYNC( 2 );

    writeEA<E, Mode, Byte, TasCycle>(ea, result);

    #ifdef TAS_SPINLOCK
    if constexpr(Mode != DataRegisterDirect)
        TAS_CYCLE_END();
    #endif

    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opTst(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    defaultFlags<Size>(result);
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmetic(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    prefetch<E, SampleIPL>();

    if constexpr (Size == Long) {
        if constexpr(isDirectMode<Mode>())  SYNC(4);
        else                                SYNC(2);
    }

    writeRegD<Size>(reg, arithmetic<Inst, Size>( result, readRegD<Size>(reg) ));
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCmp(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    prefetch<E, SampleIPL>();
    if constexpr(Size == Long) SYNC( 2 );
    arithmetic<Cmp, Size>( result, readRegD<Size>(reg) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCas(uint16_t opcode) -> void {
    uint32_t result, ea;
    int dc = irc & 7;
    int du = (irc >> 6) & 7;

    readExtensionWord<E>();
    if (!prefetchBusCycles)
        SYNC(2);

    result = readEA<E, Mode, Size>(opcode & 7, ea);

    auto compare = readRegD( dc );

    arithmetic<Cmp, Size>( clip<Size>(compare), result );

    SYNC(7);

    if (z) {
        write<E, Size>(ea, readRegD<Size>( du ));
    } else {
        writeRegD<Size>( dc, result );
    }

    prefetch<E, SampleIPL>();

    if (!prefetchBusCycles)
        SYNC(2);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opCas2(uint16_t opcode) -> void {
    int dc1 = irc & 7;
    int du1 = (irc >> 6) & 7;
    int rn1 = (irc >> 12) & 15;
    readExtensionWord<E>();
    if (!prefetchBusCycles)
        SYNC(2);
    int dc2 = irc & 7;
    int du2 = (irc >> 6) & 7;
    int rn2 = (irc >> 12) & 15;
    readExtensionWord<E>();
    if (!prefetchBusCycles)
        SYNC(2);

    auto compare1 = readRegD( dc1 );
    auto compare2 = readRegD( dc2 );

    uint32_t addr1 = readRegR( rn1 );
    uint32_t addr2 = readRegR( rn2 );

    uint32_t data1 = read<E, Size>(addr1);
    uint32_t data2 = read<E, Size>(addr2);

    arithmetic<Cmp, Size>( clip<Size>(compare1), data1 );
    SYNC(13);

    if (z) {
        arithmetic<Cmp, Size>( clip<Size>(compare2), data2 );
        if (z) {
            write<E, Size>(addr1, readRegD<Size>( du1 ));
            write<E, Size>(addr2, readRegD<Size>( du2 ));
            prefetch<E, SampleIPL>();
            if (!prefetchBusCycles)
                SYNC(2);
            return;
        }
    }
    SYNC(3);
    writeRegD<Size>( dc1, data1 );
    writeRegD<Size>( dc2, data2 );

    prefetch<E, SampleIPL>();
    if (!prefetchBusCycles)
        SYNC(2);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmeticEA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    result = arithmetic<Inst, Size>(readRegD<Size>(reg), result);
    prefetch<E, SampleIPL>();

    if constexpr (Size == Long && isRegisterMode<Mode>()) SYNC(4);
    writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmeticA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    result = sign<Size>(result);

    prefetch<E, SampleIPL>();
    if constexpr( Size == Word || isDirectMode<Mode>() ) SYNC(4);
    else SYNC(2);

    writeRegA(reg, (Inst == Adda) ? (result + readRegA<Long>(reg)) : (readRegA<Long>(reg) - result) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCmpa(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    result = sign<Size>(result);
    arithmetic<Cmp, Long>( result, readRegA<Long>(reg) );
    prefetch<E, SampleIPL>();
    SYNC( 2 );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMul(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Word>(opcode & 7, ea);

    prefetch<E, SampleIPL>();
    cyclesMul<Inst>(result);
    if constexpr(Inst == Mulu)  result = result * readRegD<Word>(reg);
    else                        result = (int16_t)result * (int16_t)readRegD<Word>(reg);

    defaultFlags<Long>(result);
    writeRegD<Long>(reg, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opDiv(uint16_t opcode) -> void {
    uint32_t result = 0;
    uint32_t divisor, ea;
    int reg = (opcode >> 9) & 7;
    divisor = readEA<E, Mode, Word>(opcode & 7, ea);

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
        trapException<E>(5);
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

    int postCycles;

    if (overflow) {
        if constexpr(Inst == Divu)  SYNC(6);
        else postCycles = cyclesDiv<Divs>(dividend, divisor);

        v = n = true;
        c = z = false;
    } else {
        postCycles = cyclesDiv<Inst>(dividend, divisor);

        defaultFlags<Word>(result);
        writeRegD<Long>(reg, result);
    }

    prefetch<E, SampleIPL>();

    if constexpr (Inst == Divs) {
        if (postCycles)
            SYNC(postCycles);
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveDataRegisterDirect(uint16_t opcode) -> void {
    opMove<E, DataRegisterDirect, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirect(uint16_t opcode) -> void {
    opMove<E, AddressRegisterIndirect, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithPostIncrement(uint16_t opcode) -> void {
    opMove<E, AddressRegisterIndirectWithPostIncrement, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithPreDecrement(uint16_t opcode) -> void {
    opMove<E, AddressRegisterIndirectWithPreDecrement, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithDisplacement(uint16_t opcode) -> void {
    opMove<E, AddressRegisterIndirectWithDisplacement, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithIndex(uint16_t opcode) -> void {
    opMove<E, AddressRegisterIndirectWithIndex, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAbsoluteShort(uint16_t opcode) -> void {
    opMove<E, AbsoluteShort, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAbsoluteLong(uint16_t opcode) -> void {
    opMove<E, AbsoluteLong, Mode, Size>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> inline auto M68Core::opMove(uint16_t opcode) -> void {
    // Inst is target Mode
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    if constexpr(Inst == DataRegisterDirect)
        writeRegD<Size>(reg, result);
    else {
        uint16_t IR;
        if constexpr(Inst == AbsoluteLong && !isDirectMode<Mode>())
            ea = calcEA<E, Inst, Size, SkipExtension>(reg);
        else if constexpr(Inst == AddressRegisterIndirectWithPreDecrement)
            ea = calcEA<E, Inst, Size, ConcurrentAdrCalc>(reg);
        else
            ea = calcEA<E, Inst, Size>(reg);

        if constexpr (Inst == AddressRegisterIndirectWithPreDecrement) {
            if constexpr (Size != Long)
                writeRegA(reg, ea);

            if constexpr(Size == Long) IR = ird;
            prefetch<E, SampleIPL>();
        }

        if (E < Engine::M68020 && misaligned<Size>(ea)) {
            if constexpr(Size == Long && Inst == AddressRegisterIndirectWithPreDecrement) ird = IR;
            setMoveCCWhenAddressError<Mode, Inst, Size>(result);
            addressExceptionMoveEA<E, Inst, Size>(ea);
            return;
        }

        if constexpr (Inst == AddressRegisterIndirectWithPostIncrement)
            writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));

        if constexpr (Inst == AddressRegisterIndirectWithPreDecrement) {
            if constexpr (Size == Long)
                writeRegA(reg, ea);

            write<E, Size>(ea, result);
        } else if constexpr( (Inst == AddressRegisterIndirectWithPostIncrement)
                || ( Inst == AddressRegisterIndirect && ((Size == Long) || isDirectMode<Mode>()) )
                || ( Inst == AddressRegisterIndirectWithDisplacement && isDirectMode<Mode>() )
                || ( Inst == AddressRegisterIndirectWithIndex && isDirectMode<Mode>() )
                || ( Inst == AbsoluteLong && !isDirectMode<Mode>() )
        ) // check results in "iplLatches"
            write<E, Size, SampleIPL | Reverse>(ea, result);
        else
            write<E, Size, Reverse>(ea, result);

        if constexpr(Inst == AbsoluteLong && !isDirectMode<Mode>())
            readExtensionWord<E>();
    }

    defaultFlags<Size>(result);

    if constexpr(Inst == DataRegisterDirect)
        prefetch<E, SampleIPL>();
    else if constexpr(Inst != AddressRegisterIndirectWithPreDecrement) {
        // analyzed from microcode, a few modes sample IPL at last write and last prefetch (two times)
        if constexpr((Inst == AddressRegisterIndirectWithPostIncrement) ||
                    ( (Size == Long) && isDirectMode<Mode>() &&
                    (Inst == AddressRegisterIndirect || Inst == AddressRegisterIndirectWithDisplacement || Inst == AddressRegisterIndirectWithIndex))) {
            prefetch<E>();
        } else
            prefetch<E, SampleIPL>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    
    writeRegA(reg, Size == Word ? (int16_t)result : result);
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmeticI(uint16_t opcode) -> void {
    uint32_t result, ea;
    uint32_t imm = calcEA<E, Immediate, Size>(0);
    
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    
    prefetch<E, SampleIPL>();
    result = arithmetic<Inst, Size>( imm, result);
    
    if constexpr(Size == Long && isRegisterMode<Mode>()) {
        if constexpr(Inst == Cmpi)  SYNC(2);
        else                        SYNC(4);
    }
    
    if constexpr(Inst != Cmpi)
        writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmeticQ(uint16_t opcode) -> void {
    uint32_t result, ea;
    uint32_t operand = (opcode >> 9) & 7;
    if (operand == 0) operand = 8;

    result = readEA<E, Mode, Mode == AddressRegisterDirect ? Long : Size>(opcode & 7, ea);

    prefetch<E, SampleIPL>();
    if (Mode == AddressRegisterDirect)
        result = Inst == Addq ? (result + operand) : (result - operand);
    else
        result = arithmetic<Inst, Size>( operand, result);

    if (Mode == AddressRegisterDirect || (Size == Long && Mode == DataRegisterDirect)) SYNC( 4 );
    writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMoveQ(uint16_t opcode) -> void {
    uint8_t data = opcode & 0xff;

    prefetch<E, SampleIPL>();
    defaultFlags<Byte>(data);
    writeRegD((opcode >> 9) & 7, (int32_t)(int8_t)data);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opBcc(uint16_t opcode) -> void {
    SYNC(2);

    if (testCondition<Inst>()) {
        uint32_t newPC = pc + (Size == Word ? (int16_t)irc : (int8_t)(opcode & 0xff));

        if (misaligned<Long>(newPC))
            throw AddressError(newPC, pc, SF_READ | SF_PRG);

        pc = newPC;
        fullPrefetch<E, SampleIPL>();
    } else {
        SYNC(2);
        if (Size == Word) readExtensionWord<E>();
        prefetch<E, SampleIPL>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opBsr(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    SYNC(2);
    sp -= 4;

    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc,SF_DATA);

    write<E, Long, Reverse>( sp, Size == Word ? (pc + 2) : pc); // stack PC of next opcode
    appendStepOut( Size == Word ? (pc + 2) : pc );

    pc += (Size == Word) ? (int16_t)irc : (int8_t)(opcode & 0xff);
    if (misaligned<Long>(pc))
        throw AddressError(pc, pc,SF_READ | SF_PRG); // stack new PC two times

    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opDbcc(uint16_t opcode) -> void {
    SYNC(2);
    int reg = opcode & 7;
    uint32_t memPC = pc;
    
    if (!testCondition<Inst>()) {
        pc = memPC + (int16_t)irc;

        if (misaligned<Long>(pc))
            throw AddressError(pc, pc + 2,SF_READ | SF_PRG);

        firstPrefetch<E>(); // assumes branch is taken, hence prefetch from new PC. (in order to speed up most likely case)
        uint16_t word = readRegD<Word>(reg);
        writeRegD<Word>( reg, word - 1);
        
        if (word) { // while prefetching, CPU finds out if branch is really taken
            prefetch<E, SampleIPL>();
            return;
        }
    } else
        SYNC(2);
    
    pc = memPC + 2;
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opJmp(uint16_t opcode) -> void {
    uint32_t adr = calcEA<E, Mode, Long, SkipExtension>(opcode & 7);
    
    if constexpr(isIndexMode<Mode>())                                   SYNC(4);
    if constexpr(Mode == AbsoluteShort || isDisplacementMode<Mode>())   SYNC(2);

    if (misaligned<Long>(adr))
        throw AddressError(adr, (Mode == AbsoluteLong) ? (pc - 2) : pc, SF_READ | SF_PRG);

    if constexpr (E >= Engine::M68020) {
        if (t0) control |= TraceScheduled;
    }

    pc = adr;
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opJsr(uint16_t opcode) -> void {
    uint32_t adr = calcEA<E, Mode, Long, SkipExtension>(opcode & 7);

    if constexpr(isIndexMode<Mode>())                                   SYNC(4);
    if constexpr(Mode == AbsoluteShort || isDisplacementMode<Mode>())   SYNC(2);
    
    if constexpr(isAbsMode<Mode>())
        pc += 2;
    
    if (misaligned<Long>(adr))
        throw AddressError(adr, pc, SF_READ | SF_PRG);

    uint32_t& sp = regsA[7];
    sp -= 4;
    
    if constexpr(isDisplacementMode<Mode>() || isIndexMode<Mode>())
        pc += 2;

    uint32_t pcNextInstruction = pc;
    pc = adr;
    firstPrefetch<E>(); // confirmed with fx68k, first prefetch happens before stacking

    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pcNextInstruction, SF_DATA);
    
    write<E, Long, Reverse>(sp, pcNextInstruction);
    appendStepOut( pcNextInstruction );
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opLea(uint16_t opcode) -> void {
    writeRegA( (opcode >> 9) & 7, calcEA<E, Mode, Long>(opcode & 7) );
    if constexpr(isIndexMode<Mode>()) SYNC(2);
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opPea(uint16_t opcode) -> void {
    uint32_t ea = calcEA<E, Mode, Long>(opcode & 7);
    uint16_t IR = ird;

    if constexpr(!isAbsMode<Mode>()) {
        sampleInterrupt(); // like all the other IPL sample points, it happens one clock cycle sooner (confirmed with fx68k)
        if constexpr(isIndexMode<Mode>()) SYNC(2);
        prefetch<E>();
    }

    uint32_t& sp = regsA[7];
    sp -= 4;

    if (E < Engine::M68020 && misaligned<Long>(sp)) {
        ird = IR;
        throw AddressError(sp, pc, SF_DATA);
    }

    write<E, Long, Reverse>(sp, ea);

    if constexpr(isAbsMode<Mode>())
        prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMovemToReg(uint16_t opcode) -> void {
    uint16_t mask = irc;
    readExtensionWord<E>();
    uint32_t ea = calcEA<E, Mode, Size>(opcode & 7);

    if (E < Engine::M68020 && misaligned<Long>(ea))
        throw AddressError(ea, isIndexMode<Mode>() ? (pc - 2) : (pc + 2), SF_READ | (isPcMode<Mode>() ? SF_PRG : SF_DATA ));

    if constexpr(Mode == AddressRegisterIndirectWithPostIncrement)
        sampleInterrupt();

    if (mask & (1 << 0)) { writeRegD(0, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 1)) { writeRegD(1, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 2)) { writeRegD(2, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 3)) { writeRegD(3, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 4)) { writeRegD(4, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 5)) { writeRegD(5, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 6)) { writeRegD(6, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 7)) { writeRegD(7, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }

    if (mask & (1 <<  8)) { writeRegA(0, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 <<  9)) { writeRegA(1, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 10)) { writeRegA(2, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 11)) { writeRegA(3, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 12)) { writeRegA(4, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 13)) { writeRegA(5, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 14)) { writeRegA(6, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }
    if (mask & (1 << 15)) { writeRegA(7, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) ); ea += Size; }

    if constexpr(Mode == AddressRegisterIndirectWithPostIncrement)
        writeRegA( opcode & 7, ea );

    read<E, Word, FC_EA_PRG>(ea); // dummy read (brackets for Long are wrong in Yacht )

    if constexpr(Mode != AddressRegisterIndirectWithPostIncrement)
        prefetch<E, SampleIPL>();
    else
        prefetch<E>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMovemToEa(uint16_t opcode) -> void {
    uint16_t mask = irc;
    readExtensionWord<E>();

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement) {
        uint32_t ea = readRegA(opcode & 7);
        if (E < Engine::M68020 && mask && misaligned<Long>(ea))
            throw AddressError(ea - 2, pc + 2, SF_DATA);

        if (mask & (1 << 0)) { ea -= Size; write<E, Size>(ea, regsA[7] ); }
        if (mask & (1 << 1)) { ea -= Size; write<E, Size>(ea, regsA[6] ); }
        if (mask & (1 << 2)) { ea -= Size; write<E, Size>(ea, regsA[5] ); }
        if (mask & (1 << 3)) { ea -= Size; write<E, Size>(ea, regsA[4] ); }
        if (mask & (1 << 4)) { ea -= Size; write<E, Size>(ea, regsA[3] ); }
        if (mask & (1 << 5)) { ea -= Size; write<E, Size>(ea, regsA[2] ); }
        if (mask & (1 << 6)) { ea -= Size; write<E, Size>(ea, regsA[1] ); }
        if (mask & (1 << 7)) { ea -= Size; write<E, Size>(ea, regsA[0] ); }

        if (mask & (1 << 8)) { ea -= Size; write<E, Size>(ea, regsD[7] ); }
        if (mask & (1 << 9)) { ea -= Size; write<E, Size>(ea, regsD[6] ); }
        if (mask & (1 << 10)) { ea -= Size; write<E, Size>(ea, regsD[5] ); }
        if (mask & (1 << 11)) { ea -= Size; write<E, Size>(ea, regsD[4] ); }
        if (mask & (1 << 12)) { ea -= Size; write<E, Size>(ea, regsD[3] ); }
        if (mask & (1 << 13)) { ea -= Size; write<E, Size>(ea, regsD[2] ); }
        if (mask & (1 << 14)) { ea -= Size; write<E, Size>(ea, regsD[1] ); }
        if (mask & (1 << 15)) { ea -= Size; write<E, Size>(ea, regsD[0] ); }

        writeRegA( opcode & 7, ea );
    } else {
        uint32_t ea = calcEA<E, Mode, Size>(opcode & 7);
        if (E < Engine::M68020 && mask && misaligned<Long>(ea))
            throw AddressError(ea, pc + 2, SF_DATA);

        if (mask & (1 << 0)) { write<E, Size, Reverse>(ea, regsD[0] ); ea += Size; }
        if (mask & (1 << 1)) { write<E, Size, Reverse>(ea, regsD[1] ); ea += Size; }
        if (mask & (1 << 2)) { write<E, Size, Reverse>(ea, regsD[2] ); ea += Size; }
        if (mask & (1 << 3)) { write<E, Size, Reverse>(ea, regsD[3] ); ea += Size; }
        if (mask & (1 << 4)) { write<E, Size, Reverse>(ea, regsD[4] ); ea += Size; }
        if (mask & (1 << 5)) { write<E, Size, Reverse>(ea, regsD[5] ); ea += Size; }
        if (mask & (1 << 6)) { write<E, Size, Reverse>(ea, regsD[6] ); ea += Size; }
        if (mask & (1 << 7)) { write<E, Size, Reverse>(ea, regsD[7] ); ea += Size; }

        if (mask & (1 << 8)) { write<E, Size, Reverse>(ea, regsA[0] ); ea += Size; }
        if (mask & (1 << 9)) { write<E, Size, Reverse>(ea, regsA[1] ); ea += Size; }
        if (mask & (1 << 10)) { write<E, Size, Reverse>(ea, regsA[2] ); ea += Size; }
        if (mask & (1 << 11)) { write<E, Size, Reverse>(ea, regsA[3] ); ea += Size; }
        if (mask & (1 << 12)) { write<E, Size, Reverse>(ea, regsA[4] ); ea += Size; }
        if (mask & (1 << 13)) { write<E, Size, Reverse>(ea, regsA[5] ); ea += Size; }
        if (mask & (1 << 14)) { write<E, Size, Reverse>(ea, regsA[6] ); ea += Size; }
        if (mask & (1 << 15)) { write<E, Size, Reverse>(ea, regsA[7] ); ea += Size; }
    }

    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opArithmeticX(uint16_t opcode) -> void {
    int destReg = (opcode >> 9) & 7;

    prefetch<E, SampleIPL>();
    if constexpr(Size == Long)  SYNC(4);

    writeRegD<Size>( destReg, arithmetic<Inst, Size>(readRegD<Size>( opcode & 7 ), readRegD<Size>( destReg ) ) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opArithmeticBCD(uint16_t opcode) -> void {
    int destReg = (opcode >> 9) & 7;

    prefetch<E, SampleIPL>();
    SYNC(2);

    writeRegD<Size>( destReg, bcd<Inst>(readRegD<Size>( opcode & 7 ), readRegD<Size>( destReg ) ) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opArithmeticXEa(uint16_t opcode) -> void {
    uint32_t result, dest, ea;
    int srcReg = opcode & 7;
    int destReg = (opcode >> 9) & 7;

    ea = readRegA( srcReg );
    ea -= (Size == Byte && srcReg == 7) ? 2 : Size;
    SYNC(2);

    if (E < Engine::M68020 && misaligned<Size>(ea)) {
        if constexpr(Size == Word) writeRegA(srcReg, ea);
        throw AddressError(Size == Long ? (ea + 2) : ea, pc + 2, SF_READ | SF_DATA);
    }

    writeRegA(srcReg, ea);
    result = read<E, Size, Reverse>(ea);

    ea = readRegA( destReg );
    ea -= (Size == Byte && destReg == 7) ? 2 : Size;

    if (E < Engine::M68020 && misaligned<Size>(ea)) {
        if constexpr(Size == Word) writeRegA(destReg, ea);
        throw AddressError(Size == Long ? (ea + 2) : ea, pc + 2, SF_READ | SF_DATA);
    }

    writeRegA(destReg, ea);
    if constexpr(Size != Long)  dest = read<E, Size, SampleIPL>(ea);
    else                        dest = read<E, Size, Reverse>(ea);

    if constexpr(Inst == Abcd || Inst == Sbcd)
        result = bcd<Inst>(result, dest );
    else
        result = arithmetic<Inst, Size>(result, dest );

    if constexpr(Size == Long) write<E, Word, SampleIPL>( ea + 2, result & 0xffff );

    prefetch<E>();

    if constexpr(Size == Long)  write<E, Word>( ea, (result >> 16) & 0xffff );
    else                        write<E, Size>( ea, result );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opCmpm(uint16_t opcode) -> void {
    uint32_t result, dest, ea;
    int srcReg = opcode & 7;
    int destReg = (opcode >> 9) & 7;

    ea = readRegA( srcReg );
    if (E < Engine::M68020 && misaligned<Size>(ea))
        throw AddressError(ea, pc + 2, SF_READ | SF_DATA);

    result = read<E, Size>(ea);
    writeRegA(srcReg, ea + ((Size == Byte && srcReg == 7) ? 2 : Size) );

    ea = readRegA( destReg );
    if (E < Engine::M68020 && misaligned<Size>(ea))
        throw AddressError(ea, pc + 2, SF_READ | SF_DATA);

    sampleInterrupt();
    dest = read<E, Size>(ea);
    writeRegA(destReg, ea + ((Size == Byte && destReg == 7) ? 2 : Size) );

    arithmetic<Cmp, Size>(result, dest );
    prefetch<E>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opCcr(uint16_t opcode) -> void {
    uint8_t data = irc & 0xff;
    readExtensionWord<E>();
    SYNC(8); // confirmed with fx68k
    ccr<Inst>( data );
    fullPrefetch<E, SampleIPL>(); // first fetch is dummy at same address like extension word
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opSr(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint16_t data = irc;
    readExtensionWord<E>();
    SYNC(8);
    sr<Inst>( data );
    fullPrefetch<E, SampleIPL>(); // first fetch is dummy at same address like extension word
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opChk(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    uint32_t dataD = readRegD<Word>(reg);

    z = zero<Word>( dataD );
    v = c = n = false;

    SYNC(4);
    if ( (int16_t)dataD > (int16_t)result) {
        SYNC(4);
        n = negative<Word>(dataD); // z, n flag changes happen not here, but later in exception routine
        return trapException<E>( 6 );
    }

    SYNC(2);
    if ((int16_t)dataD < 0) {
        SYNC(4);
        n = true;
        return trapException<E>( 6 );
    }
    
    prefetch<E, SampleIPL>(); // at the end, Yacht is wrong
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opChk2(uint16_t opcode) -> void {
    int32_t value;
    uint32_t result, result2, ea;
    bool useExc = irc & 0x800;
    int reg = (irc >> 12) & 0xf;
    if (reg < 8)
        value = sign<Size>(readRegD<Size>( reg ));
    else
        value = sign<Long>(readRegA( reg & 7 ));

    readExtensionWord<E>();
    if (!prefetchBusCycles)
        SYNC(2);

    result = readEA<E, Mode, Size>(opcode & 7, ea);

    result2 = read<E, Size, FC_EA_PRG>(ea + Size);

    int32_t boundL = sign<Size>(result);
    int32_t boundU = sign<Size>(result2);

    if (boundL <= boundU)
        c = value < boundL || value > boundU;
    else
        c = value < boundL && value > boundU;

    z = value == boundL || value == boundU;

    setChk2UndefinedFlags(boundL, boundU, value);

    SYNC(9);

    if (useExc && c) {
        SYNC(6);
        trapException<E>(6);
    } else {
        prefetch<E, SampleIPL>();
        if (!prefetchBusCycles)
            SYNC(3);
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveFromSr(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    prefetch<E, SampleIPL>();
    if constexpr(isRegisterMode<Mode>()) SYNC(2);

    writeEA<E, Mode, Size>(ea, getSR() );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveToSr(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    SYNC(4);
    setSR( result );
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveToCcr(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    SYNC(4);
    setCCR( result & 0xff );
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExgDxDy(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
    SYNC(2);
    std::swap( regsD[opcode & 7], regsD[(opcode >> 9) & 7 ] );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExgAxAy(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
    SYNC(2);
    std::swap( regsA[opcode & 7], regsA[(opcode >> 9) & 7 ] );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExgAxDy(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
    SYNC(2);
    std::swap( regsA[opcode & 7], regsD[(opcode >> 9) & 7 ] );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExt(uint16_t opcode) -> void {
    uint32_t dn = readRegD<Size>( opcode & 7 );

    if constexpr(Size == Long)  dn = sign<Word>(dn);
    else                        dn = sign<Byte>(dn);

    writeRegD<Size>(opcode & 7, dn);
    defaultFlags<Size>(dn);
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMoveUspAn(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    writeRegA( opcode & 7, usp );
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMoveAnUsp(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    usp = readRegA( opcode & 7 );
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opNop(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opReset(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    SYNC(2);
    sampleInterrupt();
    SYNC(2);
    RESET_OUT(); // assert reset line after 4.5 clocks for 124 clocks (128.5)
    SYNC(124);
    prefetch<E>();
}

template<M68Core::Engine E> auto M68Core::opRte(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint32_t& sp = regsA[7];
    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc, SF_READ | SF_DATA); // always leads to halted state after 20 cycles

    uint16_t newSr = read<E, Word>( sp );
    uint32_t newPc = read<E, Long>( sp + 2 );
    // 68000: SP and status will be updated during second micro cycle of third BUS cycle.
    sp += 6;

    if (E >= Engine::M68020) {
        uint16_t format = read<E, Word>(sp) >> 12; sp += 2;
        switch (format) {
            case 0:
                break;
            case 1:
                setSR( newSr );
                newSr = read<E, Word>( sp ); sp += 2;
                newPc = read<E, Long>( sp ); sp += 4;
                (void)read<E, Word>(sp); sp += 2;
                break;
            case 2:
                (void)read<E, Long>(sp); sp += 4;
                break;
            case 0xa:
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Long>(sp); sp += 4;
                (void)read<E, Long>(sp); sp += 4;
                (void)read<E, Long>(sp); sp += 4;
                (void)read<E, Long>(sp); sp += 4;
                break;
            default:
                trapException<E>(14);
                return;
        }
    }

    setSR( newSr );

    if (misaligned<Long>(newPc))
        throw AddressError(newPc, pc, SF_READ | SF_PRG);
    if(!stepOuts.empty()) stepOuts.pop_back();
    pc = newPc;
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opRtr(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc, SF_READ | SF_DATA); // if in user mode, a misaligned SP doesn't have to lead to halted state

    uint8_t newCcr = read<E, Word>( sp ) & 0xff;
    uint32_t newPc = read<E, Long>( sp + 2 );

    sp += 6;
    setCCR( newCcr );

    if (misaligned<Long>(newPc))
        throw AddressError(newPc, pc, SF_READ | SF_PRG);

    if(!stepOuts.empty()) stepOuts.pop_back();
    pc = newPc;
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opRts(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc, SF_READ | SF_DATA); // if in user mode, a misaligned SP doesn't have to lead to halted state

    uint32_t newPc = read<E, Long>( sp );
    sp += 4;

    if (misaligned<Long>(newPc))
        // after stacking current PC, new PC is copied to PC and stacked as fault address.
        // for simplicity, that is not emulated and not needed either, because PC is updated from vector a few cycles later.
        throw AddressError(newPc, pc, SF_READ | SF_PRG);

    if(!stepOuts.empty()) stepOuts.pop_back();
    pc = newPc;
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opStop(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    sampleInterrupt(); // if detected, no stop at all. like all the other sampling points (one clock cycle sooner)
    setSR( irc );
    control |= Stop;
    SYNC(4);
    pc += 4; // no extension word, no prefetch -> internal: SYNC(2), pc += 2, SYNC(2) pc += 2
    // e.g. IRQ detected one clock before opcode edge = 4 extra cycles
    // e.g. IRQ detected at opcode edge = 8 extra cycles
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opSwap(uint16_t opcode) -> void {
    uint32_t data = readRegD<Long>(opcode & 7);
    data = (data >> 16) | (data & 0xffff) << 16;
    writeRegD<Long>(opcode & 7, data);
    defaultFlags<Long>(data);
    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opTrap(uint16_t opcode) -> void {
    SYNC(4);
    trapException<E>( (opcode & 0xf) + 32 );
}

template<M68Core::Engine E> auto M68Core::opTrapv(uint16_t opcode) -> void {
    if (v) {
        read<E, Word, FC_PRG>(pc); // doesn't update irc
        trapException<E>( 7 );
    } else
        prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opLink(uint16_t opcode) -> void {
    int16_t displacement = (int16_t)irc;
    readExtensionWord<E>();
    uint32_t _regA = readRegA(opcode & 7);
    uint32_t sp = regsA[7];
    sp -= 4;
    writeRegA(opcode & 7, sp); // happens 1.5 CPU cycles after reading extension word

    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc + 2, SF_DATA);

    sampleInterrupt();
    write<E, Long, Reverse>(sp, _regA);
    regsA[7] = sp + displacement; // happens 1.5 CPU cycles after "long write"
    prefetch<E>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opUnlink(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    uint32_t newSP = readRegA<Long>(opcode & 7);
    if (E < Engine::M68020 && misaligned<Long>(newSP))
        throw AddressError(newSP, pc + 2, SF_READ | SF_DATA);

    uint32_t data = read<E, Word>( newSP & 0xffffff ) << 16;
    data |= read<E, Word, SampleIPL>( (newSP + 2) & 0xffffff );

    sp = newSP + 4; // happens after first micro cycle of prefetch. when using register 7, this step happens too (visible in fx68k)
    writeRegA( opcode & 7, data ); // when using register 7, previous step is useless
    prefetch<E>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMovepReg(uint16_t opcode) -> void {
    uint32_t ea = calcEA<E, AddressRegisterIndirectWithDisplacement, Byte>( opcode & 7 );

    uint32_t result = 0;
    if constexpr(Size == Long) {
        result |= read<E, Byte>(ea) << 24; ea += 2;
        result |= read<E, Byte>(ea) << 16; ea += 2;
    }

    result |= read<E, Byte>(ea) << 8; ea += 2;
    result |= read<E, Byte, SampleIPL>(ea);

    writeRegD<Size>((opcode >> 9) & 7, result);
    prefetch<E>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMovepEa(uint16_t opcode) -> void {
    uint32_t ea = calcEA<E, AddressRegisterIndirectWithDisplacement, Byte>( opcode & 7 );

    uint32_t result = readRegD<Long>((opcode >> 9) & 7);

    if constexpr(Size == Long) {
        write<E, Byte>(ea, (result >> 24) & 0xff); ea += 2;
        write<E, Byte>(ea, (result >> 16) & 0xff); ea += 2;
    }

    write<E, Byte>(ea, (result >> 8) & 0xff); ea += 2;
    write<E, Byte>(ea, result & 0xff);

    prefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opBkpt(uint16_t opcode) -> void {
    // when external hardware recognizes Bkpt CPU Space cycle, its able to inject instruction word via data bus
    // A1200 doesn't respond to Bkpt CPU Space Cycle -> results in illegal exception
    // int nr = opcode & 7;
    (void)read<E, Size>(pc);
    illegal<E>(opcode);
}

}
