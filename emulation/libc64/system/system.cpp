
#include "system.h"
#include "../input/input.h"
#include "../prg/prg.h"
#include "../tape/tape.h"
#include "../vicII/fast/vicIIFast.h"
#include "../vicII/vicII.h"
#include "../disk/iec.h"
#include "../sid/sid.h"
#include "keyBuffer.h"
#include "gluelogic.h"
#include "../../tools/powersupply.h"
#include "../../tools/serializer.h"
#include "../../tools/rand.h"
#include "../expansionPort/freezer/actionReplayMK2.h"
#include "clipboard.h"
#include "firmware.h"
#include <cstring>

#include "expansion.cpp"
#include "serialization.cpp"
#include "map.cpp"
#include "../traps/traps.h"

namespace LIBC64 {

System* system = nullptr;
ExpansionPort* expansionPort = nullptr;
CIA::M6526* cia1 = nullptr;
CIA::M6526* cia2 = nullptr;
Emulator::SystemTimer sysTimer;

System::System(Interface* interface) {

    this->interface = interface;

    ram = new uint8_t[ 64 * 1024 ];
    colorRam = new uint8_t[ 1 * 1024 ];
    kernalRom = (uint8_t*)Firmware::kernalRom;
    basicRom = (uint8_t*)Firmware::basicRom;
    charRom = (uint8_t*)Firmware::charRom;

    createExpansions();
    vicIICycle = new VicIICycle;
    vicIIFast = new VicIIFast;
    vicII = vicIICycle;

    for (unsigned i = 0; i < 7; i++)
        sids[i] = new Sid( Sid::Type::MOS_6581 );

    sid = new Sid( Sid::Type::MOS_6581 );

    Sid::calcSerializationSizeForSevenMoreSids();

    Sid::registerGlobalCallbacks();

    requestedSids = 0;

    input = new Input;
    for (auto& media : interface->mediaGroups[Interface::MediaGroupIdProgram].media) {
        auto prg = new Prg;
        prg->media = &media;
        prgs.push_back( prg );
    }
    prgInUse = prgs[0];

    keyBuffer = new KeyBuffer;
    glueLogic = new GlueLogic();
    crop = new Emulator::Crop<uint8_t>;
    powerSupply = new Emulator::PowerSupply;
    tape = new Tape( &interface->mediaGroups[Interface::MediaGroupIdTape].media[0] );
    iecBus = new IecBus( &interface->mediaGroups[Interface::MediaGroupIdDisk] );

    cia1 = new CIA::M6526( 1, &sysTimer );
    cia2 = new CIA::M6526( 2, &sysTimer );
    cpu = new M6510;
    
    traps = new Traps;

    readRam = [this](uint16_t addr) {

        return this->ram[ addr ];
    };

    writeRam = [this](uint16_t addr, uint8_t value) {

        this->ram[ addr ] = value;
    };

    writeRamAt80To9F = [this](uint16_t addr, uint8_t value) {
        // some Cartridges listen here and writes value in their own RAM
        expansionPort->listenToWritesAt80To9F(addr, value);

        this->ram[ addr ] = value;
    };

    writeRamAtA0ToBF = [this](uint16_t addr, uint8_t value) {
        // some Cartridges listen here and writes value in their own RAM
        expansionPort->listenToWritesAtA0ToBF(addr, value);

        this->ram[ addr ] = value;
    };

    readCharRom = [this](uint16_t addr) {

        return this->charRom[ addr & 0xfff ];
    };

    readKernalRom = [this](uint16_t addr) {

        if (expansionPort->hasHiramCableConnected())
            return expansionPort->readRomH(addr);

        return (uint8_t) this->kernalRom[ addr & 0x1fff ];
    };

    readBasicRom = [this](uint16_t addr) {

        return (uint8_t) this->basicRom[ addr & 0x1fff ];
    };

    readRomL = [this](uint16_t addr) {

        return expansionPort->readRomL( addr & 0x1fff );
    };

    readRomH = [this](uint16_t addr) {

        return expansionPort->readRomH( addr & 0x1fff );
    };

    writeRomL = [this](uint16_t addr, uint8_t value) {

        expansionPort->writeRomL( addr, value );
    };

    writeRomH = [this](uint16_t addr, uint8_t value) {

        expansionPort->writeRomH( addr, value );
    };

    writeUltimaxRomL = [this](uint16_t addr, uint8_t value) {

        expansionPort->writeUltimaxRomL( addr, value );
    };

    writeUltimaxRomH = [this](uint16_t addr, uint8_t value) {

        expansionPort->writeUltimaxRomH( addr, value );
    };

    readUltimaxA0 = [this](uint16_t addr) {
        return expansionPort->readUltimaxA0( addr & 0x1fff );
    };

    writeUltimaxA0 = [this](uint16_t addr, uint8_t value) {

        expansionPort->writeUltimaxA0( addr, value );
    };

    writeUnmapped = [this](uint16_t addr, uint8_t value) {
        // do nothing
    };

    readUnmapped = [this](uint16_t addr) {
        return vicII->lastReadPhase1();
    };

    writeIo1Reg = [this](uint16_t addr, uint8_t value) {

        if (Sid::extraSids) {
            Sid::updateClock();
            Sid::writeSidIO( addr, value );
        }

        expansionPort->writeIo1(addr, value);
    };

    readIo1Reg = [this](uint16_t addr) {

        if (Sid::extraSids) {
            Sid* _sid = Sid::getSidByAdr( addr, true );
            if (_sid) {
                Sid::updateClock();
                return _sid->readIO( addr );
            }
        }

        return expansionPort->readIo1(addr);
    };

    writeIo2Reg = [this](uint16_t addr, uint8_t value) {

        if (Sid::extraSids) {
            Sid::updateClock();
            Sid::writeSidIO( addr, value );
        }
        expansionPort->writeIo2(addr, value);
    };

    readIo2Reg = [this](uint16_t addr) {

        if (Sid::extraSids) {
            Sid* _sid = Sid::getSidByAdr( addr, true );
            if (_sid) {
                Sid::updateClock();
                return _sid->readIO( addr );
            }
        }

        return expansionPort->readIo2(addr);
    };

    writeSidReg = [this](uint16_t addr, uint8_t value) {

        Sid::updateClock();

        if (Sid::extraSids)
            return Sid::writeSid( addr, value );

        sid->writeIO( addr, value );
    };

    writeDebugReg = [this](uint16_t addr, uint8_t value) {
        if ( (addr & 0xff) == 0xff) {
            debugCart.exitCode = value;
            debugCart.exit = true;
        }

        Sid::updateClock();

        if (Sid::extraSids)
            return Sid::getSidByAdr( addr )->writeIO( addr, value );

        sid->writeIO(addr, value);
    };

    readSidReg = [this](uint16_t addr) {

        Sid::updateClock();

        if (Sid::extraSids)
            return Sid::getSidByAdr( addr )->readIO( addr );

        return sid->readIO( addr );
    };

    writeVicReg = [this](uint16_t addr, uint8_t value) {

        vicII->writeReg( addr & 0xff, value );
    };

    readVicReg = [this](uint16_t addr) {

        return vicII->readReg( addr & 0xff );
    };

    writeCia1Reg = [this](uint16_t addr, uint8_t value) {

        cia1->write( addr, value );
    };

    readCia1Reg = [this](uint16_t addr) {

        return cia1->read( addr );
    };

    writeCia2Reg = [this](uint16_t addr, uint8_t value) {

        cia2->write( addr, value );
    };

    readCia2Reg = [this](uint16_t addr) {

        return cia2->read(addr);
    };

    writeColorRam = [this](uint16_t addr, uint8_t value) {

        colorRam[ addr & 0x3ff ] = value;
    };

    readColorRam = [this](uint16_t addr) {

        return (colorRam[ addr & 0x3ff ] & 0xf) | ( vicII->lastReadPhase1() & ~0xf );
    };

    Sid::audioRefresh = [this](int16_t sample) {
        if (!runAhead.pos)
            this->interface->audioSample( sample, sample );
    };

    Sid::audioRefreshStereo = [this](int16_t sampleL, int16_t sampleR) {
        if (!runAhead.pos)
            this->interface->audioSample( sampleL, sampleR );
    };

    Sid::getPotX = [this]() {

        return input->readPotX();
    };

    Sid::getPotY = [this]() {

        return input->readPotY();
    };

    cia1->irqCall = [this](bool state) {
        if (state)
            irqIncomming |= 2;
        else
            irqIncomming &= ~2;

        cpu->setIrq( irqIncomming != 0 );
    };

    cia1->serialOut = [this](bool bit) {

        if (secondDriveCable.burstUse) {
            diskIdleOff();
            iecBus->serialShift(bit);
        }
    };

    cia2->irqCall = [this](bool state) {
        if (state)
            nmiIncomming |= 2;
        else
            nmiIncomming &= ~2;

        cpu->setNmi( nmiIncomming != 0 );
    };

    cia1->readPort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if ( port == CIA::Base::PORTA )
            return input->readCiaPortA( lines );

        return input->readCiaPortB( lines );
    };

