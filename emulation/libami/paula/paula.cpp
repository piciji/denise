
#include "paula.h"
#include "../agnus/agnus.h"
#include "../system/system.h"
#include "../interface.h"
#include "irq.cpp"
#include "audio.cpp"
#include "filter.cpp"
#include "fdc.cpp"
#include "pot.cpp"
#include "serial.cpp"


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
    filterMode = 0;
    loopBack = false;
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
            case 1: out |= 16; break;
            case 2: out |= 20; break;
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
        if (dskTransferLength == 2) out &= ~48;
        else if (dskTransferLength == 1) out &= ~60;
    }

    return out;
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

    if ((adkcon & 0x400) && !(_adkcon & 0x400))
        dskShifterPos = 0;
}

auto Paula::dmaCon(uint16_t value) -> void {
    // Audio and Disk DMA usage will be evaluated only by Paula
    if (channels[0].AUDxON != ((value & 0x201) == 0x201)) {
        channels[0].AUDxON ^= 1;
        toggleAudioDMA<0>();
    }
    if (channels[1].AUDxON != ((value & 0x202) == 0x202)) {
        channels[1].AUDxON ^= 1;
        toggleAudioDMA<1>();
    }
    if (channels[2].AUDxON != ((value & 0x204) == 0x204)) {
        channels[2].AUDxON ^= 1;
        toggleAudioDMA<2>();
    }
    if (channels[3].AUDxON != ((value & 0x208) == 0x208)) {
        channels[3].AUDxON ^= 1;
        toggleAudioDMA<3>();
    }

    dmaDisk = (value & 0x210) == 0x210;
}

auto Paula::powerOff() -> void {

}

auto Paula::serialize(Emulator::Serializer& s, bool light) -> void {
    s.integer((uint8_t&)diskState);
    s.integer(intena);
    s.integer(intreq);
    s.integer(adkcon);
    s.integer(int2Current);
    s.integer(int6Current);
    s.integer(vBlankIntr);
    s.integer(filterMode);
    s.integer(useLedFilter);

    s.integer(dskLen);
    s.integer(dskSync);
    s.integer(dskTransferLength);
    s.integer(fifo);
    s.integer(fifoPos);
    s.integer(dskSyncCycle);
    s.integer(dskShifter);
    s.integer(dskShifterPos);
    s.integer(fdcCycles);
    s.integer(dskBytr);
    s.integer(fifoReady);

    s.integer(ipl);
    s.integer(iplCounter);

    s.integer(intreqAud0Clock);
    s.integer(intreqAud1Clock);
    s.integer(intreqAud2Clock);
    s.integer(intreqAud3Clock);
    s.integer(intreqBltClock);
    s.integer(intreqTbeClock);
    s.integer(intreqRbfClock);
    s.integer(intreqCia1Clock);
    s.integer(intreqCia2Clock);

    s.integer(intUpdClock);
    s.integer(intreqLast);

    s.integer(serPer);
    s.integer(serDat);
    s.integer(serShifter);
    s.integer(receiveShifter);
    s.integer(receiveCounter);
    s.integer(serdatR);
    s.integer(loopBack);
    s.integer(rxd);
    s.integer(txd);
    s.integer(overrun);
    s.integer(serialTransferEvent);
    s.integer(serialReceiveEvent);

    s.integer(turbo);
    s.integer(turboRequested);

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
    s.integer(pot.running);

    for(int c = 0; c < 4; c++) {
        Channel& cha = channels[c];
        s.integer(cha.AUDxON);
        s.integer(cha.dr);
        s.integer(cha.dsr);
        s.integer(cha.intreq2);
        s.integer(cha.percount);
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
    intreqLast = 0;
    intUpdClock = 0;
    adkcon = 0;
    int2Current = false;
    int6Current = false;
    vBlankIntr = true;
    useLedFilter = true;
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

    for(auto& cha : channels) {
        cha.AUDxON = false;
        cha.dr = false;
        cha.dsr = false;
        cha.intreq2 = false;
        cha.percount = INT64_MAX;
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
    dskTransferLength = 0;
    fifo = 0;
    fifoPos = 0;
    dskSyncCycle = 0;
    dskShifter = 0;
    dskShifterPos = 0;
    fifoReady = false;
    fdcCycles = FDC_BIT;
    dskBytr = 0;
    diskState = DiskState::OFF;
    iplCounter = 0;
    ipl = 0;
    intreqAud0Clock = INT64_MAX;
    intreqAud1Clock = INT64_MAX;
    intreqAud2Clock = INT64_MAX;
    intreqAud3Clock = INT64_MAX;
    intreqBltClock = INT64_MAX;
    intreqTbeClock = INT64_MAX;
    intreqRbfClock = INT64_MAX;
    intreqCia1Clock = INT64_MAX;
    intreqCia2Clock = INT64_MAX;

    serialTransferEvent = INT64_MAX;
    serialReceiveEvent = INT64_MAX;

    serDat = 0;
    serPer = 0;
    serdatR = 0;
    serShifter = 0;
    receiveShifter = 0;
    receiveCounter = 0;

    rxd = true;
    txd = true;
    overrun = false;
    setFilter();
}

auto Paula::iplUpdate() -> void {
    cpu.setInterrupt( (ipl >> 16) & 7 );
    ipl = (ipl << 8) | (ipl & 0xff);
    iplCounter--;
}

auto Paula::sampleUpdate() -> void {
    sampleCycle = agnus.clock + sampleLimit;
    
    if (audioOut) {
        int32_t sampleL = channels[0].sample + channels[3].sample; // sample: 14 bit (8bit * 6bit volume), mix two samples: 15 bit
        int32_t sampleR = channels[1].sample + channels[2].sample;
        sampleL <<= 1; // 16 bit
        sampleR <<= 1;

        if (filterMode != 4) { // A1200 Off don't use filter
            int _filterMode = filterMode;
            sampleL = lowPassfilter<0>(sampleL, _filterMode);
            sampleR = lowPassfilter<1>(sampleR, _filterMode);
        }
        
        system->audioRefresh(Emulator::sclamp(16, sampleL), Emulator::sclamp(16, sampleR));
    }
}

}
