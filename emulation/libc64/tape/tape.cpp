
#include <cstring>

#include "tape.h"
#include "fetch.cpp"
#include "counter.cpp"
#include "write.cpp"
#include "serialization.cpp"
#include "../system/keyBuffer.h"
#include "../traps/traps.h"

namespace LIBC64 {
    
Tape* tape = nullptr;

Tape::Tape( Emulator::Interface::Media* mediaConnected ) : structure(this) {
    
    media = nullptr;
	this->mediaConnected = mediaConnected;

	fetchData = new uint8_t[ TAPE_FETCH_SIZE ];
	writeData = new uint8_t[ TAPE_WRITE_SIZE ];
	    
    // events
    motorOff = [this]() {
        if (!enabled)
            return;
        motorIn = false;

        updateMotorSound();
        updateCounter();
        updateDeviceState();
        if (autoStarted) {
            if (traps->installed)
                return;

            if (counter >= 15)
                system->motorChange( false );
        }
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

            if (gapsRemaining == 0) {
                setMode(Mode::Stop); //end of tape or completely rewinded
                system->motorChange( false );
            }
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
    autoStarted = false;
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

auto Tape::setMotorSound() -> void {
    if (motorIn) {
        switch(mode) {
            case Mode::Play:
            case Mode::Record:
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapePlaySpin);
                break;
            case Mode::Rewind:
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeRewindSpin);
                break;
            case Mode::Forward:
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeForwardSpin);
                break;
        }
    }
}

auto Tape::updateMotorSound(bool soft) -> void {
    if (!system->driveSounds.useTape)
        return;

    if (motorIn) {
        switch(mode) {
            case Mode::Play:
            case Mode::Record:
                system->interface->mixDriveSound(mediaConnected, soft ? DriveSound::TapePlaySpinUp : DriveSound::TapePlaySpin);
                break;
            case Mode::Rewind:
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeRewindSpin);
                break;
            case Mode::Forward:
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeForwardSpin);
                break;
            case Mode::Stop:
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeSpinDown, 1);
                break;
        }
    } else {
        system->interface->mixDriveSound(mediaConnected, DriveSound::TapeSpinDown, mode == Mode::Stop);
    }
}

auto Tape::setMotorIn( bool state ) -> void {
	
	if (!enabled)
		return;
    
    writeBuffer();
	
    if (state) {
        
		sysTimer.remove( &motorOff );
		
        if (!motorIn) {
            motorIn = true;
            updateMotorSound();
            updateDeviceState();
            if (autoStarted)
                system->motorChange( true );
            
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

            if (system->driveSounds.useTape)
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeAnyButton);

            break;
        
        case Mode::Record:
			senseOut( true );
            directionForward = true;
			writeClock = writeCounterClock = sysTimer.clock;
			// a write changes the file pos, so we have to invalidate the read buffer
			// because it's content is not aligned anymore
			fetchPos = 0;
            if (system->driveSounds.useTape)
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeAnyButton);

            break;      
			
		case Mode::Stop:
			senseOut( false );

            if (system->driveSounds.useTape)
                system->interface->mixDriveSound(mediaConnected, DriveSound::TapeStopButton);
            break;
    }
        
	this->mode = (Mode)mode;
	updateCounter();
    updateMotorSound(false);
}

auto Tape::power() -> void {
    reset();

    autoStarted = false;
    if (system->driveSounds.useTape) {
        if (loaded && !system->powerOn)
            system->interface->mixDriveSound(mediaConnected, DriveSound::TapeInsert);
    }
}

auto Tape::reset() -> void {
    writeQuestionState = 0;
    counter = 0;
	counterOffset = 0;
	writePos = 0;
	writeClock = writeCounterClock = sysTimer.clock;
	cycles = 0;	
	directionForward = lastDirectionForward = true;
	fetchPos = 0;
	fetchSize = 0;
	motorIn = false;
	curPos = 0x14;
	mode = Mode::Stop;
	nextMode = Mode::Stop;
	writeBit = true;	
	gapsRemaining = nextGap();
}