    cia1->writePort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if ( port == CIA::Base::PORTA ) {
            if (lines->ioa != lines->ioaOld)
                input->writeCiaPortA(lines);
        } else {
            if (lines->iob != lines->iobOld)
                input->writeCiaPortB( lines );
        }
    };

    cia2->readPort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if ( port == CIA::Base::PORTA ) {
            diskIdleOff();

            return (uint8_t) ( (lines->ioa & 0x3f) | iecBus->readCia() );

        } else if (secondDriveCable.parallelUserport) {
            diskIdleOff();
            return (uint8_t)(cia2->lines.iob & iecBus->readParallelWithHandshake());
        }

        return lines->iob;
    };

    cia2->writePort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if ( port == CIA::Base::PORTA ) {
            if (lines->ioaOld == lines->ioa)
                return;
            // the c64 II or c64c has another glue logic for updating the vic bank
            glueLogic->setVBank( ( ~(lines->ioa & 3) ) & 3, !lines->praChange );

            if (diskSilence.idle ) {
                if (iecBus->checkForIdleWrite( ~lines->ioa ))
                    return;

                iecBus->resetTicks();
            }

            if (iecBus->writeCia( ~lines->ioa )) {
                diskSilence.idle = false;
                diskSilence.idleFrames = 0;
                driveCycleSyncingUpdate();
            }
        } else if (secondDriveCable.parallelUserport && lines->prbChange) {
            diskIdleOff();
            // Port B with parallel cable
            iecBus->writeParallelHandshake();
        }
    };

    crop->removeBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {

        vicII->setBorderData();

        top = vicII->crop.top;
        bottom = vicII->crop.bottom;
        left = vicII->crop.left;
        right = vicII->crop.right;
    };

    crop->monitorBorderCallback = [this](unsigned& top, unsigned& bottom, unsigned& left, unsigned& right) {

        top = vicII->crop.topOverscan;
        bottom = vicII->crop.bottomOverscan;
        left = vicII->crop.leftOverscan;
        right = vicII->crop.rightOverscan;
    };

    tape->setReadTransition = [this]() {

        cia1->setFlag();
    };

    tape->read = [this](uint8_t* buffer, unsigned length, unsigned offset) {

        return this->interface->readMedia(tape->getMedia(), buffer, length, offset);
    };

    tape->write = [this](uint8_t* buffer, unsigned length, unsigned offset) {

        return this->interface->writeMedia(tape->getMedia(), buffer, length, offset);
    };

    tape->senseOut = [this](bool state) {
        // following refers to cpu input mode for lines 1 - 6
        // last 3 lines are always forced up
        // sense line is forced up when datasette stopped
        // and forced down when datasette is running
        // all other lines are not forced up or down in input mode?
        // means switching from output to input mode doesn't change line
        // Note: when Dattasette not connected: motor line is forced down

        if (!state)
            cpu->updateIoLines( 0x17 );

        else
            cpu->updateIoLines( 0x7, 0x10 );
    };

    countDownPowerSupply = [this]() {
        cia1->tod( );
        cia2->tod( );

        sysTimer.add( &countDownPowerSupply, powerSupply->nextTickCount(), Emulator::SystemTimer::Action::UpdateExisting );
    };

    sysTimer.registerCallback( { &countDownPowerSupply, 1 } );

    // connect keyboard
    for( auto& device : interface->devices ) {
        if (device.isKeyboard()) {
            input->keyboard.setDevice( &device );
            break;
        }
    }
    
    traps->add({"SerialListen", 0xED24, 0xEDAB, { 0x20, 0x97, 0xEE }, []() { traps->attention(); } });
    traps->add({"SerialSaListen", 0xED37, 0xEDAB, { 0x20, 0x8E, 0xEE }, []() { traps->attention(); } });
    traps->add({"SerialSendByte", 0xED41, 0xEDAB, { 0x20, 0x97, 0xEE }, []() { traps->send(); } });
    traps->add({"SerialReceiveByte", 0xEE14, 0xEDAB, { 0xA9, 0x00, 0x85 }, []() { traps->receive(); } });
    traps->add({"SerialReady", 0xEEA9, 0xEDAB, { 0xAD, 0x00, 0xDD }, []() { traps->ready(); } });
}

