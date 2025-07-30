
#include <USBSIDInterface.h>
#include "usbSidPico.h"
#include "../system/system.h"

namespace LIBC64 {

USBSIDPico::USBSIDPico(System& system) : system(system), sysTimer(system.sysTimer) {

    flush = [this]() { 
        setflush_USBSID(usbsid);
        lastClock = sysTimer.clock;
        this->sysTimer.add( &flush, rasterRate, Emulator::SystemTimer::UpdateExisting );
    };

    sysTimer.registerCallback( {&flush, 1} );
}

auto USBSIDPico::open(bool initState) -> bool {
    if (!usbsid) {
        usbsid = create_USBSID();

        setdiffsize_USBSID(usbsid, diffSize);
        setbuffsize_USBSID(usbsid, buffSize);

        if (init_USBSID(usbsid, true, true) < 0) {
            close_USBSID(usbsid);
            usbsid = nullptr;
            return false;
        }
        
        setstereo_USBSID(usbsid, system.interface->stats.stereoSound);
    } else
        reset_USBSID(usbsid);

    // check emulation/libc64/vicII/base.cpp -> setModel() for line and frame cycles
    setclockrate_USBSID(usbsid, system.vicII->frequency(), true);
    rasterRate = getrasterrate_USBSID(usbsid);
    sysTimer.add( &flush, rasterRate, Emulator::SystemTimer::UpdateExisting );
    lastClock = sysTimer.clock;

    // if (initState)
        // setInitialState();

    return true;
}

auto USBSIDPico::close() -> void {
    if (usbsid) {
        sysTimer.remove(&flush);
        mute_USBSID(usbsid);
        close_USBSID(usbsid);
        usbsid = nullptr;
    }
}

auto USBSIDPico::setBuffSize(unsigned value) -> void {
    if (value == buffSize)
        return;

    buffSize = value;
    setbuffsize_USBSID(usbsid, value);
    restartringbuffer_USBSID(usbsid);
}

auto USBSIDPico::setDiffSize(unsigned value) -> void {
    if (value == diffSize)
        return;

    diffSize = value;
    setdiffsize_USBSID(usbsid, value);
}

auto USBSIDPico::store(uint8_t addr, uint8_t val, int chipNr) -> void {
    unsigned cycles = sysTimer.fallBackCycles(lastClock);
    writeringcycled_USBSID(usbsid, addr + (chipNr * 0x20), val, cycles);
    lastClock = sysTimer.clock;
}

auto USBSIDPico::updateStereo() -> void {
    if (usbsid) // no check in interface ?
        setstereo_USBSID(usbsid, system.interface->stats.stereoSound);
}

auto USBSIDPico::serialize(Emulator::Serializer& s, bool light) -> void {
    s.integer(enabled);
    s.integer(buffSize);
    s.integer(diffSize);
    s.integer(lastClock);

    if (!light && enabled && (s.mode() == Emulator::Serializer::Mode::Load) ) {
        if (!usbsid)
            open();
        else {
            setdiffsize_USBSID(usbsid, diffSize);
            setbuffsize_USBSID(usbsid, buffSize);
            setstereo_USBSID(usbsid, system.interface->stats.stereoSound);
            setclockrate_USBSID(usbsid, system.vicII->frequency(), true);
            rasterRate = getrasterrate_USBSID(usbsid);
        }
        // setInitialState();
    }
}

auto USBSIDPico::setInitialState() -> void {
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
