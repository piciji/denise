
#include "drive1541.h"
#include "../iec.h"
#include "mechanics.cpp"
#include "mechanicsP64.cpp"
#include "mechanicsG64.cpp"
#include "serialization.cpp"
#include "../../system/firmware.h"
#include "../../../tools/gcr.h"

// for 300 rpm = 5 rotation / sec = 16.000.000 / 5

namespace LIBC64 {

// valid for 1 MHz operation
// one cpu cycle is 16 reference(drive) cycles.
// we do only progress 6 instead of 8 in first half cycle because of a possible
// external overflow is recognized by cpu within 400 ns.
 
// in case of a VIA READ there is more time a change can be read back ~ 875 ns within cycle.
// 6 ref cycles are progressed in first half cycle already, so we need 8 more to get 14 of 16 ref cycles.
    
// for each cycle:

// 6 ref cycles:    check for external overflow
// + 8    
// 14 ref cycles:   VIA2 read back Changes
// + 2    
// 16 ref cycles:   complete cycle
// + 6
// repeat this pattern    

// the distance between "overflow" checking and maximum "Read back" time is 8 ref cycles, phase shifted by 2 ref cycles.
// the relative	distance matters, so we can step in 8 ref cycle chunks which is handled in rotateP64 and rotateG64
    
#define SYNC \
    cpu->handleSo();                                                    \
    if (operation & USERDATA_LEVEL) {                   \
        rotateD64();                                                        \
    } else if (operation & ENCODEDDATA_LEVEL) {            \
        rotateG64();                                                  \
    } else {                                                                \
        rotateP64();                                                  \
    }                                                                       \
    via1->process();                                                        \
    via2->process();                                                        \
    if (operation & DRIVE_MODE_157x)                                        \
        cia->clock();         \
    cycleCounter += iecBus->cpuCylcesPerSecond;             \
    if (attachDelay)              \
        attachDelay--;
    
auto Drive1541::sync() -> void {
    SYNC 
}

auto Drive1541::cpuWrite(uint16_t addr, uint8_t data) -> void {
    SYNC

    if (operation & DRIVE_MODE_154x) {
        if ((addr & 0x9800) == 0)
            ram[addr & 0x7ff] = data;

        else if ((addr & 0x9c00) == 0x1800)
            via1->write(addr, data);

        else if ((addr & 0x9c00) == 0x1c00)
            via2->write(addr, data);

    } else {
        if ((addr & 0xf000) == 0)
            ram[addr & 0x7ff] = data;

        else if ((addr & 0xfc00) == 0x1800)
            via1->write(addr, data);

        else if ((addr & 0xfc00) == 0x1c00) {
            byteReady = false;
            via2->write(addr, data);
        } else if ((addr & 0xc000) == 0x4000) {
            cia->write(addr, data);
        }
        else if ((addr & 0xe000) == 0x2000);
            // todo
    }
}

auto Drive1541::cpuRead(uint16_t addr) -> uint8_t {
    SYNC

    if (operation & DRIVE_MODE_154x) {
        if (addr & 0x8000)
            return rom[addr & 0x3fff];

        else if ((addr & 0x9800) == 0)
            return ram[addr & 0x7ff];

        else if ((addr & 0x9c00) == 0x1800)
            return via1->read(addr);

        else if ((addr & 0x9c00) == 0x1c00)
            return via2->read(addr);

        else
            return addr >> 8;
    }

    if (addr & 0x8000)
        return rom[addr & 0x7fff];

    else if ((addr & 0xf000) == 0)
        return ram[addr & 0x7ff];

    else if ((addr & 0xfc00) == 0x1800)
        return via1->read(addr);

    else if ((addr & 0xfc00) == 0x1c00) {
        // TED line of U6 clears the Byte line in 2 Mhz mode.
        // Line is connected to Chip select of VIA 2. any access of VIA2 clears the line.
        byteReady = false;
        return via2->read(addr);

    } else if ((addr & 0xc000) == 0x4000) {
        return cia->read(addr);
    }
    else if ((addr & 0xe000) == 0x2000) {
        return 0;
    }
    else
        return addr >> 8;
}

Drive1541::Drive1541(uint8_t number, Emulator::Interface::Media* mediaConnected ) {
     
    this->number = number; 
	this->mediaConnected = mediaConnected;
	
	structure1541.number = number;
	type = Type::D1541II;
	operation = 0;

	emulateDxxMoreAccurate = false;
    media = nullptr;
    wasAttachDetached = false;

    frequency = 1000000;
    refCyclesInCpuCycle = 16;
    
    ram = new uint8_t[ 2 * 1024 ];

    rom1541II = (uint8_t*)Firmware::drive1541IIRom;
    rom1541 = (uint8_t*)Firmware::drive1541Rom;
    rom1541C = (uint8_t*)Firmware::drive1541CRom;
    rom1571 = (uint8_t*)Firmware::drive1571Rom;
    rom1570 = (uint8_t*)Firmware::drive1570Rom;
    rom = rom1541II;
    
    via1 = new Via( 1 );
    via2 = new Via( 2 );
    cia = new Cia8520( 3 );
    cpu = new M6502(this);

    cia->serialOut = [this](bool bit) {

        if (dataDirection) {
            system->diskIdleOff();

            if (system->burstMode.use) {
                cia1->serialIn(bit);
            }
        }
    };

    cia->irqCall = [this](bool state) {
        if (state)
            irqIncomming |= 4;
        else
            irqIncomming &= ~4;

        cpu->setIrq( irqIncomming != 0 );
    };

    via1->irqCall = [this](bool state) {                
        if (state)
            irqIncomming |= 1;
        else
            irqIncomming &= ~1;

        cpu->setIrq( irqIncomming != 0 );
    };
    
    via2->irqCall = [this](bool state) {
		if (state)
			irqIncomming |= 2;
		else
			irqIncomming &= ~2; 
		
        cpu->setIrq( irqIncomming != 0 );
    };

    //PB 7, CB2: ATN IN
    //PB 6,5: Device address preset switches
    //PB 4:	ATN acknowledge OUT
    //PB 3:	CLOCK OUT
    //PB 2:	CLOCK IN
    //PB 1:	DATA OUT
    //PB 0:	DATA IN
    
    via1->writePort = [this]( Via::Port port, Via::Lines* lines ) {        
        
        if (port == Via::Port::B) {
            
            if (lines->iob != lines->iobOld) {
            
                updateBus();
                                            
                iecBus->updatePort();
            }
        } else {

            if (type == Type::D1570 || type == Type::D1571) {
                dataDirection = !!(lines->ioa & 2);

                if ((lines->ioa ^ lines->ioaOld) & 0x20) {
                    updateCycleSpeed(lines->ioa & 0x20, false);
                }
            }

            if (type == Type::D1571) {
                uint8_t _side = side;
                side = !!(lines->ioa & 4);

                if (!structure1541.hasSecondSide())
                    side = 0;

                if (side != _side)
                    changeHalfTrack( 0 );
            }
        }
    };   
    
    via1->readPort = [this]( Via::Port port, Via::Lines* lines ) {
        
        if (port == Via::Port::B) {
            // invert the three input bits, add device number  
            return (uint8_t)( ((0x1a | iecBus->readVia()) ^ 0x85) | (this->number << 5) ); 
        }

        // port A
        if (type == Type::D1570 || type == Type::D1571) {
            return (uint8_t) ( ( ( (byteReady ? 0 : 0x80) | ((currentHalftrack == 0) ? 0 : 1) | 0x7e ) & ~lines->ddra) | ( lines->pra & lines->ddra ) );

        } else if (type == Type::D1541C) {
            return (uint8_t) ( ( ( ((currentHalftrack == 0) ? 0 : 1) | 0xfe ) & ~lines->ddra) | ( lines->pra & lines->ddra ) );
        }

        return lines->ioa;
    };
    
    via2->writePort = [this]( Via::Port port, Via::Lines* lines ) {        
        
        if (port == Via::Port::B) {
            
            if (lines->iob & 4) { // stepper motor works only when drive motor is active

                uint8_t step = ((lines->iob & 3) - (currentHalftrack & 3)) & 3;

                if (step != 0)
                    changeHalfTrack( step );                
            }                            
            
            speedZone = (lines->iob >> 5) & 3;                        

            if ((lines->iob ^ lines->iobOld) & 4) {
                // motor switched between on/off 
                 motorOn = (lines->iob & 4) != 0;
                 if (!motorOn)
                    motorOffInit();
                
                updateDeviceState();

                bool _loadingState = false;
                for( auto drive : iecBus->drivesEnabled ) {
                    if (drive->motorOn) {
                        _loadingState = true;
                        break;
                    }
                }

                if (structure1541.autoStarted)
                    system->motorChange( _loadingState );
            }
            
            // LED status change
            if ((lines->iob ^ lines->iobOld) & 8)
                updateDeviceState();
            
        } else {
            // port A
            writeValue = lines->ioa;
        }
    };
        
    via2->readPort = [this]( Via::Port port, Via::Lines* lines ) {

        if (port == Via::Port::B) {

            // only bit 7 and 4 are input bits, all others reads 1 in input mode
            return ( (syncFound() | writeprotectSense() | 0x6f) & ~lines->ddrb)
                | (lines->prb & lines->ddrb); // output mode
        }

        // port A
        return (latchedByte & ~lines->ddra) | ( lines->pra & lines->ddra );
    };
    
    via2->ca2Out = [this]( bool state ) {
        
        byteReadyOverflow = state;
    };

    via2->cb2Out = [this]( bool state ) {

        if ( readMode != state ) {
            readMode = state;
            updateDeviceState();
        }
    };
    
    structure1541.write = [this](uint8_t* buffer, unsigned length, unsigned offset) {
		
		return system->interface->writeMedia( getMedia(), buffer, length, offset );
	};
    
    for(unsigned i = 0; i < motorOff.CHUNKS; i++)
        motorOff.chunkSize.push_back( 0 );
} 

Drive1541::~Drive1541() {    
    
    delete[] ram;
}

auto Drive1541::updateDeviceState() -> void {
        
    system->interface->updateDeviceState( getMediaConnected(), !readMode, (side * MAX_TRACKS_1541 * 2) + currentHalftrack + 2, via2->lines.iob & 8, !motorOn );
}

// missing BUS communication
auto Drive1541::updateIdleDeviceState() -> void {
    
    system->interface->updateDeviceState( getMediaConnected(), !readMode, (side * MAX_TRACKS_1541 * 2) + currentHalftrack + 2, false, true );

    if (structure1541.autoStarted)
        system->motorChange( false );
}

auto Drive1541::updateBus() -> void {
    
    clockOut = !((via1->lines.iob >> 3) & 1);    
    dataOut =  !((via1->lines.iob >> 1) & 1);
    atnOut =  (via1->lines.iob >> 4) & 1;            

    if ( iecBus->atnOut == atnOut )
        dataOut = 0;
}

auto Drive1541::power( ) -> void {    
    
    std::memset(ram, 0, 2 * 1024);
    setFirmwareByType();

    via1->reset();
    via2->reset();
    cia->reset();

    via1->cb1In(1);
    via1->ca2In(1);
    via1->cb2In(1);
    via2->cb1In(1);
    via2->cb2In(1); // read

    irqIncomming = 0;
    clockOut = dataOut = atnOut = 1;  
    cycleCounter = 0;
    speedZone = 0;
    byteReadyOverflow = false;
    readMode = true;
    byteReady = true;
    cpu->power();    
 
    ue7Counter = uf4Counter = 0;
    randCounter = 0;
    randomizer.initXorShift( 0x1234abcd );
    
    motorOn = false;
    motorOff.slowDown = false;
    readBuffer = writeBuffer = 0;
    writeValue = 0x55;
    latchedByte = 0x55;
    ue3Counter = 0;
    accum = 0;
    headOffset = 0;
    currentHalftrack = 17 * 2;
    stepDirection = 0;
    structure1541.autoStarted = false;
    structure1541.serializationSize = 0;
    pulseIndex = -1;
    pulseDelta = 1;
    comperatorFlipFlop = false;
    uf6aFlipFlop = false;
    pulseDuration = 0;
    side = 0;
    dataDirection = true;
    syncPos = 0;
    updateCycleSpeed(false);
    changeHalfTrack(0);
    randomizeRpm();
}

auto Drive1541::updateCycleSpeed(bool mhz2x, bool init) -> void {

    if (mhz2x) {
        //system->interface->log("2 mhz", 1);
        refCyclesInCpuCycle = 8;
        frequency = 2000000;
        if (!init) {
            cycleCounter *= 2;
            attachDelay <<= 1;
            driveCycles = frequency;
        }
        syncPosRead = (int64_t)(-0.875 * (double)iecBus->cpuCylcesPerSecond);
        syncPosWrite = (int64_t)(0.875 * (double)iecBus->cpuCylcesPerSecond);

    } else {
        //system->interface->log("1 mhz", 1);
        refCyclesInCpuCycle = 16;
        frequency = 1000000;
        if (!init) {
            cycleCounter /= 2;
            attachDelay >>= 1;
            driveCycles = frequency;
        }
        syncPosRead = (int64_t)(-0.455 * (double)iecBus->cpuCylcesPerSecond);
        syncPosWrite = (int64_t)(0.455 * (double)iecBus->cpuCylcesPerSecond);
    }

    setSyncPos( syncPos );
}

auto Drive1541::setSyncPos(int direction) -> void {
    if (direction < 0)
        syncPos = syncPosRead;
    else if (direction > 0)
        syncPos = syncPosWrite;
    else
        syncPos = 0;
}

auto Drive1541::powerOff( ) -> void {  
    write();  
    motorOn = false;
}

auto Drive1541::setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {

    switch (typeId) {
        case Interface::FirmwareIdVC1541II:
            if (!data || (size != 16384))
                data = (uint8_t*)Firmware::drive1541IIRom;
            rom1541II = data;
            break;
        case Interface::FirmwareIdVC1541:
            if (!data || (size != 16384))
                data = (uint8_t*)Firmware::drive1541Rom;
            rom1541 = data;
            break;
        case Interface::FirmwareIdVC1541C:
            if (!data || (size != 16384))
                data = (uint8_t*)Firmware::drive1541CRom;
            rom1541C = data;
            break;
        case Interface::FirmwareIdVC1571:
            if (!data || (size != 32768))
                data = (uint8_t*)Firmware::drive1571Rom;
            rom1571 = data;
            break;
        case Interface::FirmwareIdVC1570:
            if (!data || (size != 32768))
                data = (uint8_t*)Firmware::drive1570Rom;
            rom1570 = data;
            break;
    }
}

auto Drive1541::setViaTransition( bool state ) -> void {
	
	// we need to check how much the drive is ahead of the c64.
    // if the drive is more than two cycles ahead we need to manually register
    // IRQ in CPU, because the drive cpu run a few cycles without knowing from interrupt.
    // NOTE: the drive CPU is interrupted before IRQ sample cycle.
    // so it can only pass opcode edge when not fully synced. means not the sample cycle is missable
    // but the recognition cycle.
    // for performance and code complexity reasons i have decided the drive CPU can only be interrupted
	// before read/write access and before an irq sample cycle,
	// but not during address generation or interrupt service routine. (because there is no VIA access)
	
	// we check by half cycles, hence CPU IRQ line must be stable during second half cycle for recognition

	int64_t half = iecBus->cpuCylcesPerSecond >> 1;

	if (cycleCounter >= (iecBus->cpuCylcesPerSecond + half)) {
		// expects CPU has missed IRQ recognition
		via1->ca1In( state, false);
		via1->handleInterrupt();

	} else if (cycleCounter >= half )
		// expects IRQ recognition this cycle
		via1->ca1In( state, false);

	else
		// expects IRQ recognition next cycle
		via1->ca1In( state, true);	
}

auto Drive1541::detach() -> void {
    write();
    
    if (loaded)
        attachDelay = DISC_DELAY;

    if (iecBus->powerOn && use2Mhz() )
        attachDelay <<= 1;
    
    structure1541.detach();
    motorOff.slowDown = false;
    
    loaded = false;
    pulseIndex = -1;
    pulseDelta = 1; // to reload quickly

    if (type == Type::D1570 || type == Type::D1571)
        via1->ca2In( true );
}

auto Drive1541::attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size, bool loadGracefully ) -> void {
    this->media = media;
    detach();
    accum = 0;
    randCounter = 0;
    uf6aFlipFlop = comperatorFlipFlop = false;
    uf4Counter = ue7Counter = 0;
    ue3Counter = 0;
    
	structure1541.media = media;

    wasAttachDetached = attachDelay != 0;
    attachDelay = DISC_DELAY * 3;

    if (iecBus->powerOn && use2Mhz() )
        attachDelay <<= 1;

    if ( !structure1541.attach( data, size, loadGracefully ) )
        return;

    postAttach();
}

