
#include <cstring>

#include "tape.h"
#include "fetch.cpp"
#include "counter.cpp"
#include "write.cpp"
#include "serialization.cpp"
#include "../system/system.h"
#include "../system/keyBuffer.h"

namespace LIBC64 {
    
Tape* tape = nullptr;

Tape::Tape( Emulator::Events* events ) {
    
    this->events = events;
	fetchData = new uint8_t[ TAPE_FETCH_SIZE ];
	writeData = new uint8_t[ TAPE_WRITE_SIZE ];
	    
    // events
    motorOff = [this]() {

        motorIn = false;        
        updateCounter();
    };

    worker = [this]() {
        if (!motorIn || mode == Mode::Stop || mode == Mode::Record)
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
            		
		this->events->remove( &worker );
		
        if (gaps)
            this->events->add( &worker, gaps * speedAdjustment());

        updateCounter();
    };     
	
	delayMode = [this]() {
		setMode( nextMode );
	};        

	enabled = false;
    loaded = false;
    cylcesPerSecond = 0;
	reset();
    
    events->registerCallback( { {&motorOff, 1}, {&worker, 1}, {&delayMode, 1} } );
}    

Tape::~Tape() {
    delete[] fetchData;
	delete[] writeData;
}


auto Tape::setMotorIn( bool state ) -> void {
	
	if (!enabled)
		return;
    
    writeBuffer();
	
    if (state) {
        
		events->remove( &motorOff );
		
        if (!motorIn) {
            motorIn = true;
            
            if (!events->has( &worker ))
                events->add( &worker, TAPE_MOTOR_DELAY );
        }
        
    } else {
        
        if (motorIn && !events->has( &motorOff ) )
			events->add( &motorOff, TAPE_MOTOR_DELAY );
    }       
}

auto Tape::getMode( ) -> Mode {
    return mode;
}

auto Tape::setMode( unsigned mode ) -> void {
	
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
		events->add( &delayMode, 40000 ); // roughly 2 frames
		return;
	}
	
	events->remove( &delayMode );
	
    lastDirectionForward = directionForward;
    
    switch(mode) {
        case Mode::Rewind:
            adjust = 0;
        case Mode::Play:
        case Mode::Forward:			
			senseOut( true );
            directionForward = mode != Mode::Rewind;
            
            if (motorIn && !events->has( &worker ))
                events->add( &worker, TAPE_MOTOR_DELAY );
                
            break;
        
        case Mode::Record:
			senseOut( true );
            directionForward = true;
			cyclesElapsed = 0;	
			// a write changes the file pos, so we have to invalidate the read buffer
			// because it's content is not aligned anymore
			fetchPos = 0;
			adjust = 0;
            break;      
			
		case Mode::Stop:
			senseOut( false );
			break;
    }
        
	this->mode = (Mode)mode;
	updateCounter();
}

auto Tape::reset() -> void {
    counter = 0;
	counterOffset = 0;
	writePos = 0;
	cyclesElapsed = 0;
	cycles = 0;	
	directionForward = lastDirectionForward = true;
	adjust = 0;
	fetchPos = 0;
	fetchSize = 0;
	motorIn = false;
	pos = 0x14;
	mode = Mode::Stop;
	nextMode = Mode::Stop;
    currentCounter.cstate = CounterState::NoOperation;
	writeBit = true;	
	gapsRemaining = nextGap();
}

auto Tape::load(uint8_t* data, unsigned size) -> void {		
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
	adjust = 0;
	directionForward = lastDirectionForward = true;
	mode = nextMode = Mode::Stop;

	while( true ) {

		unsigned gaps = nextGap( );

		if (gaps == 0)
			break;

		cyclesTotal += gaps;
	}		
	
	senseOut( false );	
	
	reset();
}

auto Tape::unload() -> void {
    setMode( Mode::Stop );
	writeBuffer();
	
	this->size = 0;
	this->data = 0;
	loaded = false;
	motorIn = false;
	gapsRemaining = 0;
	updateState( CounterState::NoOperation, 0 );
    currentCounter.cstate = CounterState::NoOperation;
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

auto Tape::clock() -> void {    
    
	if (mode == Mode::Record)
		cyclesElapsed++;
}

auto Tape::setEnabled( bool state ) -> void {
	
	enabled = state;
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
        this->setMode( Tape::Mode::Play );
    };
    system->keyBuffer->add( action );
    
    action.callback = nullptr;
    action.callbackId = 0;
    
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};
    action.delay = 800;
    action.alternateBuffer.clear();
    action.blinkingCursor = true;    
    system->keyBuffer->add(action);
    
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = {'R', 'U', 'N', '\r'};    
    system->keyBuffer->add(action);    
}

}
