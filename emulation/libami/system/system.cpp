
#include "system.h"
#include "../interface.h"
#include "../input/input.h"
#include "../../tools/sanitizer.h"
#include  "../../tools/macros.h"
#include "serialization.cpp"
#include "dongle.cpp"
#include "../expansionPort/builtinHD.h"

namespace LIBAMI {

System::System(Interface* interface) :
interface(interface),
cia1(1),
cia2(2),
cpu(agnus),
denise(this, agnus, input),
diskDrives { {0, this, agnus, cia2}, {1, this, agnus, cia2}, {2, this, agnus, cia2}, {3, this, agnus, cia2} },
hardDrives { {0, this, agnus}, {1, this, agnus}, {2, this, agnus}, {3, this, agnus} },
paula(this, agnus, cpu, input, diskDrives[0], diskDrives[1], diskDrives[2], diskDrives[3]),
agnus(this, cpu, denise, paula, cia1, cia2, input, rtc),
input(this, agnus, cia1),
rtc(agnus) {

    cia1.logOut = [this, interface](const char* info, bool newLine , bool hex ) {
        interface->log("CIA1:" + (std::string)info, newLine);
    };

    cia2.logOut = [this, interface](const char* info, bool newLine , bool hex ) {
        interface->log("CIA2:" + (std::string)info, newLine);
    };

    cia1.serialOut = [this](bool spLine, bool cntLine) {
        // Keyboard computer is not interested in CNT line changes, triggered by CIA
        input.keyboard.handshake(spLine);
    };


    cia1.readPort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {
        uint8_t out;
        if ( port == Cia<MOS_8520>::PORTA ) {
            out = input.readCia();

            for(auto& drive : diskDrives) {
                if (drive.connected)
                    out &= drive.readCiaPortA();
            }

            out = (lines->pra & lines->ddra) | (out & ~lines->ddra);
            return out;
        }

        out = 0xff;
        input.readParallelportCIA1B(out);
        out = (lines->prb & lines->ddrb) | (out & ~lines->ddrb);
        return out;
    };

    cia1.writePort = [this, interface]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA ) {
            if ((lines->ioa ^ lines->ioaOld) & 1)
                agnus.setOVL(lines->ioa & 1);

            if ((lines->ioa ^ lines->ioaOld) & 2) {
                paula.setLedFilter((lines->ioa & 2) == 0 );
            }

            if ((lines->ioa ^ lines->ioaOld) & 0x40) {
                input.writeCiaPort1(lines->ioa & 0x40);
            }

            if ((lines->ioa ^ lines->ioaOld) & 0x80) {
                input.writeCiaPort2(lines->ioa & 0x80);
            }

            if (dongle.connected())
                dongleCiaWrite<false>(lines);
        } else {
            // parallel port
        }
    };

    cia1.irqCall = [this]( bool state ) {
        paula.scheduleIntreqCia1(state);
    };

    cia2.readPort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA ) {
            uint8_t out = lines->ioa;
            if (dongle.connected())
                dongleCiaRead<true>(lines, out);

            input.readParallelportCIA2A(out);
            return out;
        }

        return lines->iob;
    };

    cia2.writePort = [this]( Cia<MOS_8520>::Port port, Cia<MOS_8520>::Lines* lines ) {

        if ( port == Cia<MOS_8520>::PORTA ) {
            if (dongle.connected())
                dongleCiaWrite<true>(lines);

            cia2.setCNTAndSP( lines->ioa & 2, lines->ioa & 1 );

        } else if (lines->iob != lines->iobOld) {
            DiskDrive* active = nullptr;
            for(auto& drive : diskDrives) {
                if (drive.connected) {
                    drive.writeCiaPortB(lines->iob, lines->iobOld);
                    if (!active && drive.selected)
                        active = &drive;
                }
            }
            if (!active)
                active = &diskDrives[0];

            paula.setActiveDrive(active);
        }
    };

    cia2.irqCall = [this]( bool state ) {
        paula.scheduleIntreqCia2(state);
    };

    crop.monitorBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {
        left = 31; // 384 CRT monitor, 344 CRT TV
        right = 9;

        if (denise.hiresFrame) {
            left <<= 1;
            right <<= 1;
        }

    //    bool _ntsc = this->interface->stats.isNtsc();

        top = 7;
        bottom = 2;

        if (agnus.laceFrame & 3) {
            top <<= 1;
            bottom <<= 1;
        }
    };

    crop.removeBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {
        left = agnus.crop.left;
        right = agnus.crop.right;
        top = agnus.crop.top;
        bottom = agnus.crop.bottom;
    };

    paula.activeDrive = &diskDrives[0];
    ntsc = false;
    firmwareChanged = true;
    dongle.type = DongleNone;
    dongle.control = 0;
    dongle.clock = 0;
}

