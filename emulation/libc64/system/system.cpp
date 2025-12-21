
#include "system.h"
#include "../input/input.h"
#include "../prg/prg.h"
#include "../vicII/base.h"
#include "../sid/sid.h"
#include "keyBuffer.h"
#include "../../tools/powersupply.h"
#include "../../tools/serializer.h"
#include "../../tools/rand.h"
#include "../expansionPort/freezer/actionReplayMK2.h"
#include "clipboard.h"
#include "firmware.h"
#include "testbench.h"
#include <cstring>

#include "expansion.cpp"
#include "serialization.cpp"
#include "map.cpp"
#include "../expansionPort/gameCart/businessBasic.h"
#include "../traps/traps.h"

namespace LIBC64 {

System::System(Interface* interface) :
sidManager(this),
input(this, interface, cia1),
glueLogic(this),
vicIICycle(this),
vicIIFast(this),
iecBus(this, &interface->mediaGroups[Interface::MediaGroupIdDisk]),
tape( this, &interface->mediaGroups[Interface::MediaGroupIdTape].media[0] ),
cia1( 1, sysTimer ),
cia2( 2, sysTimer ),
traps( this ),
cpu(this, sysTimer, cia1, cia2, iecBus, traps) {

    this->interface = interface;

    ram = new uint8_t[ 64 * 1024 ];
    colorRam = new uint8_t[ 1 * 1024 ];
    kernalRom = (uint8_t*)Firmware::kernalRom;
    basicRom = (uint8_t*)Firmware::basicRom;
    charRom = (uint8_t*)Firmware::charRom;

    debugCart = new DebugCart(this);

    createExpansions();
    setCycleRenderer();

    sidManager.calcSerializationSizeForSevenMoreSids();

    sidManager.registerCallbacks();
    glueLogic.registerCallbacks();
    tape.registerCallbacks();
    cpu.registerCallbacks();
    cia1.registerCallbacks();
    cia2.registerCallbacks();

    requestedSids = 0;

    tapeNoise.circularBuffer.resize(500, 0);

    for (auto& media : interface->mediaGroups[Interface::MediaGroupIdProgram].media) {
        auto prg = new Prg(this);
        prg->media = &media;
        prgs.push_back( prg );
    }
    prgInUse = prgs[0];

    crop = new Emulator::Crop<uint8_t>;
    powerSupply = new Emulator::PowerSupply;
    keyBuffer = new KeyBuffer(this, tape);

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
            return expansionPort->readRomH(addr & 0x1fff);

        return (uint8_t) this->kernalRom[ addr & 0x1fff ];
    };

    peekKernalRom = [this](uint16_t addr) {

        if (expansionPort->hasHiramCableConnected())
            return expansionPort->peekRomH(addr & 0x1fff);

        return (uint8_t) this->kernalRom[ addr & 0x1fff ];
    };

    readBasicRom = [this](uint16_t addr) {

        return (uint8_t) this->basicRom[ addr & 0x1fff ];
    };

    readRomL = [this](uint16_t addr) {

        return expansionPort->readRomL( addr & 0x1fff );
    };

    peekRomL = [this](uint16_t addr) {

        return expansionPort->peekRomL( addr & 0x1fff );
    };

    readRomH = [this](uint16_t addr) {

        return expansionPort->readRomH( addr & 0x1fff );
    };

