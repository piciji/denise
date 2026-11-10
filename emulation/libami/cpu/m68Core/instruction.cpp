
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
            case Asl: case Rol: case Ror:   busCycles -= 8; break;
            case Asr:                       busCycles -= 6; break;
            case Lsl: case Lsr:             busCycles -= 4; break;
            case Roxl: case Roxr:           busCycles -= 12; break;
        }
        syncInternal<CheckIfPipeRunsDry>();
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
            case Asl: case Rol: case Ror:   busCycles -= 8; break;
            case Asr: case Lsl: case Lsr:   busCycles -= 6; break;
            case Roxl: case Roxr:           busCycles -= 12; break;
        }
        syncInternal<CheckIfPipeRunsDry>();
    } else
        SYNC( (Size == Long ? 4 : 2) + shift * 2);

    writeRegD<Size>(reg, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opShift(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Word>(opcode & 7, ea);
    if constexpr (E == Engine::M68020)
        idleSequencer();

    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        switch (Inst) {
            case Rol: case Ror:                                 busCycles -= 4; break;
            case Asl:                                           busCycles -= prefetchLast() ? 4 : 3; break;
            case Asr: case Lsl: case Lsr: case Roxl: case Roxr: busCycles -= 2; break;
        }
        syncInternal();
    }

    writeEA<E, Mode, Word>(ea, singleShifter<Inst>(result) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opBit(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    int bits = (Size == Long) ? (regsD[ reg ] & 31) : (regsD[ reg ] & 7);

    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    result = bit<Inst>(result, bits);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isDirectMode<Mode>() || (Inst == Btst)) {
            busCycles -= prefetchLast() ? 5 : 4;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 1;
            syncInternal();
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

    result = readEA<E, Mode, Size, ImmEA>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isRegisterMode<Mode>())
        idleSequencer();
    
    result = bit<Inst>(result, bits);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>() || (Inst == Btst)) {
            busCycles -= prefetchLast() ? 5 : 4;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 1;
            syncInternal();
        }
    } else if constexpr(Mode == DataRegisterDirect) cyclesBit<Inst>(bits);

    if constexpr(Inst != Btst) writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opBitFieldEA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int ofField = (irc >> 6) & 0x1f;
    unsigned wiField = irc & 0x1f;

    unsigned width = (irc & 0x20) ? (regsD[wiField & 7] & 0x1f) : wiField;
    if (width == 0)
        width = 32;

    unsigned _offset = (irc & 0x800) ? regsD[ofField & 7] : ofField;

    int offset = static_cast<int>(_offset);

    int reg = (irc >> 12) & 7;

    readExtensionWord<E>();

    ea = calcEA<E, Mode, Size, ImmEACalc>(opcode & 7);

    ea += offset >> 3; // negative offset e.g. -18 corresponds to -3 bytes

    offset &= 7; // negative offset e.g. -18 corresponds to bit 6

    unsigned shift = width + static_cast<unsigned>(offset);

    result = read<E, Size, FC_EA_PRG>(ea);

    idleSequencer();

    prefetch<E, SampleIPL>();

    switch (Inst) {
        case Bfchg:
        case Bfclr:
        case Bfset:
            result = bitField<Inst>(result, offset, bfMask<false>( width, offset ));
            busCycles -= 10;
            syncInternal();
            write<E, Size>( ea, result );

            if (shift > 32) {
                result = read<E, Byte, FC_EA_PRG>(ea + 4);
                idleSequencer();
                busCycles -= 2;
                syncInternal();
                auto mask = static_cast<uint8_t>(0xff << (40 - shift));
                if constexpr (Inst == Bfchg) write<E, Byte>(ea + 4, result ^ mask);
                if constexpr (Inst == Bfset) write<E, Byte>(ea + 4, result | mask);
                if constexpr (Inst == Bfclr) write<E, Byte>(ea + 4, result & ~mask);
                z &= zero<Byte>(result & mask);
            }

            break;
        default:
        case Bftst:
            (void)bitField<Inst>(result, offset, bfMask<false>( width, offset ));
            busCycles -= prefetchLast() ? 9 : 8;
            syncInternal();

            if (shift > 32) {
                result = read<E, Byte, FC_EA_PRG>(ea + 4);
                idleSequencer();
                busCycles -= 1;
                syncInternal();
                auto mask = static_cast<uint8_t>(0xff << (40 - shift));
                z &= zero<Byte>(result & mask);
            }
            break;
        case Bfextu:
        case Bfexts:
            result = bfShiftLeft<false>( result, offset );
            busCycles -= 10;
            syncInternal();

            if (shift > 32) {
                result |= (read<E, Byte, FC_EA_PRG>(ea + 4) << offset) >> 8;
                idleSequencer();
                busCycles -= 2;
                syncInternal();
            }
            result = bitFieldExtract<Inst>(result, width);
            writeRegD<Size>(reg, result);
            break;
        case Bfffo:
            result = bfShiftLeft<false>( result, offset );
            busCycles -= 21;
            syncInternal();

            if (shift > 32) {
                result |= (read<E, Byte, FC_EA_PRG>(ea + 4) << offset) >> 8;
                idleSequencer();
                busCycles -= 5;
                syncInternal();
            }
            result = bitFieldFind(result, _offset, width);
            writeRegD<Size>(reg, result);
            break;
        case Bfins:
            result = bitFieldInsert<false>(result, readRegD<Size>( reg ), offset, width);
            busCycles -= 9;
            syncInternal();
            write<E, Size>( ea, result );

            if (shift > 32) {
                result = read<E, Byte, FC_EA_PRG>(ea + 4);
                uint8_t insert = readRegD<Byte>( reg ) << (40 - shift);
                auto mask = static_cast<uint8_t>(0xff << (40 - shift));
                write<E, Byte>(ea + 4, (result & ~mask) | insert);
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
            busCycles -= 12;
            break;
        default:
        case Bftst:
            (void)bitField<Inst>(result, offset, bfMask( width, offset ));
            busCycles -= prefetchLast() ? 7 : 6;
            break;
        case Bfextu:
        case Bfexts:
            result = bitFieldExtract<Inst>( bfShiftLeft( result, offset ), width);
            writeRegD<Size>(reg, result);
            busCycles -= 8;
            break;
        case Bfffo:
            result = bitFieldFind( bfShiftLeft( result, offset ), _offset, width);
            writeRegD<Size>(reg, result);
            busCycles -= 18;
            break;
        case Bfins:
            result = bitFieldInsert(result, readRegD<Size>( reg ), offset, width);
            writeRegD<Size>(regEA, result);
            busCycles -= 10;
            break;
    }
    syncInternal<CheckIfPipeRunsDry>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opClr(uint16_t opcode) -> void {
    uint32_t ea;
    if constexpr (E == Engine::M68020)
        ea = calcEaUpdatePrePost<E, Mode, Size, EACalc>( opcode & 7 );
    else
        readEA<E, Mode, Size>(opcode & 7, ea); // dummy read
    
    z = true;
    n = v = c = false;
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 1;
            syncInternal();
        }
    } else if constexpr( Mode == DataRegisterDirect && Size == Long)
        SYNC( 2 );

    writeEA<E, Mode, Size>(ea, 0);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opNbcd(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Byte>(opcode & 7, ea);

    if constexpr (E == Engine::M68020 && !isRegisterMode<Mode>())
        idleSequencer();

    result = bcd<E, Sbcd>(result, 0);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 6;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 6;
            syncInternal();
        }
    } else if constexpr( Mode == DataRegisterDirect )
        SYNC( 2 );

    writeEA<E, Mode, Byte>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opNeg(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    if constexpr (E == Engine::M68020 && !isRegisterMode<Mode>())
        idleSequencer();

    result = arithmetic<Inst, Size>(result, 0);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 1;
            syncInternal();
        }
    } else if constexpr( Mode == DataRegisterDirect && Size == Long)
        SYNC( 2 );

    writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opScc(uint16_t opcode) -> void {
    uint32_t result, ea;
    if constexpr (E == Engine::M68020) {
        ea = calcEaUpdatePrePost<E, Mode, Size, EACalc>( opcode & 7 );
        if constexpr (!isRegisterMode<Mode>())
            idleSequencer();
    } else
        readEA<E, Mode, Byte>(opcode & 7, ea);

    result = testCondition<Inst>() ? 0xff : 0;

    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 4;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 3;
            syncInternal();
        }
    } else if ( result && Mode == DataRegisterDirect )
        SYNC( 2 );

    writeEA<E, Mode, Byte>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opTas(uint16_t opcode) -> void {
    uint32_t result, ea;
    if constexpr (E == Engine::M68020 && !isRegisterMode<Mode>()) {
        ea = calcEaUpdatePrePost<E, Mode, Size, EACalc>( opcode & 7 );
        result = read<E, Size, FC_EA_PRG>(ea);
        idleSequencer();
        SYNC(3);
    } else
        result = readEA<E, Mode, Byte>(opcode & 7, ea);

    // address strobe (AS) line remains asserted after read cycle of TAS, so read and following write are indivisible.
    // checks if MSB changed, which is found out via status flags.
    // if AS line is respected, second CPU will not access between read and write cycles
    // note: don't rely on TAS for Amiga, because DMA accesses don't respect AS line of CPU
    defaultFlags<Byte>(result);
    result |= 0x80;

    if constexpr (E < Engine::M68020) {
        if ( Mode != DataRegisterDirect )
            SYNC( 2 );
    }

    writeEA<E, Mode, Byte>(ea, result);

    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 4;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= prefetchLast() ? 4 : 3;
            syncInternal();
        }
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opTst(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    defaultFlags<Size>(result);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isDirectMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 2;
            syncInternal();
        }
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmetic(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isDirectMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 2;
            syncInternal();
        }
    } else if constexpr (Size == Long) {
        if constexpr(isDirectMode<Mode>())  SYNC(4);
        else                                SYNC(2);
    }

    writeRegD<Size>(reg, arithmetic<Inst, Size>( result, readRegD<Size>(reg) ));
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCmp(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        if constexpr (isDirectMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 2;
            syncInternal();
        }
    } else if constexpr(Size == Long)
        SYNC( 2 );

    arithmetic<Cmp, Size>( result, readRegD<Size>(reg) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCas(uint16_t opcode) -> void {
    uint32_t result, ea;
    int dc = irc & 7;
    int du = (irc >> 6) & 7;

    readExtensionWord<E>();

    result = readEA<E, Mode, Size, ImmEACalc>(opcode & 7, ea);
    idleSequencer();

    auto compare = readRegD( dc );

    arithmetic<Cmp, Size>( clip<Size>(compare), result );

    prefetch<E, SampleIPL>();

    busCycles -= prefetchLast() ? 10 : 9;
    syncInternal();

    if (z)
        write<E, Size>(ea, readRegD<Size>( du ));
    else
        writeRegD<Size>( dc, result );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opCas2(uint16_t opcode) -> void {
    int dc1 = irc & 7;
    int du1 = (irc >> 6) & 7;
    int rn1 = (irc >> 12) & 15;
    readExtensionWord<E>();
    busCycles -= 2;
    int dc2 = irc & 7;
    int du2 = (irc >> 6) & 7;
    int rn2 = (irc >> 12) & 15;
    readExtensionWord<E>();
    busCycles -= 2;

    auto compare1 = readRegD( dc1 );
    auto compare2 = readRegD( dc2 );

    uint32_t addr1 = readRegR( rn1 );
    uint32_t addr2 = readRegR( rn2 );

    syncInternal<ForceIdle>();
    uint32_t data1 = read<E, Size>(addr1);
    uint32_t data2 = read<E, Size>(addr2);
    idleSequencer();

    arithmetic<Cmp, Size>( clip<Size>(compare1), data1 );
    prefetch<E, SampleIPL>();

    if (z) {
        arithmetic<Cmp, Size>( clip<Size>(compare2), data2 );
        if (z) {
            busCycles -= prefetchLast() ? 12 : 9;
            write<E, Size>(addr1, readRegD<Size>( du1 ));
            write<E, Size>(addr2, readRegD<Size>( du2 ));
            return;
        }
    }
    busCycles -= prefetchLast() ? 15 : 12;
    syncInternal();
    writeRegD<Size>( dc1, data1 );
    writeRegD<Size>( dc2, data2 );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmeticEA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isRegisterMode<Mode>())
        idleSequencer();

    result = arithmetic<Inst, Size>(readRegD<Size>(reg), result);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 1;
            syncInternal();
        }
    } else if constexpr (Size == Long && isRegisterMode<Mode>())
        SYNC(4);

    writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmeticA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    result = sign<Size>(result);

    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isDirectMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 2;
            syncInternal();
        }
    } else {
        if constexpr( Size == Word || isDirectMode<Mode>() ) SYNC(4);
        else SYNC(2);
    }

    writeRegA(reg, (Inst == Adda) ? (result + readRegA<Long>(reg)) : (readRegA<Long>(reg) - result) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCmpa(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    result = sign<Size>(result);
    arithmetic<Cmp, Long>( result, readRegA<Long>(reg) );
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isDirectMode<Mode>()) {
            busCycles -= 4;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 4;
            syncInternal();
        }
    } else
        SYNC( 2 );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMul(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Word>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        if constexpr (isDirectMode<Mode>()) {
            busCycles -= prefetchLast() ? 28 : 27;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= prefetchLast() ? 28 : 27;
            syncInternal();
        }
    } else
        cyclesMul<Inst>(result);

    if constexpr(Inst == Mulu)  result = result * readRegD<Word>(reg);
    else                        result = (int16_t)result * (int16_t)readRegD<Word>(reg);

    defaultFlags<Long>(result);
    writeRegD<Long>(reg, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMull(uint16_t opcode) -> void {
    uint32_t src, ea;
    uint64_t result;
    int dh = irc & 7;
    int dl = (irc >> 12) & 7;
    int mode = (irc >> 10) & 3;
    readExtensionWord<E>();

    src = readEA<E, Mode, Size, ImmEA>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    prefetch<E, SampleIPL>();

    switch (mode) {
        case 0:
            result = mullu<false>( src, readRegD( dl ) );
            writeRegD( dl, static_cast<uint32_t>(result) );
            break;
        case 1:
            result = mullu<true>( src, readRegD( dl ) );
            writeRegD( dl, static_cast<uint32_t>(result) );
            writeRegD( dh, static_cast<uint32_t>(result >> 32) );
            break;
        case 2:
            result = mulls<false>( src, readRegD( dl ) );
            writeRegD( dl, static_cast<uint32_t>(result) );
            break;
        case 3:
            result = mulls<true>( src, readRegD( dl ) );
            writeRegD( dl, static_cast<uint32_t>(result) );
            writeRegD( dh, static_cast<uint32_t>(result >> 32) );
            break;
    }

    busCycles -= prefetchLast() ? 44 : 43;
    syncInternal<isDirectMode<Mode>() ? CheckIfPipeRunsDry : 0>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opDiv(uint16_t opcode) -> void {
    uint32_t result = 0;
    uint32_t divisor, ea;
    int reg = (opcode >> 9) & 7;
    divisor = readEA<E, Mode, Word>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    uint32_t dividend = readRegD<Long>(reg);

    if (divisor == 0) {
        v = c = false;
        if constexpr(Inst == Divu) {
            n = negative<Long>(dividend);
            z = (dividend & 0xffff0000) == 0;
            if constexpr (E == Engine::M68020)
                v = true;
        } else {
            n = false;
            z = true;
        }
        SYNC( (E == Engine::M68020) ? 5 : 8);
        if constexpr (E >= Engine::M68020)
            checkT0();
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

    if constexpr (E == Engine::M68000) {
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
    } else {
        if (overflow) {
            v = true;
            if constexpr(Inst == Divu) {
                if (negative<Long>(dividend)) n = true;
            } else {
                n = c = z = false;
                auto uDividend = uint32_t(std::abs((int32_t)dividend));
                auto uDivisor = uint16_t(std::abs((int16_t)divisor));

                if ((uDividend >> 16) < uDivisor) {
                    uint32_t quot = uDividend / uDivisor;
                    n = negative<Byte>(quot);
                    z = zero<Byte>(quot);
                }
            }
        } else {
            defaultFlags<Word>(result);
            writeRegD<Long>(reg, result);
        }

        prefetch<E, SampleIPL>();

        if constexpr(Inst == Divu)
            busCycles -= 44;
        else
            busCycles -= prefetchLast() ? 57 : 56;

        syncInternal<isDirectMode<Mode>() ? CheckIfPipeRunsDry : 0>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opDivl(uint16_t opcode) -> void {
    uint32_t ea;
    uint64_t dividend, divisor;

    int dr = irc & 7;
    int dq = (irc >> 12) & 7;
    int mode = (irc >> 10) & 3;
    readExtensionWord<E>();

    divisor = readEA<E, Mode, Size, ImmEA>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    if (mode & 1)
        dividend = (uint64_t)readRegD(dr) << 32 | readRegD(dq);
    else
        dividend = readRegD(dq);

    if (divisor == 0) {
        c = false;
        if (mode & 2) {
            n = false;
            z = true;
        } else {
            v = true;
            n = negative<Long>(dividend);
            z = static_cast<int32_t>(dividend) == 0;
        }
        SYNC( 5);
        checkT0();
        trapException<E>(5);
        return;
    }

    prefetch<E, SampleIPL>();

    if ((mode & 2) == 0) {
        uint64_t quotient = dividend / divisor;

        if (quotient > 0xffffffff) {
            v = true;
            c = false;
            n = negative<Long>(dividend);
            z = static_cast<int32_t>(dividend) == 0;
        } else {
            uint64_t remainder = dividend % divisor;
            v = false;
            c = false;
            n = negative<Long>(quotient);
            z = zero<Long>(quotient);

            writeRegD( dr, remainder );
            writeRegD( dq, quotient );
        }

        busCycles -= prefetchLast() ? 79 : 78;
        syncInternal<isDirectMode<Mode>() ? CheckIfPipeRunsDry : 0>();
    } else {
        int64_t _dividend;

        if (mode & 1) {
            _dividend = static_cast<int64_t>(dividend);
        } else 
            _dividend = static_cast<int64_t>(static_cast<int32_t>(dividend));

        auto _divisor = static_cast<int64_t>(static_cast<int32_t>(divisor));

        int64_t quotient = _dividend / _divisor;

        if ((dividend == 0x8000000000000000UL && static_cast<int32_t>(divisor) == -1)
            || ((quotient & 0xffffffff80000000) != 0 && (quotient & 0xffffffff80000000) != 0xffffffff80000000)) {
            v = true;
            setDivslUndefinedFlags(mode & 1 ? Long : Word, static_cast<int64_t>(dividend), static_cast<int32_t>(divisor));
        } else {
            int64_t remainder = _dividend % _divisor;
            if ((static_cast<int32_t>(remainder) < 0) != (_dividend < 0))
                remainder = -remainder;

            v = false;
            c = false;
            n = negative<Long>(quotient);
            z = zero<Long>(quotient);

            writeRegD( dr, remainder );
            writeRegD( dq, quotient );
        }

        busCycles -= prefetchLast() ? 91 : 90;
        syncInternal<isDirectMode<Mode>() ? CheckIfPipeRunsDry : 0>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveDataRegisterDirect(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();

    writeRegD<Size>(reg, result);
    defaultFlags<Size>(result);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<isDirectMode<Mode>() ? CheckIfPipeRunsDry : 0>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirect(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);

    if constexpr (E <  Engine::M68020) {
        ea = calcEA<E, AddressRegisterIndirect, Size>(reg);

        if (misaligned<Size>(ea)) {
            setMoveCCWhenAddressError<Mode, AddressRegisterIndirect, Size>(result);
            addressExceptionMoveEA<E, AddressRegisterIndirect, Size>(ea);
            return;
        }
        defaultFlags<Size>(result);

        if constexpr(((Size == Long) || isDirectMode<Mode>()) ) {
            write<E, Size, SampleIPL | Reverse>(ea, result);
            prefetch<E>();
        } else {
            write<E, Size, Reverse>(ea, result);
            prefetch<E, SampleIPL>();
        }
    } else {
        busCycles -= 1; // for move: target address calculation can overlap with read
        ea = readRegA( reg );

        defaultFlags<Size>(result);
        prefetch<E, SampleIPL>();
        syncInternal();
        if constexpr (isRegisterMode<Mode>())
            busCycles -= 1;

        write<E, Size>(ea, result);
        if constexpr (!isRegisterMode<Mode>())
            busCycles -= 2;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithPostIncrement(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);

    if constexpr (E < Engine::M68020) {
        ea = calcEA<E, AddressRegisterIndirectWithPostIncrement, Size>(reg);

        if (misaligned<Size>(ea)) {
            setMoveCCWhenAddressError<Mode, AddressRegisterIndirectWithPostIncrement, Size>(result);
            addressExceptionMoveEA<E, AddressRegisterIndirectWithPostIncrement, Size>(ea);
            return;
        }

        writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));
        write<E, Size, SampleIPL | Reverse>(ea, result);
        defaultFlags<Size>(result);
        prefetch<E>();

    } else {
        prefetch<E, SampleIPL>();
        if constexpr (isDirectMode<Mode>()) {
            if (!prefetchLast()) {
                syncInternal();
                SYNC(1);
                idleSequencer();
            } else
                busCycles -= 1;
        } else
            busCycles -= 1;

        syncInternal();
        if constexpr (isRegisterMode<Mode>())
            busCycles -= 1;
        ea = readRegA( reg );

        writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));
        defaultFlags<Size>(result);
        write<E, Size>(ea, result);
        if constexpr (!isRegisterMode<Mode>())
            busCycles -= 2;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithPreDecrement(uint16_t opcode) -> void {
    [[maybe_unused]] uint16_t IR;
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);

    if constexpr (E < Engine::M68020) {
        ea = calcEA<E, AddressRegisterIndirectWithPreDecrement, Size, ConcurrentAdrCalc>(reg);

        if constexpr (Size != Long)
            writeRegA(reg, ea);

        if constexpr(Size == Long) IR = ird;
        prefetch<E, SampleIPL>();

        if (misaligned<Size>(ea)) {
            if constexpr(Size == Long) ird = IR;
            setMoveCCWhenAddressError<Mode, AddressRegisterIndirectWithPreDecrement, Size>(result);
            addressExceptionMoveEA<E, AddressRegisterIndirectWithPreDecrement, Size>(ea);
            return;
        }

        if constexpr (Size == Long)
            writeRegA(reg, ea);

        write<E, Size>(ea, result);

        defaultFlags<Size>(result);
    } else {
        busCycles -= 2;
        ea = readRegA( reg );
        ea -= (Size == Byte && reg == 7) ? 2 : Size;
        writeRegA(reg, ea);

        prefetch<E, SampleIPL>();
        syncInternal();
        if constexpr (isRegisterMode<Mode>())
            busCycles -= 1;

        defaultFlags<Size>(result);
        write<E, Size>(ea, result);
        if constexpr (!isRegisterMode<Mode>())
            busCycles -= 2;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithDisplacement(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);

    if constexpr (E < Engine::M68020) {
        ea = calcEA<E, AddressRegisterIndirectWithDisplacement, Size>(reg);

        if (misaligned<Size>(ea)) {
            setMoveCCWhenAddressError<Mode, AddressRegisterIndirectWithDisplacement, Size>(result);
            addressExceptionMoveEA<E, AddressRegisterIndirectWithDisplacement, Size>(ea);
            return;
        }

        if constexpr ( isDirectMode<Mode>() )
            write<E, Size, SampleIPL | Reverse>(ea, result);
        else
            write<E, Size, Reverse>(ea, result);

        defaultFlags<Size>(result);

        if constexpr ( (Size == Long) && isDirectMode<Mode>() )
            prefetch<E>();
        else
            prefetch<E, SampleIPL>();

    } else {
        busCycles -= 2;
        ea = readRegA( reg ) + (int16_t)irc;
        readExtensionWord<E>();
        if (prefetchLast())
            SYNC(1);
        prefetch<E, SampleIPL>();
        if (prefetchLast())
            SYNC(1);
        syncInternal();
        if constexpr (isRegisterMode<Mode>())
            busCycles -= 1;

        defaultFlags<Size>(result);
        write<E, Size>(ea, result);
        if constexpr (!isRegisterMode<Mode>())
            busCycles -= 2;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAddressRegisterIndirectWithIndex(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);

    if constexpr (E < Engine::M68020) {
        ea = calcEA<E, AddressRegisterIndirectWithIndex, Size>(reg);

        if (misaligned<Size>(ea)) {
            setMoveCCWhenAddressError<Mode, AddressRegisterIndirectWithIndex, Size>(result);
            addressExceptionMoveEA<E, AddressRegisterIndirectWithIndex, Size>(ea);
            return;
        }

        if constexpr ( isDirectMode<Mode>() )
            write<E, Size, SampleIPL | Reverse>(ea, result);
        else
            write<E, Size, Reverse>(ea, result);

        defaultFlags<Size>(result);

        if constexpr ( (Size == Long) && isDirectMode<Mode>() )
            prefetch<E>();
        else
            prefetch<E, SampleIPL>();

    } else {
        ea = calcEA<E, AddressRegisterIndirectWithIndex, Size, EACalc>(reg);
        if constexpr (Mode == Immediate)
            busCycles += 2;
        else if constexpr (!isRegisterMode<Mode>())
            busCycles -= 1;

        prefetch<E, SampleIPL>();
        if (prefetchLast())
            SYNC(2);
        syncInternal();
        if constexpr (isRegisterMode<Mode>())
            busCycles -= 1;

        defaultFlags<Size>(result);
        write<E, Size>(ea, result);
        if constexpr (!isRegisterMode<Mode>())
            busCycles -= 2;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAbsoluteShort(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);

    if constexpr (E < Engine::M68020) {
        ea = calcEA<E, AbsoluteShort, Size>(reg);

        if (misaligned<Size>(ea)) {
            setMoveCCWhenAddressError<Mode, AbsoluteShort, Size>(result);
            addressExceptionMoveEA<E, AbsoluteShort, Size>(ea);
            return;
        }
        write<E, Size, Reverse>(ea, result);
        defaultFlags<Size>(result);
        prefetch<E, SampleIPL>();

    } else {
        busCycles -= 1;
        ea = (int16_t)irc;
        readExtensionWord<E>();
        if (prefetchLast())
            SYNC(1);

        prefetch<E, SampleIPL>();
        if (prefetchLast())
            SYNC(1);
        syncInternal();
        if constexpr (isRegisterMode<Mode>())
            busCycles -= 1;

        defaultFlags<Size>(result);
        write<E, Size>(ea, result);
        if constexpr (!isRegisterMode<Mode>())
            busCycles -= 2;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveAbsoluteLong(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size, Mode == Immediate ? EACalc : 0>(opcode & 7, ea);

    if constexpr (E < Engine::M68020) {
        if constexpr(!isDirectMode<Mode>())
            ea = calcEA<E, AbsoluteLong, Size, DelayPrefetch>(reg);
        else
            ea = calcEA<E, AbsoluteLong, Size>(reg);

        if (misaligned<Size>(ea)) {
            setMoveCCWhenAddressError<Mode, AbsoluteLong, Size>(result);
            addressExceptionMoveEA<E, AbsoluteLong, Size>(ea);
            return;
        }

        if constexpr ( !isDirectMode<Mode>() ) {
            write<E, Size, SampleIPL | Reverse>(ea, result);
            readExtensionWord<E>();
        } else {
            write<E, Size, Reverse>(ea, result);
        }

        defaultFlags<Size>(result);
        prefetch<E, SampleIPL>();

    } else {
        busCycles -= isDirectMode<Mode>() ? 3 : 2;
        ea = irc << 16;
        readExtensionWord<E>();
        ea |= irc;
        readExtensionWord<E>();

        prefetch<E, SampleIPL>();
        syncInternal();
        if constexpr (isRegisterMode<Mode>())
            busCycles -= 1;

        defaultFlags<Size>(result);
        write<E, Size>(ea, result);
        if constexpr (!isRegisterMode<Mode>())
            busCycles -= 2;
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveA(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isDirectMode<Mode>())
        idleSequencer();
    
    writeRegA(reg, Size == Word ? (int16_t)result : result);
    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<isDirectMode<Mode>() ? CheckIfPipeRunsDry : 0>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opArithmeticI(uint16_t opcode) -> void {
    uint32_t result, ea;
    uint32_t imm = calcEA<E, Immediate, Size, ImmEA>(0);
    
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020 && !isRegisterMode<Mode>())
        idleSequencer();
    
    prefetch<E, SampleIPL>();
    result = arithmetic<Inst, Size>( imm, result);

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= Inst == Cmpi ? 2 : 1;
            syncInternal();
        }
    } else if constexpr(Size == Long && isRegisterMode<Mode>()) {
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
    if constexpr (E == Engine::M68020 && !isRegisterMode<Mode>())
        idleSequencer();

    prefetch<E, SampleIPL>();
    if (Mode == AddressRegisterDirect)
        result = Inst == Addq ? (result + operand) : (result - operand);
    else
        result = arithmetic<Inst, Size>( operand, result);

    if constexpr (E == Engine::M68020) {
        if constexpr (isRegisterMode<Mode>()) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        } else {
            busCycles -= 1;
            syncInternal();
        }
    } else if constexpr (Mode == AddressRegisterDirect || (Size == Long && Mode == DataRegisterDirect))
        SYNC( 4 );

    writeEA<E, Mode, Size>(ea, result);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMoveQ(uint16_t opcode) -> void {
    uint8_t data = opcode & 0xff;

    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();
    }

    defaultFlags<Byte>(data);
    writeRegD((opcode >> 9) & 7, (int32_t)(int8_t)data);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opBcc(uint16_t opcode) -> void {
    if constexpr (E < Engine::M68020)
        SYNC(2);
    else if constexpr (E == Engine::M68020)
        busCycles -= 2;

    if (testCondition<Inst>()) {
        uint32_t disp = Size == Byte ? (uint8_t)opcode : irc;
        uint32_t newPC;

        if constexpr (Size == Long) {
            newPC = pc;
            disp = disp << 16 | stageB;
            newPC = static_cast<int64_t>(newPC) + static_cast<int64_t>(sign<Size>( disp ));
        } else
            newPC = pc + sign<Size>(disp);

        if constexpr (E == Engine::M68020) {
            busCycles -= 1;
            syncInternal();
        }

        if (misaligned<Long>(newPC))
            throw AddressError(newPC, E >= Engine::M68020 ? pcEdge : pc, SF_READ | SF_PRG);

        pc = newPC;
        if constexpr (E >= Engine::M68020)
            checkT0();

        fullPrefetch<E, SampleIPL>();
    } else {
        if constexpr (E < Engine::M68020)
            SYNC(2);
        else
            syncInternal();

        if constexpr (Size == Word || Size == Long) {
            readExtensionWord<E>();
            if constexpr (E == Engine::M68020)
                busCycles -= prefetchLast() ? 3 : 2;
        }
        if constexpr (Size == Long)
            readExtensionWord<E>();

        prefetch<E, SampleIPL>();
        if constexpr (E == Engine::M68020) {
            busCycles -= prefetchLast() ? 3 : 2;
            syncInternal<CheckIfPipeRunsDry>();
        }
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opBsr(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    if constexpr (E < Engine::M68020)
        SYNC(2);
    else if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<ForceIdle>();
    }
    sp -= 4;

    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc,SF_DATA);

    uint32_t disp = Size == Byte ? static_cast<uint8_t>(opcode) : irc;
    uint32_t newPC;

    if constexpr (Size == Long) {
        newPC = pc;
        disp = disp << 16 | stageB;
        incPc();
        newPC = static_cast<int64_t>(newPC) + static_cast<int64_t>(sign<Size>( disp ));
    } else
        newPC = pc + sign<Size>(disp);

    write<E, Long, Reverse>( sp, (Size == Word || Size == Long) ? (pc + 2) : pc); // stack PC of next opcode
    appendStepOut( (Size == Word || Size == Long) ? (pc + 2) : pc );

    pc = newPC;

    if constexpr (E >= Engine::M68020) {
        busCycles -= 5;
        syncInternal();
        busCycles += 3;
        checkT0();
    }

    if (misaligned<Long>(pc))
        throw AddressError(pc, E >= Engine::M68020 ? pcEdge : pc,SF_READ | SF_PRG); // stack new PC two times

    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opDbcc(uint16_t opcode) -> void {
    if constexpr (E < Engine::M68020)
        SYNC(2);
    else if constexpr (E == Engine::M68020) {
        busCycles -= 2;
    }

    int reg = opcode & 7;
    uint32_t memPC = pc;

    if (!testCondition<Inst>()) {
        pc = memPC + (int16_t)irc;

        uint16_t word = readRegD<Word>(reg);

        if (E >= Engine::M68020 ) {
            writeRegD<Word>( reg, word - 1);

            if (misaligned<Long>(pc))
                throw AddressError(pc, pc,SF_READ | SF_PRG);
        } else {
            if (misaligned<Long>(pc))
                throw AddressError(pc, pc + 2,SF_READ | SF_PRG);

            writeRegD<Word>( reg, word - 1);
        }

        if constexpr (E >= Engine::M68020) {
            irc = stageB;
            busCycles -= 1;
            syncInternal();
        }

        firstPrefetch<E>(); // assumes branch is taken, hence prefetch from new PC. (in order to speed up most likely case)
        
        if (word) { // while prefetching, CPU finds out if branch is really taken
            if constexpr (E >= Engine::M68020) {
                checkT0();
                busCycles -= 3;
                secondPrefetch<E>();
            }

            prefetch<E, SampleIPL>();

            if constexpr (E >= Engine::M68020)
                syncInternal();
            return;
        }

        if constexpr (E == Engine::M68020) {
            stageB = irc;
            busCycles -= 5;
        }

    } else {
        if constexpr (E < Engine::M68020)
            SYNC(2);
        else if constexpr (E == Engine::M68020) {
            busCycles -= 2;
            syncInternal();
        }
    }

    if (E >= Engine::M68020) {
        pc = memPC;
        readExtensionWord<E, FirstPrefetch>();
        prefetch<E, SampleIPL>();
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();

    } else {
        pc = memPC + 2;
        fullPrefetch<E, SampleIPL>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opJmp(uint16_t opcode) -> void {
    uint32_t _pc = pc;
    uint32_t adr = calcEA<E, Mode, Long, SkipExtension | EACalc>(opcode & 7);

    if constexpr (E == Engine::M68020) {
        busCycles -= 1;
        syncInternal();
    } else {
        if constexpr(isIndexMode<Mode>())                                   SYNC(4);
        if constexpr(Mode == AbsoluteShort || isDisplacementMode<Mode>())   SYNC(2);
    }

    if (misaligned<Long>(adr)) {
        if constexpr (E >= Engine::M68020) {
            if constexpr (isIndexMode<Mode>()) _pc = pc + 2;
        }

        throw AddressError(adr, _pc, SF_READ | SF_PRG);
    }

    pc = adr;

    if constexpr (E >= Engine::M68020)
        checkT0();
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opJsr(uint16_t opcode) -> void {
    uint32_t adr = calcEA<E, Mode, Long, SkipExtension>(opcode & 7);

    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal();
    } else {
        if constexpr(isIndexMode<Mode>())                                   SYNC(4);
        if constexpr(Mode == AbsoluteShort || isDisplacementMode<Mode>())   SYNC(2);
    }

    uint32_t& sp = regsA[7];

    if constexpr (E >= Engine::M68020) {
        sp -= 4;

        if (misaligned<Long>(adr))
            throw AddressError(adr, adr, SF_READ | SF_PRG);
    } else {
        if (misaligned<Long>(adr)) {
            if constexpr(isDisplacementMode<Mode>() || isIndexMode<Mode>())
                throw AddressError(adr, pc - 2, SF_READ | SF_PRG);
            else
                throw AddressError(adr, pc, SF_READ | SF_PRG);
        }

        sp -= 4;
    }

    uint32_t pcNextInstruction = pc;
    pc = adr;
    appendStepOut( pcNextInstruction );

    if constexpr (E >= Engine::M68020) {
        write<E, Long>(sp, pcNextInstruction);
        checkT0();
        fullPrefetch<E, SampleIPL>();

    } else {
        firstPrefetch<E>(); // confirmed with fx68k, first prefetch happens before stacking

        if (misaligned<Long>(sp))
            throw AddressError(sp, pcNextInstruction, SF_DATA);

        write<E, Long, Reverse>(sp, pcNextInstruction);
        prefetch<E, SampleIPL>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opLea(uint16_t opcode) -> void {
    writeRegA( (opcode >> 9) & 7, calcEA<E, Mode, Long, EACalc>(opcode & 7) );

    if constexpr (E < Engine::M68020) {
        if constexpr(isIndexMode<Mode>()) SYNC(2);
    }

    prefetch<E, SampleIPL>();

    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opPea(uint16_t opcode) -> void {
    uint32_t ea = calcEA<E, Mode, Long, EACalc>(opcode & 7);

    if constexpr (E < Engine::M68020) {
        uint16_t IR = ird;

        if constexpr(!isAbsMode<Mode>()) {
            sampleInterrupt();
            if constexpr(isIndexMode<Mode>()) SYNC(2);
            prefetch<E>();
        }

        uint32_t& sp = regsA[7];
        sp -= 4;

        if (misaligned<Long>(sp)) {
            ird = IR;
            throw AddressError(sp, pc, SF_DATA);
        }

        write<E, Long, Reverse>(sp, ea);

        if constexpr(isAbsMode<Mode>())
            prefetch<E, SampleIPL>();

    } else {
        uint32_t& sp = regsA[7];
        sp -= 4;
        prefetch<E, SampleIPL>();
        busCycles -= 2;
        syncInternal();
        write<E, Long>(sp, ea);
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMovemToReg(uint16_t opcode) -> void {
    uint16_t mask = irc;
    readExtensionWord<E>();
    uint32_t ea = calcEA<E, Mode, Size, ImmEACalc>(opcode & 7);

    if (E < Engine::M68020 && misaligned<Long>(ea))
        throw AddressError(ea, isIndexMode<Mode>() ? (pc - 2) : (pc + 2), SF_READ | (isPcMode<Mode>() ? SF_PRG : SF_DATA ));

    if constexpr (E == Engine::M68020) {
        idleSequencer();
        SYNC(5);
    }

    if constexpr(Mode == AddressRegisterIndirectWithPostIncrement)
        sampleInterrupt();

    for (int r = 0; r <= 15; r++) {
        if (mask & (1 << r)) {

            writeRegR(r, sign<Size>( read<E, Size, FC_EA_PRG>(ea) ) );

            if constexpr (E == Engine::M68020) {
                busCycles -= 4;
                syncInternal<ForceIdle>();
            }

            ea += Size;
        }
    }

    if constexpr(Mode == AddressRegisterIndirectWithPostIncrement)
        writeRegA( opcode & 7, ea );

    if constexpr (E < Engine::M68020)
        read<E, Word, FC_EA_PRG>(ea); // dummy read (brackets for Long are wrong in Yacht )

    if constexpr(Mode != AddressRegisterIndirectWithPostIncrement)
        prefetch<E, SampleIPL>();
    else
        prefetch<E>();

    if constexpr (E == Engine::M68020) {
        busCycles -= 4;
        syncInternal();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMovemToEa(uint16_t opcode) -> void {
    uint16_t mask = irc;
    readExtensionWord<E>();
    int reg = opcode & 7;

    if constexpr (E == Engine::M68020) {
        idleSequencer();
        SYNC(2);
    }

    if constexpr(Mode == AddressRegisterIndirectWithPreDecrement) {
        uint32_t ea = readRegA(reg);
        if (E < Engine::M68020 && mask && misaligned<Long>(ea))
            throw AddressError(ea - 2, pc + 2, SF_DATA);

        for (int r = 15; r >= 0; r-- ) {
            if (mask & (0x8000 >> r)) {
                ea -= Size;

                if constexpr (E == Engine::M68020)
                    writeRegA( reg, ea );

                if (r & 8)
                    write<E, Size>(ea, regsA[r & 7] );
                else
                    write<E, Size>(ea, regsD[r] );
            }
        }

        if constexpr (E < Engine::M68020)
            writeRegA( reg, ea );
    } else {
        uint32_t ea = calcEA<E, Mode, Size, ImmEACalc>(reg);
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

    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opArithmeticX(uint16_t opcode) -> void {
    int destReg = (opcode >> 9) & 7;
    if constexpr (E < Engine::M68020) {
        prefetch<E, SampleIPL>();
        if constexpr(Size == Long)  SYNC(4);

    } else {
        idleSequencer();
        prefetch<E, SampleIPL>();
        busCycles -= 2;
        syncInternal();
    }

    writeRegD<Size>( destReg, arithmetic<Inst, Size>(readRegD<Size>( opcode & 7 ), readRegD<Size>( destReg ) ) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opArithmeticBCD(uint16_t opcode) -> void {
    int destReg = (opcode >> 9) & 7;
    if constexpr (E < Engine::M68020) {
        prefetch<E, SampleIPL>();
        SYNC(2);

    } else {
        idleSequencer();
        SYNC(2);
        prefetch<E, SampleIPL>();
        busCycles -= 2;
        syncInternal();
    }

    writeRegD<Size>( destReg, bcd<E, Inst>(readRegD<Size>( opcode & 7 ), readRegD<Size>( destReg ) ) );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opArithmeticXEa(uint16_t opcode) -> void {
    uint32_t result, dest, ea;
    int srcReg = opcode & 7;
    int destReg = (opcode >> 9) & 7;

    ea = readRegA( srcReg );
    ea -= (Size == Byte && srcReg == 7) ? 2 : Size;

    if constexpr (E < Engine::M68020) {
        SYNC(2);
        if (misaligned<Size>(ea)) {
            if constexpr(Size == Word) writeRegA(srcReg, ea);
            throw AddressError(Size == Long ? (ea + 2) : ea, pc + 2, SF_READ | SF_DATA);
        }
    } else {
        prefetch<E, SampleIPL>();
        busCycles -= 2;
        syncInternal<ForceIdle>();
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
        result = bcd<E, Inst>(result, dest );
    else
        result = arithmetic<Inst, Size>(result, dest );

    if constexpr (E == Engine::M68020) {
        idleSequencer();
        if constexpr (Inst == Abcd || Inst == Sbcd)
            busCycles -= 5;
        else
            busCycles -= 1;

        syncInternal();
        write<E, Size>( ea, result );
    } else {
        if constexpr(Size == Long) write<E, Word, SampleIPL>( ea + 2, result & 0xffff );

        prefetch<E>();

        if constexpr(Size == Long)  write<E, Word>( ea, (result >> 16) & 0xffff );
        else                        write<E, Size>( ea, result );
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opCmpm(uint16_t opcode) -> void {
    uint32_t result, dest, ea;
    int srcReg = opcode & 7;
    int destReg = (opcode >> 9) & 7;

    ea = readRegA( srcReg );
    if (E < Engine::M68020 && misaligned<Size>(ea))
        throw AddressError(ea, pc + 2, SF_READ | SF_DATA);

    if constexpr (E == Engine::M68020) {
        busCycles -= 1;
        syncInternal();
    }

    result = read<E, Size>(ea);
    writeRegA(srcReg, ea + ((Size == Byte && srcReg == 7) ? 2 : Size) );

    ea = readRegA( destReg );
    if (E < Engine::M68020 && misaligned<Size>(ea))
        throw AddressError(ea, pc + 2, SF_READ | SF_DATA);

    sampleInterrupt();
    dest = read<E, Size>(ea);
    writeRegA(destReg, ea + ((Size == Byte && destReg == 7) ? 2 : Size) );

    arithmetic<Cmp, Size>(result, dest );

    if constexpr (E == Engine::M68020) {
        idleSequencer();
        prefetch<E>();
        busCycles -= 2;
        syncInternal();
    } else
        prefetch<E>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opCcr(uint16_t opcode) -> void {
    uint8_t data = irc & 0xff;
    readExtensionWord<E>();

    if constexpr (E == Engine::M68020) {
        ccr<Inst>( data );
        checkT0();
        busCycles -= 9;
        syncInternal();
        fullPrefetch<E, SampleIPL>();
    } else {
        SYNC(8); // confirmed with fx68k
        ccr<Inst>( data );
        fullPrefetch<E, SampleIPL>(); // first fetch is dummy at same address like extension word
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opSr(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint16_t data = irc;
    readExtensionWord<E>();

    if constexpr (E == Engine::M68020) {
        checkT0();
        sr<Inst>( data );
        busCycles -= 9;
        syncInternal();
        fullPrefetch<E, SampleIPL>();
    } else {
        SYNC(8);
        sr<Inst>( data );
        fullPrefetch<E, SampleIPL>(); // first fetch is dummy at same address like extension word
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opChk(uint16_t opcode) -> void {
    uint32_t result, ea;
    int reg = (opcode >> 9) & 7;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020)
        idleSequencer();

    uint32_t dataD = readRegD<Size>(reg);

    if constexpr (E == Engine::M68000 ) {
        z = zero<Word>( dataD );
        v = c = n = false;
    } else
        setChkUndefinedFlags<Size>(sign<Size>(result), sign<Size>(dataD));

    SYNC(4);
    if ( sign<Size>(dataD) > sign<Size>(result)) {
        SYNC(4);
        n = negative<Size>(dataD); // z, n flag changes happen not here, but later in exception routine
        if constexpr (E >= Engine::M68020 )
            checkT0();
        return trapException<E>( 6 );
    }

    if constexpr (E < Engine::M68020) {
        SYNC(2);
    } else
        SYNC(1);

    if (sign<Size>(dataD) < 0) {
        SYNC(4);
        n = true;
        if constexpr (E >= Engine::M68020 )
            checkT0();
        return trapException<E>( 6 );
    }
    
    prefetch<E, SampleIPL>(); // Yacht is wrong
    if constexpr (E == Engine::M68020 ) {
        busCycles -= 3;
        syncInternal();
    }
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
    result = readEA<E, Mode, Size, ImmEA>(opcode & 7, ea);
    result2 = read<E, Size, FC_EA_PRG>(ea + Size);

    int32_t boundL = sign<Size>(result);
    int32_t boundU = sign<Size>(result2);

    if (value == boundL || value == boundU) {
        z = true;
        c = false;
    } else {
        z = false;

        if (boundL <= boundU)
            c = value < boundL || value > boundU;
        else
            c = value < boundL && value > boundU;
    }

    setChk2UndefinedFlags(boundL, boundU, value);

    idleSequencer();
    SYNC(10);

    if (useExc && c) {
        SYNC(6);
        checkT0();
        trapException<E>(6);
    } else {
        prefetch<E, SampleIPL>();
        if (!prefetchLast())
            SYNC(3);
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveFromSr(uint16_t opcode) -> void {
    uint32_t ea;

    if constexpr (E >= Engine::M68020) {
        if (!s)
            return privilegeException<E>();

        int reg = opcode & 7;

        ea = calcEaUpdatePrePost<E, Mode, Size, EACalc>(reg);

        idleSequencer();

        SYNC(2);

        prefetch<E, SampleIPL>();

        busCycles -= 2;

        syncInternal();

    } else {
        readEA<E, Mode, Size>(opcode & 7, ea);

        prefetch<E, SampleIPL>();
        if constexpr(isRegisterMode<Mode>()) SYNC(2);
    }

    writeEA<E, Mode, Size>(ea, getSR() );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveToSr(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);
    if constexpr (E == Engine::M68020) {
        idleSequencer();
        SYNC(5);
    } else
        SYNC(4);

    if constexpr (E >= Engine::M68020)
        checkT0();
    setSR( result );
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveToCcr(uint16_t opcode) -> void {
    uint32_t result, ea;
    result = readEA<E, Mode, Size>(opcode & 7, ea);

    setCCR( result & 0xff );

    if constexpr (E == Engine::M68020) {
        idleSequencer();
        SYNC(2);
        prefetch<E, SampleIPL>();
        busCycles -= 2;
        syncInternal();
    } else {
        SYNC(4);
        fullPrefetch<E, SampleIPL>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExgDxDy(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();
    } else
        SYNC(2);
    std::swap( regsD[opcode & 7], regsD[(opcode >> 9) & 7 ] );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExgAxAy(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();
    } else
        SYNC(2);
    std::swap( regsA[opcode & 7], regsA[(opcode >> 9) & 7 ] );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExgAxDy(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();
    } else
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
    if constexpr (E == Engine::M68020) {
        busCycles -= 4;
        syncInternal<CheckIfPipeRunsDry>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opExtb(uint16_t opcode) -> void {
    uint32_t dn = readRegD<Size>( opcode & 7 );

    dn = sign<Byte>(dn);

    writeRegD<Size>(opcode & 7, dn);
    defaultFlags<Size>(dn);
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        busCycles -= 4;
        syncInternal<CheckIfPipeRunsDry>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMoveUspAn(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    writeRegA( opcode & 7, usp );
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMoveAnUsp(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    usp = readRegA( opcode & 7 );
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        busCycles -= 2;
        syncInternal<CheckIfPipeRunsDry>();
    }
}

template<M68Core::Engine E> auto M68Core::opNop(uint16_t opcode) -> void {
    prefetch<E, SampleIPL>();
    if constexpr (E == Engine::M68020) {
        // syncs sequencer: no head (cycles which overlaps with instruction before)
        idleSequencer();
        if (!prefetchLast())
            SYNC(2);
        // syncs sequencer: no tail (cycles which overlaps with instruction after)
    }
}

template<M68Core::Engine E> auto M68Core::opReset(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    SYNC(2);
    sampleInterrupt();
    SYNC(2);
    RESET_OUT(); // assert reset line after 4.5 clocks for 124 clocks (128.5)

    if constexpr (E < Engine::M68020) {
        SYNC(124);
        prefetch<E>();
    } else {
        idleSequencer();
        SYNC(512);
        prefetch<E>();
        busCycles -= 2;
        syncInternal<ForceIdle>();
    }
}

template<M68Core::Engine E> auto M68Core::opRte(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint32_t& sp = regsA[7];
    uint16_t newSr;
    uint32_t newPc;

    if constexpr (E >= Engine::M68020) {
        busCycles -= 1;
        syncInternal<ForceIdle>();
        while (true) {
            uint8_t format = read<E, Word>(sp + 6) >> 12;

            if (format == 0) {
                newSr = read<E, Word>( sp );
                newPc = read<E, Word>( sp + 2 ) << 16;
                newPc |= read<E, Word>( sp + 4 );
                sp += 8;
                idleSequencer();
                busCycles -= 5;
                break;
            } if (format == 1) {
                newSr = read<E, Word>( sp );
                (void)read<E, Word>( sp + 2 );
                (void)read<E, Word>( sp + 4 );
                SYNC(3);
                sp += 8;
                setSR( newSr );
                continue;
            } if (format == 2) {
                newSr = read<E, Word>( sp );
                newPc = read<E, Long>( sp + 2 );
                (void)read<E, Long>( sp + 8 );
                sp += 8 + 4;
                idleSequencer();
                busCycles -= 5;
                break;
            } if (format == 9) { // copro
                newSr = read<E, Word>( sp );
                newPc = read<E, Long>( sp + 2 );
                sp += 8 + 12;
                idleSequencer();
                break;
            } if (format == 0xa) {
                newSr = read<E, Word>( sp );
                newPc = read<E, Long>( sp + 2 );
                sp += 8;
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Word>(sp); sp += 2;
                (void)read<E, Long>(sp); sp += 4;
                (void)read<E, Long>(sp); sp += 4;
                (void)read<E, Long>(sp); sp += 4;
                (void)read<E, Long>(sp); sp += 4;
                idleSequencer();
                busCycles -= 8;
                break;
            } if (format == 0xb) {
                newSr = read<E, Word>( sp );
                newPc = read<E, Long>( sp + 2 );
                for (int j = 0; j < 21; j++)
                    (void)read<E, Long>(sp + 8 + j * 4);

                sp += 8 + 84;
                idleSequencer();
                busCycles -= 16;
                break;
            }

            return formatException<E>();
        }
        checkT0();
    } else {

        if (misaligned<Long>(sp))
            throw AddressError(sp, pc, SF_READ | SF_DATA); // always leads to halted state after 20 cycles

        newSr = read<E, Word>( sp );
        newPc = read<E, Long>( sp + 2 );
        // 68000: SP and status will be updated during second micro cycle of third BUS cycle.
        sp += 6;
    }

    setSR( newSr );

    if (misaligned<Long>(newPc))
        throw AddressError(newPc, E >= Engine::M68020 ? pcEdge : pc, SF_READ | SF_PRG);

    if(!stepOuts.empty()) stepOuts.pop_back();

    pc = newPc;
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opRtd(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    busCycles -= 1;
    syncInternal<ForceIdle>();

    uint32_t newPc = read<E, Long>(sp);
    sp += 4 + static_cast<int16_t>(irc);

    if (misaligned<Long>(newPc))
        throw AddressError(newPc, pc, SF_READ | SF_PRG);

    pc = newPc;
    busCycles -= 6;
    idleSequencer();
    checkT0();
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opRtr(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc, SF_READ | SF_DATA); // if in user mode, a misaligned SP doesn't have to lead to halted state

    if constexpr (E == Engine::M68020) {
        busCycles -= 1;
        syncInternal<ForceIdle>();
    }

    uint8_t newCcr = read<E, Word>( sp ) & 0xff;
    uint32_t newPc = read<E, Long>( sp + 2 );

    sp += 6;
    setCCR( newCcr );

    if (misaligned<Long>(newPc))
        throw AddressError(newPc, pc, SF_READ | SF_PRG);

    if(!stepOuts.empty()) stepOuts.pop_back();
    pc = newPc;
    if constexpr (E >= Engine::M68020) {
        idleSequencer();
        busCycles -= 4;
        checkT0();
    }
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opRts(uint16_t opcode) -> void {
    uint32_t& sp = regsA[7];
    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc, SF_READ | SF_DATA); // if in user mode, a misaligned SP doesn't have to lead to halted state

    if constexpr (E == Engine::M68020) {
        busCycles -= 1;
        syncInternal<ForceIdle>();
    }

    uint32_t newPc = read<E, Long>( sp );
    sp += 4;

    if (misaligned<Long>(newPc))
        // after stacking current PC, new PC is copied to PC and stacked as fault address.
        // for simplicity, that is not emulated and not needed either, because PC is updated from vector a few cycles later.
        throw AddressError(newPc, E >= Engine::M68020 ? pcEdge : pc, SF_READ | SF_PRG);

    if(!stepOuts.empty()) stepOuts.pop_back();
    pc = newPc;

    if constexpr (E >= Engine::M68020) {
        idleSequencer();
        busCycles -= 6;
        checkT0();
    }
    fullPrefetch<E, SampleIPL>();
}

template<M68Core::Engine E> auto M68Core::opStop(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    sampleInterrupt(); // if detected, no stop at all. like all the other sampling points (one clock cycle sooner)
    setSR( irc );
    control |= Stop;
    pcEdge = pc + 2;

    if constexpr (E == Engine::M68020)
        idleSequencer();

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
    if constexpr (E == Engine::M68020) {
        busCycles -= 4;
        syncInternal<CheckIfPipeRunsDry>();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opTrap(uint16_t opcode) -> void {
    SYNC(4);
    if constexpr (E >= Engine::M68020)
        checkT0();
    trapException<E>( (opcode & 0xf) + 32 );
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opTrapcc(uint16_t opcode) -> void {
    if constexpr (Size == Word) {
        readExtensionWord<E>();
        busCycles -= 2;
    } else if constexpr (Size == Long) {
        readExtensionWord<E>();
        busCycles -= prefetchLast() ? 3 : 2;
        readExtensionWord<E>();
        busCycles -= prefetchLast() ? 3 : 2;
    }
    busCycles -= 2;
    syncInternal();

    if (testCondition<Inst>()) {
        SYNC(4);
        checkT0();
        trapException<E>( 7 );
        return;
    }

    prefetch<E, SampleIPL>();
    busCycles -= 2;
    syncInternal<CheckIfPipeRunsDry>();
}

template<M68Core::Engine E> auto M68Core::opTrapv(uint16_t opcode) -> void {
    if (v) {
        if constexpr (E >= Engine::M68020) {
            checkT0();
            SYNC(6);
        } else
            read<E, Word, PRG>(pc); // doesn't update irc

        trapException<E>( 7 );
    } else {
        if constexpr (E == Engine::M68020) {
            busCycles -= 2;
            syncInternal();
        }
        prefetch<E, SampleIPL>();
        if constexpr (E == Engine::M68020) {
            busCycles -= 2;
            syncInternal<CheckIfPipeRunsDry>();
        }
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opLink(uint16_t opcode) -> void {
    uint32_t displacement = irc;
    readExtensionWord<E>();
    if constexpr (E == Engine::M68020)
        busCycles -= prefetchLast() ? 3 : 1;

    if constexpr (Size == Long) {
        displacement = displacement << 16 | irc;
        readExtensionWord<E>();
        busCycles -= prefetchLast() ? 3 : 1;
    }

    if constexpr (E >= Engine::M68020) {
        prefetch<E>();
        busCycles -= prefetchLast() ? 3 : 1;
        syncInternal();
    }

    uint32_t _regA = readRegA(opcode & 7);
    uint32_t sp = regsA[7];
    sp -= 4;
    writeRegA(opcode & 7, sp); // 68000: happens 1.5 CPU cycles after reading extension word

    if (E < Engine::M68020 && misaligned<Long>(sp))
        throw AddressError(sp, pc + 2, SF_DATA);

    sampleInterrupt();
    write<E, Long, Reverse>(sp, _regA);
    regsA[7] = sp + sign<Size>(displacement); // 68000: happens 1.5 CPU cycles after "long write"

    if constexpr (E < Engine::M68020)
        prefetch<E>();
    else
        busCycles -= 2;
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opUnlink(uint16_t opcode) -> void {
    uint32_t data;
    uint32_t& sp = regsA[7];
    uint32_t newSP = readRegA<Long>(opcode & 7);
    if (E < Engine::M68020 && misaligned<Long>(newSP))
        throw AddressError(newSP, pc + 2, SF_READ | SF_DATA);

    if constexpr (E >= Engine::M68020) {
        busCycles -= 1;
        syncInternal<ForceIdle>();
        sampleInterrupt();
        data = read<E, Long>( newSP & 0xffffff );
        idleSequencer();
    } else {
        data = read<E, Word>( newSP & 0xffffff ) << 16;
        data |= read<E, Word, SampleIPL>( (newSP + 2) & 0xffffff );
    }

    sp = newSP + 4; // happens after first micro cycle of prefetch. when using register 7, this step happens too (visible in fx68k)
    writeRegA( opcode & 7, data ); // when using register 7, previous step is useless
    prefetch<E>();
    if constexpr (E >= Engine::M68020) {
        busCycles -= 2;
        syncInternal();
    }
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
    if constexpr (E == Engine::M68020) {
        idleSequencer();
        busCycles -= 4;
        syncInternal();
    }
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
    if constexpr (E == Engine::M68020) {
        busCycles -= 3;
        syncInternal();
    }
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opBkpt(uint16_t opcode) -> void {
    // when external hardware recognizes Bkpt CPU Space cycle, its able to inject instruction word via data bus
    // A1200 doesn't respond to Bkpt CPU Space Cycle -> results in illegal exception
    // int nr = opcode & 7;
    (void)read<E, Size>(pc);
    illegal<E>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opPackDn(uint16_t opcode) -> void {
    uint32_t src = readRegD( opcode & 7 ) + irc;
    readExtensionWord<E>();
    busCycles -= prefetchLast() ? 4 : 3;
    uint8_t dest = ((src >> 4) & 0xf0) | (src & 0xf);
    writeRegD<Byte>( (opcode >> 9) & 7, dest );
    prefetch<E, SampleIPL>();
    busCycles -= prefetchLast() ? 4 : 3;
    syncInternal<CheckIfPipeRunsDry>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opPackPd(uint16_t opcode) -> void {
    uint32_t result, ea;
    uint32_t adjust = irc;
    readExtensionWord<E>();
    result = readEA<E, AddressRegisterIndirectWithPreDecrement, Word>( opcode & 7, ea );
    prefetch<E, SampleIPL>();
    busCycles -= 3;

    uint32_t src = result + adjust;
    uint8_t dest = ((src >> 4) & 0xf0) | (src & 0xf);

    int reg = (opcode >> 9) & 7;
    ea = calcEA<E, AddressRegisterIndirectWithPreDecrement, Byte>(reg);
    writeRegA(reg, ea);
    write<E, Byte>(ea, dest);
    busCycles -= 1;
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opUnpackDn(uint16_t opcode) -> void {
    uint32_t src = readRegD( opcode & 7 );
    uint32_t dest = ((src << 4 & 0xf00) | (src & 0xf)) + irc;
    readExtensionWord<E>();
    busCycles -= prefetchLast() ? 4 : 3;

    writeRegD<Word>( (opcode >> 9) & 7, dest );
    prefetch<E, SampleIPL>();
    busCycles -= prefetchLast() ? 6 : 5;
    syncInternal<CheckIfPipeRunsDry>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opUnpackPd(uint16_t opcode) -> void {
    uint32_t result, ea;
    uint32_t adjust = irc;
    readExtensionWord<E>();
    result = readEA<E, AddressRegisterIndirectWithPreDecrement, Byte>( opcode & 7, ea );
    prefetch<E, SampleIPL>();
    busCycles -= 3;

    uint32_t dest = ((result << 4 & 0xf00) | (result & 0xf)) + adjust;

    int reg = (opcode >> 9) & 7;
    ea = calcEA<E, AddressRegisterIndirectWithPreDecrement, Word>(reg);
    writeRegA(reg, ea);
    write<E, Word>(ea, dest);
    busCycles -= 1;
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMovecRcRx(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint8_t rn = (irc >> 12) & 0xf;
    uint16_t ctl = irc & 0xfff;

    switch (ctl) {
        case 0: writeRegR( rn, sfc ); break;
        case 1: writeRegR( rn, dfc ); break;
        case 2: writeRegR( rn, cacr ); break;
        case 0x800: writeRegR( rn, usp ); break;
        case 0x801: writeRegR( rn, vbr ); break;
        case 0x802: writeRegR( rn, caar ); break;
        case 0x803: writeRegR( rn, m ? regsA[7] : msp ); break;
        case 0x804: writeRegR( rn, m ? ssp : regsA[7] ); break;
        default:
            illegal<E>(opcode);
            return;
    }

    readExtensionWord<E>();
    busCycles -= prefetchLast() ? 4 : 3;
    prefetch<E, SampleIPL>();
    busCycles -= prefetchLast() ? 4 : 3;
    syncInternal<CheckIfPipeRunsDry>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Size> auto M68Core::opMovecRxRc(uint16_t opcode) -> void {
    if (!s)
        return privilegeException<E>();

    uint8_t rn = (irc >> 12) & 0xf;
    uint16_t ctl = irc & 0xfff;

    uint32_t value = readRegR( rn );

    switch (ctl) {
        case 0: sfc = value & 7; break;
        case 1: dfc = value & 7; break;
        case 2: setCacr(value); break;
        case 0x800: usp = value; break;
        case 0x801: vbr = value; break;
        case 0x802: caar = value; break;
        case 0x803: (m ? regsA[7] : msp) = value; break;
        case 0x804: (m ? ssp : regsA[7]) = value; break;
        default:
            illegal<E>(opcode);
            return;
    }
    busCycles -= 6;
    readExtensionWord<E>();
    busCycles -= prefetchLast() ? 4 : 3;
    prefetch<E, SampleIPL>();
    busCycles -= prefetchLast() ? 4 : 3;
    syncInternal<CheckIfPipeRunsDry>();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoves(uint16_t opcode) -> void {
    uint32_t ea;
    if (!s)
        return privilegeException<E>();

    int reg = opcode & 7;
    uint16_t ext = irc;
    int rn = ext >> 12;

    readExtensionWord<E>();

    if (ext & 0x800) {
        uint32_t data = readRegR<Size>( rn );

        ea = calcEA<E, Mode, Size, ImmEACalc>( reg );

        if constexpr (Mode == AddressRegisterIndirectWithPreDecrement) {
            writeRegA(reg, ea);

            if (rn == (reg | 8))
                data -= (reg == 7 && Size == Byte) ? 2 : Size;

        }
        if constexpr (Mode == AddressRegisterIndirectWithPostIncrement) {
            writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));

            if (rn == (reg | 8))
                data += (reg == 7 && Size == Byte) ? 2 : Size;
        }

        write<E, Size, DFC>(ea, data);

    } else {
        uint32_t result = readEA<E, Mode, Size, SFC | ImmEACalc>( reg, ea );

        writeRegR<Size>(rn, result);
    }

    prefetch<E, SampleIPL>();
    busCycles -= prefetchLast() ? (ext & 0x800 ? 7 : 8) : 2;
    idleSequencer();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opMoveCcr(uint16_t opcode) -> void {
    int reg = opcode & 7;
    uint32_t ea = calcEaUpdatePrePost<E, Mode, Size, EACalc>( reg );
    busCycles -= 1;
    syncInternal();
    prefetch<E, SampleIPL>();
    busCycles -= prefetchLast() ? 3 : 1;
    syncInternal();

    writeEA<E, Mode, Size>(ea, getCCR());
    idleSequencer();
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCpSave(uint16_t opcode) -> void {
    if (!s)
        privilegeException<E>();
    else
        lineF<E>(opcode);
}

template<M68Core::Engine E, uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::opCpRestore(uint16_t opcode) -> void {
    if (!s)
        privilegeException<E>();
    else
        lineF<E>(opcode);
}

}