System::~System() {

    delete[] ram;
    delete[] colorRam;
    delete iecBus;
    delete vicIIFast;
    delete vicIICycle;
    destroyExpansions();
}

auto System::setFirmware( unsigned typeId, uint8_t* data, unsigned size ) -> void {

    switch (typeId) {
        case Interface::FirmwareIdKernal:
            if (!data || (size != 8192))
                data = (uint8_t*)Firmware::kernalRom;
            kernalRom = data;
            break;
        case Interface::FirmwareIdBasic:
            if (!data || (size != 8192))
                data = (uint8_t*)Firmware::basicRom;
            basicRom = data;
            break;
        case Interface::FirmwareIdChar:
            if (!data || (size != 4096))
                data = (uint8_t*)Firmware::charRom;
            charRom = data;
            break;
        default:
            iecBus->setFirmware( typeId, data, size );
            break;
    }
}

auto System::power( bool softReset ) -> void {
    sysTimer.clear();

    if( !softReset )
        initRam( ram );

    expansionPort->reset( softReset );

    mode = (expansionPort->isExrom() << 1) | expansionPort->isGame();

    vicBank = 0;

    mode <<= 3;
    mode |= 7; // charen = hiram = loram = 1 
    irqIncomming = 0;
    nmiIncomming = 0;
    rdyIncomming = 0;

    memoryCpu.unmap(0x0, 0xff);
    remapCpu();

    Sid::resetAll();

    cia1->reset();
    cia2->reset();
    input->reset();

    tape->reset();
    glueLogic->reset();

    powerSupply->init( vicII->frequency(), vicII->isNTSCGeometry() ? 60 : 50 );
    tape->setCyclesPerSecond( vicII->frequency() );
    iecBus->setCpuCyclesPerSecond( vicII->frequency() );

    sysTimer.add( &countDownPowerSupply, powerSupply->nextTickCount(), Emulator::SystemTimer::Action::UpdateExisting );
    initDebugCart();

    iecBus->power();
    diskSilence.idle = false;
    diskSilence.idleFrames = 0;
    burstOrParallelUpdate();

    if( !softReset ) {
        setCycleRenderer( cycleRendererNextBoot );

        vicIICycle->power();
        vicIIFast->power();
        cpu->power();
        observer.enterRom = false;
        observer.memoryAccesses = 0;
    } else {
        // vic hasn't a reset line ... means no change ?
        cpu->reset();
    }
    // cpu doesn't leave halted state by reset request   
    //cpu->setRdy( false );
    vicII->setUltimax( isUltimax() );

    cpu->updateIoLines( 0x17, !tape->isEnabled() ? 0x20 : 0 );

    if( !softReset ) {
        calcSerializationSize();
        if (requestedSids)
            serializationSize += Sid::serializationSizeForSevenMoreSids;

        fastForward.config = 0;
        fastForward.frameCounter = 0;
        fastForward.renderNext = false;
    }

    kernalBootComplete = false;
    KeyBuffer::Action action;
    action.mode = KeyBuffer::Mode::WaitDelay;
    if (iecBus->drives[0]->speeder)
        action.delay = (unsigned)(interface->stats.fps * ((iecBus->drives[0]->speeder == 10 || iecBus->drives[0]->speeder == 11)
            ? 0.9 : 0.5) );
    else
        action.delay = (unsigned)(interface->stats.fps * 2.2);

    if ( !expansionPort->isBootable() ) {
        system->keyBuffer->add( action, false );

        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};

        if (dynamic_cast<ActionReplayMK2*>(expansionPort))
            action.buffer = {'L', 'O', 'A', 'D', 'E', 'R'};

        action.delay = 0;
        action.blinkingCursor = true;
        action.callbackId = 1;
        action.callback = [this]() { kernalBootComplete = true; };
        system->keyBuffer->add( action );


    } else {
        action.callbackId = 1;
        action.callback = [this]() { kernalBootComplete = true; };
        system->keyBuffer->add( action, false );
    }

    traps->install();
    traps->reset();

    powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
    keyBuffer->reset();
    sid->powerOff();
    iecBus->powerOff();
}

