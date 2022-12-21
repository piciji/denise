
#include "system.h"
#include "../input/input.h"
#include "../../tools/sanitizer.h"
#include  "../../tools/macros.h"
#include "serialization.cpp"

namespace LIBAMI {

System* system = nullptr;

System::System(Interface* interface) :
cia1(1),
cia2(2),
cpu(agnus),
denise(agnus, input),
disks { {agnus}, {agnus}, {agnus}, {agnus} },
paula(agnus, cpu, input, disks[0], disks[1], disks[2], disks[3]),
agnus(cpu, denise, paula, cia1, cia2, input),
input(agnus, cia1, interface) {

    this->interface = interface;

    cia1.serialOut = [this](bool spLine, bool cntLine) {
        // Keyboard computer is not interested in CNT line changes, triggered by CIA
        input.keyboard.handshake(spLine);
    };


    cia1.readPort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA )
            return (uint8_t)(input.readCiaPortA( ) & lines->ioa);

        return (uint8_t)0xff;
    };

    cia1.writePort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA ) {
         //   if (lines->ioa != lines->ioaOld)
            if ((lines->ioa ^ lines->ioaOld) & 1)
                agnus.setOVL(lines->ioa & 1);

            paula.setLedFilter(lines->ioa & 2);

        } else {
            //if (lines->iob != lines->iobOld)

        }
    };

    cia1.irqCall = [this]( bool state ) {
        paula.setInt2(state);
    };

    cia2.writePort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA ) {
            cia2.setCNTAndSP( lines->ioa & 2, lines->ioa & 1 );
        }
    };

    cia2.irqCall = [this]( bool state ) {
        paula.setInt6(state);
    };

    crop.monitorBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {
        left = 21; // 384 CRT monitor, 342 CRT TV
        right = 21;

        top = agnus.ntsc ? 5 : 7;
        bottom = agnus.ntsc ? 1 : 14;

        if (denise.hiresFrame) {
            left <<= 1;
            right <<= 1;
        }
        if (denise.useInterlace & 0x80) {
            top <<= 1;
            bottom <<= 1;
        }
    };

    crop.removeBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {
        left = denise.crop.left;
        right = denise.crop.right;
    };
}

auto System::power(bool softReset, bool resetInstruction) -> void {

    agnus.power(softReset);

    if (!resetInstruction) {
        if (!softReset) {
            calcSerializationSize();

            cpu.power();

            fastForward.config = 0;
            fastForward.frameCounter = 0;
            fastForward.renderNext = false;

        } else {
            cpu.reset();
        }
    }

    cia1.reset();
    cia2.reset();
    input.reset();

    powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
    agnus.powerOff();
    updateStats();
}

auto System::run() -> void {
    leaveEmulation = false;
    runAhead.pos = 0;

    input.initFrame();

    if (agnus.resetFromKeyboard)
        agnus.waitKeyboardReset();

    bool useRunAhead = !fastForward.config && runAhead.frames && agnus.womLocked();

    if (useRunAhead) {
        runAhead.pos = runAhead.frames;
        denise.disableSequencer( runAhead.performance );
        paula.disableAudioOut( runAhead.frames > 1 );
    }

    labelRunAhead:

    while( !leaveEmulation )
        cpu.process();

    if (useRunAhead) {
        if (runAhead.frames == runAhead.pos) {
            serializeLight();
        }

        if (runAhead.pos) {
            if (runAhead.pos == 2)
                paula.disableAudioOut(false);

            if (--runAhead.pos == 0) {
                if (!denise.useSequencer()) {
                    denise.disableSequencer(false);
                }
            }
            leaveEmulation = false;
            goto labelRunAhead;
        }

        unserializeLight();
    }

    agnus.setEventInactive<Agnus::EVENT_LEAVE_EMULATION>();
}

auto System::informAboutKeyUpdate() -> void {
    input.sampling.emergencyPolling = true; // call from another thread
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
            agnus.kickRom = data;
            agnus.kickRomMask = size - 1;
            break;
        case 1:
            agnus.extRom = data;
            agnus.extRomMask = size - 1;
            break;
    }
}

