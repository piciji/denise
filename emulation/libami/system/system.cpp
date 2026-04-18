
#include "system.h"
#include "../interface.h"
#include "../input/input.h"
#include "../../tools/sanitizer.h"
#include  "../../tools/macros.h"
#include "serialization.cpp"
#include "dongle.cpp"
#include "../expansionPort/builtinHD.h"
#include "../../tools/memory.h"

typedef Emulator::Interface::DebuggerAction DebuggerAction;
typedef Emulator::Interface::DebuggerTheme DebuggerTheme;

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

        if (denise.frameMode == Denise::SHRES_FRAME) {
            left <<= 2;
            right <<= 2;
        } else if (denise.frameMode == Denise::HIRES_FRAME) {
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

    crop.screenshotBorderCallback = [this](unsigned _w, unsigned _h) {
        Emulator::Interface::Crop _c = {0};

        if (_w == 320) {
            _c.left = 45 << (unsigned)denise.frameMode;
            _c.right = 19 << (unsigned)denise.frameMode;
            _c.top = 20;
            _c.bottom = 13;
        } else {
            _c.left = 31 << (unsigned)denise.frameMode;
            _c.right = 9 << (unsigned)denise.frameMode;
            _c.top = 7;
            _c.bottom = 2;
        }

        if (agnus.laceFrame & 3) {
            _c.top <<= 1;
            _c.bottom <<= 1;
        }

        return _c;
    };

    agnus.debugger.crop.removeBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {
        crop.removeBorderCallback(top, bottom, left, right);
    };

    agnus.debugger.crop.monitorBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {
        crop.monitorBorderCallback(top, bottom, left, right);
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
    interface->updateLedState(Emulator::Interface::LedId::Power, agnus.ecsAndHigher() ? 2 : 0);
    history.reset();
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

    runAhead.active = allowRunAhead();

    if (history.enable()) {
        if (history.rewind) {
            runAhead.active = false;
            auto memState = history.apply();
            if (memState) {
                unserializeLight(*memState, true);
                agnus.memState = memState;
            }
        } else {
            auto memState = history.remember();
            if (memState)
                serializeLight(*memState, true);
            else
                agnus.memState = history.getCurMemstate();
        }
    }

    if (runAhead.active) {
        runAhead.pos = runAhead.frames;
        denise.setDisableSequencer( runAhead.performance ? 1 : 2 );
        paula.disableAudioOut( runAhead.frames > 1 );

        agnus.updateEvent<Agnus::EVENT_LEAVE_EMULATION>(((227 * 312) * (runAhead.frames + 1)) + 30000);
    } else
        agnus.updateEvent<Agnus::EVENT_LEAVE_EMULATION>(227 * 312 + 30000); // Blitter could block CPU too long

    if (agnus.debugger.action == DebuggerAction::UIRequestedStop) {
        debuggerUpdate();
    }

    labelRunAhead:

    if (agnus.debugger.dmaLog) {
        while( !leaveEmulation ) {
            cpu.process();
            agnus.logNextOpcode();
        }
    } else {
        while( !leaveEmulation ) {
            cpu.process();
        }
    }

    denise.process(); // keep up, so we don't need to serialize BplUpdate
    if (runAhead.active) {
        if (runAhead.frames == runAhead.pos) {
            serializeLight(runAhead.memState);
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

        unserializeLight(runAhead.memState);
    }

    DiskDrive::randomizeRpm(agnus.frequency(), paula.turbo);
    for(auto& drive : diskDrives)
        drive.updateRpm();

    if (observer.stateChange)
        informAboutStateChange();
}

auto System::debuggerUpdate() -> void {
    agnus.debuggerUpdateEvent();

    debuggerSnapshot.mutex.lock();
    updateDebuggerSnapshot();
    interface->debugger(&debuggerSnapshot); // callback needs to unlock mutex
    agnus.debugger.action = DebuggerAction::None;
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
            Emulator::copyMemory<uint8_t>( agnus.kickRom, agnus.kickRomSize, data, size );
            agnus.kickRomMask = size ? (Emulator::powerOfTwo( size ) - 1) : 0;
            break;
        case 1:
            Emulator::copyMemory<uint8_t>( agnus.extRom, agnus.extRomSize, data, size );
            agnus.extRomMask = size ? (Emulator::powerOfTwo( size ) - 1) : 0;
            break;
    }
}

auto System::videoRefresh( uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t options) -> void {
    if (!runAhead.pos && frame) {
        if (agnus.debugger.dmaLog & Agnus::DmaLogView)
            options |= 0x80;
        crop.apply( frame, width, height, linePitch, options & 0x8f );
        // for lightguns
        // input.drawCursor();
    }

    if (warp.config & (unsigned)Interface::WarpMode::NoVideoOut)
        frame = nullptr;

    else if (warp.renderNext) {
        warp.renderNext = false;
        denise.setDisableSequencer( (warp.config & (unsigned)Interface::WarpMode::NoVideoSequencer) ? 1 : 2 );

    } else if (warp.config & (unsigned)Interface::WarpMode::ReduceVideoOutput) {
        if ((options & 0xc0) == 0) // lace frame toggle
            frame = nullptr;

        if ((++warp.frameCounter & 15) == 0) {
            warp.frameCounter = 0;
            denise.setDisableSequencer( 0 );
            warp.renderNext = true;
        }
    }

    if (!runAhead.pos) {
        this->interface->videoRefresh(frame, width, height, linePitch, options & 0xf);
    }

    leaveEmulation = true;
}

auto System::audioRefresh(int16_t left, int16_t right) -> void {
    if (!runAhead.pos) {
        this->interface->audioSample(left, right);
    }
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
    } else if (model == 3) {
        agnus.model = Agnus::Model::ECS;
        denise.model = Denise::Model::ECS;
    }
}

