
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
#include "../expansionPort/actionReplay/actionReplayMK2.h"
#include <cstring>

#include "expansion.cpp"
#include "serialization.cpp"

namespace Firmware {
	#include "firmware.cpp"
}

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

    readCharRom = [this](uint16_t addr) {
        if ( !this->charRom ) 
            return (uint8_t)0xff;
        
        return this->charRom[ addr & 0xfff ];
    };

    readKernalRom = [this](uint16_t addr) {
        if (!this->kernalRom)
            return (uint8_t)0xff;
        
        return (uint8_t) this->kernalRom[ addr & 0x1fff ];
    };

    readBasicRom = [this](uint16_t addr) {
        if (!this->basicRom) 
            return (uint8_t)0xff;

        return (uint8_t) this->basicRom[ addr & 0x1fff ];
    };
    
    readRomL = [this](uint16_t addr) {

        return expansionPort->readRomL( addr & 0x1fff );
    };

    readRomH = [this](uint16_t addr) {

        return expansionPort->readRomH( addr & 0x1fff );
    };
    
    readRomHLow = [this](uint16_t addr) {

        return expansionPort->readRomH( addr & 0xfff );
    };
    
    readRomHHi = [this](uint16_t addr) {

        return expansionPort->readRomH( 0x1000 | (addr & 0xfff) );
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

        return expansionPort->readUltimaxA0( addr );
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
        
        if ( port == CIA::Base::PORTA )
            input->writeCiaPortA( lines );
        
        input->writeCiaPortB( lines );
    };
    
    cia2->readPort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {
        
        if ( port == CIA::Base::PORTA ) {            
			if (diskSilence.idle) {
				diskSilence.idle = false;
				iecBus->resetTicks();
			}
			
			diskSilence.idleFrames = 0;
			
            return (uint8_t) ( (lines->ioa & 0x3f) | iecBus->readCia() );
        }
        
        return lines->iob;
    };
    
    cia2->writePort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {
        
        if ( port == CIA::Base::PORTA ) {
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
            }
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
        case 0:
			if (!data) {
				data = (uint8_t*)Firmware::kernalRom;
			} else if (size < sizeof(Firmware::kernalRom)) {
                Firmware::buildAlternateRom( Firmware::kernalRomAlt, data, sizeof(Firmware::kernalRom), size );
                data = (uint8_t*) Firmware::kernalRomAlt;
            }
            kernalRom = data;
            break;
        case 1:
			if (!data) {
				data = (uint8_t*)Firmware::basicRom;
			} else if (size < sizeof(Firmware::basicRom)) {
                Firmware::buildAlternateRom( Firmware::basicRomAlt, data, sizeof(Firmware::basicRom), size );
                data = (uint8_t*) Firmware::basicRomAlt;
            }
            basicRom = data;
            break;
        case 2:
			if (!data) {
				data = (uint8_t*)Firmware::charRom;
			} else if (size < sizeof(Firmware::charRom)) {
                Firmware::buildAlternateRom( Firmware::charRomAlt, data, sizeof(Firmware::charRom), size );
                data = (uint8_t*) Firmware::charRomAlt;
            }
            charRom = data;
            break;
        case 3:
			if (!data) {
				data = (uint8_t*)Firmware::drive1541Rom;
			} else if (size < sizeof(Firmware::drive1541Rom)) {
                Firmware::buildAlternateRom( Firmware::drive1541RomAlt, data, sizeof(Firmware::drive1541Rom), size );
                data = (uint8_t*) Firmware::drive1541RomAlt;
            }
            iecBus->setFirmware( data );
            break;
    }   
}