auto Drive1541::postAttach() -> void {
    pulseIndex = gcrTrack->firstPulse;

    loaded = true;

    if (type == Type::D1570 || type == Type::D1571)
        via1->ca2In( !writeProtected );

    operation &= ~(USERDATA_LEVEL | ENCODEDDATA_LEVEL | FLUXDATA_LEVEL);

    if (structure1541.type == Structure1541::Type::D64 || structure1541.type == Structure1541::Type::D71) {
        if (emulateDxxMoreAccurate)
            operation |= ENCODEDDATA_LEVEL;
        else
            operation |= USERDATA_LEVEL;
    } else if (structure1541.type == Structure1541::Type::G64 || structure1541.type == Structure1541::Type::G71)
        operation |= ENCODEDDATA_LEVEL;
    else if (structure1541.type == Structure1541::Type::P64 || structure1541.type == Structure1541::Type::P71)
        operation |= FLUXDATA_LEVEL;
}

auto Drive1541::setWriteProtect(bool state) -> void {
    
    writeProtected = state;
}

auto Drive1541::writeprotectSense() -> uint8_t {

    if (attachDelay) {
        if (wasAttachDetached) {
            if ( (attachDelay > DISC_DELAY) && (attachDelay < (DISC_DELAY << 1)))
                return 0x10;
        }
        return 0;
    }
    
    if (!loaded)
        return 0x10;
    
    return writeProtected ? 0 : 0x10;
}