auto System::getModel() -> uint8_t {
    if (agnus.model == Agnus::Model::OCS_A1000)
        return 0;
    if (agnus.model == Agnus::Model::OCS)
        return 1;
    if (agnus.model == Agnus::Model::ECS && denise.model == Denise::Model::OCS)
        return 2;
    if (agnus.model == Agnus::Model::ECS && denise.model == Denise::Model::ECS)
        return 3;

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
    history.reset();
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

    history.reset();
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

auto System::cropFrame( Emulator::Interface::CropType type, Emulator::Interface::Crop _crop ) -> void {
    crop.settings.type = type;
    crop.settings.crop = _crop;
    agnus.debugger.crop.settings.type = type;
    agnus.debugger.crop.settings.crop = _crop;
}

auto System::debuggerAdd(DebuggerTheme theme, DebuggerAction action, unsigned addr, unsigned addrTo) -> void {
    switch (theme) {
        case DebuggerTheme::Video:
            debuggerSnapshot.themes |= (unsigned)theme;
            denise.debugger.setEnable( true );
            break;
        case DebuggerTheme::Bus:
            switch (action) {
                case DebuggerAction::DmaView:
                    agnus.debugger.enableDmaView(true, addr == 0);
                    break;
                case DebuggerAction::DmaLog:
                    debuggerSnapshot.themes |= (unsigned)theme;
                    agnus.resetDebuggerDma();
                    agnus.debugger.enableDmaLog(true);
                    leaveEmulation = true;
                    break;
                case DebuggerAction::DmaWatch:
                    agnus.debugger.dmaWatchers[addrTo & 3] = addr | (0x80 << 24);
                    break;
                default: break;
            } break;
        case DebuggerTheme::CheckpointsCPU1:
            cpu.debuggerAdd( action, addr, addrTo );
            break;

        case DebuggerTheme::Agnus:
            debuggerSnapshot.themes |= (unsigned)theme;
            break;

        case DebuggerTheme::Copper:
            switch (action) {
                case DebuggerAction::None:
                    agnus.copper.flagDebugAction(Copper::LogList, true);
                    debuggerSnapshot.themes |= (unsigned)theme;
                    break;
                default:
                    agnus.copper.debuggerAdd(action, addr);
                    break;
            } break;

        case DebuggerTheme::Blitter:
            switch (action) {
                case DebuggerAction::None:
                    debuggerSnapshot.themes |= (unsigned)theme;
                    break;
                case DebuggerAction::SoftstopBlitter:
                    agnus.debugger.softStopBlitterDma();
                    break;
                default: break;
            }
            break;

        case DebuggerTheme::Unspecified: {
            switch (action) {
                case DebuggerAction::Line:
                    agnus.debugger.stopLine = addr;
                case DebuggerAction::Frame:
                    agnus.debugger.oneTimeAction = action;
                    break;
                case DebuggerAction::AutoUpdate:
                    if (addr == 1) {
                        updateDebuggerSnapshot();
                    }
                    break;
                case DebuggerAction::UIRequestedStop:
                    agnus.debugger.action = DebuggerAction::UIRequestedStop;
                    break;
                default: break;
            }
        } break;
        default:
            debuggerSnapshot.themes |= (unsigned)theme;
            break;
    }

    agnus.debuggerUpdateEvent();
}

auto System::debuggerRemove(DebuggerTheme theme, DebuggerAction action, std::optional<unsigned> addr) -> void {
    switch (theme) {
        case DebuggerTheme::Video:
            debuggerSnapshot.themes &= ~(unsigned)theme;
            denise.debugger.setEnable( false );
            break;
        case DebuggerTheme::Bus:
            switch (action) {
                case DebuggerAction::DmaView:
                    agnus.debugger.enableDmaView(false, !addr.has_value() || (addr.value_or(0) == 0));
                    break;
                case DebuggerAction::DmaLog:
                    debuggerSnapshot.themes &= ~(unsigned)theme;
                    agnus.debugger.enableDmaLog(false);
                    break;
                case DebuggerAction::DmaWatch:
                    agnus.debugger.dmaWatchers[addr.value_or(0) & 3] = 0;
                    break;
                default: break;
            } break;
        case DebuggerTheme::CheckpointsCPU1:
            if (addr.has_value())
                cpu.debuggerRemove( action, addr.value_or(0) );
            else
                cpu.debuggerRemove( action );
            break;

        case DebuggerTheme::Agnus:
            debuggerSnapshot.themes &= ~(unsigned)theme;
            break;

        case DebuggerTheme::Copper:
            switch (action) {
                case DebuggerAction::None:
                    agnus.copper.flagDebugAction(Copper::LogList, false);
                    debuggerSnapshot.themes &= ~(unsigned)theme;
                    break;
                default:
                    if (addr.has_value())
                        agnus.copper.debuggerRemove( action, addr.value_or(0) );
                    else
                        agnus.copper.debuggerRemove( action );
                    break;
            } break;

        case DebuggerTheme::Blitter:
            debuggerSnapshot.themes &= ~(unsigned)theme;
            break;

        default:
            debuggerSnapshot.themes &= ~(unsigned)theme;
            break;
    }
    agnus.debuggerUpdateEvent();
}

auto System::setWatchpointCondition(DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool {
    return cpu.setWatchpointCondition( action, addr, hitCount, hitCountMode, expression, expressionMode );
}

auto System::editMemory(uint32_t addr, std::vector<uint16_t> values) -> void {
    for (int i = 0; i < values.size(); i++) {
        uint32_t a = (addr + (i * 2) ) & 0xffffff;
        uint16_t v = values[i] & 0xffff;

        agnus.editWord( a, v );
    }
}

auto System::updateDebuggerSnapshot() -> void {
    agnus.updateSnapshot(debuggerSnapshot);

    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::CPU)
        cpu.updateSnapshot(debuggerSnapshot);
    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::Memory)
        agnus.updateMemorySnapshot(debuggerSnapshot);
    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::CIA)
        updateCiaDebuggerSnapshot(debuggerSnapshot);
    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::Video) {
        denise.updateSnapshot(debuggerSnapshot);
        agnus.updateVideoSnapshot( debuggerSnapshot );
    }
    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::Bus)
        agnus.updateDmaSnapshot( debuggerSnapshot );
    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::Copper)
        agnus.copper.updateDmaSnapshot( debuggerSnapshot );
    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::Blitter)
        agnus.blitter.updateDmaSnapshot( debuggerSnapshot );
    if (debuggerSnapshot.themes & (unsigned)DebuggerTheme::Agnus)
        agnus.updatePtrSnapshot( debuggerSnapshot );
}

