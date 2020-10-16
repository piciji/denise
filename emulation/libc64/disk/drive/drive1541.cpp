
#include "drive1541.h"
#include "mechanics.cpp"
#include "serialization.cpp"
#include "../../../tools/gcr.h"

namespace LIBC64 {
   
// one cpu cycle is 16 reference cycles.
// we do only progress 6 instead of 8 in first half cycle because of a possible
// external overflow is recognized by cpu within 400 ns.
 
// in case of a VIA2 READ there is more time a change can be read back ~ 875 ns within cycle.
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
    
#define SYNC    \
    if (useAccuracy())  \
        rotateG64(8);   \
    cpu->handleSo();   \
    processDelays();    \
    if (useAccuracy())  \
        rotateG64(8);   \
    else \
        rotateD64();    \
    via1->process();    \
    via2->process();     \
    cycleCounter += iecBus->cpuCylcesPerSecond;
    
auto Drive1541::sync() -> void {
    SYNC 
}

auto Drive1541::cpuWrite(uint16_t addr, uint8_t data) -> void {
    SYNC    
    memory.write( addr, data );
}

auto Drive1541::cpuRead(uint16_t addr) -> uint8_t {
    SYNC    
    return memory.read( addr );
}
    
Drive1541::Drive1541(uint8_t number) {
     
    this->number = number; 
	structure1541.number = number;
    
    media = nullptr;
    
    ram = new uint8_t[ 2 * 1024 ];
    rom = nullptr;
    romSize = 0;        
    
    via1 = new Via( 1 );
    via2 = new Via( 2 );
    
    cpu = new M6502(this);
    
    calculateRefCyclesPerRevolution();            
          
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
    
    writeVia1Reg = [this](uint16_t addr, uint8_t value) {

        via1->write( addr, value );
    };

    readVia1Reg = [this](uint16_t addr) {
	
        return via1->read( addr );
    };

    writeVia2Reg = [this](uint16_t addr, uint8_t value) {
		
        via2->write( addr, value );
    };

    readVia2Reg = [this](uint16_t addr) {

        return via2->read(addr);
    };

    readRam = [this](uint16_t addr) {

        return this->ram[ addr ];
    };
    
    writeRam = [this](uint16_t addr, uint8_t value) {

        this->ram[ addr ] = value;
    };

    readRom = [this](uint16_t addr) {

        if ( !this->rom )
            return (uint8_t)0xff;
        
        return this->rom[addr];
    };
    
    writeUnmapped = [this](uint16_t addr, uint8_t value) {
        // do nothing
    };
    
    readUnmapped = [this](uint16_t addr) {
        return addr >> 8;
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
        }                
    };   
    
    via1->readPort = [this]( Via::Port port, Via::Lines* lines ) {
        
        if (port == Via::Port::B) {            
            // invert the three input bits, add device number  
            return (uint8_t)( ((0x1a | iecBus->readVia()) ^ 0x85) | (this->number << 5) ); 
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
                
                updateState( );
            }
            
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
        
        uint8_t value = 0;
        
        if ( !readMode || attachDelay );
        else
            value = readBuffer & 0xff;
        
        return (value & ~lines->ddra) | ( lines->pra & lines->ddra );
    };
    
    via2->ca2Out = [this]( bool state ) {
        
        byteReadyOverflow = state;
    };
    
    via2->cb2Out = [this]( bool state ) {
            
        if ( readMode != state )
            updateState( );
        
        readMode = state;                
    };
    
    structure1541.write = [this](uint8_t* buffer, unsigned length, unsigned offset) {
		
		return system->interface->writeMedia( getMedia(), buffer, length, offset );
	};
    
    updateState = [this]() {		
        
		system->interface->updateDriveState( getMedia(), getTrackState(), (currentHalftrack + 2) / 2 );
	};
    
    remap();
    
    for(unsigned i = 0; i < motorOff.CHUNKS; i++)
        motorOff.chunkSize.push_back( 0 );
} 

Drive1541::~Drive1541() {    
    
    delete[] ram;
}

auto Drive1541::updateBus() -> void {
    
    clockOut = !((via1->lines.iob >> 3) & 1);    
    dataOut =  !((via1->lines.iob >> 1) & 1);
    atnOut =  (via1->lines.iob >> 4) & 1;            

    if ( iecBus->atnOut == atnOut )
        dataOut = 0;
}

auto Drive1541::remap( bool romOnly ) -> void {
    
    // some gaps remain unmapped
    if(romOnly)
        memory.map( &readUnmapped, &writeUnmapped, 0x80, 0xff, Memory::Mode::Direct);
    else    
        memory.map( &readUnmapped, &writeUnmapped, 0x00, 0xff, Memory::Mode::Direct);
    
    if (!romOnly) {
        // overmap ram, rom, io
        memory.map( &readRam, &writeRam, 0x00, 0x07);
        memory.map( &readRam, &writeRam, 0x20, 0x27);
        memory.map( &readRam, &writeRam, 0x40, 0x47);
        memory.map( &readRam, &writeRam, 0x60, 0x67);

        memory.map( &readVia1Reg, &writeVia1Reg, 0x18, 0x1b);
        memory.map( &readVia1Reg, &writeVia1Reg, 0x38, 0x3b);
        memory.map( &readVia1Reg, &writeVia1Reg, 0x58, 0x5b);
        memory.map( &readVia1Reg, &writeVia1Reg, 0x78, 0x7b);

        memory.map( &readVia2Reg, &writeVia2Reg, 0x1c, 0x1f);
        memory.map( &readVia2Reg, &writeVia2Reg, 0x3c, 0x3f);
        memory.map( &readVia2Reg, &writeVia2Reg, 0x5c, 0x5f);
        memory.map( &readVia2Reg, &writeVia2Reg, 0x7c, 0x7f);
    }
    
    memory.map( &readRom, 0x80, 0xbf, Memory::Mode::Linear, 0, romSize);
    memory.map( &readRom, 0xc0, 0xff, Memory::Mode::Linear, 0, romSize);
}