System::~System() {
    DiskStructure::destroyIPF();
}

auto System::power(bool softReset, bool resetInstruction) -> void {
    crop.latest.frame = nullptr;
    agnus.power(softReset, resetInstruction);

    if (!resetInstruction) {
        if (!softReset) {
            calcSerializationSize();

            cpu.power();

            warp.config = 0;
            warp.frameCounter = 0;
            warp.renderNext = false;

        } else {
            cpu.reset();
        }
    }

    denise.power(resetInstruction || softReset);
    paula.power();
    cia1.reset();
    cia2.reset();
    input.reset();
    rtc.reset(softReset);

    for(auto& drive : diskDrives)
        drive.power();

    for (auto& drive : hardDrives)
        drive.power(resetInstruction || softReset);

    if (resetInstruction || softReset) {
        agnus.resetFps();
        updateStats();
        interface->fpsChanged();
    }

    dongle.control = 0;
    dongle.clock = 0;
    powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
    agnus.powerOff();
    for(auto& drive : diskDrives)
        drive.powerOff();
    updateStats();
}

auto System::run() -> void {
    leaveEmulation = false;
    runAhead.pos = 0;

    input.initFrame();

    if (agnus.resetFromKeyboard)
        agnus.waitKeyboardReset();

    bool useRunAhead = allowRunAhead();

    if (useRunAhead) {
        runAhead.pos = runAhead.frames;
        denise.setDisableSequencer( runAhead.performance ? 1 : 2 );
        paula.disableAudioOut( runAhead.frames > 1 );

        agnus.updateEvent<Agnus::EVENT_LEAVE_EMULATION>(((227 * 312) * (runAhead.frames + 1)) + 30000);
    } else
        agnus.updateEvent<Agnus::EVENT_LEAVE_EMULATION>(227 * 312 + 30000); // Blitter could block CPU too long

    labelRunAhead:

    while( !leaveEmulation ) {
        cpu.process();
#ifdef LOG_CPU_STATE
        cpu.logState();
#endif
    }

    denise.process(); // keep up, so we don't need to serialize BplUpdate
    if (useRunAhead) {
        if (runAhead.frames == runAhead.pos) {
            serializeLight();
        }

        if (runAhead.pos) {
            if (runAhead.pos == 2)
                paula.disableAudioOut(false);

            if (--runAhead.pos == 0) {
                if (!denise.useSequencer()) {
                    denise.setDisableSequencer(0);
                }
            }
            leaveEmulation = false;
            goto labelRunAhead;
        }

        unserializeLight();
    }

    DiskDrive::randomizeRpm(agnus.frequency(), paula.turbo);
    for(auto& drive : diskDrives)
        drive.updateRpm();

    if (observer.stateChange)
        informAboutStateChange();

    //agnus.setEventInactive<Agnus::EVENT_LEAVE_EMULATION>();
}

auto System::informAboutKeyUpdate() -> void {
    input.sampling.externalKeyEvent = true; // call from another thread
}

