
#include "paula.h"
#include "../agnus/agnus.h"
#include "../system/system.h"
#include "audio.cpp"
#include "filter.cpp"
#include "fdc.cpp"
#include "pot.cpp"

// todo: To determine the correct interrupt delay within Paula, it must be taken into account that the CPU only tests for interrupts in certain cycles.
// The CPU emulation synchronizes in multiples of DMA cycles (= 2 cycles). The limit of when an interrupt is detected or not happens after an odd number of CPU cycles.
// If the interrupt happens in the 2nd half of the DMA cycle, it is not recognized, although the CPU evaluates IPL in this DMA cycle.
// Synchronization after each CPU cycle slows down emulation. It is more performant to change the IPL pins one DMA cycle later in such situations.

namespace LIBAMI {

Paula::Paula(System* system, Agnus& agnus, Cpu& cpu, Input& input, DiskDrive& disk0, DiskDrive& disk1, DiskDrive& disk2, DiskDrive& disk3) :
system(system),
agnus(agnus),
disk0(disk0),
disk1(disk1),
disk2(disk2),
disk3(disk3),
cpu(cpu),
input(input) {
    sampleLimit = 0;
    enableFilter = true;
}

auto Paula::dmal() -> uint16_t {
    uint16_t out = 0;

    for(uint8_t nr = 0; nr < 4; nr++) {
        Channel& cha = channels[nr];

        if (cha.dsr) {
            out |= 1 << (nr << 1);
            cha.dsr = false;
        }

        if (cha.dr) {
            out |= 1 << ((nr << 1) + 1);
            cha.dr = false;
        }
    }

    out <<= 6;

    if (diskState == DiskState::READ) {
        switch(fifoPos) {
            case 1: out |= 1; break;
            case 2: out |= 5; break;
            case 3: out |= 21; break;
            default: break;
        }
    } else if (diskState == DiskState::WRITE) {
        switch(fifoPos) {
            case 0: out |= 63; break;
            case 1: out |= 15; break;
            case 2: out |= 3; break;
            default: break;
        }
        if (dskTansferLength == 2) out &= ~48;
        else if (dskTansferLength == 1) out &= ~60;
    }

    return out;
}

auto Paula::pulseInt3() -> void { // Blitter (Agnus generates one DMA cycle pulse)
    intreq |= 0x40;
    prepareIpl();
}

auto Paula::setInt2(bool state) -> void { // CIA 1
    // CIAs don't generate a pulse like blitter. Paula checks here on a flank. Note that as long as the interrupt signal enters Paula,
    // the CIA lines cannot be reset when writing "intreq".

    if (state && !int2Current) {
        intreq |= 8;
        prepareIpl();
    }

    int2Current = state;
}

auto Paula::setInt6(bool state) -> void { // CIA 2
    if (state && !int6Current) {
        intreq |= 0x2000;
        prepareIpl();
    }

    int6Current = state;
}

template<uint8_t nr> auto Paula::setIntAud() -> void {
    if constexpr (nr == 0)
        intreq |= 0x100;
    else if constexpr (nr == 1)
        intreq |= 0x400;
    else if constexpr (nr == 2)
        intreq |= 0x80;
    else if constexpr (nr == 3)
        intreq |= 0x200;

    prepareIpl();
}

auto Paula::setDskSyncInt() -> void {
    intreq |= 0x1000;

    prepareIpl();
}

auto Paula::setDskBlkInt(bool delayed) -> void {
    intreq |= 2;
    prepareIpl();
}

auto Paula::strhor() -> void {
    if (vBlankIntr)
        vBlankIntr = false;

    if (pot.running)
        progressPot();
}

auto Paula::strequ() -> void {
    if (!vBlankIntr) {
        vBlankIntr = true;
        intreq |= 0x20;
        prepareIpl();
    }

    if (pot.running)
        progressPot();
}

auto Paula::strvbl() -> void {
    if (pot.running)
        progressPot();
}

auto Paula::getIntena() -> uint16_t {
    return intena;
}

auto Paula::getIntreq() -> uint16_t {
    return intreq;
}

auto Paula::getAdkCon() -> uint16_t  {
    return adkcon;
}

auto Paula::setAdkCon(uint16_t value) -> void {
    uint16_t _adkcon = adkcon;

    if (value & 0x8000)
        adkcon |= value & 0x7fff;
    else
        adkcon &= ~value;

    if ((_adkcon ^ adkcon) & 0xff)
        updateModulation();
}

auto Paula::setIntena(uint16_t value) -> void {
    if (value & 0x8000)
        intena |= value & 0x7fff;
    else
        intena &= ~value;

    prepareIpl();
}

auto Paula::setIntreq(uint16_t value) -> void {
    if (value & 0x8000)
        intreq |= value & 0x7fff;
    else
        intreq &= ~value;

    if (int2Current) intreq |= 8;
    if (int6Current) intreq |= 0x2000;

    prepareIpl();
}

auto Paula::dmaCon(uint16_t value) -> void {
    // Audio and Disk DMA usage will be evaluated only by Paula
    if (channels[0].dma != ((value & 0x201) == 0x201)) {
        channels[0].dma ^= 1;
        toggleAudioDMA<0>();
    }
    if (channels[1].dma != ((value & 0x202) == 0x202)) {
        channels[1].dma ^= 1;
        toggleAudioDMA<1>();
    }
    if (channels[2].dma != ((value & 0x204) == 0x204)) {
        channels[2].dma ^= 1;
        toggleAudioDMA<2>();
    }
    if (channels[3].dma != ((value & 0x208) == 0x208)) {
        channels[3].dma ^= 1;
        toggleAudioDMA<3>();
    }

    dmaDisk = (value & 0x210) == 0x210;
}

auto Paula::powerOff() -> void {

}

auto Paula::serialize(Emulator::Serializer& s, bool light) -> void {
    s.integer(intena);
    s.integer(intreq);
    s.integer(adkcon);
    s.integer(int2Current);
    s.integer(int6Current);
    s.integer(vBlankIntr);
    s.integer(enableFilter);
    s.integer(useLedFilter);
    s.integer(pot.cntX0);
    s.integer(pot.cntY0);
    s.integer(pot.cntX1);
    s.integer(pot.cntY1);
    s.integer(pot.capX0);
    s.integer(pot.capY0);
    s.integer(pot.capX1);
    s.integer(pot.capY1);
    s.integer(pot.go);
    s.integer(pot.dischargeCounter);

    for(uint8_t c = 0; c < 4; c++) {
        Channel& cha = channels[c];
        s.integer(cha.dma);
        s.integer(cha.dr);
        s.integer(cha.dsr);
        s.integer(cha.intreq2);
        s.integer(cha.clock);
        s.integer(cha.state);
        s.integer(cha.perLatch);
        s.integer(cha.len);
        s.integer(cha.lenLatch);
        s.integer(cha.vol);
        s.integer(cha.volLatch);
        s.integer(cha.dat);
        s.integer(cha.buffer);
        s.integer(cha.sample);
        s.integer(cha.audav);
        s.integer(cha.audap);
        s.integer(cha.napnav);
    }

    s.integer(sampleLimit);
    s.integer(sampleCycle);
    s.integer(dmaDisk);


    if (!light) {
        s.floatingpoint(filters[0].rc1);
        s.floatingpoint(filters[0].rc2);
        s.floatingpoint(filters[0].rc3);
        s.floatingpoint(filters[0].rc4);
        s.floatingpoint(filters[0].rc5);
        s.floatingpoint(filters[1].rc1);
        s.floatingpoint(filters[1].rc2);
        s.floatingpoint(filters[1].rc3);
        s.floatingpoint(filters[1].rc4);
        s.floatingpoint(filters[1].rc5);

        s.floatingpoint(filter1A0);
        s.floatingpoint(filter2A0);
        s.floatingpoint(filterA0);
    }

    s.integer(dskLen);
    s.integer(dskSync);
    s.integer(dskTansferLength);
    s.integer(fifo);
    s.integer(fifoPos);
    s.integer(dskEventCycle);
    s.integer(dskSyncCycle);
    s.integer(dskShifter);
    s.integer(dskShifterPos);
    s.integer(dmaCycles);
    s.integer(dskBytr);
    s.integer(turbo);
    s.integer(ipl);
    s.integer(iplCounter);

    uint8_t driveNum = activeDrive->number;
    s.integer(driveNum);
    if (driveNum != activeDrive->number) {
        switch(driveNum) {
            default:
            case 0: activeDrive = &disk0; break;
            case 1: activeDrive = &disk1; break;
            case 2: activeDrive = &disk2; break;
            case 3: activeDrive = &disk3; break;
        }
    }
}

auto Paula::power() -> void {
    intena = 0;
    intreq = 0;
    adkcon = 0;
    int2Current = false;
    int6Current = false;
    vBlankIntr = true;
    useLedFilter = false;
    pot.cntX0 = 0;
    pot.cntY0 = 0;
    pot.cntX1 = 0;
    pot.cntY1 = 0;
    pot.capX0 = 0xff;
    pot.capY0 = 0xff;
    pot.capX1 = 0xff;
    pot.capY1 = 0xff;
    pot.go = 0;
    pot.running = false;
    pot.dischargeCounter = 0;

    for(uint8_t c = 0; c < 4; c++) {
        Channel& cha = channels[c];

        cha.dma = false;
        cha.dr = false;
        cha.dsr = false;
        cha.intreq2 = false;
        cha.clock = INT64_MAX;
        cha.state = 0;
        cha.perLatch = 0;
        cha.len = 0;
        cha.lenLatch = 0;
        cha.vol = 0;
        cha.volLatch = 0;
        cha.dat = 0;
        cha.buffer = 0;
        cha.sample = 0;
        cha.audav = false;
        cha.audap = false;
        cha.napnav = true;
    }

    dmaDisk = false;
    filters[0].reset();
    filters[1].reset();

    sampleCycle = agnus.clock + sampleLimit;

    dskLen = 0;
    dskSync = 0;
    dskTansferLength = 0;
    fifo = 0;
    fifoPos = 0;
    dskEventCycle = agnus.clock + FDC_IDLE;
    dskSyncCycle = 0;
    dskShifter = 0;
    dskShifterPos = 0;
    dmaCycles = 0;
    dskBytr = 0;
    diskState = DiskState::OFF;
    iplCounter = 0;
    ipl = 0;
}

auto Paula::prepareIpl() -> void {
    int level = 0;
    uint16_t intMask = intreq & intena;

    if (intMask && (intena & 0x4000)) {
        if (intMask & (0x4000 | 0x2000))
            level = 6;
        else if (intMask & (0x1000 | 0x0800))
            level = 5;
        else if (intMask & (0x0400 | 0x0200 | 0x0100 | 0x0080))
            level = 4;
        else if (intMask & (0x0040 | 0x0020 | 0x0010))
            level = 3;
        else if (intMask & 0x0008)
            level = 2;
        else if (intMask & (0x0001 | 0x0002 | 0x0004))
            level = 1;
    }

    int lastIPL = ipl & 0x7;

    if (level != lastIPL) {
        // todo: check exact delays
        ipl = (ipl & ~0xff) | level;

        if (((lastIPL & 1) && !(level & 1)) ||
            ((lastIPL & 2) && !(level & 2)) ||
            ((lastIPL & 4) && !(level & 4)) );
            // from WinUAE changelog it seems to matter if a bit is set or cleared.
            // CPU detect IRQs in the first half of DMA (CCK) Sample cycle (confimed with fx68k)
            // This means that a delay of only one CPU cycle is enough to miss this sample cycle and you have to wait for the sample point in the next opcode.
        else
            ipl = (ipl & ~0xff00) | (level << 8);

        iplCounter = 4;
    }
}

auto Paula::process() -> void {

    if (iplCounter) {
        cpu.setInterrupt( (ipl >> 24) & 7 );
        ipl = (ipl << 8) | (ipl & 0xff);
        iplCounter--;
    }

    if (dskEventCycle == agnus.clock) {
        switch (diskState) {
            case DiskState::WAIT_SYNC_READ:
            case DiskState::WAIT_SYNC_WRITE:
                if (turbo)
                    handleFDControllerRead<false, true>();
                // fallthrough
            case DiskState::READ:
                handleFDControllerRead();
                break;
            case DiskState::WRITE: {
                handleFDControllerWrite();
                if (turbo && dmaDisk) {
                    uint8_t repeat = (1 << turbo) - 1;
                    do {
                        if (!dskTansferLength)
                            break;
                        if (dskShifterPos == 1)
                            addToFifo( agnus.fakeDiskDma() );
                        handleFDControllerWrite();
                    } while (--repeat);
                }
            } break;
            case DiskState::INSTANT_BLK_INT:
                if (useInstantDriveAccess())
                    setDskBlkInt();
                diskState = DiskState::OFF;
            default:
                activeDrive->rotate(dmaCycles); // rotate if motor is running, doesn't matter if drive selected or FDC is idling
                dmaCycles = FDC_IDLE;
                // todo: rotate other connected drives too
                // todo: update diskbytr and dsksync, depending which drives are selected, enabled and rotating (motor)
                break;
        }
        dskEventCycle = agnus.clock + dmaCycles;
    }

    if (sampleCycle == agnus.clock) {
        sampleCycle = agnus.clock + sampleLimit;

        if (audioOut) {
            int32_t sampleL = channels[0].sample + channels[3].sample;
            int32_t sampleR = channels[1].sample + channels[2].sample;

            if (enableFilter) {
                sampleL = lowPassfilter<0>(sampleL);
                sampleR = lowPassfilter<1>(sampleR);
            }

            system->audioRefresh(Emulator::sclamp(16, sampleL), Emulator::sclamp(16, sampleR));
        }
    }
}

}
