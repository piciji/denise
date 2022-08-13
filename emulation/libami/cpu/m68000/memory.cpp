
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

template<uint8_t Size> auto M68000::readRegD(uint8_t reg) -> uint32_t {
    return clip<Size>( regsD[reg] );
}

template<uint8_t Size> auto M68000::writeRegD(uint8_t reg, uint32_t data) -> void {
    regsD[reg] = (regsD[reg] & inverseMask<Size>()) | (data & mask<Size>());
}

template<uint8_t Size> auto M68000::readRegA(uint8_t reg) -> uint32_t {
    return clip<Size>( regsA[reg] );
}

auto M68000::writeRegA(uint8_t reg, uint32_t data) -> void {
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

template<uint8_t Size, uint8_t Flags> auto M68000::read(uint32_t adr) -> uint32_t {
    uint32_t result;

    if constexpr (Size == Long) {
        sync(2);
        if constexpr (Flags & PRG) {
            result = readWordPRG(adr & 0xffffff) << 16;
            sync(4);
            result |= readWordPRG((adr + 2) & 0xffffff);
        } else if constexpr(Flags & Reverse) { // only addx.l and subx.l
            result = readWord((adr + 2) & 0xffffff);
            sync(4);
            result |= readWord(adr & 0xffffff) << 16;
        } else {
            result = readWord(adr & 0xffffff) << 16;
            // only "unlink" fetches IPL here. to prevent splitting in two sync(2), we do a two word access.
            sync(4);
            result |= readWord((adr + 2) & 0xffffff);
        }
    } else {
        // analyzed from fx68k, the sample point happens a half micro cycle (= one clock cycle) before.
        // for simplicity and performance reasons we check it now. the emulator always adds full micro cycles.
        // this way periphery can distinguish between first and second clock cycle within last micro cycle and
        // delay interrupts if happened in second clock cycle.
        if constexpr(Flags & SampleIPL)
            sampleInterrupt();

        sync(2);

        if constexpr(Flags & TasCycle)
            tasCycleBegin();

        if constexpr (Size == Byte) {
            if constexpr (Flags & PRG)  result = readBytePRG(adr & 0xffffff);
            else                        result = readByte(adr & 0xffffff);
        } else {
            if constexpr (Flags & PRG)  result = readWordPRG(adr & 0xffffff);
            else                        result = readWord(adr & 0xffffff);
        }
    }
    sync(2);
    return result;
}

template<uint8_t Size, uint8_t Flags> auto M68000::write(uint32_t adr, uint32_t data) -> void {

    if constexpr (Size == Long) {
        sync(2);
        if (Flags & Reverse) {
            writeWord(adr & 0xffffff, data >> 16);
            sync(2);

            if constexpr(Flags & SampleIPL) // move
                sampleInterrupt();

            sync(2);
            writeWord((adr + 2) & 0xffffff, data & 0xffff);
        } else {
            writeWord((adr + 2) & 0xffffff, data & 0xffff);
            sync(4);
            writeWord(adr & 0xffffff, data >> 16);
        }
    } else {
        if constexpr(Flags & SampleIPL) // move
            sampleInterrupt();

        sync(2);
        if constexpr (Size == Byte)
            writeByte(adr & 0xffffff, data);
        else
            writeWord(adr & 0xffffff, data);
    }
    sync(2);
}

template<uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68000::readEA(uint8_t reg, uint32_t& result, uint32_t& ea) -> bool {
    ea = calcEA<Size, Mode, Flags>(reg);
    
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
        writeRegA(reg, ea + (reg == 7 && Size == Byte) ? 2 : Size);

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

template<uint8_t Mode, uint8_t Size, uint8_t Flags> auto M68000::calcEA(uint8_t reg) -> uint32_t {
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
            if constexpr((Flags & ConcurrentAdrCalc) == 0) sync(2);
            adr = readRegA( reg );
            
            adr -= (Size == Byte && reg == 7) ? 2 : Size;
            return adr;
            
        case AddressRegisterIndirectWithDisplacement:
            adr = readRegA( reg ) + (int16_t)irc;
            if constexpr((Flags & SkipExtension) == 0) readExtensionWord();
            return adr;
            
        case AddressRegisterIndirectWithIndex: {
            sync(2);
            uint32_t dispReg = readRegD( (irc & 0x7000) >> 12 );
            adr = readRegA( reg ) + (!(irc & 0x800) ? (int16_t)dispReg : dispReg) + (int8_t)(irc & 0xff);

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
            sync(2);
            uint32_t dispReg = readRegD( (irc & 0x7000) >> 12 );
            adr = pc + (!(irc & 0x800) ? (int16_t)dispReg : dispReg) + (int8_t)(irc & 0xff);

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

}