auto Drive1541::power( ) -> void {    
    
    for( unsigned i = 0; i < (2 * 1024); i++ )       
		ram[i] = 0;   

    via1->reset();
    via2->reset();  

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
    cpu->power();    
 
    ue7Counter = uf4Counter = 0;
    filter = lastFilter = 0;
    randCounter = 0;
    randomizer.initXorShift( 0x1234abcd );
    
    motorOn = false;
    motorOff.slowDown = false;
    readBuffer = writeBuffer = 0;
    writeValue = 0x55;
    bitCounter = 0;
    accum = 0;
    headOffset = 0;
    randomizeRpm();
    currentHalftrack = 17 * 2;
    stepDirection = 0;
    
    changeHalfTrack(0);
}

auto Drive1541::powerOff( ) -> void {  
    write();  
    motorOn = false;
    updateState( );
}

auto Drive1541::setFirmware(uint8_t* rom, unsigned romSize) -> void {
    
    bool needRemap = this->romSize != romSize;
    
    this->rom = rom;
    this->romSize = romSize;
    
    if(needRemap)
        remap( true );
}

auto Drive1541::setViaTransition( bool state ) -> void {
    via1->ca1In( state ); 
    
    // we need to check how much the drive is ahead of the c64.
    // if the drive is more than two cycles ahead we need to manually call the
    // cpu irq detector because the drive cpu run a few cycles without knowing from interrupt.
    // NOTE: the drive cpu is interrupted before irq sample cycle.
    // so it can only pass opcode edge when not fully synced. means not the sample cycle is missable
    // but the recognition cycle.
    // if the drive cpu could be interrupted each cycle this step wouldn't be necessary. for performance
    // and code complexity reasons i have decided the drive cpu can only be interrupted before read/write
    // access and before an irq sample cycle but not during address generation or interrupt service routine.
    if (cycleCounter >= (iecBus->cpuCylcesPerSecond << 1) )
        via1->handleInterrupt();
}

inline auto Drive1541::processDelays() -> void {

    if (detachDelay)
        detachDelay--;
    else if (attachDetachDelay)
        attachDetachDelay--;
    else if (attachDelay)
        attachDelay--;
}

auto Drive1541::detach() -> void {
    write();
    
    if (loaded)
        detachDelay = DISC_DELAY;
    
    structure1541.detach();
    motorOff.slowDown = false;
    
    loaded = false;        
}

auto Drive1541::attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size ) -> void {
    this->media = media;
    detach();
    accum = 0;
    randCounter = 0;
    filter = lastFilter = 0;
    uf4Counter = ue7Counter = 0;
    bitCounter = 0;
    
	structure1541.media = media;
    if ( !structure1541.attach( data, size ) )
        return;
    
    attachDelay = DISC_DELAY;
    
    if (detachDelay)
        attachDetachDelay = DISC_DELAY;
    else
        attachDelay = DISC_DELAY * 3;
        
    loaded = true;
}

auto Drive1541::setWriteProtect(bool state) -> void {
    
    writeProtected = state;        
}

auto Drive1541::writeprotectSense() -> uint8_t {

    if (detachDelay)
        return 0;
    
    if (attachDetachDelay)
        return 0x10;
    
    if (attachDelay)
        return 0;
    
    if (!loaded)
        return 0x10;
    
    return writeProtected ? 0 : 0x10;
}

auto Drive1541::write() -> void {
    
    if (!written)
        return;
    
    written = false;
    
    auto _imageSize = structure1541.getStateImageSize();
    if (system->serializationSize > _imageSize)
        system->serializationSize -= _imageSize;
    
    if (!loaded)
        return;

    if (!system->interface->questionToWrite(media))
        return;
    
    structure1541.storeWrittenTracks();
}

inline auto Drive1541::getTrackState() -> TrackState {
    
    if (!motorOn)
        return TrackState::NoOperation;
    
    if (currentHalftrack & 1)
        return readMode ? TrackState::ReadHalf : TrackState::WriteHalf;
    
    return readMode ? TrackState::Read : TrackState::Write;
}

inline auto Drive1541::useAccuracy() -> bool {    
    return structure1541.type == Structure1541::Type::G64;
}

auto Drive1541::setSpeed(double rpm, double wobble) -> void {
	
    this->rpm = rpm * 100.0 + 0.5;
    this->wobble = wobble * 100.0 + 0.5;

}

}
