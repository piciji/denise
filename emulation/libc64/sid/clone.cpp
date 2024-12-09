
#include "sid.h"

namespace LIBC64 {
    
auto SidManager::clone( uint8_t start, uint8_t end ) -> void {
    
    if (start >= end)
        return;
    
    for( ; start < end; start++ ) {
        
        auto ES = sids[start];
        
        ES->lastBusValue = sid->lastBusValue;        
        ES->databusDecay = sid->databusDecay;
        ES->databusDecayTime = sid->databusDecayTime;
        ES->v1 = sid->v1;
        ES->v2 = sid->v2;
        ES->v3 = sid->v3;
        ES->separateFilterInputs = false;
        
        for ( unsigned i = 0; i < 3; i++ ) {
            Sid::Voice& v = sid->voice[i];
            Sid::Voice& vES = ES->voice[i];
            
            vES.accumulator = v.accumulator;
            vES.freq = v.freq;
            vES.pw = v.pw;
            vES.pulseOutput = v.pulseOutput;
            vES.waveTemp = v.waveTemp;
            vES.waveform = v.waveform;
            vES.waveformOutput = v.waveformOutput;
            vES.osc3 = v.osc3;
            vES.test = v.test;
            vES.msbRising = v.msbRising;
            vES.sync = v.sync;
            vES.shiftRegister = v.shiftRegister;
            vES.noNoise = v.noNoise;
            vES.noiseOutput = v.noiseOutput;
            vES.noNoiseOrNoiseOutput = v.noNoiseOrNoiseOutput;
            vES.noPulse = v.noPulse;
            vES.waveZero = v.waveZero;
            vES.ringMsbMask = v.ringMsbMask;
            vES.aging = v.aging;
            vES.shiftReset = v.shiftReset;
            vES.shiftPipeline = v.shiftPipeline;
            
            vES.setType( vES.type, ES->filterType == Sid::FilterType::Chamberlin );
            
            Sid::Envelope& e = sid->envelope[i];
            Sid::Envelope& eES = ES->envelope[i];
            
            eES.state = e.state;
            eES.counter = e.counter;
            eES.env3 = e.env3;
            eES.lockEnvCounter = e.lockEnvCounter;
            eES.gateBefore = e.gateBefore;
            eES.resetRateCounter = e.resetRateCounter;
            eES.ratePeriod = e.ratePeriod;
            eES.rateCounter = e.rateCounter;
            eES.exponentialPeriod = e.exponentialPeriod;
            eES.exponentialCounter = e.exponentialCounter;
            eES.attack = e.attack;
            eES.decay = e.decay;
            eES.sustain = e.sustain;
            eES.release = e.release;            
        }
        
        ES->filter.fc = sid->filter.fc;
        ES->filter.res = sid->filter.res;
        ES->filter.filt = sid->filter.filt;
        ES->filter.mode = sid->filter.mode;
        ES->filter.vol = sid->filter.vol;
        ES->filter._1024_div_Q = sid->filter._1024_div_Q;
        ES->filter.sum = sid->filter.sum;
        ES->filter.mix = sid->filter.mix;
        ES->filter.ve = sid->filter.ve;
        ES->filter.v3 = sid->filter.v3;
        ES->filter.v2 = sid->filter.v2;
        ES->filter.v1 = sid->filter.v1;
        
        ES->filter.Vhp = 0;
        ES->filter.Vbp = 0;
        ES->filter.Vlp = 0;
        ES->filter.Vbp_x = 0;
        ES->filter.Vbp_vc = 0;
        ES->filter.Vlp_x = 0;
        ES->filter.Vlp_vc = 0;
        
        ES->filter.Vddt_Vw_2 = sid->filter.Vddt_Vw_2;
        ES->filter.Vw_bias = sid->filter.Vw_bias;
        ES->filter.VbpRes = sid->filter.VbpRes;
        ES->filter.w0 = sid->filter.w0;
        
        ES->chamberlinFilter.svfQ = sid->chamberlinFilter.svfQ;
        ES->chamberlinFilter.svfF = sid->chamberlinFilter.svfF;
        ES->chamberlinFilter.lp = sid->chamberlinFilter.lp;
        ES->chamberlinFilter.hp = sid->chamberlinFilter.hp;
        ES->chamberlinFilter.bp = sid->chamberlinFilter.bp;
        ES->chamberlinFilter.np = sid->chamberlinFilter.np;

        ES->filter.kVgt = sid->filter.kVgt;
        ES->filter.n_dac = sid->filter.n_dac;
        
        ES->externalFilter.Vlp = sid->externalFilter.Vlp;
        ES->externalFilter.Vhp = sid->externalFilter.Vhp;
        ES->externalFilter.w0lp_1_s7 = sid->externalFilter.w0lp_1_s7;
        ES->externalFilter.w0hp_1_s17 = sid->externalFilter.w0hp_1_s17;                
    }
}    
    
}