    peekRomH = [this](uint16_t addr) {

        return expansionPort->peekRomH( addr & 0x1fff );
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

    peekUltimaxA0 = [this](uint16_t addr) {
        return expansionPort->peekUltimaxA0( addr & 0x1fff );
    };

    writeUltimaxA0 = [this](uint16_t addr, uint8_t value) {

        expansionPort->writeUltimaxA0( addr, value );
    };

    writeUnmapped = [](uint16_t addr, uint8_t value) {
        // do nothing
    };

    readUnmapped = [this](uint16_t addr) {
        return vicII->lastReadPhase1();
    };

    writeIo1Reg = [this](uint16_t addr, uint8_t value) {

        if (sidManager.extraSids) {
            sidManager.updateClock();
            sidManager.writeSidIO( addr, value );
        }

        expansionPort->writeIo1(addr, value);
    };

    readIo1Reg = [this](uint16_t addr) {

        if (sidManager.extraSids) {
            Sid* _sid = sidManager.getSidByAdr( addr, true );
            if (_sid) {
                sidManager.updateClock();
                return _sid->readIO( addr );
            }
        }

        return expansionPort->readIo1(addr);
    };

    peekIo1Reg = [this](uint16_t addr) {

        if (sidManager.extraSids) {
            Sid* _sid = sidManager.getSidByAdr( addr, true );
            if (_sid) {
                sidManager.updateClock();
                return _sid->peekIO( addr );
            }
        }

        return expansionPort->peekIo1(addr);
    };

    writeIo2Reg = [this](uint16_t addr, uint8_t value) {

        if (sidManager.extraSids) {
            sidManager.updateClock();
            sidManager.writeSidIO( addr, value );
        }
        expansionPort->writeIo2(addr, value);
    };

    readIo2Reg = [this](uint16_t addr) {

        if (sidManager.extraSids) {
            Sid* _sid = sidManager.getSidByAdr( addr, true );
            if (_sid) {
                sidManager.updateClock();
                return _sid->readIO( addr );
            }
        }

        return expansionPort->readIo2(addr);
    };

    peekIo2Reg = [this](uint16_t addr) {

        if (sidManager.extraSids) {
            Sid* _sid = sidManager.getSidByAdr( addr, true );
            if (_sid) {
                sidManager.updateClock();
                return _sid->peekIO( addr );
            }
        }

        return expansionPort->peekIo2(addr);
    };

    writeSidReg = [this](uint16_t addr, uint8_t value) {

        sidManager.updateClock();

        if (sidManager.extraSids)
            return sidManager.writeSid( addr, value );

        sidManager.sid->writeIO( addr, value );
    };

    writeDebugReg = [this](uint16_t addr, uint8_t value) {
        if ( (addr & 0xff) == 0xff)
            debugCart->setExit(value);

        sidManager.updateClock();

        if (sidManager.extraSids)
            return sidManager.getSidByAdr( addr )->writeIO( addr, value );

        sidManager.sid->writeIO(addr, value);
    };

    readSidReg = [this](uint16_t addr) {

        sidManager.updateClock();

        if (sidManager.extraSids)
            return sidManager.getSidByAdr( addr )->readIO( addr );

        return sidManager.sid->readIO( addr );
    };

    peekSidReg = [this](uint16_t addr) {

        sidManager.updateClock();

        if (sidManager.extraSids)
            return sidManager.getSidByAdr( addr )->peekIO( addr );

        return sidManager.sid->peekIO( addr );
    };

    writeVicReg = [this](uint16_t addr, uint8_t value) {

        vicII->writeReg( addr & 0xff, value );
    };

    readVicReg = [this](uint16_t addr) {

        return vicII->readReg( addr & 0xff );
    };

    peekVicReg = [this](uint16_t addr) {

        return vicII->peekReg( addr & 0xff );
    };

    writeCia1Reg = [this](uint16_t addr, uint8_t value) {

        cia1.write( addr, value );
    };

    readCia1Reg = [this](uint16_t addr) {

        return cia1.read( addr );
    };

    peekCia1Reg = [this](uint16_t addr) {

        return cia1.peek( addr );
    };

    writeCia2Reg = [this](uint16_t addr, uint8_t value) {

        cia2.write( addr, value );
    };

    readCia2Reg = [this](uint16_t addr) {

        return cia2.read(addr);
    };

    peekCia2Reg = [this](uint16_t addr) {

        return cia2.peek(addr);
    };

    writeColorRam = [this](uint16_t addr, uint8_t value) {

        colorRam[ addr & 0x3ff ] = value;
    };

    readColorRam = [this](uint16_t addr) {

        return (colorRam[ addr & 0x3ff ] & 0xf) | ( vicII->lastReadPhase1() & ~0xf );
    };

    sidManager.getPotX = [this]() {

        return input.readPotX();
    };

    sidManager.getPotY = [this]() {

        return input.readPotY();
    };

    cia1.irqCall = [this](bool state) {
        if (state)
            irqIncomming |= 2;
        else
            irqIncomming &= ~2;

        expansionPort->observeIrq(irqIncomming != 0);
        cpu.setIrq( irqIncomming != 0 );
    };

    cia1.serialOut = [this](bool spLine, bool cntLine) {

        if (!cntLine && secondDriveCable.burstUse) {
            diskIdleOff();
            iecBus.serialShift(spLine);
        }
    };

    cia2.irqCall = [this](bool state) {
        if (state)
            nmiIncomming |= 2;
        else
            nmiIncomming &= ~2;

        expansionPort->observeNmi(nmiIncomming != 0);
        cpu.setNmi( nmiIncomming != 0 );
    };

    cia1.readPort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if (observer.inputFetches) {
            if (!--observer.inputFetches)
                observer.stateChange = true;
        }

        if ( port == CIA::Base::PORTA )
            return input.readCiaPortA( lines );

        return input.readCiaPortB( lines );
    };

