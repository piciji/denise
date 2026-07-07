
#define IO_MAPPING  \
    memoryCpu.map( &readVicReg, &peekVicReg, &writeVicReg, 0xd0, 0xd3);      \
                                                                \
    if (!debugCart->enable)                                     \
        memoryCpu.map( &readSidReg, &peekSidReg, &writeSidReg, 0xd4, 0xd7);  \
    else {                                                      \
        memoryCpu.map( &readSidReg, &peekSidReg, &writeSidReg, 0xd4, 0xd6);  \
        memoryCpu.map( &readSidReg, &peekSidReg, &writeDebugReg, 0xd7, 0xd7);\
    }                                                           \
    memoryCpu.map( &readColorRam, &writeColorRam, 0xd8, 0xdb);  \
    memoryCpu.map( &readCia1Reg, &peekCia1Reg, &writeCia1Reg, 0xdc, 0xdc);    \
    memoryCpu.map( &readCia2Reg, &peekCia2Reg, &writeCia2Reg, 0xdd, 0xdd);    \
    memoryCpu.map( &readIo1Reg, &peekIo1Reg, &writeIo1Reg, 0xde, 0xde);      \
    memoryCpu.map( &readIo2Reg, &peekIo2Reg, &writeIo2Reg, 0xdf, 0xdf);

