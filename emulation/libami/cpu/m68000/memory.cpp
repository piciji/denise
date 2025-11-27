
#include "m68000.h"

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

template<uint8_t Size> auto M68000::clip(uint32_t data) -> uint32_t {
    if constexpr (Size == Byte) return data & 0xff;
    if constexpr (Size == Word) return data & 0xffff;
    if constexpr (Size == Long) return data & 0xffffffff;
}

template<uint8_t Size> constexpr auto M68000::mask() -> uint32_t {
    if constexpr (Size == Byte) return 0xff;
    if constexpr (Size == Word) return 0xffff;
    if constexpr (Size == Long) return 0xffffffff;
}

template<uint8_t Size> constexpr auto M68000::inverseMask() -> uint32_t {
    if constexpr (Size == Byte) return 0xffffff00;
    if constexpr (Size == Word) return 0xffff0000;
    if constexpr (Size == Long) return 0;
}

template<uint8_t Size> constexpr auto M68000::msb() -> uint32_t {
    if constexpr (Size == Byte) return 0x80;
    if constexpr (Size == Word) return 0x8000;
    if constexpr (Size == Long) return 0x80000000;
}

template<uint8_t Size> auto M68000::negative(uint32_t data) -> bool {
    if constexpr (Size == Byte) return (data & 0x80) != 0;
    if constexpr (Size == Word) return (data & 0x8000) != 0;
    if constexpr (Size == Long) return (data & 0x80000000) != 0;
}

template<uint8_t Size> auto M68000::carry(uint64_t data) -> bool {
    if constexpr (Size == Byte) return (data & 0x100) != 0;
    if constexpr (Size == Word) return (data & 0x10000) != 0;
    if constexpr (Size == Long) return (data & 0x100000000) != 0;
}

template<uint8_t Size> auto M68000::sign(uint32_t data) -> int32_t {
    if constexpr (Size == Byte) return (int8_t)data;
    if constexpr (Size == Word) return (int16_t)data;
    if constexpr (Size == Long) return (int32_t)data;
}

template<uint8_t Size> constexpr auto M68000::bits() -> uint8_t {
    if constexpr (Size == Byte) return 8;
    if constexpr (Size == Word) return 16;
    if constexpr (Size == Long) return 32;
}

template<uint8_t Size> auto M68000::readRegD(int reg) -> uint32_t {
    return clip<Size>( regsD[reg] );
}

template<uint8_t Size> auto M68000::writeRegD(int reg, uint32_t data) -> void {
    regsD[reg] = (regsD[reg] & inverseMask<Size>()) | (data & mask<Size>());
}

template<uint8_t Size> auto M68000::readRegA(int reg) -> uint32_t {
    return clip<Size>( regsA[reg] );
}

auto M68000::writeRegA(int reg, uint32_t data) -> void {
    regsA[reg] = data;
}

template<uint8_t Flags> auto M68000::fullPrefetch() -> void {
    firstPrefetch();
    prefetch<Flags>();
}

auto M68000::firstPrefetch() -> void {
    irc = read<Word, FC_PRG>(pc);
}

template<uint8_t Flags> auto M68000::prefetch() -> void {
    ird = irc;
    pc += 2;
    irc = read<Word, Flags OR_FC_PRG>(pc);
}

template<uint8_t Flags> auto M68000::readExtensionWord() -> void {
    pc += 2;
    irc = read<Word, Flags OR_FC_PRG>(pc);
}

template<uint8_t Size> auto M68000::peekInc(uint32_t& adr, DasmHandler& d) -> uint32_t {
    switch(Size) {
        case Long:
            return peekInc<Word>(adr, d) << 16 | peekInc<Word>(adr, d);
        default:
            adr +=2;
            return d.memSnap ? *d.memSnap++ : peek(adr);
    }
}

auto M68000::peek(uint32_t adr) -> uint16_t {
    return PEEK_WORD(adr & 0xffffff);
}