auto System::initRam(uint8_t*& mem) -> void {
    uint8_t j, k, value;

    for (unsigned i = 0; i <= 0xffff; i++) {

        j = 0;

        if (memoryInit.invertEvery)
            j = ((i / memoryInit.invertEvery) & 1) ? 0xff : 0x00;

        value = memoryInit.value ^ j;

        j = k = 0;

        if (memoryInit.randomPatternLength && memoryInit.repeatRandomPattern)
            k = ((i % memoryInit.repeatRandomPattern) < memoryInit.randomPatternLength) ? Emulator::Rand::rand(0, 0xff) : 0;

        if (memoryInit.randomChance) {
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x80 : 0;
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x40 : 0;
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x20 : 0;
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x10 : 0;
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x08 : 0;
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x04 : 0;
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x02 : 0;
            j |= Emulator::Rand::rand(0, 1000) < memoryInit.randomChance ? 0x01 : 0;
        }

        value ^= k ^ j;

        mem[i] = value;
    }
}

auto System::setRunAhead(unsigned frames) -> void {
    runAhead.frames = frames;
}

auto System::setRunAheadPerformance(bool state) -> void {
    runAhead.performance = state;
}

auto System::run() -> void {
    frameComplete = false;
    runAhead.pos = 0;
    acia->connectionLock = false;

    if (cpu->callResetRoutine)
        cpu->resetRoutine();

    input->poll();
    // of course real system sends restore when key is pressed, but polling each cycle for this is useless
    // because host updates pressed keys once per frame only
    if (input->restore())
        nmiIncomming |= 1;
    else
        nmiIncomming &= ~1;

    cpu->setNmi(nmiIncomming != 0);
    iecBus->randomizeRpm();

    bool useRunAhead = !fastForward.config && runAhead.frames
                       && !keyBuffer->isPrgInjectionInQueue() && !iecBus->diskInsertInProgress;

    if (useRunAhead) {
        runAhead.pos = runAhead.frames;
        vicII->disableSequencer( runAhead.performance );
        Sid::disableAudioOut( runAhead.frames > 1 );
    }

    labelRunAhead:

    while( !frameComplete ) {
        cpu->process();
        if (!diskSilence.idle && !secondDriveCable.cycleSyncing)
            iecBus->syncDrives();
    }

    if (useRunAhead) {
        if (runAhead.frames == runAhead.pos) {
            serializeLight();
        }

        if (runAhead.pos) {
            if (runAhead.pos == 2)
                Sid::disableAudioOut(false);

            if (--runAhead.pos == 0) {
                if (!vicII->useSequencer()) {
                    vicII->disableSequencer(false);
                }
            }
            frameComplete = false;
            goto labelRunAhead;
        }

        unserializeLight();
    }

    if (observer.motorChange)
        informAboutMotorChange();

    checkDebugCart();
}

