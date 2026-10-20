
#include "m68Core.h"

#define FC_EA_PRG (isPcMode<Mode>() ? PRG : 0)

namespace M68FAMILY {

template<uint8_t Size> auto M68Core::clip(uint32_t data) -> uint32_t {
    if constexpr (Size == Byte) return data & 0xff;
    if constexpr (Size == Word) return data & 0xffff;
    if constexpr (Size == Long) return data & 0xffffffff;
}

template<uint8_t Size> constexpr auto M68Core::mask() -> uint32_t {
    if constexpr (Size == Byte) return 0xff;
    if constexpr (Size == Word) return 0xffff;
    if constexpr (Size == Long) return 0xffffffff;
}

template<uint8_t Size> constexpr auto M68Core::inverseMask() -> uint32_t {
    if constexpr (Size == Byte) return 0xffffff00;
    if constexpr (Size == Word) return 0xffff0000;
    if constexpr (Size == Long) return 0;
}

template<uint8_t Size> constexpr auto M68Core::msb() -> uint32_t {
    if constexpr (Size == Byte) return 0x80;
    if constexpr (Size == Word) return 0x8000;
    if constexpr (Size == Long) return 0x80000000;
}

template<uint8_t Size> auto M68Core::negative(uint32_t data) -> bool {
    if constexpr (Size == Byte) return (data & 0x80) != 0;
    if constexpr (Size == Word) return (data & 0x8000) != 0;
    if constexpr (Size == Long) return (data & 0x80000000) != 0;
}

template<uint8_t Size> auto M68Core::carry(uint64_t data) -> bool {
    if constexpr (Size == Byte) return (data & 0x100) != 0;
    if constexpr (Size == Word) return (data & 0x10000) != 0;
    if constexpr (Size == Long) return (data & 0x100000000) != 0;
}

template<uint8_t Size> auto M68Core::sign(uint32_t data) -> int32_t {
    if constexpr (Size == Byte) return (int8_t)data;
    if constexpr (Size == Word) return (int16_t)data;
    if constexpr (Size == Long) return (int32_t)data;
}

template<uint8_t Size> constexpr auto M68Core::bits() -> uint8_t {
    if constexpr (Size == Byte) return 8;
    if constexpr (Size == Word) return 16;
    if constexpr (Size == Long) return 32;
}

template<uint8_t Size> auto M68Core::readRegD(int reg) -> uint32_t {
    return clip<Size>( regsD[reg] );
}

template<uint8_t Size> auto M68Core::writeRegD(int reg, uint32_t data) -> void {
    regsD[reg] = (regsD[reg] & inverseMask<Size>()) | (data & mask<Size>());
}

template<uint8_t Size> auto M68Core::readRegA(int reg) -> uint32_t {
    return clip<Size>( regsA[reg] );
}

auto M68Core::writeRegA(int reg, uint32_t data) -> void {
    regsA[reg] = data;
}

template<uint8_t Size> auto M68Core::readRegR( int reg ) -> uint32_t {
    if (reg & 8)
        return readRegA<Size>(reg & 7);

    return readRegD<Size>(reg);
}

template<uint8_t Size> auto M68Core::writeRegR( int reg, uint32_t val ) -> void {
    if (reg & 8)
        writeRegA(reg & 7, sign<Size>(val));
    else
        writeRegD<Size>(reg, val);
}

template<M68Core::Engine E, unsigned Flags> auto M68Core::fullPrefetch() -> void {
    firstPrefetch<E>();
    if constexpr (E == Engine::M68020) {
        if constexpr (Flags & DelayPrefetch) {
            if (!busCycles) SYNC(2);
        }
        secondPrefetch<E>();
        prefetch<E, Flags>();
        if constexpr (Flags & DelayPrefetch) {
            if (!busCycles) SYNC(2);
        }
    } else {
        if constexpr (Flags & DelayPrefetch) SYNC(2);
        prefetch<E, Flags>();
    }
}

template<M68Core::Engine E, unsigned Flags> auto M68Core::firstPrefetch() -> void {
     if (E == Engine::M68020) {
        stageB = cachedPrefetch<Flags | FirstPrefetch | PRG >( pc );
     } else
        irc = read<E, Word, Flags | PRG>(pc);
}

template<M68Core::Engine E, unsigned Flags> auto M68Core::secondPrefetch() -> void {
    irc = stageB;
    stageB = cachedPrefetch<Flags | PRG >( pc + 2 );
}

template<M68Core::Engine E, unsigned Flags> auto M68Core::prefetch() -> void {
    ird = irc;
    pcEdge = pc;
    pc += 2;

     if (E == Engine::M68020) {
        irc = stageB;
        stageB = cachedPrefetch<Flags | PRG>( pc + 2 );
     } else
        irc = read<E, Word, Flags | PRG>(pc);
}

template<M68Core::Engine E, unsigned Flags> auto M68Core::readExtensionWord() -> void {
    pc += 2;

    if (E == Engine::M68020) {
        irc = stageB;
        stageB = cachedPrefetch<Flags | PRG>( pc + 2 );
    } else
        irc = read<E, Word, Flags | PRG>(pc);
}

template<unsigned Flags> auto M68Core::cachedPrefetch(uint32_t adr) -> uint16_t {
    busCycles = 0;
    fc = SF_PRG;

    if constexpr (Flags & EXC)
        fc |= SF_EXC;

    if (control & WatchPoint) {
        if (watchPoints.check( pc, Word, true ))
            DEBUG_POINT_REACHED(WatchPoint, watchPoints, pc);
    }

    if constexpr(Flags & SampleIPL)
        sampleInterrupt();

    // full prefetch invalidates cache holding register.
    // even without the cache enabled, BUS is accessed only for every second prefetch if the port width allows 32-bit accesses.
    if ((Flags & FirstPrefetch) || ((adr & 2) == 0) ) {
        unsigned idx = (adr & 0xfc) >> 2;

        CacheEntry& entry = cacheEntries[idx];
        uint32_t tag = (adr & ~0xff) | s;

        if ((cacr & 1) && entry.valid && entry.tag == tag) {
            cahr = entry.data;

        } else {
            uint32_t addrAligned = adr & ~3; // full prefetch from odd word wastes a BUS cycle in case of 16 bit port size

            SYNC(2);
            switch (PORT_SIZE(addrAligned)) {
                default:
                case 4:
                    cahr = READ_LONG(addrAligned);
                    busCycles = 3;
                    break;
                case 2:
                    cahr = READ_WORD(addrAligned) << 16;
                    SYNC(3);
                    cahr |= READ_WORD(addrAligned + 2);
                    busCycles = 6;
                    break;
                case 1:
                    cahr = READ_BYTE(addrAligned) << 24;
                    SYNC(3);
                    cahr |= READ_BYTE(addrAligned + 1) << 16;
                    SYNC(3);
                    cahr |= READ_BYTE(addrAligned + 2) << 8;
                    SYNC(3);
                    cahr |= READ_BYTE(addrAligned + 3);
                    busCycles = 12;
                    break;
            }

            if ((cacr & 3) == 1) {
                entry.data = cahr;
                entry.tag = tag;
                entry.valid = true;
            }
            SYNC(1);
        }

        return adr & 2 ? cahr & 0xffff : (cahr >> 16) & 0xffff;
    }

    return cahr & 0xffff;
}

template<uint8_t Size> auto M68Core::peekInc(uint32_t& adr, DasmHandler& d) -> uint32_t {
    switch(Size) {
        case Long:
            return peekInc<Word>(adr, d) << 16 | peekInc<Word>(adr, d);
        default:
            adr +=2;
            return d.memSnap ? *d.memSnap++ : peek(adr);
    }
}

auto M68Core::peek(uint32_t adr) -> uint16_t {
    return PEEK_WORD(adr & 0xffffff);
}

template<M68Core::Engine E, uint8_t Size, unsigned Flags> auto M68Core::read(uint32_t adr) -> uint32_t {
    uint32_t result;

    if constexpr (Flags & SFC)
        fc = SF_SFC;
    else
        fc = (Flags & PRG) ? SF_PRG : SF_DATA;

    if constexpr (Flags & EXC)
        fc |= SF_EXC;

    if (control & WatchPoint) {
        if (watchPoints.check( adr, Size, true ))
            DEBUG_POINT_REACHED(WatchPoint, watchPoints, adr);
    }

    if constexpr (Size == Long) {
        // for "Long" SampleIPL is not handled here: e.g. unlink
        SYNC(2);
        if constexpr (E == Engine::M68020)
            return readLongDS<Flags>( adr );

        if constexpr (!!(Flags & Reverse)) { // only addx.l and subx.l
            result = READ_WORD(adr + 2);
            SYNC(4);
            result |= READ_WORD(adr) << 16;
        } else {
            result = READ_WORD(adr) << 16;
            SYNC(4);
            result |= READ_WORD(adr + 2);
        }
    } else {
        // analyzed from fx68k, the sample point happens a half micro cycle (= one clock cycle) before.
        // for simplicity and performance reasons we check it now. the emulator always adds full micro cycles.
        // this way periphery can distinguish between first and second clock cycle within last micro cycle and
        // delay interrupts if happened in second clock cycle.
        if constexpr(Flags & SampleIPL)
            sampleInterrupt();

        SYNC(2);

        if constexpr (Size == Byte) {
            result = READ_BYTE(adr);
            if constexpr (E == Engine::M68020) {
                SYNC(1);
                return result;
            }

        } else { // Word
            if constexpr (E == Engine::M68020)
                return readWordDS<Flags>( adr );

            result = READ_WORD(adr);
        }
    }
    SYNC(2);
    return result;
}

template<unsigned Flags> auto M68Core::readWordDS(uint32_t adr) -> uint16_t {
    uint16_t result;
    auto ps = PORT_SIZE(adr);

    if ( ((ps == 2) && (adr & 1)) || ((ps == 4) && ((adr & 3) == 3)) || (ps == 1) ) {
        result = READ_BYTE(adr) << 8;
        SYNC(3);
        if constexpr(Flags & SampleIPL) sampleInterrupt();
        result |= READ_BYTE(adr + 1);
    } else
        result = READ_WORD(adr);

    SYNC(1);
    return result;
}

template<unsigned Flags> auto M68Core::readLongDS(uint32_t adr) -> uint32_t {
    uint32_t result;
    auto ps = PORT_SIZE(adr);

    if (ps == 4) {
        if ((adr & 3) == 0) {
            result = READ_LONG(adr);
        } else if ((adr & 3) == 2) {
            result = READ_WORD(adr) << 16;
            SYNC(3);
            result |= READ_WORD(adr + 2);
        } else if ((adr & 3) == 1) {
            result = READ_LONG(adr & ~1) << 8;
            SYNC(3);
            result |= READ_BYTE(adr + 3);
        } else {
            result = READ_BYTE(adr) << 24;
            SYNC(3);
            result |= READ_LONG(adr + 1) >> 8;
        }
    } else if (ps == 2) {
        if (adr & 1) {
            result = READ_BYTE(adr) << 24;
            SYNC(3);
            result |= READ_WORD(adr + 1) << 8;
            SYNC(3);
            result |= READ_BYTE(adr + 3);
        } else {
            result = READ_WORD(adr) << 16;
            SYNC(3);
            result |= READ_WORD(adr + 2);
        }
    } else {
        result = READ_BYTE(adr) << 24;
        SYNC(3);
        result |= READ_BYTE(adr + 1) << 16;
        SYNC(3);
        result |= READ_BYTE(adr + 2) << 8;
        SYNC(3);
        result |= READ_BYTE(adr + 3);
    }

    SYNC(1);
    return result;
}

template<M68Core::Engine E, uint8_t Size, unsigned Flags> auto M68Core::write(uint32_t adr, uint32_t data) -> void {
    if constexpr (Flags & DFC)
        fc = SF_DFC;
    else
        fc = (Flags & PRG) ? SF_PRG : SF_DATA;

    if constexpr (Flags & EXC)
        fc |= SF_EXC;

    if (control & (WatchPointWrite | ModifiedCode) ) {
        modifiedCode.checkAndSet( adr, Size );

        if (control & WatchPointWrite) {
            if (watchPointsWrite.check( adr, Size, true ))
                DEBUG_POINT_REACHED(WatchPointWrite, watchPointsWrite, adr);
        }
    }

    if constexpr (Size == Long) {
        SYNC(2);
        if constexpr (E == Engine::M68020) {
            writeLongDS<Flags>( adr, data );
            return;
        }

        if (Flags & Reverse) {
            WRITE_WORD(adr, data >> 16);
            SYNC(2);

            if constexpr(Flags & SampleIPL) // move
                sampleInterrupt();

            SYNC(2);
            WRITE_WORD(adr + 2, data & 0xffff);
        } else {
            WRITE_WORD(adr + 2, data & 0xffff);
            SYNC(4);
            WRITE_WORD(adr, data >> 16);
        }
    } else {
        if constexpr(Flags & SampleIPL) // move
            sampleInterrupt();

        SYNC(2);
        if constexpr (Size == Byte) {
            WRITE_BYTE(adr, data);

            if constexpr (E == Engine::M68020) {
                SYNC(1);
                return;
            }
        } else {
            if constexpr (E == Engine::M68020) {
                writeWordDS<Flags>(adr, data);
                SYNC(1);
                return;
            }

            WRITE_WORD(adr, data);
        }
    }
    SYNC(2);
}

template<unsigned Flags> auto M68Core::writeWordDS(uint32_t adr, uint16_t data) -> void {
    auto ps = PORT_SIZE(adr);

    if ( ((ps == 2) && (adr & 1)) || ((ps == 4) && ((adr & 3) == 3)) || (ps == 1) ) {
        WRITE_BYTE(adr, data >> 8);
        SYNC(3);
        if constexpr(Flags & SampleIPL) sampleInterrupt();
        WRITE_BYTE(adr + 1, data & 0xff);
    } else
        WRITE_WORD(adr, data);

    SYNC(1);
}

template<unsigned Flags> auto M68Core::writeLongDS(uint32_t adr, uint32_t data) -> void {
    uint32_t result;
    auto ps = PORT_SIZE(adr);

    if (ps == 4) {
        if ((adr & 3) == 0) {
            WRITE_LONG(adr, data);
        } else if ((adr & 3) == 2) {
            WRITE_WORD(adr, data >> 16);
            SYNC(3);
            WRITE_WORD(adr + 2, data);
        } else if ((adr & 3) == 1) {
            // CPU writes 3 bytes in one BUS cycle, can Amiga handle this directly ?
            WRITE_LONG(adr, data);
            SYNC(3);
            WRITE_BYTE(adr + 3, data);
        } else {
            WRITE_BYTE(adr, data >> 24);
            SYNC(3);
            // CPU writes 3 bytes in one BUS cycle, can Amiga handle this directly ?
            WRITE_LONG(adr, data);
        }
    } else if (ps == 2) {
        if (adr & 1) {
            WRITE_BYTE(adr, data >> 24);
            SYNC(3);
            WRITE_WORD(adr + 1, data >> 8);
            SYNC(3);
            WRITE_BYTE(adr + 3, data);
        } else {
            WRITE_WORD(adr, data >> 16);
            SYNC(3);
            WRITE_WORD(adr + 2, data);
        }
    } else {
        WRITE_BYTE(adr, data >> 24);
        SYNC(3);
        WRITE_BYTE(adr + 1, data >> 16);
        SYNC(3);
        WRITE_BYTE(adr + 2, data >> 8);
        SYNC(3);
        WRITE_BYTE(adr + 3, data);
    }

    SYNC(1);
    if constexpr(Flags & SampleIPL) sampleInterrupt();
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size, unsigned Flags> auto M68Core::readEA(int reg, uint32_t& ea) -> uint32_t {
    ea = calcEA<E, Mode, Size, Flags>(reg);
    
    if constexpr (Mode == DataRegisterDirect)
        return readRegD<Size>( reg );
    if constexpr (Mode == AddressRegisterDirect)
        return readRegA<Size>( reg );
    if constexpr (Mode == Immediate)
        return ea;

    if constexpr (Mode == AddressRegisterIndirectWithPreDecrement)
        writeRegA(reg, ea);
    
    if (E < Engine::M68020 && misaligned<Size>(ea))
        addressExceptionEA<E, Mode, Size>(ea);

    if constexpr (Mode == AddressRegisterIndirectWithPostIncrement)
        writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));

