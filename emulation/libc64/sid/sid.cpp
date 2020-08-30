
#include "sid.h"

#include "../system/system.h"
#include "multisid.cpp"
#include "register.cpp"
#include "envelope.cpp"
#include "voice.cpp"
#include "filter/main.cpp"
#include "filter/external.cpp"
#include "filter/chamberlin.cpp"
#include "serialization.cpp"
#include "../../tools/thread.h"
#include "../../tools/clamp.h"

namespace LIBC64 {
    
Sid* sid = nullptr;      
Sid* sids[7] = {nullptr};  
uint8_t Sid::sampleCounter = 0;
uint8_t Sid::sampleLimit = 2;
bool Sid::audioOut = true;
bool Sid::useExternalFilter = true;
unsigned Sid::serializationSizeForSevenMoreSids = 0;

std::function<void ( int16_t )> Sid::audioRefresh = [](int16_t sample) {};
std::function<void ( int16_t, int16_t )> Sid::audioRefreshStereo = [](int16_t sampleL, int16_t sampleR) {};   

auto Sid::useLeftChannel(bool state) -> void {
    leftChannel = state;
    updateSidUsage();
}

auto Sid::useRightChannel(bool state) -> void {
    rightChannel = state;
    updateSidUsage();
}

Sid::Sid( Type type, Emulator::Events* events ) : filter( this ), chamberlinFilter(filter) {

    lastBusValue = 0;
    this->events = events;
	
    setType( type );
    
    filterType = FilterType::Standard;
	
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
	
    ioMask = 0xD420;
    ioPos = 1;
	moreAccuracy = false;
    audioOut = true;
	//idle = true;
	//ready = false;
    powerOn = false;
    
    getPotX = []() { return 0xff; };
    getPotY = []() { return 0xff; };
    callPotUpdate = []() { };
    
    registerCallbacks();    
	
//	std::thread worker( [this] {
//
//        std::chrono::milliseconds duration(5);
//            
//		while(true) {
//			
//			while ( !ready.load() ) {
//                
//                if (idle.load())
//                    std::this_thread::sleep_for( duration );                                            
//                    
//                // consumes thread fully in non idle mode.
//                // even a thread::yield would slow down this thread too much to be usefull.
//                // without a thread::yield there is no re scheduling possible, so be carefull.
//                // this mode would crash a single core cpu hard.
//			}
//			
//			filter.clockMulti(v1, v2, v3);
//
//			externalFilter.clock( filter.outputMulti() );	    
//
//            if (++sampleCounter == SID_SAMPLE_COUNTER ) {
//                audioRefresh( externalFilter.output( ) );
//                sampleCounter = 0;
//            }
//
//			this->ready = false;
//		}
//	});	
//    
//    Emulator::setThreadPriorityRealtime( worker );
//	
//	worker.detach();
}

auto Sid::calcSerializationSizeForSevenMoreSids() -> void {
    
    Emulator::Serializer s;
    
    sid->serialize( s, false );
    
    serializationSizeForSevenMoreSids = s.size() * 7;
}

auto Sid::registerCallbacks() -> void {
    
    events->registerCallback( { &callPotUpdate, 1 } );
    
    for( unsigned i = 0; i < 3; i++ ) {   
        
        voice[i].registerCallbacks();
        
        envelope[i].registerCallbacks();
    }
}

auto Sid::disableAudioOut(bool state) -> void {
//    if (moreAccuracy && registerWrite.pipelined) {
//        // wait for worker thread
//        while (ready.load()) {}
//        applyFilterWrite();
//    }
        
    audioOut = !state;
}

auto Sid::setFilterType( FilterType filterType ) -> void {
    
    this->filterType = filterType;
    
    filter.setOldFilter( filterType == FilterType::VICE24 );
    
    for( unsigned i = 0; i < 3; i++ )
        voice[i].setType( this->type, this->filterType == FilterType::Chamberlin );
}

auto Sid::setMoreAccuracy(bool state) -> void {
    
//    if (moreAccuracy && registerWrite.pipelined) {
//        // wait for worker thread
//        while (ready.load()) {}
//        applyFilterWrite();
//    }
//    
//	moreAccuracy = state;
//    moreAccuracy = false;
//    
//	updateIdleState();
//    
//	ready = false;	
//    applyFilterWrite();
//    
//    if (moreAccuracy)
//        filter.multiPrecalculate();
}

auto Sid::updateIdleState() -> void {
    
 //   idle = !powerOn ? true : !moreAccuracy;
}

auto Sid::setResampleQuality( uint8_t val ) -> void {
    
    sampleCounter = 0;
    
    switch(val) {
        case 0: sampleLimit = 1; break;
        case 1: sampleLimit = 2; break;
        case 2: sampleLimit = 7; break;
        default:
        case 3: 
            sampleLimit = 18; break;
    }        
}

auto Sid::getResampleQuality( ) -> uint8_t {
    
        switch(sampleLimit) {
            case 1: return 0;
            case 2: return 1;
            case 7: return 2;
            default:
            case 18: 
                return 3;
    } 
        
    __builtin_unreachable(); 
}

auto Sid::setType( Type type ) -> void {

    this->type = type;
    
    for( unsigned i = 0; i < 3; i++ ) {
        voice[i].setType( type, this->filterType == FilterType::Chamberlin );
        envelope[i].setType( type );
    }	
    filter.setType( type );
    
    databusDecayTime = type == MOS_8580 ? 0xa2000 : 0x1d00;

    // update digi boost
    // it will be applied for 8580 only    
    updateDigiBoost( filter.digiBoost && type == Type::MOS_8580 );
}

auto Sid::setDigiBoost( bool state ) -> void {
    
    filter.digiBoost = state;        

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
    chamberlinFilter.reset();
    externalFilter.reset();
    databusDecay = 0;
	//ready = false;	
    
	registerWrite.pipelined = false;
    sampleCounter = 0;
    potX = potY = 0xff;
    powerOn = true;
    updateIdleState();
}

auto Sid::powerOff() -> void {
//	idle = true;
    powerOn = false;
}

auto Sid::clock() -> void {
    unsigned i;
    
    for (i = 0; i < 3; i++) {
        //both happens in parallel
        envelope[i].clock();
        voice[i].clock();
    }

    for (i = 0; i < 3; i++)
        voice[i].synchronize();    

    for (i = 0; i < 3; i++)
        voice[i].setWaveformOutput();    
    
    if (audioOut) {    
//        if (moreAccuracy) {            
//            
//            // filter calculations are threaded
//            while ( ready.load() ) { }
//
//            applyFilterWrite();
//
//            v1 = voice[0].output();
//            v2 = voice[1].output();
//            v3 = voice[2].output();
//
//            ready = true;        
//
//        } else {

        if (useExternalFilter) {
        
            if (filterType == FilterType::Chamberlin) {
                
                double _sample = chamberlinFilter.clock( (double)voice[0].output() / 255.0,(double) voice[1].output() / 255.0, (double)voice[2].output() / 255.0 );

                externalFilter.clock( Emulator::sclamp( 16, _sample ) );                               
                
            } else {

                filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

                externalFilter.clock( filter.output() );	
            }

            if (!extraSids) {
                if (++sampleCounter == sampleLimit) {
                    audioRefresh( Emulator::sclamp( 16, externalFilter.output() ) );
                    sampleCounter = 0;
                }
            } else
                curSample = externalFilter.output();
            
        } else
            withoutExternalFilter();

       // }
    }
	  	
    // bus values decay after a certain amount of time.
    // decay time differs between single bits.
    // single bit decaying is not emulated
    // but approximate time till all bits are decayed
    if (databusDecay > 0 && --databusDecay == 0 )
        lastBusValue = 0;
    
}
    
inline auto Sid::withoutExternalFilter() -> void {

    if (filterType == FilterType::Chamberlin) {

        curSample = chamberlinFilter.clock((double) voice[0].output() / 255.0, (double) voice[1].output() / 255.0, (double) voice[2].output() / 255.0);
        
    } else {

        filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

        curSample = filter.output();
    }

    if (!extraSids) {
        if (++sampleCounter == sampleLimit) {
            audioRefresh( Emulator::sclamp( 16, curSample ) );
            sampleCounter = 0;
        }
    } 
} 

}