auto System::power( bool softReset ) -> void {   
	sysTimer.clear();

	if( !softReset )
		initRam();
	    
    expansionPort->reset();
    
    mode = (expansionPort->isExrom() << 1) | expansionPort->isGame();
    
    vicBank = 0;
    mode <<= 3;
    mode |= 7; // charen = hiram = loram = 1 
    irqIncomming = 0;
	nmiIncomming = 0;
    rdyIncomming = 0;
    
	memoryCpu.unmap(0x0, 0xff);
	memoryVic.unmap(0x0, 0xff);
	remapVic();    
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
    
	if( !softReset ) {
        setCycleRenderer( cycleRendererNextBoot );
		
		vicIICycle->power();
        vicIIFast->power();
		cpu->power();        		
	} else {
		// vic hasn't a reset line ... means no change ?
		cpu->reset();
	}
    // cpu doesn't leave halted state by reset request   
    //cpu->setRdy( false );
	
    cpu->updateIoLines( 0x17, !tape->isEnabled() ? 0x20 : 0 );              
    
    kernalBootComplete = false;
    calcSerializationSize();
    if (requestedSids)
        serializationSize += Sid::serializationSizeForSevenMoreSids;
    
	fastForward.config = 0;
    fastForward.frameCounter = 0;
    fastForward.renderNext = false;
    
    if ( !expansionPort->isBootable() ) {
        KeyBuffer::Action action;

        action.mode = KeyBuffer::Mode::WaitDelay;
        action.delay = (interface->getExpansion()->isFreezer() && expansionPort->hasRom())
            ? 1 : 2;        
        action.delay = 2;
        system->keyBuffer->add( action );        
	
        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};  
        
        if (dynamic_cast<ActionReplayMK2*>(expansionPort))
            action.buffer = {'L', 'O', 'A', 'D', 'E', 'R'};  

        action.delay = 0;           
        action.blinkingCursor = true;
        action.callbackId = 1;
        action.callback = [this]() { kernalBootComplete = true; };
        system->keyBuffer->add( action );           
	}
	
	powerOn = true;
}

auto System::powerOff() -> void {
    powerOn = false;
    keyBuffer->reset();
	sid->powerOff();
    iecBus->powerOff();    
    vicII->powerOff();
}

