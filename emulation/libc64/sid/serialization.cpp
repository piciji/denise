
#include "reSid.h"

namespace LIBC64 {
    
auto ReSid::serialize(Emulator::Serializer& s, bool light) -> void {
    bool _load = s.mode() == Emulator::Serializer::Mode::Load;

    s.integer( leftChannel );
    s.integer( rightChannel );
    s.integer( ioMask );
    s.integer( ioPos );

    s.integer( curve6581 );
    s.integer( curve8580 );
    s.integer( range6581 );
    s.integer( waveStrength );
    s.integer( digiBoost );
    s.integer( useFilter );
    
    s.integer( (uint8_t&) type );
    s.integer( scaling );
    s.integer( filter.digiBoost );
    s.integer( lastBusValue );
    s.integer( databusDecay );
    s.integer( databusDecayTime );
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
        s.integer( v.contr );
        s.integer( v.shiftRegister );
        s.integer( v.noNoise );
        s.integer( v.noiseOutput );
        s.integer( v.noNoiseOrNoiseOutput );
        s.integer( v.noPulse );
        s.integer( v.waveZero );
        s.integer( v.ringMsbMask );
        s.integer( v.aging );
        s.integer( v.shiftReset );
        s.integer( v.shiftPipeline );

        Envelope& e = envelope[i];
        s.integer( (uint8_t&)e.state );
        s.integer( (uint8_t&)e.type );
        s.integer( e.counter );
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
        s.integer( e.delay );
        
        if (_load) {
            v.setType( v.type);
            e.setType( e.type );
        }
    }
    
    s.integer( (uint8_t&)filter.type );
    s.integer( filter.enabled );
    s.integer( filter.voiceMask );
    s.integer( filter.fc );
    s.integer( filter.res );
    s.integer( filter.filt );
    s.integer( filter.mode );
    s.integer( filter.vol );
    s.integer( filter._1024_div_Q );
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
    s.integer( filter.w0 );
    s.integer( filter.kVgt );
    s.integer( filter.n_dac );
    s.integer( filter.nrXFilter );
    s.integer( filter.nrXMixer );
    
    s.floatingpoint( chamberlinFilter.svfQ );
    s.floatingpoint( chamberlinFilter.svfF );
    s.floatingpoint( chamberlinFilter.lp );
    s.floatingpoint( chamberlinFilter.hp );
    s.floatingpoint( chamberlinFilter.bp );
    s.floatingpoint( chamberlinFilter.np );
  
    if (!light) {
        s.integer( externalFilter.Vlp );
        s.integer( externalFilter.Vhp );
        s.integer( externalFilter.w0lp_1_s7 );
        s.integer( externalFilter.w0hp_1_s17 );
    }
}

}