namespace LIBC64 {

typedef Emulator::Interface::DebuggerDma DebuggerDma;

auto System::remapCpu(bool speedHack) -> void {

    // speed hack is used for Final Cartridge Plus (not 3+)
    // the cart uses Ultimax in second half cycle when accessing some address ranges
    // switching causes a rebuild of memory map, which happens very often (50 FPS speed hit)
    // the hack prevents to rebuild areas, which are not accessed when cart switched to Ultimax mode

    if (expansionPort == noExpansion) {
        
        memoryCpu.map( &readRam, &writeRam, 0x0, 0x9f );

        memoryCpu.map( ((mode & 3) == 3) ? &readBasicRom : &readRam, &writeRam, 0xa0, 0xbf );

        memoryCpu.map( &readRam, &writeRam, 0xc0, 0xcf );

        if ((mode & 3) == 0)
            memoryCpu.map( &readRam, &writeRam, 0xd0, 0xdf );
        else if ((mode & 4) == 0)
            memoryCpu.map( &readCharRom, &writeRam, 0xd0, 0xdf );
        else {
            IO_MAPPING
        }

        if (mode & 2)
            memoryCpu.map( &readKernalRom, &peekKernalRom, &writeRam, 0xe0, 0xff );
        else
            memoryCpu.map( &readRam, &writeRam, 0xe0, 0xff );

        return;
    } 
    
    // full mapping for 8k, 16k, ultimax, no cart
    uint8_t subMode = mode & 7;
    uint8_t ramMode = mode & 3;
    uint8_t cartMode = (mode >> 3) & 3;
    bool ultimax = isUltimax();

    // 00 - 0f -> always ram
    memoryCpu.map( &readRam, &writeRam, 0x0, 0x0f );

    // 10 - 7f
    if ( ultimax && !speedHack )
        memoryCpu.mapFast( &readUnmapped, &writeUnmapped, 0x10, 0x7f );
    else
        memoryCpu.mapFast( &readRam, &writeRam, 0x10, 0x7f );

    // 80 - 9f
    if ( ultimax ) {
        memoryCpu.map( &readRomL, &peekRomL, 0x80, 0x9f);
        memoryCpu.map( &writeUltimaxRomL, 0x80, 0x9f );

    } else if ( (cartMode == 0 || cartMode == 1) && ramMode == 3 ) {
        memoryCpu.map( &readRomL, &peekRomL, 0x80, 0x9f);
        memoryCpu.map( &writeRomL, 0x80, 0x9f );
    } else
        memoryCpu.map( &readRam, &writeRamAt80To9F, 0x80, 0x9f );

    // a0 - bf
    if ( ultimax )
        memoryCpu.map( &readUltimaxA0, &peekUltimaxA0, &writeUltimaxA0, 0xa0, 0xbf );

    else if ( (cartMode == 1 || cartMode == 3) && ramMode == 3 ) {
        memoryCpu.map( &readBasicRom, 0xa0, 0xbf );
        memoryCpu.map( &writeRamAtA0ToBF, 0xa0, 0xbf );

    } else if (cartMode == 0 && (ramMode == 2 || ramMode == 3) ) {

        memoryCpu.map( &readRomH, &peekRomH, 0xa0, 0xbf );
        memoryCpu.map( &writeRomH, 0xa0, 0xbf );
    } else
        memoryCpu.map( &readRam, &writeRamAtA0ToBF, 0xa0, 0xbf );

    // c0 - cf
    if ( ultimax && !speedHack )
        memoryCpu.mapFast( &readUnmapped, &writeUnmapped, 0xc0, 0xcf );
    else
        memoryCpu.mapFast( &readRam, &writeRam, 0xc0, 0xcf );

    // d0 - df
    if ( (ultimax && !speedHack) || subMode == 5 || subMode == 6 || subMode == 7 ) {
        IO_MAPPING
    } else if ( (subMode == 1 || subMode == 2 || subMode == 3) && (mode != 1)  ) {

        memoryCpu.map( &readCharRom, 0xd0, 0xdf );
        memoryCpu.map( &writeRam, 0xd0, 0xdf );
    } else
        memoryCpu.map( &readRam, &writeRam, 0xd0, 0xdf );

    // e0 - ff
    if ( ultimax ) {
        memoryCpu.map( &readRomH, &peekRomH, 0xe0, 0xff);
        memoryCpu.map( &writeUltimaxRomH, 0xe0, 0xff );

    } else if (ramMode == 2 || ramMode == 3) {
        memoryCpu.map( &readKernalRom, &peekKernalRom,0xe0, 0xff );
        memoryCpu.map( &writeRam, 0xe0, 0xff );

    } else
        memoryCpu.map( &readRam, &writeRam, 0xe0, 0xff );
    
    expansionPort->memoryMapUpdated();
}

auto System::logCpu(uint16_t addr, uint8_t data, bool write, bool nextOp) -> void {
    uint8_t page = addr >> 8;
    uint8_t mapper = 0;
    Memory::Read* ptr = memoryCpu.reads[page];

    if (ptr == &readRam) mapper = 0x80 | 1;
    else if (ptr == &readVicReg) mapper = 0x80 | 2;
    else if (ptr == &readSidReg) mapper = 0x80 | 3;
    else if (ptr == &readColorRam) mapper = 0x80 | 4;
    else if (ptr == &readIo1Reg) mapper = 0x80 | 5;
    else if (ptr == &readIo2Reg) mapper = 0x80 | 6;
    else if (ptr == &readCia1Reg) mapper = 0x80 | 7;
    else if (ptr == &readCia2Reg) mapper = 0x80 | 8;
    else if (ptr == &readCharRom) mapper = 0x80 | 9;
    else if (ptr == &readKernalRom) mapper = 0x80 | 10;
    else if (ptr == &readBasicRom) mapper = 0x80 | 11;
    else if (ptr == &readRomL) mapper = 0x80 | 12;
    else if (ptr == &readRomH) mapper = 0x80 | 13;
    else if (ptr == &readUltimaxA0) mapper = 0x80 | 14;

    auto& dma = vicII->requestCurrentDmaLog();
    dma.usageCpu = mapper;
    dma.addrCpu = addr;
    dma.dataCpu = data;

    if (nextOp) {
        dma.mnemonic = DasmHandler::mnemonic(data);
        dma.hilight = DebuggerDma::Hilight::Opcode;

    } else if (write) {
        dma.mnemonic = nullptr;
        dma.hilight = DebuggerDma::Hilight::Write;

    } else {
        dma.mnemonic = nullptr;
        dma.hilight = DebuggerDma::Hilight::Default;
    }

    int i = 0;
    for (auto& dmaWatcher : debugger.dmaWatchers ) {
        if (dmaWatcher & 0x80000000) { // in use
            if (dmaWatcher & (1 << 24)) {
                if (cpu.irqPending)
                    dma.watcher[i] = 0xfffe;
                else if (cpu.nmiPending)
                    dma.watcher[i] = 0xfffa;
                else
                    dma.watcher[i] = 0;
            } else
                dma.watcher[i] = memoryCpu.peek( dmaWatcher & 0xffff );
        }
        i++;
    }
}

auto System::editMemory(DebuggerTheme theme, uint32_t addr, std::vector<uint16_t> values) -> void {
    switch (theme) {
        case DebuggerTheme::Drive8CPU:
        case DebuggerTheme::Drive9CPU:
        case DebuggerTheme::Drive10CPU:
        case DebuggerTheme::Drive11CPU:
        case DebuggerTheme::Drive8Memory:
        case DebuggerTheme::Drive9Memory:
        case DebuggerTheme::Drive10Memory:
        case DebuggerTheme::Drive11Memory:
            iecBus.editMemory( theme, addr, values );
            break;

        default:
        case DebuggerTheme::Memory:
        case DebuggerTheme::MemorySCPU:
        case DebuggerTheme::CPU:
        case DebuggerTheme::SCPU: {
            if (dynamic_cast<SuperCpu*>(expansionPort)) {
                for (int i = 0; i < values.size(); i++) {
                    uint32_t a = (addr + i) & 0xffffff;
                    uint8_t v = values[i] & 0xff;

                    superCpu->editMemory( a, v );
                }
            } else {
                for (int i = 0; i < values.size(); i++) {
                    uint16_t a = (addr + i) & 0xffff;
                    uint8_t v = values[i] & 0xff;

                    Memory::Read* ptr = memoryCpu.reads[a >> 8];
                    if (ptr == &readCharRom)            charRom[a & 0xfff] = v;
                    else if (ptr == &readKernalRom)     kernalRom[a & 0x1fff] = v;
                    else if (ptr == &readBasicRom)      basicRom[a & 0x1fff] = v;

                    memoryCpu.write(a, v);
                }
            }
        } break;
    }
}

auto System::memoryDump(uint8_t page, uint8_t* dump) -> void {
    uint8_t temp[16];
    page &= 0xf;
    page <<= 4;

    Memory::Read* ptr = memoryCpu.reads[page];

    if (ptr == &readRam) {
        for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
            *dump++ = this->ram[ addr ];
    }

    else if (ptr == &readVicReg) {
        for (unsigned addr = 0xd000; addr <= 0xd3ff; addr++ )
            *dump++ = vicII->peekReg( addr & 0xff );

        if (sidManager.extraSids) {
            for (unsigned addr = 0xd400; addr <= 0xd7ff; addr++ )
                *dump++ = sidManager.getSidByAdr( addr )->peekIO( addr );
        } else {
            for (unsigned addr = 0xd400; addr <= 0xd7ff; addr++ )
                *dump++ = sidManager.mainSid()->peekIO( addr );
        }

        uint8_t _l = vicII->lastReadPhase1() & ~0xf;
        for (unsigned addr = 0xd800; addr <= 0xdbff; addr++ )
            *dump++ = (colorRam[ addr & 0x3ff ] & 0xf) | _l;

        for (unsigned addr = 0xdc00; addr <= 0xdc0f; addr++ )
            temp[addr & 0xf] = cia1.peek( addr );

        for(int a = 0; a < 16; a++) {
            std::memcpy(dump, &temp, 16);
            dump += 16;
        }

        for (unsigned addr = 0xdd00; addr <= 0xdd0f; addr++ )
            temp[addr & 0xf] = cia2.peek( addr );

        for(int a = 0; a < 16; a++) {
            std::memcpy(dump, &temp, 16);
            dump += 16;
        }

        for (unsigned addr = 0xde00; addr <= 0xdeff; addr++ )
            *dump++ = peekIo1Reg( addr );

        for (unsigned addr = 0xdf00; addr <= 0xdfff; addr++ )
            *dump++ = peekIo2Reg( addr );
    }

    else if (ptr == &readRomL) {
        for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
            *dump++ = peekRomL( addr );
    }

    else if (ptr == &readRomH) {
        for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
            *dump++ = peekRomH( addr );
    }

    else if (ptr == &readUltimaxA0) {
        for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
            *dump++ = peekUltimaxA0( addr );
    }

    else if (ptr == &readCharRom) {
        for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
            *dump++ = charRom[ addr & 0xfff ];
    }

    else if (ptr == &readBasicRom) {
        for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
            *dump++ = basicRom[ addr & 0x1fff ];
    }

    else if (ptr == &readKernalRom) {
        if (expansionPort->hasHiramCableConnected()) {
            for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
                *dump++ = peekRomH( addr & 0x1fff );
        } else {
            for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
                *dump++ = kernalRom[ addr & 0x1fff ];
        }
    } else {
        for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
            *dump++ = vicII->lastReadPhase1();
    }
}

auto System::memoryDumpCart(uint16_t startAddr, uint16_t endAddr, uint8_t* dump) -> void {
    for (unsigned addr = startAddr; addr < endAddr; addr++) {
        if (addr >= 0x8000 && addr <= 0x9fff) {
            *dump++ = peekRomL( addr );
        } else if (addr >= 0xa000 && addr <= 0xbfff) {
            *dump++ = peekRomH( addr );
        } else
            *dump++ = 0xff;
    }
}

auto System::memoryDumpROM(uint16_t startAddr, uint16_t endAddr, uint8_t* dump) -> void {
    bool scpu = dynamic_cast<SuperCpu*>(expansionPort);

    for (unsigned addr = startAddr; addr < endAddr; addr++) {
        if (addr >= 0xa000 && addr <= 0xbfff) {
            if (scpu)
                *dump++ = superCpu->readSramB1<true>( addr );
            else
                *dump++ = basicRom[ addr & 0x1fff ];
        } else if (addr >= 0xd000 && addr <= 0xdfff) {
            *dump++ = charRom[ addr & 0xfff ];
        } else if (addr >= 0xe000) {
            if (scpu)
                *dump++ = superCpu->peekKernal( addr );
            else
                *dump++ = kernalRom[ addr & 0x1fff ];
        } else
            *dump++ = ram[addr & 0xffff];
    }
}

auto System::memoryDumpIO(uint16_t startAddr, uint16_t endAddr, uint8_t* dump) -> void {
    bool scpu = dynamic_cast<SuperCpu*>(expansionPort);

    for (unsigned addr = startAddr; addr < endAddr; addr++) {
        switch (addr & 0xff00) {
            case 0xd000:
                if (scpu) {
                    if ((addr & 0xfff0) == 0xd0b0) {
                        *dump++ = superCpu->peekIoSCPU(addr);
                        break;
                    }
                }
            case 0xd100: *dump++ = peekVicReg(addr); break;
            case 0xd200:
            case 0xd300:
                if (scpu)
                    *dump++ = superCpu->readSramB1<true>(addr);
                else
                    *dump++ = peekVicReg(addr);
                break;
            case 0xd400:
            case 0xd500:
            case 0xd600:
            case 0xd700: *dump++ = peekSidReg(addr); break;
            case 0xd800:
            case 0xd900:
            case 0xda00:
            case 0xdb00: *dump++ = readColorRam(addr); break;
            case 0xdc00: *dump++ = peekCia1Reg(addr); break;
            case 0xdd00: *dump++ = peekCia2Reg(addr); break;
            case 0xde00: *dump++ = peekIo1Reg(addr); break;
            case 0xdf00: *dump++ = peekIo2Reg(addr); break;
            default: *dump++ = 0xff; break;
        }
    }
}

}