    cia1.writePort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if ( port == CIA::Base::PORTA ) {
            if (lines->ioa != lines->ioaOld)
                input.writeCiaPortA(lines);
        } else {
            if (lines->iob != lines->iobOld)
                input.writeCiaPortB( lines );
        }
    };

    cia2.readPort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if ( port == CIA::Base::PORTA ) {
            diskIdleOff();

            return (uint8_t) ( (lines->ioa & 0x3f) | iecBus.readCia() );

        } else if (secondDriveCable.parallelUserport) {
            diskIdleOff();
            return (uint8_t)(lines->iob & iecBus.readParallelWithHandshake());
        }
        else if (input.connectUserPort()) {
            return (uint8_t)(lines->iob & input.readUserPort());
        }

        return lines->iob;
    };

    cia2.writePort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {

        if ( port == CIA::Base::PORTA ) {
            if (lines->ioaOld == lines->ioa)
                return;
            // the c64 II or c64c has another glue logic for updating the vic bank
            glueLogic.setVBank( ( ~(lines->ioa & 3) ) & 3, !lines->praChange );

            if (diskSilence.idle ) {
                if (iecBus.checkForIdleWrite( ~lines->ioa ))
                    return;

                iecBus.resetTicks();
            }

            if (iecBus.writeCia( ~lines->ioa )) {
                if (diskSilence.idle) {
                    diskSilence.idle = false;
                    driveCycleSyncingUpdate();
                }

                diskSilence.idleFrames = 0;
            }
        } else if (secondDriveCable.parallelUserport && lines->prbChange) {
            diskIdleOff();
            // Port B with parallel cable
            iecBus.writeParallelHandshake();
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

    crop->screenshotBorderCallback = [this](unsigned _w, unsigned _h) {
        return vicII->getCrop(_w, _h);
    };

    tape.setReadTransition = [this]() {

        cia1.setFlag();
    };

    tape.read = [this](uint8_t* buffer, unsigned length, unsigned offset) {

        return this->interface->readMedia(tape.media, buffer, length, offset);
    };

    tape.write = [this](uint8_t* buffer, unsigned length, unsigned offset) {

        return this->interface->writeMedia(tape.media, buffer, length, offset);
    };

    tape.senseOut = [this](bool state) {
        // following refers to cpu input mode for lines 1 - 6
        // last 3 lines are always forced up
        // sense line is forced up when datasette stopped
        // and forced down when datasette is running
        // all other lines are not forced up or down in input mode?
        // means switching from output to input mode doesn't change line
        // Note: when Datasette not connected: motor line is forced down

        if (!state)
            cpu.updateIoLines( 0x17 );

        else
            cpu.updateIoLines( 0x7, 0x10 );
    };

    countDownPowerSupply = [this]() {
        cia1.tod( );
        cia2.tod( );

        sysTimer.add( &countDownPowerSupply, powerSupply->nextTickCount(), Emulator::SystemTimer::Action::UpdateExisting );
    };

    sysTimer.registerCallback( { &countDownPowerSupply, 1 } );

    // connect keyboard
    for( auto& device : interface->devices ) {
        if (device.isKeyboard()) {
            input.keyboard.setDevice( &device );
            break;
        }
    }
    
    traps.add({"SerialListen", 0xED24, 0xEDAB, { 0x20, 0x97, 0xEE }, [this]() { traps.attention(); } });
    traps.add({"SerialSaListen", 0xED37, 0xEDAB, { 0x20, 0x8E, 0xEE }, [this]() { traps.attention(); } });
    traps.add({"SerialSendByte", 0xED41, 0xEDAB, { 0x20, 0x97, 0xEE }, [this]() { traps.send(); } });
    traps.add({"SerialReceiveByte", 0xEE14, 0xEDAB, { 0xA9, 0x00, 0x85 }, [this]() { traps.receive(); } });
    traps.add({"SerialReady", 0xEEA9, 0xEDAB, { 0xAD, 0x00, 0xDD }, [this]() { traps.ready(); } });

    traps.add({"TapeFindHeader", 0xF72F, 0xF732, { 0x20, 0x41, 0xF8 }, [this]() { traps.tapeFindHeader(); } });
    traps.add({"TapeReceive", 0xF8A1, 0xFC93, { 0x20, 0xBD, 0xFC }, [this]() { traps.tapeReceive(); } });
}

System::~System() {

    delete[] ram;
    delete[] colorRam;
    destroyExpansions();
}

auto System::setFirmware( unsigned typeId, uint8_t* data, unsigned size, bool allowPatching ) -> void {

    switch (typeId) {
        case Interface::FirmwareIdKernal:
            if (allowPatching && vicII->oldOne()) {
                if (vicII->isNTSCGeometry())
                    data = (uint8_t*)Firmware::kernalRomRev1;
                else
                    data = (uint8_t*)Firmware::kernalRomRev2;

            } else if (!data || (size != 8192))
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
            iecBus.setFirmware( typeId, data, size );
            break;
    }
}

auto System::power( bool softReset ) -> void {
    crop->latest.frame = nullptr;
    sysTimer.clear();

    if( !softReset )
        initRam( ram );

    expansionPort->reset( softReset );

    mode = (expansionPort->isExrom() << 1) | expansionPort->isGame();

    vicBank = 0;
    mhz2 &= ~0x80;

    mode <<= 3;
    if (!dynamic_cast<SuperCpu*>(expansionPort))
        mode |= 7; // charen = hiram = loram = 1 
    irqIncomming = 0;
    nmiIncomming = 0;
    rdyIncomming = 0;

    memoryCpu.unmap(0x0, 0xff);
    remapCpu();

    sidManager.resetAll();

    cia1.reset();
    cia2.reset();
    input.reset();

    tape.power();
    glueLogic.reset();

    powerSupply->init( vicII->frequency(), vicII->isNTSCGeometry() ? 60 : 50 );
    tape.setCyclesPerSecond( vicII->frequency() );
    iecBus.setCpuCyclesPerSecond( vicII->frequency() );

    sysTimer.add( &countDownPowerSupply, powerSupply->nextTickCount(), Emulator::SystemTimer::Action::UpdateExisting );
    debugCart->init();

    if( !softReset )
        iecBus.power();

    diskSilence.idle = false;
    diskSilence.idleFrames = 0;
    burstOrParallelUpdate();

    if( !softReset ) {
        setCycleRenderer( cycleRendererNextBoot );

        vicIICycle.power();
        vicIIFast.power();
        cpu.power();
        observer.enterRom = false;
        observer.memoryAccesses = 0;
        observer.stateChange = false;
        observer.motor = false;
        observer.inputLock = true;
        observer.inputFetches = 0;
    } else {
        // vic hasn't a reset line ... means no change ?
        cpu.reset();
    }

    // cpu doesn't leave halted state by reset request   
    //cpu->setRdy( false );
    vicII->setUltimax( isUltimax() );

    cpu.updateIoLines( 0x17, !tape.isEnabled() ? 0x20 : 0 );

    if( !softReset ) {
        calcSerializationSize();

        warp.config = 0;
        warp.frameCounter = 0;
        warp.renderNext = false;
    }

    tapeNoise.reset();

    kernalBootComplete = false;
    KeyBuffer::Action action;
    action.mode = KeyBuffer::Mode::WaitDelay;

    if (!debugCart->enable && expansionPort->bootSpeed())
        action.delay = (unsigned)(interface->stats.fps * expansionPort->bootSpeed());
    else if (iecBus.drives[0]->speeder && secondDriveCable.parallelUse)
        action.delay = (unsigned)(interface->stats.fps * ((iecBus.drives[0]->speeder == 10 || iecBus.drives[0]->speeder == 11)
            ? 0.9 : 0.5) );
    else
        action.delay = (unsigned)(interface->stats.fps * 2.2);


    if ( !expansionPort->isBootable() ) {
        keyBuffer->add( action, false );

        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};

        if (dynamic_cast<ActionReplayMK2*>(expansionPort))
            action.buffer = {'L', 'O', 'A', 'D', 'E', 'R'};
        else if (dynamic_cast<BusinessBasic*>(expansionPort))
            action.buffer = {'O', 'K', '.'};

        action.delay = 0;
        action.blinkingCursor = true;
        action.callbackId = 1;
        action.callback = [this]() {
            kernalBootComplete = true;
            if (traps.installDelayed)
                traps.installSerial();
        };
        keyBuffer->add( action );

        if (!dynamic_cast<SuperCpu*>(expansionPort) && (mhz2 & 1) && (Drive::globalType == Drive::Type::D1571)) {
            action.callback = nullptr;
            action.mode = KeyBuffer::Mode::Input;
            action.buffer = { 'O','P','E','N',' ','1','5',',','8',',','1','5',',',' ','"','U','0','>','M','1','"',':','C','L','O','S','E',' ','1','5', '\r' };
            action.blinkingCursor = false;
            action.delay = 0;
            keyBuffer->add(action);

            action.mode = KeyBuffer::Mode::WaitFor;
            action.buffer = { 'R', 'E', 'A', 'D', 'Y', '.' };
            action.blinkingCursor = true;
            action.delay = 2;
            keyBuffer->add(action);
        }

    } else {
        action.callbackId = 1;
        action.callback = [this]() { kernalBootComplete = true; };
        keyBuffer->add( action, false );
    }

    history.reset();
    powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
    secondDriveCable.parallelPossible = true;
    keyBuffer->reset();
    iecBus.powerOff();
    if (traps.installed)
        traps.uninstall();
}