auto System::runAheadEnableAudio() -> void {
    if (runAhead.pos == 1)
        Sid::disableAudioOut(false);
}

auto System::isUltimax() -> bool {
    return ((mode >> 3) & 3) == 2;
}

auto System::changeExpansionPortMemoryMode(bool exrom, bool game, bool noUltimaxIfVicHasTheBus) -> void {

    uint8_t cartMode = (mode >> 3) & 3;
    uint8_t cartModeNew = (exrom << 1) | game;

    vicII->setUltimax( noUltimaxIfVicHasTheBus ? false : (exrom && !game) );

    if (cartMode == cartModeNew)
        return;

    mode &= 7;
    mode |= cartModeNew << 3;

    remapCpu();
}

auto System::setFastForward( unsigned config ) -> void {
    fastForward.config = config;
    Sid::disableAudioOut(config & (unsigned) Emulator::Interface::FastForward::NoAudioOut);
    vicII->disableSequencer(config & (unsigned) Emulator::Interface::FastForward::NoVideoSequencer);
    iecBus->setFastForward(config & (unsigned) Emulator::Interface::FastForward::NoVideoSequencer);
}

auto System::setCycleRenderer(bool state) -> void {
    if (state)
        vicII = vicIICycle;
    else
        vicII = vicIIFast;
}

