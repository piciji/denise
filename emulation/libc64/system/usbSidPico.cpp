
#ifdef LIBUSB
#include <USBSID.h>
#endif
#include "usbSidPico.h"
#include "system.h"

#ifdef LIBUSB
USBSID_NS::USBSID_Class* usbsid = nullptr;
#endif
namespace LIBC64 {

USBSIDPico::USBSIDPico(System& system) : system(system), sysTimer(system.sysTimer) {
#ifdef LIBUSB
    flush = [this]() {
        unsigned cycles = sysTimer.clock - lastClock;
        if (cycles < rasterRate) {
            this->sysTimer.add( &flush, rasterRate, Emulator::SystemTimer::UpdateExisting );
        } else {
            lastClock += cycles;
            usbsid->USBSID_Flush();
            this->sysTimer.add( &flush, rasterRate, Emulator::SystemTimer::UpdateExisting );
        }
    };

#else
    flush = [this]() {};
#endif

    sysTimer.registerCallback({ &flush, 1 });
}

auto USBSIDPico::open() -> int {
    int result = 0;
#ifdef LIBUSB
    if (!usbsid) {
        usbsid = new USBSID_NS::USBSID_Class();

        usbsid->USBSID_SetDiffSize(diffSize);
        usbsid->USBSID_SetBufferSize(buffSize);

        if (usbsid->USBSID_Init(true, true) < 0) {
            delete usbsid;  /* Executes usbsid->USBSID_Close(); */
            usbsid = nullptr;
            return 0;
        }
        result = 1;
    } else {
        usbsid->USBSID_ResetRingBuffer();
        usbsid->USBSID_ResetAllRegisters();
        usbsid->USBSID_Reset();
        result = 2;
    }

    usbsid->USBSID_SetStereo(system.interface->stats.stereoSound);
    // check emulation/libc64/vicII/base.cpp -> setModel() for line and frame cycles
    usbsid->USBSID_SetClockRate(system.vicII->frequency(), true);
    rasterRate = usbsid->USBSID_GetRasterRate();
    sysTimer.add( &flush, rasterRate, Emulator::SystemTimer::UpdateExisting );
    lastClock = sysTimer.clock;
#endif
    return result;
}

auto USBSIDPico::close() -> void {
#ifdef LIBUSB
    if (usbsid) {
        sysTimer.remove(&flush);
        usbsid->USBSID_Mute();
        delete usbsid;  /* Executes usbsid->USBSID_Close(); */
    }
#endif
}

auto USBSIDPico::setBuffSize(unsigned value) -> void {
#ifdef LIBUSB
    if (!usbsid || (value == buffSize))
        return;

    buffSize = value;
    usbsid->USBSID_SetBufferSize(buffSize);
    usbsid->USBSID_RestartRingBuffer();
#endif
}

auto USBSIDPico::setDiffSize(unsigned value) -> void {
#ifdef LIBUSB
    if (!usbsid || (value == diffSize))
        return;

    diffSize = value;
    usbsid->USBSID_SetDiffSize(diffSize);
#endif
}

auto USBSIDPico::store(uint8_t addr, uint8_t val, int chipNr) -> void {
#ifdef LIBUSB
    unsigned cycles = (sysTimer.clock - lastClock);
    cycles = ((cycles > 0) ? (cycles - 1) : cycles);
    if (usbsid)
        usbsid->USBSID_WriteRingCycled(addr + (chipNr * 0x20), val, cycles);
    lastClock = sysTimer.clock;
#endif
}

auto USBSIDPico::reset() -> void {
#ifdef LIBUSB
    if (usbsid)
        usbsid->USBSID_ResetRingBuffer();
        usbsid->USBSID_ResetAllRegisters();
        usbsid->USBSID_Reset();
        usbsid->USBSID_UnMute();
#endif
}

auto USBSIDPico::updateStereo() -> void {
#ifdef LIBUSB
    if (usbsid)
        usbsid->USBSID_SetStereo(system.interface->stats.stereoSound);
#endif
}

auto USBSIDPico::serialize(Emulator::Serializer& s) -> void {
    s.integer(enabled);
    unsigned _buffSizeBefore = buffSize;
    s.integer(buffSize);
    s.integer(diffSize);
#ifdef LIBUSB
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
    }
#endif
}

}
