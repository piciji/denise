
#include "system.h"
#include "../cpu/m68000.h"
#include "../input/input.h"
#include "../../tools/sanitizer.h"
#include  "../../tools/macros.h"

namespace LIBAMI {

System* system = nullptr;
Cia* cia1 = nullptr;
Cia* cia2 = nullptr;
Emulator::SystemTimer sysTimerCia;

System::System(Interface* interface) {
    this->interface = interface;

    cia1 = new Cia( 1 );
    cia2 = new Cia( 2 );
    cpu = new M68000;
    input = new Input;

    cia1->serialOut = [this](bool bit) {


    };


    cia1->readPort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( port == Cia::PORTA )
            return (uint8_t)(input->readCiaPortA( ) & lines->ioa);

        return (uint8_t)0xff;
    };

    cia1->writePort = [this]( Cia::Port port, Cia::Lines* lines ) {

        if ( port == Cia::PORTA ) {
         //   if (lines->ioa != lines->ioaOld)

        } else {
            //if (lines->iob != lines->iobOld)

        }
    };
}

auto System::power(bool softReset) -> void {
    sysTimerCia.clear();
    mapMemory();
    setOVL(true);

    if( !softReset ) {
        cpu->power();

    } else {
        cpu->reset();
    }

    cia1->reset();
    cia2->reset();
    eClockPosition = 2;

    powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
}

auto System::run() -> void {
    leaveEmulation = false;

    while( !leaveEmulation ) {
        cpu->process();
    }
}

auto System::mapMemory() -> void {
    uint8_t kickAssignment = kickRom ? KICK_ROM : Unmapped;

    for(unsigned i = 0; i <= 0x1f; i++) // max 2 MB (mirrored)
        mapper[i] = CHIP_MEM;

    for(unsigned i = 0x20; i <= 0x9f; i++)
        mapper[i] = Unmapped; // auto config (e.g. fast ram)

    for(unsigned i = 0xa0; i <= 0xbe; i++)
        mapper[i] = MMIO_CIA; // CIA mirror or overmap for Zorro 2 IO expansion

    mapper[0xbf] = MMIO_CIA;

    for(unsigned i = 0xc0; i <= 0xdb; i++)
        mapper[i] = MMIO_CUSTOM; // mirror

    if (slowMem) { // overmap slow mem (max. 1.75 MB, not mirrored)
        uint8_t page = slowMemSize / (64 * 1024);

        for(unsigned i = 0xc0; i < (0xc0 + page); i++)
            mapper[i] = SLOW_MEM;
    }

    mapper[0xdc] = useRTC ? MMIO_RTC : MMIO_CUSTOM;

    mapper[0xdd] = Unmapped;

    for (unsigned i = 0xde; i <= 0xdf; i++)
        mapper[i] = MMIO_CUSTOM;

    if (extRom) { // AROS
        for (unsigned i = 0xe0; i <= 0xe7; i++)
            mapper[i] = EXT_ROM;
    } else {
        for (unsigned i = 0xe0; i <= 0xe7; i++)
            mapper[i] = kickAssignment; // mirror
    }

    for(unsigned i = 0xe8; i <= 0xef; i++)
        mapper[i] = Unmapped; // auto config

    for(unsigned i = 0xf0; i <= 0xf7; i++)
        mapper[i] = Unmapped; // extended ROM CD32

    for (unsigned i = 0xf8; i <= 0xff; i++)
        mapper[i] = kickAssignment;
}

auto System::setOVL(bool state) -> void {
    if (state) {
        for (unsigned i = 0x0; i < 0x8; i++)
            mapper[i] = KICK_ROM;
    } else {
        for (unsigned i = 0x0; i < 0x8; i++)
            mapper[i] = CHIP_MEM;
    }
}

auto System::readMemory(uint32_t adr) -> uint8_t {
    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            dataBus = *(chipMem + (adr & chipMemMask));
            break;
        case MMIO_CUSTOM:
            break;
        case MMIO_CIA: {
            doDMA( cpu->internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                case 0x0000: dataBus = (adr & 1) ? cia1->read<MOS_8520>( reg ) : cia2->read<MOS_8520>( reg ); break;
                case 0x1000: dataBus = (adr & 1) ? (uint8_t)dataBus : cia2->read<MOS_8520>( reg ); break;
                case 0x2000: dataBus = (adr & 1) ? cia1->read<MOS_8520>( reg ) : (dataBus >> 8); break;
                case 0x3000: dataBus = (adr & 1) ? (uint8_t)dataBus : (dataBus >> 8); break;
            }
        } break;
        case SLOW_MEM:
            dataBus = *(slowMem + (adr - 0xc00000));
            break;
        case KICK_ROM:
            dataBus = *(kickRom + (adr & kickRomMask));
            break;
        case EXT_ROM:
            dataBus = *(extRom + (adr & extRomMask));
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    return (uint8_t)dataBus;
}

