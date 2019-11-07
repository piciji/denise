
#include "system.h"
#include "../input/input.h"
#include "../prg/prg.h"
#include "../tape/tape.h"
#include "../vic/vicII.h"
#include "../disk/iec.h"
#include "../sid/sid.h"
#include "keyBuffer.h"
#include "gluelogic.h"
#include "../../tools/crop.h"
#include "../../tools/powersupply.h"
#include "../../tools/serializer.h"
#include "../../tools/rand.h"
#include <cstring>

#include "expansion.cpp"
#include "serialization.cpp"

namespace Firmware {
	#include "firmware.cpp"
}

namespace LIBC64 {
    
System* system = nullptr;
    
System::System(Interface* interface) {
    
    this->interface = interface;
	
	ram = new uint8_t[ 64 * 1024 ];
    colorRam = new uint8_t[ 1 * 1024 ];
    
    createExpansions();
	vicII = new VicII;    
	sid = new Sid( Sid::Type::MOS_6581, &events );
    input = new Input;
	prg = new Prg;
    keyBuffer = new KeyBuffer;
    glueLogic = new GlueLogic( &events );
	crop = new Emulator::Crop;
	powerSupply = new Emulator::PowerSupply;
	tape = new Tape( &events );
    iecBus = new IecBus;
    
    cia1 = new CIA::M6526( 1, &events );
    cia2 = new CIA::M6526( 2, &events );
    cpu = MOS65FAMILY::create6510();	    
   
    cpuCtx = MOS65FAMILY::createContext();
    
    cpuCtx->read = [this]( uint16_t addr ) {
        // we don't need to check for AEC low, which would decouple CPU from BUS.
        // there is no situation, where AEC is pulled low without RDY pulled low.
        // it wouldn't make any sense.
        return memoryCpu.read( addr );
    };
    
    cpuCtx->write = [this]( uint16_t addr, uint8_t value ) {
	
        if (expansionPort->isDma())
            // DMA halts CPU in next read cycle, but a write cycle happened before.
            // DMA pulls AEC low too at the same time.
            // we can not write here, because CPU is decoupled from BUS.
            return;
        
		memoryCpu.write( addr, value );
    };
    
    cpuCtx->writeSelect = [this](uint16_t addr) {
        
        if (expansionPort->isDma())
            return;
        
        // CPU doesn't update data BUS.
        // write uses last BUS value (always VIC data from first half cycle)
        // CPU signals address 0 or 1 this way only, no need for further checks.
        this->ram[ addr ] = vicII->lastReadPhase1();
    };
    
    cpuCtx->readSelect = [this]() {
        // CPU watches BUS in case of RDY.
        
        // vicII sends AEC to CPU, 3 cycles after it sends BA(RDY) to CPU.
        // expansion port DMA pulls AEC low too.
        if ( vicII->isAecLow() || expansionPort->isDma() )
            // CPU is in tri-state and decoupled from BUS
            return cpu->dataBus();

        // CPU is BUS Master in second half cycle, so it reads last BUS value from first half cycle,
        // which is always accessed by VIC.
        return vicII->lastReadPhase1();
    };
    
    cpuCtx->syncLo = [this]() {
        events.process();
        powerSupply->tick();        
        cia1->processLo();        
		vicII->phase1();
        cia2->processLo();
		sid->phase1();    
        expansionPort->cycleLo();
    };
    
    cpuCtx->syncHi = [this]() {               
        // let the cia1 process before Vic, so Vic can latch a lightpen trigger late this half cycle
        expansionPort->cycleHi();
        cia1->processHi();        
		vicII->phase2();
        cia2->processHi();
		sid->phase2();
		tape->clock();
        iecBus->countTicks();  
        input->clock();        
    };
    
    cpuCtx->updatePort = [this](uint8_t lines, uint8_t ddr) {
        
        auto modeBefore = mode;
        
        mode &= ~7;
        
        mode |= lines & 7;     
        
        if (modeBefore != mode)        
            this->remapCpu( );
		
        tape->writeIn( ((~ddr | lines) & 8) != 0 );
        tape->setMotorIn( ((lines & ddr) & 0x20) == 0 );
    };        
    
    cpu->setContext( cpuCtx ); 
    cpu->setSo(1);
    
    readRam = [this](uint16_t addr) {

        return this->ram[ addr ];
    };
    
    writeRam = [this](uint16_t addr, uint8_t value) {
    
        this->ram[ addr ] = value;
    };

    readCharRom = [this](uint16_t addr) {
        if ( !this->charRom ) 
            return (uint8_t)0xff;
        
        return this->charRom[ addr ];
    };

    readKernalRom = [this](uint16_t addr) {
        if (!this->kernalRom)
            return (uint8_t)0xff;
        
        return (uint8_t) this->kernalRom[ addr ];
    };

    readBasicRom = [this](uint16_t addr) {
        if (!this->basicRom) 
            return (uint8_t)0xff;

        return (uint8_t) this->basicRom[ addr ];
    };
    
    readRomL = [this](uint16_t addr) {

        return expansionPort->readRomL( addr );
    };

    readRomH = [this](uint16_t addr) {

        return expansionPort->readRomH( addr );
    };
    
    writeRomL = [this](uint16_t addr, uint8_t value) {

        return expansionPort->writeRomL( addr, value );
    };

    writeRomH = [this](uint16_t addr, uint8_t value) {

        return expansionPort->writeRomH( addr, value );
    };
    
    writeUltimaxRomL = [this](uint16_t addr, uint8_t value) {

        return expansionPort->writeUltimaxRomL( addr, value );
    };

    writeUltimaxRomH = [this](uint16_t addr, uint8_t value) {

        return expansionPort->writeUltimaxRomH( addr, value );
    };
    
    writeUnmapped = [this](uint16_t addr, uint8_t value) {
        // do nothing
    };
    
    readUnmapped = [this](uint16_t addr) {
        return vicII->lastReadPhase1();
    };

    writeIo1Reg = [this](uint16_t addr, uint8_t value) {
        expansionPort->writeIo1(addr, value);
    };

    readIo1Reg = [this](uint16_t addr) {
        
        return expansionPort->readIo1(addr);			
    };
    
    writeIo2Reg = [this](uint16_t addr, uint8_t value) {

        expansionPort->writeIo2(addr, value);
    };

    readIo2Reg = [this](uint16_t addr) {
		
        return expansionPort->readIo2(addr);		
    };

    writeSidReg = [this](uint16_t addr, uint8_t value) {
        
        sid->writeIOPipelined( addr, value );
    };

    writeDebugReg = [this](uint16_t addr, uint8_t value) {
        if ( (addr & 0xff) == 0xff) {
            debugCart.exitCode = value;
            debugCart.exit = true;
        }
            
        sid->writeIOPipelined(addr, value);
    };

    readSidReg = [this](uint16_t addr) {

        return sid->readIO( addr );
    };

    writeVicReg = [this](uint16_t addr, uint8_t value) {
        
        vicII->writeIOPipelined( addr & 0xff, value );
    };

    readVicReg = [this](uint16_t addr) {

        return vicII->readIO( addr & 0xff );
    };
    
    writeCia1Reg = [this](uint16_t addr, uint8_t value) {

        cia1->writePipelined( addr, value );
    };

    readCia1Reg = [this](uint16_t addr) {
	
        return cia1->read( addr );
    };

    writeCia2Reg = [this](uint16_t addr, uint8_t value) {
		
        cia2->writePipelined( addr, value );
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

    vicII->readColor = readColorRam;
    
    vicII->read = [this](uint16_t addr) {
        
        return memoryVic.read( (addr & 0x3fff) | (vicBank << 14) );        
    };
	
	vicII->readCpu = [this]() {
        // we are in second half cycle and VIC pulled BA low but doesn't own BUS.
        // it takes 3 further cycles till VIC can access BUS in second half cycle.
        // so this function is called for 3 cycles in a row.
        // first we need to find out who is BUS Master? CPU or expansion port ?
        if ( !expansionPort->isDma() )
            // at this point CPU is only halted by BA(RDY) when entering a read cycle.
            // even when cpu is halted the address is selected on BUS and the VIC reads
            // in second half cycle from this address but not the CPU.            
            return memoryCpu.read( cpu->addressBus() );
        
        // expansion port is BUS Master... same explanation as above
        return memoryCpu.read( expansionPort->addressBus() );            
    };
    
    vicII->isCharRomAccessed = [this](uint16_t addr) {
        
        addr = (addr & 0x3fff) | (vicBank << 14);
        
        return memoryVic.isLocation( addr >> 8, &readCharRom );        
    };
    
    vicII->videoRefresh = [this]( uint16_t* frame, unsigned width, unsigned height, unsigned linePitch) {
		
		crop->apply( frame, width, height, linePitch );
        // for lightguns
        input->drawCursor();
		
        this->interface->videoRefresh( frame, width, height, linePitch );
        
        frameComplete = true;		
        
        if ( !expansionPort->isBootable() )
            keyBuffer->process();
        
        cpu->hintUnblockedExecution();
    };
	
	vicII->midScreenCallback = [this]() {
		
		input->drawCursor(true);
		
		this->interface->midScreenCallback();
	};
	
	vicII->vblankCallback = [this]() {
		this->interface->finishVBlank();
	};
    
    vicII->setIrq = [this]( bool state ) {
        if (state)
            irqIncomming |= 1;
        else
            irqIncomming &= ~1;      
                    
        cpu->setIrq( irqIncomming != 0 );
    };

    vicII->setRdy = [this](bool state) {
        if (state)
            rdyIncomming |= 1;
        else
            rdyIncomming &= ~1;      
        
        cpu->setRdy( rdyIncomming != 0 );        
    };
    
    sid->audioRefresh = [this](int16_t sample) {
        
        this->interface->audioSample( sample, sample );
    };
    
    sid->getPotX = [this]() {
        
        return input->readPotX();
    };
    
    sid->getPotY = [this]() {
        
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
        
        if ( port == CIA::Base::PORTA )
            return (uint8_t) ( (lines->ioa & 0x3f) | iecBus->readCia() );
        
        return lines->iob;
    };
    
    cia2->writePort = [this]( CIA::Base::Port port, CIA::Base::Lines* lines ) {
        
        if ( port == CIA::Base::PORTA ) {
            // the c64 II or c64c has another glue logic for updating the vic bank
            glueLogic->setVBank( ( ~(lines->ioa & 3) ) & 3, !lines->praChange );
            iecBus->writeCia( ~lines->ioa );
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

	powerSupply->addCallback( [this]( ) {
		cia1->tod( );
	} );

	powerSupply->addCallback( [this]( ) {
		cia2->tod( );
	} );
	
	tape->setReadTransition = [this]() {
		
		cia1->setFlag();
	};
    
	tape->updateState = [this](unsigned mode, unsigned counter) {
		
		this->interface->updateDriveState(tape->getMedia(), mode, counter);
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
    delete vicII;
    destroyExpansions();
}

auto System::setFirmware( unsigned typeId, uint8_t* data, unsigned size ) -> void {
	
    switch (typeId) {        
        case 0:
			if (!data) {
				data = (uint8_t*)Firmware::kernalRom;
				size = sizeof(Firmware::kernalRom);
			}
            kernalRomSize = size;
            kernalRom = data;
            break;
        case 1:
			if (!data) {
				data = (uint8_t*)Firmware::basicRom;
				size = sizeof(Firmware::basicRom);
			}
            basicRomSize = size;
            basicRom = data;
            break;
        case 2:
			if (!data) {
				data = (uint8_t*)Firmware::charRom;
				size = sizeof(Firmware::charRom);
			}
            charRomSize = size;
            charRom = data;
            break;
        case 3:
			if (!data) {
				data = (uint8_t*)Firmware::drive1541Rom;
				size = sizeof(Firmware::drive1541Rom);
			}
            iecBus->setFirmware( data, size );
            break;
    }   
}

auto System::power( bool softReset ) -> void {   

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
    
	// sid, cia: difference between reset and power on ?
	sid->reset();
    cia1->reset();
    cia2->reset();
	input->reset();

	tape->reset();
    glueLogic->reset();	
    
	if (ntsc) {
		powerSupply->init( C64_FREQUENCY_NTSC, 60 );
		tape->setCyclesPerSecond( C64_FREQUENCY_NTSC );
        iecBus->setCpuCyclesPerSecond( C64_FREQUENCY_NTSC );
        
	} else {
		powerSupply->init( C64_FREQUENCY_PAL, 50 );
		tape->setCyclesPerSecond( C64_FREQUENCY_PAL );
        iecBus->setCpuCyclesPerSecond( C64_FREQUENCY_PAL );
    }
    initDebugCart();
    
    iecBus->power();   
    
	if( !softReset ) {
		vicII->setNtsc( ntsc );
		vicII->power();
		cpu->power();        		
	} else {
		// vic hasn't a reset line ... means no change ?
		cpu->reset();
	}
    // cpu doesn't leave halted state by reset request   
    cpu->setRdy( false );
	
    cpu->updateIoLines( 0x17, !tape->isEnabled() ? 0x20 : 0 );      
    events.clear();   
    
    kernalBootComplete = false;
    calcSerializationSize();
    
    if ( !expansionPort->isBootable() ) {
        KeyBuffer::Action action;
        action.mode = KeyBuffer::Mode::WaitDelay;
        action.delay = 2;        
        system->keyBuffer->add( action );

        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};  
        action.delay = 0;           
        action.blinkingCursor = true;
        action.callbackId = 1;
        action.callback = [this]() { kernalBootComplete = true; };
        system->keyBuffer->add( action );       
    }        
}

auto System::powerOff() -> void {
    keyBuffer->reset();
	sid->powerOff();
    iecBus->powerOff();    
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

auto System::run() -> void {
    frameComplete = false;
    input->poll();
    // of course real system sends restore when key is pressed, but polling each cycle for this is useless
    // because host updates pressed keys once per frame only
    if (input->restore())
        nmiIncomming |= 1;
    else
        nmiIncomming &= ~1;

    cpu->setNmi(nmiIncomming != 0);
    iecBus->randomizeRpm();
    
    while( !frameComplete ) {        
        cpu->process();             
        iecBus->syncDrives(); 
    }  
    
    checkDebugCart();
}

auto System::setNtsc(bool state) -> void {
    ntsc = state;
}

auto System::isNtsc() -> bool {
    return ntsc;
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
    memoryCpu.map( &readRam, &writeRam, 0x0, 0x0f, Memory::Mode::Direct );
	
    // 10 - 7f
    if ( ultimax )
        memoryCpu.map( &readUnmapped, &writeUnmapped, 0x10, 0x7f, Memory::Mode::Direct );
    else
        memoryCpu.map( &readRam, &writeRam, 0x10, 0x7f, Memory::Mode::Direct );
    
    // 80 - 9f
    if ( ultimax ) {
        memoryCpu.map( &readRomL, 0x80, 0x9f);
		memoryCpu.map( &writeUltimaxRomL, 0x80, 0x9f, Memory::Mode::Direct );
    
    } else if ( (cartMode == 0 || cartMode == 1) && ramMode == 3 ) {
        memoryCpu.map( &readRomL, 0x80, 0x9f);
		memoryCpu.map( &writeRomL, 0x80, 0x9f, Memory::Mode::Direct );
    } else
		memoryCpu.map( &readRam, &writeRam, 0x80, 0x9f, Memory::Mode::Direct );
	
    // a0 - bf
    if ( ultimax )
        memoryCpu.map( &readUnmapped, &writeUnmapped, 0xa0, 0xbf, Memory::Mode::Direct );
    
    else if ( (cartMode == 1 || cartMode == 3) && ramMode == 3 ) {
		memoryCpu.map( &readBasicRom, 0xa0, 0xbf, Memory::Mode::Linear, 0, basicRomSize );
        memoryCpu.map( &writeRam, 0xa0, 0xbf, Memory::Mode::Direct );
		
    } else if (cartMode == 0 && (ramMode == 2 || ramMode == 3) ) {
        
		memoryCpu.map( &readRomH, 0xa0, 0xbf );
        memoryCpu.map( &writeRomH, 0xa0, 0xbf, Memory::Mode::Direct );
    } else
        memoryCpu.map( &readRam, &writeRam, 0xa0, 0xbf, Memory::Mode::Direct );
    
    // c0 - cf
    if ( ultimax )
        memoryCpu.map( &readUnmapped, &writeUnmapped, 0xc0, 0xcf, Memory::Mode::Direct );
    else
        memoryCpu.map( &readRam, &writeRam, 0xc0, 0xcf, Memory::Mode::Direct );

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
        
        memoryCpu.map( &readCharRom, 0xd0, 0xdf, Memory::Mode::Linear, 0, charRomSize );
        memoryCpu.map( &writeRam, 0xd0, 0xdf, Memory::Mode::Direct );
    } else
        memoryCpu.map( &readRam, &writeRam, 0xd0, 0xdf, Memory::Mode::Direct );

    // e0 - ff
    if ( ultimax ) {
        memoryCpu.map( &readRomH, 0xe0, 0xff);
		memoryCpu.map( &writeUltimaxRomH, 0xe0, 0xff, Memory::Mode::Direct );
		
    } else if (ramMode == 2 || ramMode == 3) {
        memoryCpu.map( &readKernalRom, 0xe0, 0xff, Memory::Mode::Linear, 0, kernalRomSize );
		memoryCpu.map( &writeRam, 0xe0, 0xff, Memory::Mode::Direct );
		
    } else
        memoryCpu.map( &readRam, &writeRam, 0xe0, 0xff, Memory::Mode::Direct );
}
    
auto System::remapVic( ) -> void {
    bool ultimax = isUltimax();
	
	memoryVic.map( &writeUnmapped, 0x00, 0xff, Memory::Mode::Direct );
	memoryVic.map( &readRam, 0x00, 0x0f, Memory::Mode::Direct );
	
	memoryVic.unmapRead( 0x10, 0xff );
	
	if ( !ultimax ) {
		memoryVic.map( &readRam, 0x10, 0xff, Memory::Mode::Direct );	
		//overmap charrom
		memoryVic.map( &readCharRom, 0x10, 0x1f, Memory::Mode::Linear, 0, charRomSize );
		memoryVic.map( &readCharRom, 0x90, 0x9f, Memory::Mode::Linear, 0, charRomSize );
		
	} else {
		memoryVic.map( &readUnmapped, 0x10, 0xff, Memory::Mode::Direct );
		// overmap
		memoryVic.map( &readRomH, 0x30, 0x3f, Memory::Mode::Linear, 16 ); //upper half
		memoryVic.map( &readRomH, 0x70, 0x7f );
		memoryVic.map( &readRam, 0x80, 0x9f, Memory::Mode::Direct );	
		memoryVic.map( &readRomH, 0xb0, 0xbf );
		memoryVic.map( &readRam, 0xd0, 0xef, Memory::Mode::Direct );	
		memoryVic.map( &readRomH, 0xf0, 0xff );
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

}

