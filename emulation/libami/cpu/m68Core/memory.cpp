
#include "m68Core.h"

#ifdef FC_SUPPORT
#define FC_EA_PRG (isPcMode<Mode>() ? PRG : 0)
#define OR_FC_EA_PRG | FC_EA_PRG
#define FC_PRG PRG
#define OR_FC_PRG | PRG
#else
#define FC_EA_PRG 0
#define OR_FC_EA_PRG
#define FC_PRG 0
#define OR_FC_PRG
#endif

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

template<M68Core::Engine E, uint8_t Flags> auto M68Core::fullPrefetch() -> void {
    firstPrefetch<E>();
    prefetch<E, Flags>();
}

template<M68Core::Engine E> auto M68Core::firstPrefetch() -> void {
    if (E == Engine::M68020)
        irc = cachedPrefetch<FirstPrefetch OR_FC_PRG >( pc );
    else
        irc = read<Word, FC_PRG>(pc);
}

template<M68Core::Engine E, uint8_t Flags> auto M68Core::prefetch() -> void {
    ird = irc;
    pc += 2;

    if (E == Engine::M68020)
        irc = cachedPrefetch<Flags OR_FC_PRG>( pc );
    else
        irc = read<Word, Flags OR_FC_PRG>(pc);
}

template<M68Core::Engine E, uint8_t Flags> auto M68Core::readExtensionWord() -> void {
    pc += 2;

    if (E == Engine::M68020)
        irc = cachedPrefetch<Flags OR_FC_PRG>( pc );
    else
        irc = read<Word, Flags OR_FC_PRG>(pc);
}

