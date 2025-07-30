
#include "USBSID.h"
#include "usbSidPico.h"
#include "../system/system.h"

USBSID_NS::USBSID_Class* usbsid;

namespace LIBC64 {

USBSIDPico::USBSIDPico(System& system) : system(system), sysTimer(system.sysTimer) {

    flush = [this]() {
        usbsid->USBSID_SetFlush();
        lastClock = sysTimer.clock;
        this->sysTimer.add( &flush, rasterRate, Emulator::SystemTimer::UpdateExisting );
    };

    sysTimer.registerCallback( {&flush, 1} );
}

auto USBSIDPico::open() -> int {
    int result = 0;

    if (!usbsid) {
        USBSID_NS::USBSID_Class* usbsid = new USBSID_NS::USBSID_Class();

        usbsid->USBSID_SetDiffSize(diffSize);
        usbsid->USBSID_SetBufferSize(buffSize);

        if (usbsid->USBSID_Init(true, true) < 0) {
            delete usbsid;  /* Executes usbsid->USBSID_Close(); */
            return 0;
        }
        result = 1;
    } else {
        usbsid->USBSID_Reset();
        result = 2;
    }

    usbsid->USBSID_SetStereo(system.interface->stats.stereoSound);
    // check emulation/libc64/vicII/base.cpp -> setModel() for line and frame cycles
    usbsid->USBSID_SetClockRate(system.vicII->frequency(), true);
    rasterRate = usbsid->USBSID_GetRasterRate();
    sysTimer.add( &flush, rasterRate, Emulator::SystemTimer::UpdateExisting );
    lastClock = sysTimer.clock;

    return result;
}

auto USBSIDPico::close() -> void {
    if (usbsid) {
        sysTimer.remove(&flush);
        usbsid->USBSID_Mute();
        delete usbsid;  /* Executes usbsid->USBSID_Close(); */
    }
}

auto USBSIDPico::setBuffSize(unsigned value) -> void {
    if (value == buffSize)
        return;

    buffSize = value;
    usbsid->USBSID_SetBufferSize(buffSize);
    usbsid->USBSID_RestartRingBuffer();
}

auto USBSIDPico::setDiffSize(unsigned value) -> void {
    if (value == diffSize)
        return;

    diffSize = value;
    usbsid->USBSID_SetDiffSize(diffSize);
}

auto USBSIDPico::store(uint8_t addr, uint8_t val, int chipNr) -> void {
    unsigned cycles = sysTimer.fallBackCycles(lastClock);
    // unsigned cycles = sysTimer.fallBackCycles(lastClock) - 1;
    usbsid->USBSID_WriteRingCycled(addr + (chipNr * 0x20), val, cycles);
    lastClock = sysTimer.clock;
}

auto USBSIDPico::updateStereo() -> void {
    usbsid->USBSID_SetStereo(system.interface->stats.stereoSound);
}

auto USBSIDPico::serialize(Emulator::Serializer& s) -> void {
    s.integer(enabled);
    unsigned _buffSizeBefore = buffSize;
    s.integer(buffSize);
    s.integer(diffSize);

    if (enabled && (s.mode() == Emulator::Serializer::Mode::Load) ) {
        int result = open();
        if (result == 2) { // reset
            // could be changed from loaded state
            usbsid->USBSID_SetDiffSize(diffSize);
            if (_buffSizeBefore != buffSize) {
                usbsid->USBSID_SetBufferSize(buffSize);
                usbsid->USBSID_RestartRingBuffer();
            }
        }

        setInitialState();
    }
}

auto USBSIDPico::setInitialState() -> void {
    return;

    // when loading a state file or activation during emulation
    for(auto sid : system.sidManager.useSids) {
        store(0, sid->voice[0].freq & 0xff, sid->nr);
        store(1, (sid->voice[0].freq >> 8) & 0xff, sid->nr);
        store(2, sid->voice[0].pw & 0xff, sid->nr);
        store(3, (sid->voice[0].pw >> 8) & 0xf, sid->nr);
        store(4, sid->envelope[0].gateBefore | (sid->voice[0].waveform << 4) | (sid->voice[0].test << 3)
        | (sid->voice[0].sync << 1) | (sid->voice[0].ringMsbMask >> 21), sid->nr);
        store(5, (sid->envelope[0].attack << 4) | sid->envelope[0].decay, sid->nr );
        store(6, (sid->envelope[0].sustain << 4) | sid->envelope[0].release, sid->nr );

        store(7, sid->voice[1].freq & 0xff, sid->nr);
        store(8, (sid->voice[1].freq >> 8) & 0xff, sid->nr);
        store(9, sid->voice[1].pw & 0xff, sid->nr);
        store(0xa, (sid->voice[1].pw >> 8) & 0xf, sid->nr);
        store(0xb, sid->envelope[1].gateBefore | (sid->voice[1].waveform << 4) | (sid->voice[1].test << 3)
        | (sid->voice[1].sync << 1) | (sid->voice[1].ringMsbMask >> 21), sid->nr);
        store(0xc, (sid->envelope[1].attack << 4) | sid->envelope[1].decay, sid->nr );
        store(0xd, (sid->envelope[1].sustain << 4) | sid->envelope[1].release, sid->nr );

        store(0xe, sid->voice[2].freq & 0xff, sid->nr);
        store(0xf, (sid->voice[2].freq >> 8) & 0xff, sid->nr);
        store(0x10, sid->voice[2].pw & 0xff, sid->nr);
        store(0x11, (sid->voice[2].pw >> 8) & 0xf, sid->nr);
        store(0x12, sid->envelope[2].gateBefore | (sid->voice[2].waveform << 4) | (sid->voice[2].test << 3)
        | (sid->voice[2].sync << 1) | (sid->voice[2].ringMsbMask >> 21), sid->nr);
        store(0x13, (sid->envelope[2].attack << 4) | sid->envelope[2].decay, sid->nr );
        store(0x14, (sid->envelope[2].sustain << 4) | sid->envelope[2].release, sid->nr );

        store(0x15, sid->filter.fc & 7, sid->nr);
        store(0x16, sid->filter.fc >> 3, sid->nr);
        store(0x17, (sid->filter.res << 4) | sid->filter.filt, sid->nr);
        store(0x18, (sid->filter.mode & 0xf0) | sid->filter.vol, sid->nr);
    }
}

}