auto System::setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void {
    firmwareChanged |= allowPatching;

    if (size > ((512 * 1024) + 11)) // + possible 11 byte encryption header
        size = 512 * 1024 + 11;

    if (!data || !size) {
        data = nullptr;
        size = 0;
    }

    switch (typeId) {
        case 0:
        default:
            agnus.kickRom = data;
            agnus.kickRomSize = size;
            agnus.kickRomMask = size ? (Emulator::powerOfTwo( size ) - 1) : 0;
            break;
        case 1:
            agnus.extRom = data;
            agnus.extRomSize = size;
            agnus.extRomMask = size ? (Emulator::powerOfTwo( size ) - 1) : 0;
            break;
    }
}

auto System::videoRefresh( uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t options) -> void {
    if (!runAhead.pos && frame) {
        crop.apply( frame, width, height, linePitch, options & 7 );
        // for lightguns
        // input.drawCursor();
    }

    if (warp.config & (unsigned)Interface::WarpMode::NoVideoOut)
        frame = nullptr;

    else if (warp.renderNext) {
        warp.renderNext = false;
        denise.setDisableSequencer( (warp.config & (unsigned)Interface::WarpMode::NoVideoSequencer) ? 1 : 2 );

    } else if (warp.config & (unsigned)Interface::WarpMode::ReduceVideoOutput) {
        if ((options & 0xc0) == 0)
            frame = nullptr;

        if ((++warp.frameCounter & 15) == 0) {
            warp.frameCounter = 0;
            denise.setDisableSequencer( 0 );
            warp.renderNext = true;
        }
    }

    if (!runAhead.pos) {
        this->interface->videoRefresh(frame, width, height, linePitch, options & 7);
    }

    leaveEmulation = true;
}

auto System::audioRefresh(int16_t left, int16_t right) -> void {
    if (!runAhead.pos) {
        this->interface->audioSample(left, right);
    }
}

auto System::videoMidScreenCallback(uint8_t options) -> void {
    if (runAhead.pos)
        return;

  //  input.drawCursor(true);

    interface->midScreenCallback(options & 7);
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
    interface->stats.sampleIntervall = paula.sampleLimit;
    interface->stats.sampleRate = (double)agnus.frequency() / (double)paula.sampleLimit;
    interface->stats.fps = agnus.fps;
    interface->stats.stereoSound = true;

    // when software force a PAL Amiga to output NTSC and vice versa
    interface->stats.region = (agnus.fps > 59.0) ? Interface::Region::Ntsc : Interface::Region::Pal;
}

auto System::hintSlowSpeed(bool state) -> void {
    if (state)
        warp.config |= (unsigned)Interface::WarpMode::SlowSpeed;
    else
        warp.config &= ~(unsigned)Interface::WarpMode::SlowSpeed;
}

auto System::setRegion( int region ) -> void {
    ntsc = (Interface::Region)region == Interface::Region::Ntsc;
    agnus.ntsc = ntsc;
    paula.setFilter();
    agnus.resetFps();
    updateStats();
}

auto System::setResampleQuality( int value ) -> void {
    paula.setResampleQuality( value );
    updateStats();
}

auto System::setRunAhead(unsigned frames) -> void {
    runAhead.frames = frames;
    input.updateSampling();
    updateDriveSounds();
}

auto System::setWarpMode( unsigned config ) -> void {
    warp.config = config | (warp.config & (unsigned)Interface::WarpMode::SlowSpeed);
    paula.disableAudioOut(config & (unsigned) Emulator::Interface::WarpMode::NoAudioOut);

    if (config & (unsigned) Emulator::Interface::WarpMode::NoVideoSequencer) denise.setDisableSequencer( 1 );
    else if (config & (unsigned) Emulator::Interface::WarpMode::ReduceVideoOutput) denise.setDisableSequencer( 2 );
    else denise.setDisableSequencer( 0 );

    warp.frameCounter = 0;
    warp.renderNext = false;

    updateDriveSounds();
}