auto System::initRam() -> void {
    
    bool oldHalfPage = 1;
    Emulator::Rand rand;

    for( unsigned i = 0; i <= 0xffff; i++ ) {
        bool pattern = (i >> 6) & 1;
        bool halfPage = (i >> 7) & 1;
        uint8_t val = pattern ? 0xff : 0x0;
        
        if (oldHalfPage && !halfPage) { 
            // first byte of page
            ram[i] = rand.xorShift() & 0xff;
            
            if (ram[i] == val)
                ram[i] = 0xf0;

        } else {
            
            ram[i] = val; 
        }
        
        oldHalfPage = halfPage;       
    }
    // typical demo works only for a few possible values at 0x3fff.
    // could imagine that some real machines can not run this demo.
    ram[0x3fff] = 0; 
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

    bool useRunAhead = !fastForward.config && runAhead.frames && !keyBuffer->isPrgInjectionInQueue();
    
    if (useRunAhead) {        
        runAhead.pos = runAhead.frames;
        vicII->disableSequencer( runAhead.performance );
        Sid::disableAudioOut( runAhead.frames > 1 );
    }
        
    labelRunAhead:    
            
    while( !frameComplete ) {        
        cpu->process();     
        if (!diskSilence.idle)
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
    
    checkDebugCart();
}

auto System::runAheadEnableAudio() -> void {
    if (runAhead.pos == 1)
        Sid::disableAudioOut(false);    
}

auto System::isUltimax() -> bool { 
	return ((mode >> 3) & 3) == 2;
}

auto System::remapCpu( ) -> void {
    
    uint8_t subMode = mode & 7;
    uint8_t ramMode = mode & 3;
    uint8_t cartMode = (mode >> 3) & 3;
	bool ultimax = isUltimax();
    
    // 00 - 0f -> always ram
    memoryCpu.map( &readRam, &writeRam, 0x0, 0x0f );
	
    // 10 - 7f
    if ( ultimax )
        memoryCpu.map( &readUnmapped, &writeUnmapped, 0x10, 0x7f );
    else
        memoryCpu.map( &readRam, &writeRam, 0x10, 0x7f );
    
    // 80 - 9f
    if ( ultimax ) {
        memoryCpu.map( &readRomL, 0x80, 0x9f);
		memoryCpu.map( &writeUltimaxRomL, 0x80, 0x9f );
    
    } else if ( (cartMode == 0 || cartMode == 1) && ramMode == 3 ) {
        memoryCpu.map( &readRomL, 0x80, 0x9f);
		memoryCpu.map( &writeRomL, 0x80, 0x9f );
    } else
		memoryCpu.map( &readRam, &writeRamAt80To9F, 0x80, 0x9f );
	
    // a0 - bf
    if ( ultimax )
        memoryCpu.map( &readUltimaxA0, &writeUltimaxA0, 0xa0, 0xbf );
    
    else if ( (cartMode == 1 || cartMode == 3) && ramMode == 3 ) {
		memoryCpu.map( &readBasicRom, 0xa0, 0xbf );
        memoryCpu.map( &writeRam, 0xa0, 0xbf );
		
    } else if (cartMode == 0 && (ramMode == 2 || ramMode == 3) ) {
        
		memoryCpu.map( &readRomH, 0xa0, 0xbf );
        memoryCpu.map( &writeRomH, 0xa0, 0xbf );
    } else
        memoryCpu.map( &readRam, &writeRam, 0xa0, 0xbf );
    
    // c0 - cf
    if ( ultimax )
        memoryCpu.map( &readUnmapped, &writeUnmapped, 0xc0, 0xcf );
    else
        memoryCpu.map( &readRam, &writeRam, 0xc0, 0xcf );

    // d0 - df
    if ( ultimax || subMode == 5 || subMode == 6 || subMode == 7 ) {
        memoryCpu.map( &readVicReg, &writeVicReg, 0xd0, 0xd3);
        
        if (!debugCart.enable)
            memoryCpu.map( &readSidReg, &writeSidReg, 0xd4, 0xd7);
        else {
            memoryCpu.map( &readSidReg, &writeSidReg, 0xd4, 0xd6);
            memoryCpu.map( &readSidReg, &writeDebugReg, 0xd7, 0xd7);
        }
        memoryCpu.map( &readColorRam, &writeColorRam, 0xd8, 0xdb);
        memoryCpu.map( &readCia1Reg, &writeCia1Reg, 0xdc, 0xdc);
        memoryCpu.map( &readCia2Reg, &writeCia2Reg, 0xdd, 0xdd);
        memoryCpu.map( &readIo1Reg, &writeIo1Reg, 0xde, 0xde);
        memoryCpu.map( &readIo2Reg, &writeIo2Reg, 0xdf, 0xdf);
        
    } else if ( (subMode == 1 || subMode == 2 || subMode == 3) && (mode != 1)  ) {
        
        memoryCpu.map( &readCharRom, 0xd0, 0xdf );
        memoryCpu.map( &writeRam, 0xd0, 0xdf );
    } else
        memoryCpu.map( &readRam, &writeRam, 0xd0, 0xdf );

    // e0 - ff
    if ( ultimax ) {
        memoryCpu.map( &readRomH, 0xe0, 0xff);
		memoryCpu.map( &writeUltimaxRomH, 0xe0, 0xff );
		
    } else if (ramMode == 2 || ramMode == 3) {
        memoryCpu.map( &readKernalRom, 0xe0, 0xff );
		memoryCpu.map( &writeRam, 0xe0, 0xff );
		
    } else
        memoryCpu.map( &readRam, &writeRam, 0xe0, 0xff );
}
    
auto System::remapVic( ) -> void {
    bool ultimax = isUltimax();
	
	memoryVic.map( &writeUnmapped, 0x00, 0xff );
	memoryVic.map( &readRam, 0x00, 0x0f );
	
	memoryVic.unmapRead( 0x10, 0xff );
	
	if ( !ultimax ) {
		memoryVic.map( &readRam, 0x10, 0xff );	
		//overmap charrom
		memoryVic.map( &readCharRom, 0x10, 0x1f );
		memoryVic.map( &readCharRom, 0x90, 0x9f );
		
	} else {
		memoryVic.map( &readUnmapped, 0x10, 0xff );
		// overmap
		memoryVic.map( &readRomHHi, 0x30, 0x3f ); //upper half
		memoryVic.map( &readRomHLow, 0x70, 0x7f );
		memoryVic.map( &readRam, 0x80, 0x9f );	
        
        memoryVic.map( &readUltimaxA0, 0xa0, 0xaf );
        
		memoryVic.map( &readRomHLow, 0xb0, 0xbf );
		memoryVic.map( &readRam, 0xd0, 0xef );	
		memoryVic.map( &readRomHLow, 0xf0, 0xff );
	}
}

auto System::changeExpansionPortMemoryMode(bool exrom, bool game) -> void {
	
	uint8_t cartMode = (mode >> 3) & 3;
	uint8_t cartModeNew = (exrom << 1) | game;
	
	if (cartMode == cartModeNew)
		return;
	
	bool ultimaxBefore = isUltimax();
	
	mode &= 7;
	mode |= cartModeNew << 3;	
	
	if (ultimaxBefore != isUltimax())
		remapVic();
	
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
			if (++diskSilence.idleFrames > 120) {
				diskSilence.idle = true;
				diskSilence.idleFrames = 0;
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

	if (!runAhead.pos)
		this->interface->videoRefresh8( frame, width, height, linePitch );

	frameComplete = true;

	if ( !expansionPort->isBootable() )
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

}