auto Drive1541::write() -> void {
    
    if (!written)
        return;
    
    written = false;

    if (structure1541.serializationSize) {
        system->serializationSize -= structure1541.serializationSize;
        structure1541.serializationSize = 0;
    }
    
    if (!loaded)
        return;

    if (!system->interface->questionToWrite(media))
        return;
    
    structure1541.storeWrittenTracks();
}

auto Drive1541::setSpeed(unsigned rpmScaled) -> void {

    this->rpm = rpmScaled;
}

auto Drive1541::setWobble(unsigned wobbleScaled) -> void {

    this->wobble = wobbleScaled;
}

auto Drive1541::setType( Type type ) -> void {
    this->type = type;

    updateCycleSpeed(false);

    operation &= ~(DRIVE_MODE_154x | DRIVE_MODE_157x);

    if (type == Type::D1541II || type == Type::D1541 || type == Type::D1541C)
        operation |= DRIVE_MODE_154x;

    else if (type == Type::D1571 || type == Type::D1570)
        operation |= DRIVE_MODE_157x;

    setFirmwareByType();
}

auto Drive1541::setFirmwareByType( ) -> void {
    switch (type) {
        default:
        case Type::D1541II: rom = rom1541II;break;
        case Type::D1541:   rom = rom1541; break;
        case Type::D1541C:  rom = rom1541C; break;
        case Type::D1571:   rom = rom1571; break;
        case Type::D1570:   rom = rom1570; break;
    }
}

}
