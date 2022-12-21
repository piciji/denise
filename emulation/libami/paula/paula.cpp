
#include "paula.h"
#include "../agnus/agnus.h"
#include "../cpu/m68000.h"
#include "audio.cpp"
#include "filter.cpp"
#include "drive.cpp"
#include "../system/system.h"
#include "../input/input.h"
#include "../input/controlPort/controlPort.h"
#include "../../tools/clamp.h"

#define INT2_1 1
#define INT2_2 2
#define INT2_3 4
#define INT2_4 8
#define INT2_5 0x10

#define INT3_1 0x20
#define INT3_2 0x40
#define INT3_3 0x80
#define INT3_4 0x100
#define INT3_5 0x200

#define INT6_1 0x400
#define INT6_2 0x800
#define INT6_3 0x1000
#define INT6_4 0x2000
#define INT6_5 0x4000

#define INT_VBL_1 0x8000
#define INT_VBL_2 0x10000
#define INT_VBL_3 0x20000
#define INT_VBL_4 0x40000

#define INT_UPD_1 0x80000
#define INT_UPD_2 0x100000

#define INT_AUD0_1 0x200000
#define INT_AUD0_2 0x400000
#define INT_AUD0_3 0x800000
#define INT_AUD0_4 0x1000000

#define INT_AUD1_1 0x2000000
#define INT_AUD1_2 0x4000000
#define INT_AUD1_3 0x8000000
#define INT_AUD1_4 0x10000000

#define INT_AUD2_1 0x20000000
#define INT_AUD2_2 0x40000000
#define INT_AUD2_3 0x80000000
#define INT_AUD2_4 0x100000000

#define INT_AUD3_1 0x200000000
#define INT_AUD3_2 0x400000000
#define INT_AUD3_3 0x800000000
#define INT_AUD3_4 0x1000000000

#define INT_MASK (INT3_1 | INT6_1 | INT_VBL_1 | INT_UPD_1 | INT_AUD0_1 | INT_AUD1_1 | INT_AUD2_1 | INT_AUD3_1)

// todo: To determine the correct interrupt delay within Paula, it must be taken into account that the CPU only tests for interrupts in certain DMA cycles.
// The CPU emulation synchronizes in multiples of DMA cycles (= 2 cycles). The limit of when an interrupt is detected or not happens after an odd number of CPU cycles.
// If the interrupt happens in the 2nd half of the DMA cycle, it is not recognized, although the CPU evaluates IPL in this DMA cycle.
// Synchronization after each CPU cycle slows down emulation. It is more performant to change the IPL pins one DMA cycle later.