template<uint8_t Flags> auto M68Core::cachedPrefetch(uint32_t adr) -> uint16_t {
    if (control & WatchPoint) {
        if (watchPoints.check( pc, Word, true ))
            DEBUG_POINT_REACHED(WatchPoint, watchPoints, pc);
    }

    if constexpr(Flags & SampleIPL)
        sampleInterrupt();

    SYNC(2);

    // full prefetch invalidates cache holding register
    if ((Flags & FirstPrefetch) || ((adr & 2) == 0) ) {
        unsigned idx = (adr & 0xfc) >> 2;

        CacheEntry& entry = cacheEntries[idx];
        uint32_t tag = (adr & ~0xff) | s;

        if ((cacr & 1) && entry.valid && entry.tag == tag) {
            cahr = entry.data;

        } else {
            uint32_t addrAligned = adr & ~3; // full prefetch from odd word wastes a BUS cycle in case of 16 bit port size

            switch (PORT_SIZE(addrAligned)) {
                default:
                case 4:
                    cahr = READ_LONG_PRG(addrAligned); break;
                case 2:
                    cahr = READ_WORD_PRG(addrAligned) << 16;
                    SYNC(3);
                    cahr |= READ_WORD_PRG(addrAligned + 2);
                    break;
                case 1:
                    cahr = READ_BYTE_PRG(addrAligned) << 24;
                    SYNC(3);
                    cahr |= READ_BYTE_PRG(addrAligned + 1) << 16;
                    SYNC(3);
                    cahr |= READ_BYTE_PRG(addrAligned + 2) << 8;
                    SYNC(3);
                    cahr |= READ_BYTE_PRG(addrAligned + 3);
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

template<uint8_t Size, uint8_t Flags> auto M68Core::read(uint32_t adr) -> uint32_t {
    uint32_t result;

    if (control & WatchPoint) {
        if (watchPoints.check( adr, Size, true ))
            DEBUG_POINT_REACHED(WatchPoint, watchPoints, adr);
    }

    if constexpr (Size == Long) {
        SYNC(2);
        if constexpr (!!(Flags & PRG)) {
            result = READ_WORD_PRG(adr & 0xffffff) << 16;
            SYNC(4);
            result |= READ_WORD_PRG((adr + 2) & 0xffffff);
        } else if constexpr (!!(Flags & Reverse)) { // only addx.l and subx.l
            result = READ_WORD((adr + 2) & 0xffffff);
            SYNC(4);
            result |= READ_WORD(adr & 0xffffff) << 16;
        } else {
            result = READ_WORD(adr & 0xffffff) << 16;
            // only "unlink" fetches IPL here. to prevent splitting in two SYNC(2), we do a two word access.
            SYNC(4);
            result |= READ_WORD((adr + 2) & 0xffffff);
        }
    } else {
        // analyzed from fx68k, the sample point happens a half micro cycle (= one clock cycle) before.
        // for simplicity and performance reasons we check it now. the emulator always adds full micro cycles.
        // this way periphery can distinguish between first and second clock cycle within last micro cycle and
        // delay interrupts if happened in second clock cycle.
        if constexpr(Flags & SampleIPL)
            sampleInterrupt();

        SYNC(2);

        #ifdef TAS_SPINLOCK
        if constexpr(Flags & TasCycle)
            TAS_CYCLE_BEGIN();
        #endif
        if constexpr (Size == Byte) {
            if constexpr (!!(Flags & PRG))  result = READ_BYTE_PRG(adr & 0xffffff);
            else                        result = READ_BYTE(adr & 0xffffff);
        } else {
            if constexpr (!!(Flags & PRG))  result = READ_WORD_PRG(adr & 0xffffff);
            else                        result = READ_WORD(adr & 0xffffff);
        }
    }
    SYNC(2);
    return result;
}

template<uint8_t Size, uint8_t Flags> auto M68Core::write(uint32_t adr, uint32_t data) -> void {

    if (control & (WatchPointWrite | ModifiedCode) ) {
        modifiedCode.checkAndSet( adr, Size );

        if (control & WatchPointWrite) {
            if (watchPointsWrite.check( adr, Size, true ))
                DEBUG_POINT_REACHED(WatchPointWrite, watchPointsWrite, adr);
        }
    }

    if constexpr (Size == Long) {
        SYNC(2);
        if (Flags & Reverse) {
            WRITE_WORD(adr & 0xffffff, data >> 16);
            SYNC(2);

            if constexpr(Flags & SampleIPL) // move
                sampleInterrupt();

            SYNC(2);
            WRITE_WORD((adr + 2) & 0xffffff, data & 0xffff);
        } else {
            WRITE_WORD((adr + 2) & 0xffffff, data & 0xffff);
            SYNC(4);
            WRITE_WORD(adr & 0xffffff, data >> 16);
        }
    } else {
        if constexpr(Flags & SampleIPL) // move
            sampleInterrupt();

        SYNC(2);
        if constexpr (Size == Byte)
            WRITE_BYTE(adr & 0xffffff, data);
        else
            WRITE_WORD(adr & 0xffffff, data);
    }
    SYNC(2);
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68Core::readEA(int reg, uint32_t& result, uint32_t& ea) -> bool {
    ea = calcEA<E, Mode, Size, Flags>(reg);
    
    if constexpr (Mode == DataRegisterDirect) {
        result = readRegD<Size>( reg );
        return true;
    } if constexpr (Mode == AddressRegisterDirect) {
        result = readRegA<Size>( reg );
        return true;
    } if constexpr (Mode == Immediate) {
        result = ea;
        return true;
    }

    if constexpr (Mode == AddressRegisterIndirectWithPreDecrement)
        writeRegA(reg, ea);
    
    if (misaligned<Size>(ea))
        return addressExceptionEA<E, Mode, Size>(ea), false;

    if constexpr (Mode == AddressRegisterIndirectWithPostIncrement)
        writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));

    return result = read<Size, Flags OR_FC_EA_PRG>(ea), true;
}

template<uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68Core::writeEA(uint32_t ea, uint32_t data) -> void {
    if constexpr(Mode == DataRegisterDirect)
        writeRegD<Size>(ea, data);
    else if constexpr(Mode == AddressRegisterDirect)
        writeRegA(ea, data);
    else
        write<Size, Flags>(ea, data);
}

template<M68Core::Engine E, uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68Core::calcEA(int reg) -> uint32_t {
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
            if constexpr((Flags & ConcurrentAdrCalc) == 0) SYNC(2);
            adr = readRegA( reg );
            
            adr -= (Size == Byte && reg == 7) ? 2 : Size;
            return adr;
            
        case AddressRegisterIndirectWithDisplacement:
            adr = readRegA( reg ) + (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
            return adr;
            
        case AddressRegisterIndirectWithIndex: {
            SYNC(2);

            int index = (irc >> 12) & 7;
            uint32_t dispReg = (irc & 0x8000) ? readRegA( index ) : readRegD( index );
            uint32_t an = readRegA( reg );
            int8_t d = (int8_t)irc;

            adr = (int64_t)an + (int64_t)((irc & 0x800) ? dispReg : (int16_t)dispReg) + (int64_t)d;

            if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
            return adr;
        }
        case AbsoluteShort:
            adr = (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
            return adr;
            
        case AbsoluteLong:
            adr = irc << 16;
            readExtensionWord<E>();
            adr |= irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
            return adr;
            
        case ProgramCounterIndirectWithDisplacement:
            adr = pc + (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
            return adr;
            
        case ProgramCounterIndirectWithIndex: {
            SYNC(2);

            int index = (irc >> 12) & 7;
            uint32_t dispReg = (irc & 0x8000) ? readRegA( index ) : readRegD( index );
            int8_t d = (int8_t)irc;

            adr = (int64_t)pc + (int64_t)((irc & 0x800) ? dispReg : (int16_t)dispReg) + (int64_t)d;

            if constexpr((Flags & SkipExtension) == 0) readExtensionWord<E>();
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
                adr |= irc;
            }
            readExtensionWord<E>();
            return adr;
    }
}

template<uint8_t Mode, uint8_t Size> auto M68Core::prepareEaForDasm(uint32_t& adr, uint8_t reg, DasmHandler& d) -> void {
    d.mode = Mode;
    d.reg = reg;

    switch(Mode) {
        case ProgramCounterIndirectWithIndex:
        case ProgramCounterIndirectWithDisplacement:
            d.pc = adr;
        case AddressRegisterIndirectWithDisplacement:
        case AbsoluteShort:
        case AddressRegisterIndirectWithIndex:
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