auto Tape::load(Emulator::Interface::Media* media, uint8_t* data, unsigned size) -> void {		
    this->media = media;
    bool attachDetach = loaded;
	unload();
	
	this->rawData = data;
    this->rawSize = size;
    
    if (!readHeader()) {
		loaded = false;
		return;
	}
		
	loaded = true;

	cyclesTotal = 0;
	curPos = 0x14;
	directionForward = lastDirectionForward = true;
	mode = nextMode = Mode::Stop;

	while( true ) {

		unsigned gaps = nextGap( );

		if (gaps == 0)
			break;

		cyclesTotal += gaps;
	}

    structure.setData( rawData, rawSize );
	
	reset();

    if (system->powerOn && system->driveSounds.useTape)
        system->interface->mixDriveSound( mediaConnected, DriveSound::TapeInsert, attachDetach );
}

auto Tape::unload() -> void {
    setMode( Mode::Stop );
	writeBuffer();

    if (loaded) {
        if (system->powerOn && system->driveSounds.useTape)
            system->interface->mixDriveSound( mediaConnected, DriveSound::TapeEject );
    }
	
	this->rawSize = 0;
	this->rawData = nullptr;
	loaded = false;
	motorIn = false;
    writeQuestionState = 0;
	gapsRemaining = 0;
}

auto Tape::readHeader() -> bool {
	
	if (rawSize < 21)
		return false;
	
	uint8_t* header;
	
	if (rawData)
		header = rawData;
	
	else {		
		header = new uint8_t[20];
		
		if ( read( header, 20, 0 ) != 20 )
			return false;				
	}
	
	if (std::memcmp(header, "C64-TAPE-RAW", 12))
		return false;
			
	version = header[0xc];
	
	if (!rawData)
		delete[] header;
			
	return true;
}

auto Tape::setEnabled( bool state ) -> void {
	enabled = state;
}

auto Tape::setWobble(bool state) -> void {
	wobble = state;
}

auto Tape::getListing() -> std::vector<Emulator::Interface::Listing>& {

    return structure.getListing();
}

auto Tape::selectListing( unsigned pos, bool useTraps ) -> void {

    if (pos == 0)
        pos = 1;

    curPos = 0x14;
    if (structure.setFile(pos - 1))  {
        if (pos > 1) {
            advanceCounterToPos( structure.getCurFile()->offset );
        }
    }

    fetchPos = 0;

    if (useTraps) {
        if (!traps->testForComplexTapeLoader())
            traps->installTape();
    }

    if ( system->kernalBootComplete )
        return;

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

    action.callbackId = 4;
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};
    action.delay = 800; // seconds
    action.alternateBuffer.clear();
    action.blinkingCursor = true;
    action.callback = nullptr;
    action.waitCallback = [this]( KeyBuffer::Action* action) {
        if (system->checkForAutoStarter()) {
			// keep check for "ready" alive, some games autostart first file and then load another file without autostarting it
			action->position = 0;
			action->delay = 5000; // frames
			action->waitCallback = nullptr;
         //   system->keyBuffer->reset();
            system->interface->autoStartFinish(true);
        }
    };
    system->keyBuffer->add(action);

    action.callbackId = 5;
    action.callback = [this]() {
        system->interface->autoStartFinish(true);
    };
    action.waitCallback = nullptr;
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = {'R', 'U', 'N', '\r'};
    system->keyBuffer->add(action);

    system->keyBuffer->forceDefaultKernalDelay(); // a possible speeder use shorter boot time

    autoStarted = true;
}

auto Tape::advanceCounterToPos(unsigned pos) -> void {
    bool _longGap;
    unsigned gaps;
    curPos = 0x14; // skip tape header
    fetchPos = 0;

    while( true ) {
        gaps = fetchGap(_longGap);

        cycles += gaps;

        if (gaps == 0 || (curPos >= pos))
            break;
    }

    updateCounter();
    curPos = pos;
    fetchPos = 0;
    gapsRemaining = 0;
}

auto Tape::setPosition( unsigned pos, bool find ) -> void {

    if (find) {
        sysTimer.remove(&worker);
        curPos = pos;
        system->interface->mixDriveSound(mediaConnected, DriveSound::TapeSpinDown);
    } else {
        sysTimer.remove(&motorOff);
        sysTimer.add(&worker, 1, Emulator::SystemTimer::Action::UpdateExisting);

        advanceCounterToPos(pos);
    }

    fetchPos = 0;
}

}