template<uint8_t Size, uint8_t Flags> auto M68000::read(uint32_t adr) -> uint32_t {
    uint32_t result;

    if ((control & WatchPoint) && watchPoints.check( adr, Size )) {
        DEBUG_POINT_REACHED(DebuggerAction::Watchpoint, adr);
    }

    if constexpr (Size == Long) {
        SYNC(2);
        if constexpr (Flags & PRG) {
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
            if constexpr (Flags & PRG)  result = READ_BYTE_PRG(adr & 0xffffff);
            else                        result = READ_BYTE(adr & 0xffffff);
        } else {
            if constexpr (Flags & PRG)  result = READ_WORD_PRG(adr & 0xffffff);
            else                        result = READ_WORD(adr & 0xffffff);
        }
    }
    SYNC(2);
    return result;
}

template<uint8_t Size, uint8_t Flags> auto M68000::write(uint32_t adr, uint32_t data) -> void {

    if (control & (WatchPoint | ModifiedCode) ) {
        modifiedCode.checkAndSet( adr, Size );

        if ((control & WatchPoint) && watchPoints.check( adr, Size ))
            DEBUG_POINT_REACHED(DebuggerAction::Watchpoint, adr);
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

template<uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68000::readEA(int reg, uint32_t& result, uint32_t& ea) -> bool {
    ea = calcEA<Mode, Size, Flags>(reg);
    
    if (Mode == DataRegisterDirect)
        return result = readRegD<Size>( reg ), true;
    else if (Mode == AddressRegisterDirect)
        return result = readRegA<Size>( reg ), true;
    else if (Mode == Immediate)
        return result = ea, true;

    if constexpr (Mode == AddressRegisterIndirectWithPreDecrement)
        writeRegA(reg, ea);
    
    if (misaligned<Size>(ea))
        return addressExceptionEA<Mode, Size>(ea), false;

    if constexpr (Mode == AddressRegisterIndirectWithPostIncrement)
        writeRegA(reg, ea + ((reg == 7 && Size == Byte) ? 2 : Size));

    return result = read<Size, Flags OR_FC_EA_PRG>(ea), true;
}

template<uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68000::writeEA(uint32_t ea, uint32_t data) -> void {
    if constexpr(Mode == DataRegisterDirect)
        writeRegD<Size>(ea, data);
    else if constexpr(Mode == AddressRegisterDirect)
        writeRegA(ea, data);
    else
        write<Size, Flags>(ea, data);
}

template<uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68000::calcEA(int reg) -> uint32_t {
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
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord();
            return adr;
            
        case AddressRegisterIndirectWithIndex: {
            SYNC(2);

            int index = (irc >> 12) & 7;
            uint32_t dispReg = (irc & 0x8000) ? readRegA( index ) : readRegD( index );
            uint32_t an = readRegA( reg );
            int8_t d = (int8_t)irc;

            adr = (int64_t)an + (int64_t)((irc & 0x800) ? dispReg : (int16_t)dispReg) + (int64_t)d;

            if constexpr((Flags & SkipExtension) == 0) readExtensionWord();
            return adr;
        }
        case AbsoluteShort:
            adr = (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord();
            return adr;
            
        case AbsoluteLong:
            adr = irc << 16;
            readExtensionWord();
            adr |= irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord();
            return adr;
            
        case ProgramCounterIndirectWithDisplacement:
            adr = pc + (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord();
            return adr;
            
        case ProgramCounterIndirectWithIndex: {
            SYNC(2);

            int index = (irc >> 12) & 7;
            uint32_t dispReg = (irc & 0x8000) ? readRegA( index ) : readRegD( index );
            int8_t d = (int8_t)irc;

            adr = (int64_t)pc + (int64_t)((irc & 0x800) ? dispReg : (int16_t)dispReg) + (int64_t)d;

            if constexpr((Flags & SkipExtension) == 0) readExtensionWord();
            return adr;
        }
        case Immediate:
            if constexpr(Size == Byte) {
                adr = irc & 0xff;
            } else if constexpr(Size == Word) {
                adr = irc;
            } else {
                adr = irc << 16;
                readExtensionWord();
                adr |= irc;
            }
            readExtensionWord();
            return adr;
    }
}

template<uint8_t Mode, uint8_t Size> auto M68000::prepareEaForDasm(uint32_t& adr, uint8_t reg, DasmHandler& d) -> void {
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