auto System::updateStats() -> void {
    interface->stats.region = vicII->isNTSCGeometry() ? Interface::Region::Ntsc : Interface::Region::Pal;
    interface->stats.sampleRate = (double)vicII->frequency() / (double)Sid::sampleLimit;
    interface->stats.fps = 1.0 / ( (double)vicII->cyclesPerFrame() / (double)vicII->frequency() );
    interface->stats.stereoSound = Sid::isStereo();
}

auto System::updateStatsStereo() -> void {
    interface->stats.stereoSound = Sid::isStereo();
}

auto System::useExtraSids(uint8_t requestedSids) -> void {
    auto requestedSidsBefore = this->requestedSids;

    this->requestedSids = requestedSids;
    Sid::updateSidUsage();

    if (!powerOn)
        return;

    if (requestedSids && !requestedSidsBefore)
        serializationSize += Sid::serializationSizeForSevenMoreSids;
    else if (!requestedSids && requestedSidsBefore)
        serializationSize -= Sid::serializationSizeForSevenMoreSids;

    Sid::clone( requestedSidsBefore, requestedSids );
}

auto System::updatePort(uint8_t lines, uint8_t ddr) -> void {

    if (!powerOn)
        return;

    auto modeBefore = mode;

    mode &= ~7;

    mode |= lines & 7;

    if (modeBefore != mode)
        this->remapCpu( );

    tape->writeIn( ((~ddr | lines) & 8) != 0 );
    tape->setMotorIn( ((lines & ddr) & 0x20) == 0 );
}