auto System::initRam(uint8_t*& mem) -> void {
    uint8_t j, k, value;
    Emulator::Rand rand;
    rand.initXorShift(0x1234abcd + (Emulator::Rand::rand() & 0xffff) );

    for (unsigned i = 0; i <= 0xffff; i++) {

        j = k = 0;

        if (memoryInit.invertEvery)
            j = (((i + memoryInit.offset) / memoryInit.invertEvery) & 1) ? 0xff : 0x00;

        if (memoryInit.secondInvertEvery)
            k = ((i / memoryInit.secondInvertEvery) & 1) ? memoryInit.secondValue : 0x00;

        value = memoryInit.value ^ j ^ k;

        j = k = 0;

        if (memoryInit.randomPatternLength && memoryInit.repeatRandomPattern)
            k = ((i % memoryInit.repeatRandomPattern) < memoryInit.randomPatternLength) ? (Emulator::Rand::rand() & 0xff) : 0;

        if (memoryInit.randomChance) {
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x80 : 0;
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x40 : 0;
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x20 : 0;
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x10 : 0;
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x08 : 0;
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x04 : 0;
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x02 : 0;
            j |= rand.xorShift(0, 1000) < memoryInit.randomChance ? 0x01 : 0;
        }

        value ^= k ^ j;

        mem[i] = value;
    }
}

