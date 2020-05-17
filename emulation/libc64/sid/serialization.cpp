
#include "sid.h"

namespace LIBC64 {

auto Sid::serialize(Emulator::Serializer& s) -> void {
    
    if (moreAccuracy) {
        // wait for worker thread
		while ( ready.load() ) { }
    }
    
    // reserved for more accuracy
    bool reserved = false;
    
    s.integer( (uint8_t&) type );
    s.integer( digiBoost );
    s.integer( lastBusValue );
    s.integer( databusDecay );
    s.integer( databusDecayTime );
    s.integer( registerWrite.pipelined );
    s.integer( registerWrite.addr );
    s.integer( registerWrite.value );
    //s.integer( moreAccuracy );
    s.integer( reserved );
    s.integer( sampleCounter );
    s.integer( potX );
    s.integer( potY );
    s.integer( v1 );
    s.integer( v2 );
    s.integer( v3 );
    
    for ( unsigned i = 0; i < 3; i++ ) {
        Voice& v = voice[i];

        s.integer( (uint8_t&)v.type );
        s.integer( v.accumulator );
        s.integer( v.freq );
        s.integer( v.pw );
        s.integer( v.pulseOutput );
        s.integer( v.waveTemp );
        s.integer( v.waveform );
        s.integer( v.waveformOutput );
        s.integer( v.osc3 );
        s.integer( v.test );
        s.integer( v.msbRising );
        s.integer( v.sync );
        s.integer( v.shiftRegister );
        s.integer( v.noNoise );
        s.integer( v.noiseOutput );
        s.integer( v.noNoiseOrNoiseOutput );
        s.integer( v.noPulse );
        s.integer( v.waveZero );
        s.integer( v.ringMsbMask );

        if (s.mode() == Emulator::Serializer::Mode::Load) {
            v.setType( v.type ); // update pointer                
        }

        Envelope& e = envelope[i];

        s.integer( (uint8_t&)e.state );
        s.integer( (uint8_t&)e.type );
        s.integer( e.counter );
        s.integer( e.counterTemp );
        s.integer( e.env3 );
        s.integer( e.lockEnvCounter );
        s.integer( e.gateBefore );
        s.integer( e.resetRateCounter );
        s.integer( e.ratePeriod );
        s.integer( e.rateCounter );
        s.integer( e.exponentialPeriod );
        s.integer( e.exponentialCounter );
        s.integer( e.attack );
        s.integer( e.decay );
        s.integer( e.sustain );
        s.integer( e.release );
        
        if (s.mode() == Emulator::Serializer::Mode::Load) {
            e.setType( e.type ); // update pointer                
        }
    }
    
    s.integer( (uint8_t&)filter.type );
    s.integer( filter.enabled );
    s.integer( filter.voiceMask );
    s.integer( filter.bias );
    s.integer( filter.fc );
    s.integer( filter.res );
    s.integer( filter.filt );
    s.integer( filter.mode );
    s.integer( filter.vol );
    s.integer( filter._8_div_Q );
    s.integer( filter.sum );
    s.integer( filter.mix );
    s.integer( filter.ve );
    s.integer( filter.v3 );
    s.integer( filter.v2 );
    s.integer( filter.v1 );
    s.integer( filter.Vhp );
    s.integer( filter.Vbp );
    s.integer( filter.Vlp );
    s.integer( filter.Vbp_x );
    s.integer( filter.Vbp_vc );
    s.integer( filter.Vlp_x );
    s.integer( filter.Vlp_vc );
    s.integer( filter.Vddt_Vw_2 );
    s.integer( filter.Vw_bias );
    s.integer( filter.VbpRes );
    
    for ( unsigned i = 0; i < 2; i++ ) {
        s.integer( filter.calculated[i].kVgt );
        s.integer( filter.calculated[i].n_dac );
    }    

    s.integer( externalFilter.Vlp );
    s.integer( externalFilter.Vhp );
    s.integer( externalFilter.w0lp_1_s7 );
    s.integer( externalFilter.w0hp_1_s17 );
    
    if (s.mode() == Emulator::Serializer::Mode::Load) {
        setMoreAccuracy( moreAccuracy );
    }
        
}

}
