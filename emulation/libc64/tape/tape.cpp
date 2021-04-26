
#include <cstring>

#include "tape.h"
#include "fetch.cpp"
#include "counter.cpp"
#include "write.cpp"
#include "serialization.cpp"
#include "../system/keyBuffer.h"

namespace LIBC64 {
    
Tape* tape = nullptr;

Tape::Tape( Emulator::Interface::Media* mediaConnected ) {
    
    media = nullptr;
	this->mediaConnected = mediaConnected;

	fetchData = new uint8_t[ TAPE_FETCH_SIZE ];
	writeData = new uint8_t[ TAPE_WRITE_SIZE ];
	    
    // events
    motorOff = [this]() {
        if (!enabled)
            return;
        motorIn = false;
        updateCounter();
        updateDeviceState();
        if (counter >= 10)
            system->interface->informDriveLoading(false);
    };
	
    worker = [this]() {
        if (!enabled || !motorIn || mode == Mode::Stop || mode == Mode::Record)
            return;
        
        if (directionForward != lastDirectionForward) {            
            gapsRemaining = nextGap() - gapsRemaining;
            lastDirectionForward = directionForward;
        }

        if (!gapsRemaining) {

            if (loaded && (mode == Mode::Play))
                setReadTransition(); // cia flag
            // gap cycles till next flux transition
            gapsRemaining = nextGap();

            if (gapsRemaining == 0)
				setMode( Mode::Stop ); //end of tape or completely rewinded
        }
		
		unsigned gaps;
		
        if (gapsRemaining > TAPE_MAX_EVENT_DELAY) {
            gapsRemaining -= TAPE_MAX_EVENT_DELAY;
            gaps = TAPE_MAX_EVENT_DELAY;
        } else {
            gaps = gapsRemaining;
            gapsRemaining = 0;
        }
		
        // use real gap count for tape length
        if (directionForward)
            cycles += gaps;
        
        else {
            
            if (gaps > cycles) {
            
                if (!loaded) {
                    cycles = cycles999 - ( gaps - cycles );
                } else
                    cycles = 0;
                
            } else
                cycles -= gaps;    
        }            		
			
        if (gaps)
            sysTimer.add( &worker, gaps * speedAdjustment());
		
        updateCounter();
    };     
	
	delayMode = [this]() {
        if( !enabled)
            return;
        
		setMode( nextMode );
		updateDeviceState();
	};        

	enabled = false;
    loaded = false;
    cylcesPerSecond = 0;
	reset();
    
    sysTimer.registerCallback( { {&motorOff, 1}, {&worker, 1}, {&delayMode, 1} } );
}    

Tape::~Tape() {
    delete[] fetchData;
	delete[] writeData;
}

auto Tape::updateDeviceState() -> void {
    system->interface->updateDeviceState( getMediaConnected(), mode == Mode::Record, counter, false, !motorIn );
}

auto Tape::setMotorIn( bool state ) -> void {
	
	if (!enabled)
		return;
    
    writeBuffer();
	
    if (state) {
        
		sysTimer.remove( &motorOff );
		
        if (!motorIn) {
            motorIn = true;
            updateDeviceState();
            system->interface->informDriveLoading(true);
            
            if (!sysTimer.has( &worker ))
                sysTimer.add( &worker, TAPE_MOTOR_DELAY );
        }
        
    } else {
        
        if (motorIn && !sysTimer.has( &motorOff ) )
			sysTimer.add( &motorOff, TAPE_MOTOR_DELAY );
	}
}       

auto Tape::getMode( ) -> Mode {
    return mode;
}

auto Tape::setMode( unsigned mode, bool buttonPress ) -> void {
	
	if (!enabled)
		return;
	
	if ( mode == Mode::ResetCounter ) {
		resetCounter();
		return;
	}
	
	writeBuffer(); //if some data left
	
	if (this->mode == mode)
		return;
	
	if ( ( this->mode != Tape::Mode::Stop ) && ( mode != Tape::Mode::Stop ) ) {		
		// real datasette can not switch from one mode to another without pressing
		// stop button before, so we trigger a stop event now and a few frames later
		// the requested event.
		// why does this matter? stop state changes tape sense, which software could poll
		nextMode = (Mode)mode;	
		setMode( Mode::Stop );
        if (!sysTimer.has(&delayMode))
            sysTimer.add( &delayMode, 40000 ); // roughly 2 frames
		return;
	}
	
    if (sysTimer.delay( &delayMode ))
        sysTimer.remove( &delayMode );
	
    lastDirectionForward = directionForward;
    
    switch(mode) {
        case Mode::Rewind:
        case Mode::Play:
        case Mode::Forward:
			senseOut( true );
            directionForward = mode != Mode::Rewind;
            
            if (motorIn && !sysTimer.has( &worker )) {
                sysTimer.add(&worker, TAPE_MOTOR_DELAY);
            }
                
            break;
        
        case Mode::Record:
			senseOut( true );
            directionForward = true;
			writeClock = sysTimer.clock;	
			// a write changes the file pos, so we have to invalidate the read buffer
			// because it's content is not aligned anymore
			fetchPos = 0;
            break;      
			
		case Mode::Stop:
			senseOut( false );
			break;
    }
        
	this->mode = (Mode)mode;
	updateCounter();
}

auto Tape::reset() -> void {
    writeQuestionState = 0;
    counter = 0;
	counterOffset = 0;
	writePos = 0;
	writeClock = sysTimer.clock;
	cycles = 0;	
	directionForward = lastDirectionForward = true;
	fetchPos = 0;
	fetchSize = 0;
	motorIn = false;
	pos = 0x14;
	mode = Mode::Stop;
	nextMode = Mode::Stop;
	writeBit = true;	
	gapsRemaining = nextGap();
}

auto Tape::load(Emulator::Interface::Media* media, uint8_t* data, unsigned size) -> void {		
    this->media = media;
	unload();
	
	this->data = data;
    this->size = size;
    
    if (!readHeader()) {
		loaded = false;
		return;
	}
		
	loaded = true;

	cyclesTotal = 0;
	pos = 0x14;
	directionForward = lastDirectionForward = true;
	mode = nextMode = Mode::Stop;

	while( true ) {

		unsigned gaps = nextGap( );

		if (gaps == 0)
			break;

		cyclesTotal += gaps;
	}			
	
	reset();
}

auto Tape::unload() -> void {
    setMode( Mode::Stop );
	writeBuffer();
	
	this->size = 0;
	this->data = 0;
	loaded = false;
	motorIn = false;
    writeQuestionState = 0;
	gapsRemaining = 0;
}

auto Tape::readHeader() -> bool {
	
	if (size < 21)
		return false;
	
	uint8_t* header;
	
	if (data)		
		header = data;
	
	else {		
		header = new uint8_t[20];
		
		if ( read( header, 20, 0 ) != 20 )
			return false;				
	}
	
	if (std::memcmp(header, "C64-TAPE-RAW", 12))
		return false;
			
	version = header[0xc];
	
	if (!data)
		delete[] header;
			
	return true;
}

auto Tape::setEnabled( bool state ) -> void {
	
	enabled = state;
}

auto Tape::setWobble(bool state) -> void {
	wobble = state;
}

auto Tape::selectListing( unsigned pos ) -> void {
    // at the moment only position at 0 possible

    KeyBuffer::Action action;
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = {'L', 'O', 'A', 'D', '\r'};
    system->keyBuffer->add(action);    
    
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'P', 'R', 'E', 'S', 'S', ' ', 'P', 'L', 'A', 'Y', ' ', 'O', 'N', ' ', 'T', 'A', 'P', 'E'};  
    action.blinkingCursor = false;
    action.delay = 0;
    action.callbackId = 3;
	action.callback = [this]() {
        this->setMode( Tape::Mode::Play, true );
	};
    system->keyBuffer->add( action );
    

    action.callbackId = 0;
    
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};
    action.delay = 800;
    action.alternateBuffer.clear();
    action.blinkingCursor = true;
    action.callback = [this]() {
        system->interface->informDriveLoading(false);
    };
    system->keyBuffer->add(action);

    action.callback = nullptr;
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = {'R', 'U', 'N', '\r'};    
    system->keyBuffer->add(action);    
}

}