auto System::run() -> void {
    bool haltMainCpu = expansionPort->haltMainCpu();
    leaveEmulation = false;
    runAhead.pos = 0;
    acia->connectionLock = false;
    input.poll();

    if (input.restore())
        nmiIncomming |= 1;
    else
        nmiIncomming &= ~1;

    cpu.setNmi(nmiIncomming != 0);
    expansionPort->observeNmi(nmiIncomming != 0);
    iecBus.randomizeRpm();

    runAhead.active = !warp.config && runAhead.frames && !traps.installed
        && !cpu.inDebugMode() && !keyBuffer->isPrgInjectionInQueue();

    if (history.enable()) {
        if (history.rewind) {
            runAhead.active = false;
            auto memState = history.apply();
            if (memState) {
                unserializeLight(*memState, true);
                expansionPort->memChange = &memState->trackers[0];
                if (expansionPort->expander)
                    expansionPort->expander->memChange = &memState->trackers[1];
            }
        } else {
            auto memState = history.remember();
            if (memState)
                serializeLight(*memState, true);
            else {
                expansionPort->memChange = &history.getCurMemstate()->trackers[0];
                if (expansionPort->expander)
                    expansionPort->expander->memChange = &history.getCurMemstate()->trackers[1];
            }
        }
    }

    if (runAhead.active) {
        runAhead.pos = runAhead.frames;
        vicII->disableSequencer( runAhead.performance );
        sidManager.disableAudioOut( runAhead.frames > 1 );
    }

    labelRunAhead:

    if (haltMainCpu) {
        while( !leaveEmulation ) {
            expansionPort->clock();
            if (!diskSilence.idle && !secondDriveCable.cycleSyncing)
                iecBus.syncDrives();
        }
    } else if (!mhz2) {
        while( !leaveEmulation ) {
            cpu.process<false>();
            if (!diskSilence.idle && !secondDriveCable.cycleSyncing)
                iecBus.syncDrives();
        }
    } else {
        while( !leaveEmulation ) {
            cpu.process<true>();
            if (!diskSilence.idle && !secondDriveCable.cycleSyncing)
                iecBus.syncDrives();
        }
    }

    if (runAhead.active) {
        if (runAhead.frames == runAhead.pos) {
            serializeLight(runAhead.memState);
        }

        if (runAhead.pos) {
            if (runAhead.pos == 2)
                sidManager.disableAudioOut(false);

            if (--runAhead.pos == 0) {
                if (!vicII->useSequencer()) {
                    vicII->disableSequencer(false);
                }
            }
            leaveEmulation = false;
            goto labelRunAhead;
        }

        unserializeLight(runAhead.memState);
    }

    if (observer.stateChange)
        informAboutStateChange();

    debugCart->check();

    if (debugger.action != Emulator::Interface::DebuggerAction::None) {
        interface->debugger(debugger.action, debugger.addr, cpu.hasModifiedCode());
        debugger.action = Emulator::Interface::DebuggerAction::None;
    }
}

auto System::isUltimax() -> bool {
    return ((mode >> 3) & 3) == 2;
}

auto System::changeExpansionPortMemoryMode(bool exrom, bool game, bool noUltimaxIfVicHasTheBus, bool speedHack) -> void {

    uint8_t cartMode = (mode >> 3) & 3;
    uint8_t cartModeNew = (exrom << 1) | game;

    vicII->setUltimax( noUltimaxIfVicHasTheBus ? false : (exrom && !game) );

    if (cartMode == cartModeNew)
        return;

    mode &= 7;
    mode |= cartModeNew << 3;

    remapCpu(speedHack);
}

auto System::hintSlowSpeed(bool state) -> void {
    if (state)
        warp.config |= (unsigned)Interface::WarpMode::SlowSpeed;
    else
        warp.config &= ~(unsigned)Interface::WarpMode::SlowSpeed;
}

auto System::setRunAhead(unsigned frames) -> void {
    runAhead.frames = frames;
    input.updateSampling();
    updateDriveSounds();
}

auto System::setWarpMode( unsigned config ) -> void {
    warp.config = config | (warp.config & (unsigned)Interface::WarpMode::SlowSpeed);
    sidManager.disableAudioOut(config & (unsigned) Emulator::Interface::WarpMode::NoAudioOut);
    vicII->disableSequencer(config & (unsigned) Emulator::Interface::WarpMode::NoVideoSequencer);
    updateDriveSounds();

    if (!config && sidManager.hasIntensifiedPseudoStereo())
        sidManager.applyOffsetPseudoStereo();
}

auto System::setFloppySounds(bool state) -> void {
    driveSounds.requestFloppy = state;
    updateDriveSounds();
}

auto System::setTapeSounds(bool state) -> void {
    driveSounds.requestTape = state;
    updateDriveSounds();
}

auto System::updateDriveSounds() -> void {
    driveSounds.useFloppy = driveSounds.requestFloppy && !warp.config;
    driveSounds.useTape = driveSounds.requestTape && !warp.config;

    if (powerOn) {
        if (driveSounds.useFloppy)
            iecBus.updateDriveSounds();
        if (driveSounds.useTape)
            tape.setMotorSound();
    }
}

auto System::setCycleRenderer(bool state) -> void {
    if (state)
        vicII = &vicIICycle;
    else
        vicII = &vicIIFast;

    cpu.vicII = vicII;
    input.setVic( vicII );
    expansionPort->vicII = vicII;
}

auto System::updateStats() -> void {
    interface->stats.region = vicII->isNTSCGeometry() ? Interface::Region::Ntsc : Interface::Region::Pal;
    interface->stats.sampleIntervall = sidManager.sampleLimit;
    interface->stats.sampleRate = (double)vicII->frequency() / (double)sidManager.sampleLimit;
    interface->stats.fps = 1.0 / ( (double)vicII->cyclesPerFrame() / (double)vicII->frequency() );
    interface->stats.stereoSound = sidManager.isStereo();
    history.reset();
}

auto System::updateStatsStereo() -> void {
    interface->stats.stereoSound = sidManager.isStereo();
}