auto System::updateCiaDebuggerSnapshot(DebuggerSnapshot& snap) -> void {
    auto& c1p0 = snap.cia[0].port[0];
    c1p0.pr = cia1.lines.pra;
    c1p0.ddr = cia1.lines.ddra;
    c1p0.io = cia1.peek( 0 );
    c1p0.timer = cia1.timerA.counter;
    c1p0.timerLatch = cia1.timerA.latch;
    c1p0.oneshot = cia1.timerA.oneshot;
    c1p0.pbOut = cia1.timerA.control & 2;
    c1p0.toggleOut = cia1.timerA.control & 4;
    c1p0.timerRunning = !!cia1.timerA.run;

    auto& c1p1 = snap.cia[0].port[1];
    c1p1.pr = cia1.lines.prb;
    c1p1.ddr = cia1.lines.ddrb;
    c1p1.io = cia1.peek( 1 );
    c1p1.timer = cia1.timerB.counter;
    c1p1.timerLatch = cia1.timerB.latch;
    c1p1.oneshot = cia1.timerB.oneshot;
    c1p1.pbOut = cia1.timerB.control & 2;
    c1p1.toggleOut = cia1.timerB.control & 4;
    c1p1.timerRunning = !!cia1.timerB.run;

    auto& c2p0 = snap.cia[1].port[0];
    c2p0.pr = cia2.lines.pra;
    c2p0.ddr = cia2.lines.ddra;
    c2p0.io = cia2.peek( 0 );
    c2p0.timer = cia2.timerA.counter;
    c2p0.timerLatch = cia2.timerA.latch;
    c2p0.oneshot = cia2.timerA.oneshot;
    c2p0.pbOut = cia2.timerA.control & 2;
    c2p0.toggleOut = cia2.timerA.control & 4;
    c2p0.timerRunning = !!cia2.timerA.run;

    auto& c2p1 = snap.cia[1].port[1];
    c2p1.pr = cia2.lines.prb;
    c2p1.ddr = cia2.lines.ddrb;
    c2p1.io = cia2.peek( 1 );
    c2p1.timer = cia2.timerB.counter;
    c2p1.timerLatch = cia2.timerB.latch;
    c2p1.oneshot = cia2.timerB.oneshot;
    c2p1.pbOut = cia2.timerB.control & 2;
    c2p1.toggleOut = cia2.timerB.control & 4;
    c2p1.timerRunning = !!cia2.timerB.run;

    snap.cia[0].icr = cia1.icr;
    snap.cia[0].icrMask = cia1.icrmask;
    snap.cia[1].icr = cia2.icr;
    snap.cia[1].icrMask = cia2.icrmask;

    snap.cia[0].tod = cia1.todc;
    snap.cia[0].todAlarm = cia1.alarm;
    snap.cia[1].tod = cia2.todc;
    snap.cia[1].todAlarm = cia2.alarm;

    snap.cia[0].sdr = cia1.sdr;
    snap.cia[0].shiftCount = cia1.sdrShiftCount;
    snap.cia[1].sdr = cia2.sdr;
    snap.cia[1].shiftCount = cia2.sdrShiftCount;
}

template auto System::dongleJoydat<false>(uint16_t& val) -> void;
template auto System::dongleJoydat<true>(uint16_t& val) -> void;

}