auto System::videoRefresh( uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t interlace) -> void {
    if (!runAhead.pos && frame) {
        crop.apply( frame, width, height, linePitch );
        // for lightguns
        // input.drawCursor();
    }

    if (fastForward.config & (unsigned)Interface::FastForward::NoVideoOut)
        frame = nullptr;

    else if (fastForward.renderNext) {
        fastForward.renderNext = false;
        denise.disableSequencer( fastForward.config & (unsigned)Interface::FastForward::NoVideoSequencer );

    } else if (fastForward.config & (unsigned)Interface::FastForward::ReduceVideoOutput) {
        frame = nullptr;

        if ((++fastForward.frameCounter & 15) == 0) {
            fastForward.frameCounter = 0;
            denise.disableSequencer( false );
            fastForward.renderNext = true;
        }
    }

    if (!runAhead.pos) {
        this->interface->videoRefresh(frame, width, height, linePitch, interlace);
    }

    leaveEmulation = true;
}

auto System::audioRefresh(int16_t left, int16_t right) -> void {
    if (!runAhead.pos) {
        this->interface->audioSample(left, right);
    }
}

auto System::videoMidScreenCallback() -> void {
    if (runAhead.pos)
        return;

  //  input.drawCursor(true);

    interface->midScreenCallback();
}

auto System::setModel(uint8_t model) -> void {

    if (model == 0) {
        agnus.model = Agnus::Model::OCS_A1000;
        denise.model = Denise::Model::OCS_A1000;
    } else if (model == 1) {
        agnus.model = Agnus::Model::OCS;
        denise.model = Denise::Model::OCS;
    } else if (model == 2) {
        agnus.model = Agnus::Model::ECS;
        denise.model = Denise::Model::OCS;
    }
}

auto System::getModel() -> uint8_t {
    if (agnus.model == Agnus::Model::OCS_A1000)
        return 0;
    if (agnus.model == Agnus::Model::OCS)
        return 1;
    if (agnus.model == Agnus::Model::ECS && denise.model == Denise::Model::OCS)
        return 2;

    return 2;
}

auto System::updateStats() -> void {
    interface->stats.region = agnus.ntsc ? Interface::Region::Ntsc : Interface::Region::Pal;
    interface->stats.sampleIntervall = paula.sampleLimit;
    interface->stats.sampleRate = (double)agnus.frequency() / (double)paula.sampleLimit;
    interface->stats.fps = agnus.fps;
    interface->stats.stereoSound = true;
}

auto System::hintSlowSpeed(bool state) -> void {
    if (state)
        fastForward.config |= (unsigned)Interface::FastForward::SlowSpeed;
    else
        fastForward.config &= ~(unsigned)Interface::FastForward::SlowSpeed;
}

auto System::setRegion( Interface::Region region ) -> void {
    agnus.ntsc = region == Interface::Region::Ntsc;
    paula.setFilter();
    agnus.resetFps();
    updateStats();
}

auto System::setResampleQuality( int value ) -> void {
    paula.setResampleQuality( value );
    updateStats();
}

auto System::setFastForward( unsigned config ) -> void {
    fastForward.config = config | (fastForward.config & (unsigned)Interface::FastForward::SlowSpeed);
    paula.disableAudioOut(config & (unsigned) Emulator::Interface::FastForward::NoAudioOut);
    denise.disableSequencer(config & (unsigned) Emulator::Interface::FastForward::NoVideoSequencer);
    //disk.setFastForward(config & (unsigned) Emulator::Interface::FastForward::NoVideoSequencer);
   // updateDriveSounds();
}

auto System::setChipmem(unsigned value) -> void {
    unsigned size;

    switch(value) {
        case 0: size = 256 * 1024; break;
        default:
        case 1: size = 512 * 1024; break;
        case 2: size = 1024 * 1024; break;
        case 3: size = 2048 * 1024; break;
    }

    agnus.setChipmem(size);
}

auto System::getChipmem() -> unsigned {
    switch (agnus.chipMemMask + 1) {
        case 256 * 1024: return 0;
        case 512 * 1024: return 1;
        case 1024 * 1024: return 2;
        case 2048 * 1024: return 3;
    }
    return 1;
}

auto System::setSlowmem(unsigned value) -> void {
    unsigned size;

    switch(value) {
        default:
        case 0: size = 0; break;
        case 1: size = 512 * 1024; break;
        case 2: size = 1024 * 1024; break;
        case 3: size = 1536 * 1024; break;
        case 4: size = 1792 * 1024; break;
    }

    agnus.setSlowmem(size);
}

auto System::getSlowmem() -> unsigned {
    switch (agnus.slowMemSize) {
        case 0: return 0;
        case 512 * 1024: return 1;
        case 1024 * 1024: return 2;
        case 1536 * 1024: return 3;
        case 1792 * 1024: return 4;
    }
    return 0;
}

}