namespace LIBAMI {

Paula::Paula(Agnus& agnus, Cpu& cpu, Input& input, Disk& disk0, Disk& disk1, Disk& disk2, Disk& disk3) :
agnus(agnus),
drives { {disk0}, {disk1}, {disk2}, {disk3} },
cpu(cpu),
input(input) {

    callbackStateMachine = [&](uint8_t job, uint16_t data) {
        Channel& cha0 = channels[0];
        Channel& cha1 = channels[1];
        Channel& cha2 = channels[2];
        Channel& cha3 = channels[3];
        uint64_t clock = agnus.clock;

        if (cha0.clock == clock) stateMainloop<0>();
        if (cha1.clock == clock) stateMainloop<1>();
        if (cha2.clock == clock) stateMainloop<2>();
        if (cha3.clock == clock) stateMainloop<3>();
    };

    agnus.addEvent<Agnus::EVENT_AUDIO_STATE>( &callbackStateMachine );

    sampleLimit = 0;
    enableFilter = true;
}

auto Paula::dmal() -> uint16_t {
    uint16_t out = 0;

    for(uint8_t nr = 0; nr < 4; nr++) {
        Channel& cha = channels[nr];

        if (cha.dr) {
            out |= 1 << (nr << 1);
            cha.dr = false;
        }

        if (cha.dsr) {
            out |= 1 << ((nr << 1) + 1);
            cha.dsr = false;
        }
    }
    out <<= 6;

    return out;
}

auto Paula::setInt2(bool state) -> void { // CIA 1
    if (state && !int2Current)
        irqDelay |= INT2_1;

    int2Current = state;
}

auto Paula::setInt6(bool state) -> void { // CIA 2
    if (state && !int6Current)
        irqDelay |= INT6_1;

    int6Current = state;
}

template<uint8_t nr> auto Paula::setIntAudMoreDelayed() -> void {
    if constexpr (nr == 0)
        irqDelay |= INT_AUD0_1;
    else if constexpr (nr == 1)
        irqDelay |= INT_AUD1_1;
    else if constexpr (nr == 2)
        irqDelay |= INT_AUD2_1;
    else if constexpr (nr == 3)
        irqDelay |= INT_AUD3_1;
}

template<uint8_t nr> auto Paula::setIntAud() -> void {
    if constexpr (nr == 0)
        irqDelay |= INT_AUD0_2;
    else if constexpr (nr == 1)
        irqDelay |= INT_AUD1_2;
    else if constexpr (nr == 2)
        irqDelay |= INT_AUD2_2;
    else if constexpr (nr == 3)
        irqDelay |= INT_AUD3_2;
}

auto Paula::pulseInt3() -> void { // Blitter (Agnus generates one DMA cycle pulse)
    irqDelay |= INT3_1;
}

auto Paula::strhor() -> void {
    if (vBlankIntr)
        vBlankIntr = false;

    progressPot();
}

auto Paula::strequ() -> void {
    if (!vBlankIntr) {
        vBlankIntr = true;
        irqDelay |= INT_VBL_1;
    }
    progressPot();
}

auto Paula::strvbl() -> void {
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

    irqDelay |= INT_UPD_1;
}

auto Paula::setIntreq(uint16_t value) -> void {
    if (value & 0x8000)
        intreq |= value & 0x7fff;
    else
        intreq &= ~value;

    irqDelay |= INT_UPD_1;
}

#define POT_DIR_X0 0x200
#define POT_DIR_Y0 0x800
#define POT_DIR_X1 0x2000
#define POT_DIR_Y1 0x8000

#define POT_DAT_X0 0x100
#define POT_DAT_Y0 0x400
#define POT_DAT_X1 0x1000
#define POT_DAT_Y1 0x4000

auto Paula::pot0Dat() -> uint16_t {
    return (pot.cntY0 << 8) | pot.cntX0;
}

auto Paula::pot1Dat() -> uint16_t {
    return (pot.cntY1 << 8) | pot.cntX1;
}

auto Paula::potGoR() -> uint16_t {
    uint16_t out = 0;
    if (pot.capX0 == 255) out |= POT_DAT_X0;
    if (pot.capY0 == 255) out |= POT_DAT_Y0;
    if (pot.capX1 == 255) out |= POT_DAT_X1;
    if (pot.capY1 == 255) out |= POT_DAT_Y1;

    return out;
}

auto Paula::progressPot() -> void {
    uint16_t sum;

    if (pot.dischargeCounter) {
        if (--pot.dischargeCounter)  {
            // probably not fully discharged after a single line
            if ((pot.go & POT_DIR_X0) == 0) pot.capX0 = 0;
            if ((pot.go & POT_DIR_Y0) == 0) pot.capY0 = 0;
            if ((pot.go & POT_DIR_X1) == 0) pot.capX1 = 0;
            if ((pot.go & POT_DIR_Y1) == 0) pot.capY1 = 0;
        }
    } else {
        // charge unit is not connected in output mode
        if (pot.capX0 != 255) {
            pot.cntX0++;
            if ((pot.go & POT_DIR_X0) == 0) {
                sum = pot.capX0 + input.controlPort1->getPotX();
                pot.capX0 = sum > 0xff ? 0xff : sum;
            }
        }
        if (pot.capY0 != 255) {
            pot.cntY0++;
            if ((pot.go & POT_DIR_Y0) == 0) {
                sum = pot.capY0 + input.controlPort1->getPotY();
                pot.capY0 = sum > 0xff ? 0xff : sum;
            }
        }
        if (pot.capX1 != 255) {
            pot.cntX1++;
            if ((pot.go & POT_DIR_X1) == 0) {
                sum = pot.capX1 + input.controlPort2->getPotX();
                pot.capX1 = sum > 0xff ? 0xff : sum;
            }
        }
        if (pot.capY1 != 255) {
            pot.cntY1++;
            if ((pot.go & POT_DIR_Y1) == 0) {
                sum = pot.capY1 + input.controlPort2->getPotY();
                pot.capY1 = sum > 0xff ? 0xff : sum;
            }
        }
    }
}

auto Paula::potGo(uint16_t value) -> void {
    pot.go = value;

    // charge/discharge fast in output mode, hence counter is free running in output + lo and stopped in output + hi
    if (pot.go & POT_DIR_X0) pot.capX0 = (pot.go & POT_DAT_X0) ? 255 : 0;
    if (pot.go & POT_DIR_Y0) pot.capY0 = (pot.go & POT_DAT_Y0) ? 255 : 0;
    if (pot.go & POT_DIR_X1) pot.capX1 = (pot.go & POT_DAT_X1) ? 255 : 0;
    if (pot.go & POT_DIR_Y1) pot.capY1 = (pot.go & POT_DAT_Y1) ? 255 : 0;

    if (value & 1) {
        pot.cntY0 = pot.cntX0 = pot.cntY1 = pot.cntX1 = 0;
        pot.dischargeCounter = agnus.ntsc ? 7 : 8;
    }
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

auto Paula::updateInt() -> void {
    uint8_t level = 0;
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

    cpu.setInterrupt( level );
}

auto Paula::powerOff() -> void {

}

auto Paula::serialize(Emulator::Serializer& s, bool light) -> void {
    s.integer(intena);
    s.integer(intreq);
    s.integer(adkcon);
    s.integer(irqDelay);
    s.integer(int2Current);
    s.integer(int6Current);
    s.integer(vBlankIntr);
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
        s.integer(cha.per);
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
}

auto Paula::power() -> void {
    intena = 0;
    intreq = 0;
    adkcon = 0;
    irqDelay = 0;
    int2Current = false;
    int6Current = false;
    vBlankIntr = true;
    useLedFilter = false;
    pot.cntX0 = 0;
    pot.cntY0 = 0;
    pot.cntX1 = 0;
    pot.cntY1 = 0;
    pot.capX0 = 0;
    pot.capY0 = 0;
    pot.capX1 = 0;
    pot.capY1 = 0;
    pot.go = 0;
    pot.dischargeCounter = 0;

    for(uint8_t c = 0; c < 4; c++) {
        Channel& cha = channels[c];

        cha.dma = false;
        cha.dr = false;
        cha.dsr = false;
        cha.intreq2 = false;
        cha.clock = 0;
        cha.state = 0;
        cha.per = 0;
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
        cha.napnav = false;
    }

    dmaDisk = false;
    audioOut = true;
    filters[0].reset();
    filters[1].reset();

    sampleCycle = agnus.clock + sampleLimit;
}

auto Paula::process() -> void {

    if (irqDelay) {
        uint64_t _irqDelay = irqDelay;
        if (_irqDelay & (INT2_5 | INT3_5 | INT6_5 | INT_VBL_4 | INT_UPD_2 | INT_AUD0_4 | INT_AUD1_4 | INT_AUD2_4 | INT_AUD3_4)) {

            if (_irqDelay & INT2_5) // CIA 1
                intreq |= 8;

            if (_irqDelay & INT_VBL_4)
                intreq |= 0x20;
            if (_irqDelay & INT3_5)
                intreq |= 0x40;
            if (_irqDelay & INT_AUD2_4)
                intreq |= 0x80;
            if (_irqDelay & INT_AUD0_4)
                intreq |= 0x100;
            if (_irqDelay & INT_AUD3_4)
                intreq |= 0x200;
            if (_irqDelay & INT_AUD1_4)
                intreq |= 0x400;
            if (_irqDelay & INT6_5) // CIA 2
                intreq |= 0x2000;

            updateInt();
        }

        irqDelay = (irqDelay << 1) & ~INT_MASK;
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