auto System::setFloppySounds(bool state) -> void {
    requestFloppySound = state;
    updateDriveSounds();
}

auto System::updateDriveSounds() -> void {
    bool state = requestFloppySound && !warp.config;

    for(auto& drive : diskDrives)
        drive.enableSounds(state);
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

auto System::setFastmem(unsigned value) -> void {
    unsigned size;

    switch(value) {
        default:
        case 0: size = 0; break;
        case 1: size = 64 * 1024; break;
        case 2: size = 128 * 1024; break;
        case 3: size = 256 * 1024; break;
        case 4: size = 512 * 1024; break;
        case 5: size = 1024 * 1024; break;
        case 6: size = 2048 * 1024; break;
        case 7: size = 4096 * 1024; break;
        case 8: size = 8192 * 1024; break;
    }

    agnus.setFastmem(size);
}

auto System::getFastmem() -> unsigned {
    switch (agnus.fastMemSize) {
        case 0: return 0;
        case 64 * 1024: return 1;
        case 128 * 1024: return 2;
        case 256 * 1024: return 3;
        case 512 * 1024: return 4;
        case 1024 * 1024: return 5;
        case 2048 * 1024: return 6;
        case 4096 * 1024: return 7;
        case 8192 * 1024: return 8;
    }
    return 0;
}

auto System::setDiskDrivesEnabled( uint8_t count ) -> void {
    for( auto& drive : diskDrives )
        drive.connected = drive.number < count;
}

auto System::getDiskDrivesEnabled() -> uint8_t {
    uint8_t out = 0;
    for( auto& drive : diskDrives )
        if (drive.connected) out++;

    return out;
}

auto System::setHardDrivesEnabled(uint8_t count) -> void {
    for (auto& drive : hardDrives)
        drive.connected = drive.number < count;
}

auto System::getHardDrivesEnabled() -> uint8_t {
    uint8_t out = 0;
    for (auto& drive : hardDrives)
        if (drive.connected) out++;

    return out;
}

auto System::observeInputFetches() -> void {
    if (observer.inputFetches) {
        if (!--observer.inputFetches)
            observer.stateChange = true;
    }
}

auto System::hintObserverMotorChange(bool state) -> void {
    if (!state) {
        for(auto& drive : diskDrives) {
            if (drive.connected && drive.motor && drive.selected) {
                state = true;
                break;
            }
        }
    }

    if (state && !observer.motor)
        observer.inputFetches = 15;

    observer.motor = state;
    observer.stateChange = true;
}

auto System::informAboutStateChange() -> void {
    observer.stateChange = false;
    uint8_t newState = observer.motor;
    if (!observer.inputFetches)
        newState |= 2;

    interface->hintAutoWarp( newState );
}

auto System::setRTC(bool state) -> void {
    agnus.useRTC = state;
}

auto System::useRTC() -> bool {
    return agnus.useRTC;
}

auto System::setOverclock( unsigned factor ) -> void {
    agnus.overclock.cycles = 0;

    switch(factor) {
        case 0: agnus.overclock.speed = 0; break;
        case 1: agnus.overclock.speed = 4; break;
        case 2: agnus.overclock.speed = 8; break;
        case 3: agnus.overclock.speed = 16; break;
    }
}

auto System::getOverclock() -> unsigned {
    switch (agnus.overclock.speed) {
        default:
        case 0: return 0;
        case 4: return 1;
        case 8: return 2;
        case 16: return 3;
    }
}

auto System::setHDDAsync(bool state) -> void {
    asyncHDDAccess = state;
}

auto System::getHDDAsync() -> bool {
    return asyncHDDAccess;
}

template auto System::dongleJoydat<false>(uint16_t& val) -> void;
template auto System::dongleJoydat<true>(uint16_t& val) -> void;

}