    return read<E, Size, Flags | FC_EA_PRG>(ea);
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size, unsigned Flags> auto M68Core::writeEA(uint32_t ea, uint32_t data) -> void {
    if constexpr(Mode == DataRegisterDirect)
        writeRegD<Size>(ea, data);
    else if constexpr(Mode == AddressRegisterDirect)
        writeRegA(ea, data);
    else
        write<E, Size, Flags>(ea, data);
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size, unsigned Flags> auto M68Core::calcEaUpdatePrePost(int reg) -> uint32_t {
    uint32_t ea = calcEA<E, Mode, Size>(reg);

    if constexpr (Mode == AddressRegisterIndirectWithPreDecrement)
        writeRegA(reg, ea);

    if constexpr (Mode == AddressRegisterIndirectWithPostIncrement)
        writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));

    return ea;
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size, unsigned Flags> auto M68Core::calcEA(int reg) -> uint32_t {
    uint32_t adr;
    
    switch(Mode) {
        case DataRegisterDirect:
        case AddressRegisterDirect:
            return reg;
            
        case AddressRegisterIndirect:
            return readRegA( reg );
            
        case AddressRegisterIndirectWithPostIncrement:
            return readRegA( reg );
            
        case AddressRegisterIndirectWithPreDecrement:
            if constexpr (E == Engine::M68000) {
                if constexpr((Flags & ConcurrentAdrCalc) == 0) SYNC(2);
            }
            adr = readRegA( reg );
            
            adr -= (Size == Byte && reg == 7) ? 2 : Size;
            return adr;
            
        case AddressRegisterIndirectWithDisplacement:
            adr = readRegA( reg ) + (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) {
                readExtensionWord<E>();
                if constexpr (E == Engine::M68020) {
                    if (!busCycles) SYNC(2);
                }
            } else incPc();

            return adr;
            
        case AddressRegisterIndirectWithIndex: {
            SYNC(2);
            if constexpr (E == Engine::M68020) {
                if (irc & 0x100)
                    adr = indirectIndexFull<E, Flags>( readRegA( reg ) );
                else
                    adr = indirectIndexBrief<E, Flags, true>( readRegA( reg )) ;
            } else
                adr = indirectIndexBrief<E, Flags>( readRegA( reg ) );

            return adr;
        }
        case AbsoluteShort:
            adr = (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
            else incPc();
            return adr;
            
        case AbsoluteLong:
            adr = irc << 16;
            if constexpr (E >= Engine::M68020 && (Flags & SkipExtension)) {
                adr |= stageB;
                incPc();
            } else {
                readExtensionWord<E>();
                adr |= irc;
            }
            if constexpr((Flags & (SkipExtension | DelayPrefetch) ) == 0) readExtensionWord<E>();
            if constexpr(Flags & SkipExtension) incPc();
            return adr;
            
        case ProgramCounterIndirectWithDisplacement:
            adr = pc + (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) {
                readExtensionWord<E>();
                if constexpr (E == Engine::M68020) {
                    if (!busCycles) SYNC(2);
                }
            } else incPc();

            return adr;
            
        case ProgramCounterIndirectWithIndex: {
            SYNC(2);
            if constexpr (E == Engine::M68020) {
                if (irc & 0x100)
                    adr = indirectIndexFull<E, Flags>( pc );
                else
                    adr = indirectIndexBrief<E, Flags, true>( pc );
            } else
                adr = indirectIndexBrief<E, Flags>( pc );

            return adr;
        }
        case Immediate:
            if constexpr(Size == Byte) {
                adr = irc & 0xff;
            } else if constexpr(Size == Word) {
                adr = irc;
            } else {
                adr = irc << 16;
                readExtensionWord<E>();
                if constexpr (E == Engine::M68020) {
                    if (!busCycles) SYNC(2);
                }
                adr |= irc;
            }
            readExtensionWord<E>();
            return adr;
    }
}

template <M68Core::Engine E, unsigned Flags, bool Scale> auto M68Core::indirectIndexBrief(uint32_t B) -> uint32_t {
    int index = (irc >> 12) & 7;
    uint32_t dispReg = (irc & 0x8000) ? readRegA( index ) : readRegD( index );
    auto d = static_cast<int8_t>(irc);
    auto xn = static_cast<int64_t>((irc & 0x800) ? dispReg : static_cast<int16_t>(dispReg));

    if constexpr (Scale) {
        int64_t scale = (irc >> 9) & 3;
        xn <<= scale;
    }
    
    if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
    else incPc();

    return static_cast<int64_t>(B) + xn + static_cast<int64_t>(d);
}

template <M68Core::Engine E, unsigned Flags> auto M68Core::indirectIndexFull(uint32_t B) -> uint32_t {
    uint32_t addr;
    uint32_t xn = 0;
    uint32_t baseD = 0;
    uint32_t outerD = 0;
    uint8_t iis = irc & 7;

    if (irc & 0x80)
        B = 0;

    if ((irc & 0x40) == 0) {
        int scale = (irc >> 9) & 3;
        int index = (irc >> 12) & 7;
        uint32_t dispReg = (irc & 0x8000) ? readRegA( index ) : readRegD( index );
        xn = ((irc & 0x800) ? dispReg : static_cast<int16_t>(dispReg)) << scale;
    }

    const int bdSize = (irc >> 4) & 3;
    const int odSize = irc & 3;

    if constexpr(!!(Flags & SkipExtension)) {
        int extWords = bdSize == 3 ? 2 : (bdSize == 2 ? 1 : 0);
        extWords += odSize == 3 ? 2 : (odSize == 2 ? 1 : 0);

        (extWords > 1) ? readExtensionWord<E>() : incPc();

        if (bdSize & 2)
            baseD = fetchDisplacementSkipped<E>(bdSize & 1, extWords);

        if (odSize & 2)
            outerD = fetchDisplacementSkipped<E>(odSize & 1, extWords);

    } else {
        readExtensionWord<E>();

        if (bdSize & 2)
            baseD = fetchDisplacement<E>(bdSize & 1);

        if (odSize & 2)
            outerD = fetchDisplacement<E>(odSize & 1);
    }

    if (iis & 4)
        addr = read<E, Long, Flags>(B + baseD) + xn + outerD;
    else if (iis & 3)
        addr = read<E, Long, Flags>(B + baseD + xn) + outerD;
    else
        addr = B + baseD + xn;

    return addr;
}

template <M68Core::Engine E> auto M68Core::fetchDisplacement(bool longDis) -> uint32_t {
    uint32_t dis = irc;
    readExtensionWord<E>();

    if (longDis) {
        SYNC(4);
        dis = dis << 16 | irc;
        readExtensionWord<E>();
    } else {
        SYNC(2);
        dis = sign<Word>(static_cast<uint16_t>(dis));
    }

    return dis;
}

template <M68Core::Engine E> auto M68Core::fetchDisplacementSkipped(bool longDis, int& extWords) -> uint32_t {
    uint32_t dis;
    if (extWords > 2) {
        dis = irc;
        readExtensionWord<E>();
    } else {
        dis = extWords == 2 ? irc : stageB;
        incPc();
    }
    extWords--;

    if (longDis) {
        SYNC(4);

        if (extWords > 2) {
            dis = dis << 16 | irc;
            readExtensionWord<E>();
        } else {
            dis = dis = dis << 16 | (extWords == 2 ? irc : stageB);
            incPc();
        }
        extWords--;
    } else {
        SYNC(2);
        dis = sign<Word>(static_cast<uint16_t>(dis));
    }

    return dis;
}

template<uint8_t Mode, uint8_t Size> auto M68Core::prepareEaForDasm(uint32_t& adr, uint8_t reg, DasmHandler& d) -> void {
    d.mode = Mode;
    d.reg = reg;

    switch(Mode) {
        case ProgramCounterIndirectWithIndex:
            d.pc = adr;
        case AddressRegisterIndirectWithIndex:
            d.ext = peekInc<Word>(adr, d);
            d.ext2 = 0;
            d.ext3 = 0;

            if (d.ext & 0x100) {
                uint8_t bdSize = (d.ext >> 4) & 3;
                uint8_t odSize = d.ext & 3;

                if (bdSize & 2)
                    d.ext2 = bdSize & 1 ? peekInc<Long>(adr, d) : static_cast<int16_t>(peekInc<Word>(adr, d));
                if (odSize & 2)
                    d.ext3 = odSize & 1 ? peekInc<Long>(adr, d) : static_cast<int16_t>(peekInc<Word>(adr, d));
            }
            break;

        case ProgramCounterIndirectWithDisplacement:
            d.pc = adr;
        case AddressRegisterIndirectWithDisplacement:
        case AbsoluteShort:
            d.ext = peekInc<Word>(adr, d);
            break;

        case AbsoluteLong:
            d.ext = peekInc<Long>(adr, d);
            break;

        case Immediate:
            d.ext = clip<Size>(peekInc<Size>(adr, d));
            break;

        default:
            break;
    }
}

}