auto System::videoRefresh( uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void {

    if (diskSilence.active) {
        if (!diskSilence.idle) {
            if (++diskSilence.idleFrames > 200) {
                diskSilence.idle = true;
                diskSilence.idleFrames = 0;
                driveCycleSyncingUpdate();
                iecBus->resetDriveState();
            }
        }
    }

    if (!runAhead.pos && frame) {
        crop->apply( frame, width, height, linePitch );
        // for lightguns
        input->drawCursor();
    }

    if (fastForward.config & (unsigned)Interface::FastForward::NoVideoOut)
        frame = nullptr;

    else if (fastForward.renderNext) {
        fastForward.renderNext = false;
        vicII->disableSequencer( fastForward.config & (unsigned)Interface::FastForward::NoVideoSequencer );

    } else if (fastForward.config & (unsigned)Interface::FastForward::ReduceVideoOutput) {
        frame = nullptr;

        if ((++fastForward.frameCounter & 15) == 0) {
            fastForward.frameCounter = 0;
            vicII->disableSequencer( false );
            fastForward.renderNext = true;
        }
    }

    if (!runAhead.pos) {
        this->interface->videoRefresh8(frame, width, height, linePitch);

        if (iecBus->diskInsertInProgress)
            iecBus->insertDiskGracefully();
    }

    frameComplete = true;

    if ( keyBuffer->hasJobs )
        keyBuffer->process();
}

auto System::setVicIrq( bool state ) -> void {
    if (state)
        irqIncomming |= 1;
    else
        irqIncomming &= ~1;

    cpu->setIrq( irqIncomming != 0 );
}

auto System::setVicRdy(bool state) -> void {
    if (state)
        rdyIncomming |= 1;
    else
        rdyIncomming &= ~1;

    cpu->setRdy( rdyIncomming != 0 );
}

auto System::VicMidScreenCallback() -> void {

    if (runAhead.pos)
        return;

    input->drawCursor(true);

    interface->midScreenCallback();
}

auto System::VicVblankCallback() -> void {
    if (!runAhead.pos)
        interface->finishVBlank();
}

auto System::pasteText( std::string buffer ) -> void {
    keyBuffer->paste( buffer );
}

auto System::copyText( ) -> std::string {

    Clipboard clipboard;
    return clipboard.getText();
}

auto System::checkForAutoStarter() -> bool {

    if (!observer.enterRom) {

        if (memoryCpu.isLocation( cpu->pc >> 8, &readKernalRom ))
            observer.enterRom = true;

        observer.memoryAccesses = 0;
    } else {

        if (memoryCpu.isLocation( cpu->pc >> 8, &readRam ))
            observer.memoryAccesses++;

        if (observer.memoryAccesses > 2)
            return true;
    }

    return false;
}

auto System::motorChange(bool state) -> void {
    observer.motorChange = true;
    observer.motor = state;
}

auto System::informAboutMotorChange() -> void {
    observer.motorChange = false;
    interface->informDriveLoading( observer.motor );
}

auto System::burstOrParallelUpdate() -> void {
    secondDriveCable.burstUse = secondDriveCable.burstRequested && secondDriveCable.burstPossible && iecBus->drivesConnected;
    secondDriveCable.parallelUse = secondDriveCable.parallelRequested && secondDriveCable.parallelPossible && iecBus->drivesConnected;
    secondDriveCable.parallelExpansion = secondDriveCable.parallelUse && (expansionPort == fastloader);
    secondDriveCable.parallelUserport = secondDriveCable.parallelUse && (expansionPort != fastloader);

    driveCycleSyncingUpdate();
}

auto System::driveCycleSyncingUpdate() -> void {

    secondDriveCable.cycleSyncing = (secondDriveCable.burstUse || secondDriveCable.parallelExpansion || secondDriveCable.parallelUserport) && !diskSilence.idle;
}

auto System::diskIdleOff() -> void {
    if (diskSilence.idle) {
        diskSilence.idle = false;
        iecBus->resetTicks();
        driveCycleSyncingUpdate();
    }
    diskSilence.idleFrames = 0;
}

auto System::readParallelWithHandshake() -> uint8_t {
    uint8_t out = 0xff;

    if (secondDriveCable.parallelUserport ) {
        cia2->setFlag();
        out = cia2->lines.iob;
    } else if (secondDriveCable.parallelExpansion ) {
        if ( (fastloader->mode & FASTLOADER_PIA_PORT_A) == FASTLOADER_PIA_PORT_A ) { // PROLOGIC
            fastloader->pia.ca1In(false);
            out = fastloader->pia.ioa;
        } else if ( (fastloader->mode & FASTLOADER_VIA_PORT_B) == FASTLOADER_VIA_PORT_B ) { // PROF DOS
            fastloader->via.cb1In(false);
            out = fastloader->via.lines.iob;
        } else if ( (fastloader->mode & FASTLOADER_VIA_PORT_A) == FASTLOADER_VIA_PORT_A ) { // TURBO TRANS
            fastloader->via.cb2In(false);
            out = fastloader->via.lines.ioa;
        }
    }
    return out;
}

auto System::readParallel() -> uint8_t {
    uint8_t out = 0xff;

    if (secondDriveCable.parallelUserport ) {
        out = cia2->lines.iob;
    } else if (secondDriveCable.parallelExpansion ) {
        if ( (fastloader->mode & FASTLOADER_PIA_PORT_A) == FASTLOADER_PIA_PORT_A ) {
            out = fastloader->pia.ioa;
        } else if ( (fastloader->mode & FASTLOADER_VIA_PORT_B) == FASTLOADER_VIA_PORT_B ) {
            out = fastloader->via.lines.iob;
        } else if ( (fastloader->mode & FASTLOADER_VIA_PORT_A) == FASTLOADER_VIA_PORT_A ) {
            out = fastloader->via.lines.ioa;
        }
    }
    return out;
}

auto System::writeParallelHandshake() -> void {

    if (secondDriveCable.parallelUserport ) {
        cia2->setFlag();
    } else if (secondDriveCable.parallelExpansion ) {
        if ( (fastloader->mode & FASTLOADER_PIA_PORT_A) == FASTLOADER_PIA_PORT_A ) {
            fastloader->pia.ca1In(false);
        } else if ( (fastloader->mode & FASTLOADER_VIA_PORT_B) == FASTLOADER_VIA_PORT_B ) {
            fastloader->via.cb1In(false);
        } else if ( (fastloader->mode & FASTLOADER_VIA_PORT_A) == FASTLOADER_VIA_PORT_A ) {
            fastloader->via.ca1In(false);
        }
    }
}

}