auto System::readMemory16(uint32_t adr) -> uint16_t {
    // 68k is big endian, modern architecture is little endian
    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            dataBus = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));
            break;
        case MMIO_CUSTOM:
            break;
        case MMIO_CIA: {
            // always leads to the time for the next CIA cycle, after which the register is accessed.
            doDMA( cpu->internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                case 0x0000: dataBus = cia1->read<MOS_8520>( reg ) | (cia2->read<MOS_8520>( reg ) << 8); break;
                case 0x1000: dataBus = (dataBus >> 8) | (cia2->read<MOS_8520>( reg ) << 8); break;
                case 0x2000: dataBus = cia1->read<MOS_8520>( reg ) | (dataBus << 8); break;
                // case 0x3000: break; get last BUS value, should be IRC in most cases
                // todo: check for 0x3000 if Agnus asserts VPA (E-Clock cycle) too
            }
        } break;
        case SLOW_MEM:
            dataBus = _swapWord(*(uint16_t*)(slowMem + (adr - 0xc00000)));
            break;
        case KICK_ROM:
            dataBus = _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask)));
            break;
        case EXT_ROM:
            dataBus = _swapWord(*(uint16_t*)(extRom + (adr & extRomMask)));
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    return dataBus;
}

auto System::writeMemory(uint32_t adr, uint8_t value) -> void {
    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            dataBus = *(chipMem + (adr & chipMemMask)) = value;
            break;
        case MMIO_CUSTOM:
            break;
        case MMIO_CIA: {
            doDMA( cpu->internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1->write<MOS_8520>( reg, value );
            if ((adr & 0x2000) == 0)
                cia2->write<MOS_8520>( reg, value );
        } break;
        case SLOW_MEM:
            dataBus = *(slowMem + (adr - 0xc00000)) = value;
            break;
        case KICK_ROM:
            break;
        case EXT_ROM:
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    dataBus = value;
}

auto System::writeMemory16(uint32_t adr, uint16_t value) -> void {
    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);
            break;
        case MMIO_CUSTOM:
            break;
        case MMIO_CIA: {
            doDMA( cpu->internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1->write<MOS_8520>( reg, (uint8_t)value );
            if ((adr & 0x2000) == 0)
                cia2->write<MOS_8520>( reg, (uint8_t)(value >> 8) );
        } break;
        case SLOW_MEM:
            *(uint16_t*)(slowMem + (adr - 0xc00000)) = _swapWord(value);
            break;
        case KICK_ROM:
        case EXT_ROM:
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    dataBus = value;
}

auto System::setMemory(unsigned typeId, unsigned size) -> void {
    switch (typeId) {
        case 0: // Chip mem
        default: {
            if (size == 0)
                size = 512 * 1024;
            else if (size > (2 * 1024 * 1024))
                size = 2 * 1024 * 1024;
            else
                size = Emulator::powerOfTwo(size);

            unsigned mask = size - 1;

            if (mask == chipMemMask)
                break;

            if (chipMem)
                delete[] chipMem;

            chipMem = new uint8_t[size];
            chipMemMask = mask;
        } break;

        case 1: { // slow mem
            if (size > (1792 * 1024))
                size = 1792 * 1024;

            if (size == slowMemSize)
                break;

            if (slowMem)
                delete[] slowMem;

            slowMem = nullptr;
            if (size)
                slowMem = new uint8_t[size];
            slowMemSize = size;
        } break;
    }
}

auto System::setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {
    if (size >= (512 * 1024))
        size = 512 * 1024;
    else
        size = Emulator::powerOfTwo( size );

    if (!data || !size) {
        data = nullptr;
        size = 1;
    }

    switch (typeId) {
        case 0:
        default:
            kickRom = data;
            kickRomMask = size - 1;
            break;
        case 1:
            extRom = data;
            extRomMask = size - 1;
            break;
    }
}

auto System::doDMA( unsigned cycles) -> void {
    cycles >>= 1;

    while( cycles ) {

        eClockPosition += 2;
        if (eClockPosition == 10) {
            // CIA accesses must be tuned to E-Clock. One CIA BUS cycle corresponds to 2 + [6,8,10,12,14] + 2 CPU cycles.
            // The programming is coordinated in such a way that the CIA is first driven forward by a cycle and then the register access takes place.
            // (By CPU design) the last 2 CPU cycles always take place after access. So the CIA has to progress a cycle beforehand to make sure that the order is right.
            // Tests that evaluate CIA timers are difficult because while waiting for access, the CIA internally progresses 1 or 2 cycles.
            // To make matters worse, the E-Clock phase can change with each cold start.
            eClockPosition = 0;
            sysTimerCia.process<false>();
            cia1->clock();
            cia2->clock();
        }

        cycles--;
    }
}

}