auto System::useExtraSids(uint8_t requestedSids) -> void {
    auto requestedSidsBefore = this->requestedSids;

    this->requestedSids = requestedSids;
    sidManager.updateSidUsage();

    if (!powerOn)
        return;

    if (requestedSids && !requestedSidsBefore) {
        serializationSize += sidManager.serializationSizeForSevenMoreSids;
        serializationSizeLight += sidManager.serializationSizeForSevenMoreSids;
    } else if (!requestedSids && requestedSidsBefore) {
        serializationSize -= sidManager.serializationSizeForSevenMoreSids;
        serializationSizeLight -= sidManager.serializationSizeForSevenMoreSids;
    }

    sidManager.clone( requestedSidsBefore, requestedSids );
}

auto System::updatePort(uint8_t lines, uint8_t ddr) -> void {

    if (!powerOn)
        return;

    auto modeBefore = mode;

    mode &= ~7;

    mode |= lines & 7;

    if (modeBefore != mode)
        this->remapCpu( );

    tape.writeIn( ((~ddr | lines) & 8) != 0 );
    tape.setMotorIn( ((lines & ddr) & 0x20) == 0 );
}

auto System::videoRefresh( uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void {

    if (diskSilence.active) {
        if (!diskSilence.idle) {
            if ((++diskSilence.idleFrames & 3) == 0) {
                if (diskSilence.idleFrames > (iecBus.has1581() ? 600 : 200) ) {
                    diskSilence.idle = true;
                    diskSilence.idleFrames = 0;
                    driveCycleSyncingUpdate();
                    iecBus.resetDriveState();
                }
            }
        }
    }

    if (!runAhead.pos && frame) {
        crop->apply( frame, width, height, linePitch );
        // for lightguns
        input.drawCursor();
    }

    if (warp.config & (unsigned)Interface::WarpMode::NoVideoOut)
        frame = nullptr;

    else if (warp.renderNext) {
        warp.renderNext = false;
        vicII->disableSequencer( warp.config & (unsigned)Interface::WarpMode::NoVideoSequencer );

    } else if (warp.config & (unsigned)Interface::WarpMode::ReduceVideoOutput) {
        frame = nullptr;

        if ((++warp.frameCounter & 15) == 0) {
            warp.frameCounter = 0;
            vicII->disableSequencer( false );
            warp.renderNext = true;
        }
    }

    if (!runAhead.pos) {
        this->interface->videoRefresh8(frame, width, height, linePitch);
    }

    leaveEmulation = true;

    if ( keyBuffer->hasJobs )
        keyBuffer->process();
}

auto System::setVicIrq( bool state ) -> void {
    if (state)
        irqIncomming |= 1;
    else
        irqIncomming &= ~1;

    expansionPort->observeIrq(irqIncomming != 0);
    cpu.setIrq( irqIncomming != 0 );
}

auto System::setVicRdy(bool state) -> void {
    if (state)
        rdyIncomming |= 1;
    else
        rdyIncomming &= ~1;

    expansionPort->observeRdy(rdyIncomming != 0);
    cpu.setRdy( rdyIncomming != 0 );
}

auto System::pasteText( std::string buffer ) -> void {
    keyBuffer->paste( buffer );
}

auto System::copyText( ) -> std::string {

    Clipboard clipboard(this);
    return clipboard.getText();
}

auto System::checkForAutoStarter() -> bool {
    bool isSuperCpu = dynamic_cast<SuperCpu*>(expansionPort);

    if (!observer.enterRom) {
        if (isSuperCpu) {
            if (superCpu->executeKernalRom())
                observer.enterRom = true;
        } else if (memoryCpu.isLocation( cpu.pc >> 8, &readKernalRom ))
            observer.enterRom = true;

        observer.memoryAccesses = 0;
    } else {

        if (isSuperCpu) {
            if (superCpu->executeHostRam())
                observer.memoryAccesses++;
        } else if (memoryCpu.isLocation( cpu.pc >> 8, &readRam ))
            observer.memoryAccesses++;

        if (observer.memoryAccesses > 2)
            return true;
    }

    return false;
}

auto System::autoStartFinish(bool soft) -> void {
    observer.inputLock = false;
    interface->autoStartFinish(soft);
}

auto System::hintObserverLEDChange(bool state) -> void {
    if (!observer.inputLock && observer.motor && state) {
        observer.stateChange = true;
        observer.inputFetches = 15;
    }
}

auto System::hintObserverMotorChange(bool state) -> void {
    if (!traps.installed && kernalBootComplete)
        observer.stateChange = true;

    if (!observer.inputLock && state && !observer.motor)
        observer.inputFetches = 15;

    observer.motor = state;
}

auto System::informAboutStateChange() -> void {
    observer.stateChange = false;
    uint8_t newState = observer.motor;
    if (!observer.inputLock && !observer.inputFetches)
        newState |= 2;

    interface->hintAutoWarp( newState );
}

auto System::burstOrParallelUpdate() -> void {
    secondDriveCable.burstUse = secondDriveCable.burstRequested && secondDriveCable.burstPossible && iecBus.drivesConnected;
    secondDriveCable.parallelUse = secondDriveCable.parallelRequested && secondDriveCable.parallelPossible && iecBus.drivesConnected;
    secondDriveCable.parallelExpansion = secondDriveCable.parallelUse && (expansionPort == fastloader);
    secondDriveCable.parallelUserport = secondDriveCable.parallelUse && (expansionPort != fastloader);

    driveCycleSyncingUpdate();
}

auto System::driveCycleSyncingUpdate() -> void {

    secondDriveCable.cycleSyncing = (secondDriveCable.burstUse || secondDriveCable.parallelExpansion || secondDriveCable.parallelUserport)
        && !diskSilence.idle;
}

auto System::diskIdleOff() -> void {
    if (diskSilence.idle) {
        diskSilence.idle = false;
        iecBus.resetTicks();
        driveCycleSyncingUpdate();
    }
    diskSilence.idleFrames = 0;
}

auto System::readParallelWithHandshake() -> uint8_t {
    uint8_t out = 0xff;

    if (secondDriveCable.parallelUserport ) {
        cia2.setFlag();
        out = cia2.lines.iob;
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
        out = cia2.lines.iob;
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
        cia2.setFlag();
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

auto System::isC64C() -> bool {
    return glueLogic.type == GlueLogic::Type::CustomIC;
}

auto System::activateDebugCart(unsigned limitCycles) -> void {
    debugCart->set( true, limitCycles );
}

auto System::enabledDebugCart() -> bool {
    return debugCart->enable;
}

auto System::setTapeLoadingNoise(unsigned volume) -> void {
    tapeNoise.enabled = volume > 0;
    tapeNoise.amplitude = (int)volume * -10;
    tapeNoise.reset();
}

auto System::TapeNoise::reset() -> void {
    active = false;
    duration = 0;
    amplitude = (amplitude > 0) ? -amplitude : amplitude;
    circularBuffer.reset();
}

auto System::tapeNoiseSetSample( unsigned duration ) -> void {
    unsigned half = duration >> 1;
    tapeNoise.circularBuffer.write( (duration & 1) ? (half + 1) : half );
    tapeNoise.circularBuffer.write( half );
    tapeNoise.active = true;
}

auto System::audioRefresh(int16_t sample) -> void {
    if (!runAhead.pos) {
        if (tapeNoise.enabled) {
            if (tapeNoise.active) {
                if (tapeNoise.duration > 0) {
                    sample = (int)sample + tapeNoise.amplitude;
                    tapeNoise.duration -= sidManager.sampleLimit;
                } else if (tapeNoise.circularBuffer.pending()) {
                    tapeNoise.duration += (int)tapeNoise.circularBuffer.read() - 1;
                    tapeNoise.amplitude = -tapeNoise.amplitude;
                    sample = (int)sample + tapeNoise.amplitude;
                } else
                    tapeNoise.active = false;
            }
        }

        interface->audioSample(sample, sample);
    }
}

auto System::audioRefreshStereo(int16_t sampleL, int16_t sampleR) -> void {
    if (!runAhead.pos) {
        if (tapeNoise.enabled) {
            if (tapeNoise.active) {
                if (tapeNoise.duration > 0) {
                    sampleL = (int)sampleL + tapeNoise.amplitude;
                    sampleR = (int)sampleR + tapeNoise.amplitude;
                    tapeNoise.duration -= sidManager.sampleLimit;
                } else if (tapeNoise.circularBuffer.pending()) {
                    tapeNoise.duration += (int)tapeNoise.circularBuffer.read() - 1;
                    tapeNoise.amplitude = -tapeNoise.amplitude;
                    sampleL = (int)sampleL + tapeNoise.amplitude;
                    sampleR = (int)sampleR + tapeNoise.amplitude;
                } else
                    tapeNoise.active = false;
            }
        }

        interface->audioSample(sampleL, sampleR);
    }
}

auto System::jam(Emulator::Interface::Media* media) -> void {
    if (processFrame())
        interface->jam(media);
}

auto System::getPrgInstance(Emulator::Interface::Media* media) -> Prg* {

    for(auto prg : prgs) {
        if (prg->media == media )
            return prg;
    }

    return nullptr;
}

auto System::set2Mhz(bool state) -> void {
    mhz2 = state ? (mhz2 | 1) : (mhz2 & ~1);
    if (mhz2 == 0) {
        cpu.setClock(false);
        interface->updateLedState(Emulator::Interface::LedId::MHz2, 0);
    }
}

auto System::toggle2Mhz() -> bool {
    if (mhz2 & 0x80) {
        cpu.setClock(false);
        mhz2 = mhz2 & ~0x80;
    } else {
        cpu.setClock(true, true);
        mhz2 = mhz2 | 0x80;
    }
    interface->updateLedState(Emulator::Interface::LedId::MHz2, (mhz2 & 0x80) ? 1 : 0);
    return mhz2 & 0x80;
}

auto System::debuggerAdd(Emulator::Interface::DebuggerCpu cpuModel, Emulator::Interface::DebuggerAction action, uint32_t addr, uint32_t addrTo) -> void {
    switch (action) {
        case Emulator::Interface::DebuggerAction::Line:
        case Emulator::Interface::DebuggerAction::Frame:
            vicII->debuggerAction = action;
            break;
        default:
            if (cpuModel == Emulator::Interface::DebuggerCpu::C65c816)
                superCpu->debuggerAdd((WDCFAMILY::W65816::DebuggerAction)action, addr, addrTo);
            else if (cpuModel == Emulator::Interface::DebuggerCpu::C6510)
                cpu.debuggerAdd( (M6510::DebuggerAction)action, static_cast<uint16_t>(addr), static_cast<uint16_t>(addrTo) );

            break;
    }
}

auto System::debuggerRemove(Emulator::Interface::DebuggerCpu cpuModel, Emulator::Interface::DebuggerAction action, unsigned addr) -> void {
    if (cpuModel == Emulator::Interface::DebuggerCpu::C65c816)
        superCpu->debuggerRemove( (WDCFAMILY::W65816::DebuggerAction)action, addr);
    else if (cpuModel == Emulator::Interface::DebuggerCpu::C6510)
        cpu.debuggerRemove( (M6510::DebuggerAction)action, addr );
}

auto System::debuggerEnable(Emulator::Interface::DebuggerCpu cpuModel, Emulator::Interface::DebuggerAction action, unsigned addr) -> void {
    if (cpuModel == Emulator::Interface::DebuggerCpu::C65c816)
        superCpu->debuggerEnable((WDCFAMILY::W65816::DebuggerAction)action, addr);
    else if (cpuModel == Emulator::Interface::DebuggerCpu::C6510)
        cpu.debuggerEnable( (M6510::DebuggerAction)action, addr );
}

auto System::debuggerDisable(Emulator::Interface::DebuggerCpu cpuModel, Emulator::Interface::DebuggerAction action, unsigned addr) -> void {
    if (cpuModel == Emulator::Interface::DebuggerCpu::C65c816)
        superCpu->debuggerDisable((WDCFAMILY::W65816::DebuggerAction)action, addr);
    else if (cpuModel == Emulator::Interface::DebuggerCpu::C6510)
        cpu.debuggerDisable( (M6510::DebuggerAction)action, addr );
}

auto System::debuggerDisableAll(Emulator::Interface::DebuggerCpu cpuModel) -> void {
    if (cpuModel == Emulator::Interface::DebuggerCpu::C65c816)
        superCpu->debuggerDisableAll();
    else if (cpuModel == Emulator::Interface::DebuggerCpu::C6510)
        cpu.debuggerDisableAll();
}

auto System::debuggerStepOver() -> void {
    if (expansionPort->haltMainCpu())
        superCpu->debuggerStepOver();
    else
        cpu.debuggerStepOver();
}

auto System::debuggerStepInto() -> void {
    if (expansionPort->haltMainCpu())
        superCpu->debuggerStepInto();
    else
        cpu.debuggerStepInto();
}

auto System::debuggerStepOut() -> bool {
    if (expansionPort->haltMainCpu())
        return superCpu->debuggerStepOut();

    return cpu.debuggerStepOut();
}

auto System::getMemoryDumpBank(uint8_t bank, uint8_t* dump) -> void {
    if (expansionPort->haltMainCpu())
        dynamic_cast<SuperCpu*>(expansionPort)->memoryDumpBank(bank, dump);
}

auto System::getMemoryDumpPage(uint8_t page, uint8_t* dump) -> void {
    if (expansionPort->haltMainCpu())
        dynamic_cast<SuperCpu*>(expansionPort)->memoryDumpPage(page, dump);
    else
        memoryDump(page, dump);
}

auto System::debugPointReached(Emulator::Interface::DebuggerAction action, unsigned addr) -> void {
    leaveEmulation = true;
    debugger.action = action;
    debugger.addr = addr;
}

auto System::updateDebuggerSnapshot(DebuggerSnapshot& snap) -> void {
    if (expansionPort->haltMainCpu()) {
        dynamic_cast<SuperCpu*>(expansionPort)->updateSnapshot(snap);
    } else {
        cpu.updateSnapshot(snap);
        snap.mode = mode;
        snap.superCpu = false;

        for (uint8_t a = 0; a <= 0xf; a++ ) {
            if (memoryCpu.isLocation(a << 4, &readRam)) snap.mapper[a] = 1;
            else if (memoryCpu.isLocation(a << 4, &readVicReg)) snap.mapper[a] = 2;
            else if (memoryCpu.isLocation(a << 4, &readCharRom)) snap.mapper[a] = 3;
            else if (memoryCpu.isLocation(a << 4, &readKernalRom)) snap.mapper[a] = 4;
            else if (memoryCpu.isLocation(a << 4, &readBasicRom)) snap.mapper[a] = 5;
            else if (memoryCpu.isLocation(a << 4, &readRomL)) snap.mapper[a] = 6;
            else if (memoryCpu.isLocation(a << 4, &readRomH)) snap.mapper[a] = 7;
            else if (memoryCpu.isLocation(a << 4, &readUltimaxA0)) snap.mapper[a] = 8;
            else snap.mapper[a] = 0; // unmapped
        }
    }

    vicII->updateSnapshot(snap);
}

auto System::disassemble(unsigned addr, unsigned& bytes) -> std::string {
    if (expansionPort->haltMainCpu())
        return dynamic_cast<SuperCpu*>(expansionPort)->disassemble(addr, bytes);

    return cpu.disassemble(addr, bytes);
}

auto System::disassembleData(unsigned addr, unsigned bytes) -> std::string {
    if (expansionPort->haltMainCpu())
        return dynamic_cast<SuperCpu*>(expansionPort)->disassembleData(addr, bytes);

    return cpu.disassembleData( (uint16_t)addr, bytes );
}

auto System::disassembleTrace(unsigned i, uint8_t& flags) -> std::string {
    if (expansionPort->haltMainCpu())
        return dynamic_cast<SuperCpu*>(expansionPort)->disassembleTrace(i, flags);

    return cpu.disassembleTrace( i, flags );
}

}
