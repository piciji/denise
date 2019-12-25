
#include "sid.h"

#include "register.cpp"
#include "envelope.cpp"
#include "voice.cpp"
#include "filter/main.cpp"
#include "filter/external.cpp"
#include "serialization.cpp"

namespace LIBC64 {
      
Sid* sid = nullptr;        
    
Sid::Sid( Type type, Emulator::Events* events ) : filter( this ) {

	digiBoost = false;
    lastBusValue = 0;
    this->events = events;
	
    setType( type );
	
	Envelope::dac6581.generate();	
	Envelope::dac8580.generate();

	Voice::dac6581.generate();	
	Voice::dac8580.generate();
	
	voice[0].setSyncSource( &voice[2] );
	voice[1].setSyncSource( &voice[0] );
	voice[2].setSyncSource( &voice[1] );
	
	for( unsigned i = 0; i < 3; i++ ) {       
        voice[i].envelope = &envelope[i];
        voice[i].events = events;
        envelope[i].events = events;
    }
	
	moreAccuracy = false;
    audioOut = true;
	idle = true;
	ready = false;
    powerOn = false;
    
    getPotX = []() { return 0xff; };
    getPotY = []() { return 0xff; };
    callPotUpdate = []() { };
    
    registerCallbacks();
	
	std::thread worker( [this] {

        std::chrono::milliseconds duration(5);
            
		while(true) {
			
			while ( !ready.load() ) {
                
                if (idle.load())
                    std::this_thread::sleep_for( duration );                                            
                    
                // consumes thread fully in non idle mode.
                // even a thread::yield would slow down this thread too much to be usefull.
                // without a thread::yield there is no re scheduling possible, so be carefull.
                // this mode would crash a single core cpu hard.
			}
			
			filter.clockMulti(v1, v2, v3);

			externalFilter.clock( filter.outputMulti() );	    

            if (++sampleCounter == SID_SAMPLE_COUNTER ) {
                audioRefresh( externalFilter.output( ) );
                sampleCounter = 0;
            }

			if(registerWriteThreaded.pipelined)
				// if filter update, do it now
				writeIOFilter( this->registerWriteThreaded.addr, this->registerWriteThreaded.value );

			this->ready = false;
		}
	});	
	
	worker.detach();
}

auto Sid::registerCallbacks() -> void {
    
    events->registerCallback( { &callPotUpdate, 1 } );
    
    for( unsigned i = 0; i < 3; i++ ) {   
        
        voice[i].registerCallbacks();
        
        envelope[i].registerCallbacks();
    }
}

auto Sid::setMoreAccuracy(bool state) -> void {
	
	moreAccuracy = state;
	updateIdleState();
    
	ready = false;	
    
    if (moreAccuracy)
        filter.multiPrecalculate();
}

auto Sid::updateIdleState() -> void {
    
    idle = !powerOn ? true : !moreAccuracy;
}

auto Sid::setType( Type type ) -> void {

    this->type = type;
    
    for( unsigned i = 0; i < 3; i++ ) {
        voice[i].setType( type );
        envelope[i].setType( type );
    }	
    filter.setType( type );
    
    databusDecayTime = type == MOS_8580 ? 0xa2000 : 0x1d00;

    // update digi boost
    // it will be applied for 8580 only    
    updateDigiBoost( digiBoost && type == Type::MOS_8580 );
}

auto Sid::setDigiBoost( bool state ) -> void {
    
    digiBoost = state;

    if (type == Type::MOS_6581)
        return;
    
    updateDigiBoost( state );
}

auto Sid::updateDigiBoost( bool state ) -> void {
    filter.setVoiceMask( state ? 0xf : 0x7 );
    filter.input( state ? -32768 : 0 );
}

auto Sid::reset() -> void {
    
    for( unsigned i = 0; i < 3; i++ ) {                                
        
        envelope[i].reset();
        
        voice[i].reset();
    }
    filter.reset();
    externalFilter.reset();
    databusDecay = 0;
	ready = false;	
    
	registerWrite.pipelined = false;
    sampleCounter = 0;
    potX = potY = 0xff;
    powerOn = true;
    updateIdleState();
}

auto Sid::powerOff() -> void {
	idle = true;
    powerOn = false;
}

auto Sid::phase1() -> void {
    
    for( unsigned i = 0; i < 3; i++ ) {
        //both happens in parallel
        envelope[i].clock();
        voice[i].clock();
    }
	
	for( unsigned i = 0; i < 3; i++ )
		voice[i].synchronize();
	
	for( unsigned i = 0; i < 3; i++ )
		voice[i].setWaveformOutput();
}

auto Sid::phase2() -> void {
	
    if (audioOut) {    
        if (moreAccuracy) {
            // filter calculations are threaded
            while ( ready.load() ) { }

            v1 = voice[0].output();
            v2 = voice[1].output();
            v3 = voice[2].output();

            registerWriteThreaded = registerWrite;

            ready = true;        

        } else {

            filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

            externalFilter.clock( filter.output() );	    

            if (++sampleCounter == SID_SAMPLE_COUNTER ) {
                audioRefresh( externalFilter.output( ) );
                sampleCounter = 0;
            }
        }
    }
	  	
    // bus values decay after a certain amount of time.
    // decay time differs between single bits.
    // single bit decaying is not emulated
    // but approximate time till all bits are decayed
    if (databusDecay > 0 && --databusDecay == 0 )
        lastBusValue = 0;

	if ( registerWrite.pipelined ) {
		registerWrite.pipelined = false;
		// register write is Sid internal valid at the end of second half cycle ?
		// don't do a possible filter register update here for threaded version
		// it could be done during calculation and crash		
		writeIO( registerWrite.addr, registerWrite.value, !audioOut || !moreAccuracy );
	}	
}
    
}
