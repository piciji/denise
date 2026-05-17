
#include "reSid.h"

namespace LIBC64 {

auto ReSid::clone(Sid* src, bool keepProps) -> void {

    if (!keepProps) {
        leftChannel = src->leftChannel;
        rightChannel = src->rightChannel;
        ioPos = src->ioPos;
        ioMask = src->ioMask;
    }
    sampleRate = src->sampleRate;

    if (dynamic_cast<ReSid*>(src)) {
        ReSid* _s = dynamic_cast<ReSid*>(src);

        if (!keepProps) {
            scaling = _s->scaling;
            databusDecayTime = _s->databusDecayTime;
        }

        lastBusValue = _s->lastBusValue;
        databusDecay = _s->databusDecay;

        v1 = _s->v1;
        v2 = _s->v2;
        v3 = _s->v3;

        for ( unsigned i = 0; i < 3; i++ ) {
            Voice& v = _s->voice[i];
            Voice& vES = voice[i];

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
            vES.contr = v.contr;
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

            vES.setType( keepProps ? vES.type : v.type );

            Envelope& e = _s->envelope[i];
            Envelope& eES = envelope[i];

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
            eES.delay = e.delay;

            eES.setType( keepProps ? vES.type : v.type );
        }

        auto& sf = _s->filter;
        if (!keepProps) {
            filter.digiBoost = sf.digiBoost;
            filter.type = sf.type;
        }
        filter.enabled = sf.enabled;
        if (!keepProps)
            filter.voiceMask = sf.voiceMask;

        filter.bias6581 = sf.bias6581;
        filter.bias8580 = sf.bias8580;

        filter.fc = sf.fc;
        filter.res = sf.res;
        filter.filt = sf.filt;
        filter.mode = sf.mode;
        filter.vol = sf.vol;
        filter._1024_div_Q = sf._1024_div_Q;
        filter.sum = sf.sum;
        filter.mix = sf.mix;
        if (!keepProps)
            filter.ve = sf.ve;
        else {
            updateDigiBoost(filter.digiBoost && type == Type::MOS_8580);
        }
        filter.v3 = sf.v3;
        filter.v2 = sf.v2;
        filter.v1 = sf.v1;

        if (keepProps) {
            filter.Vhp = 0;
            filter.Vbp = 0;
            filter.Vlp = 0;
            filter.Vbp_x = 0;
            filter.Vbp_vc = 0;
            filter.Vlp_x = 0;
            filter.Vlp_vc = 0;
        } else {
            filter.Vhp = sf.Vhp;
            filter.Vbp = sf.Vbp;
            filter.Vlp = sf.Vlp;
            filter.Vbp_x = sf.Vbp_x;
            filter.Vbp_vc = sf.Vbp_vc;
            filter.Vlp_x = sf.Vlp_x;
            filter.Vlp_vc = sf.Vlp_vc;
        }

        filter.Vddt_Vw_2 = sf.Vddt_Vw_2;
        filter.Vw_bias = sf.Vw_bias;
        filter.VbpRes = sf.VbpRes;
        filter.w0 = sf.w0;

        filter.kVgt = sf.kVgt;
        filter.n_dac = sf.n_dac;

        auto& sCLin = _s->chamberlinFilter;
        chamberlinFilter.svfQ = sCLin.svfQ;
        chamberlinFilter.svfF = sCLin.svfF;
        chamberlinFilter.lp = sCLin.lp;
        chamberlinFilter.hp = sCLin.hp;
        chamberlinFilter.bp = sCLin.bp;
        chamberlinFilter.np = sCLin.np;

        auto& sExt = _s->externalFilter;
        externalFilter.Vlp = sExt.Vlp;
        externalFilter.Vhp = sExt.Vhp;
        externalFilter.w0lp_1_s7 = sExt.w0lp_1_s7;
        externalFilter.w0hp_1_s17 = sExt.w0hp_1_s17;
    }
}

auto ReSid::updateSnapshot(DebuggerSnapshot& snap) -> void {
    auto& s = snap.sids[nr];

    s.volume = filter.vol;
    s.filter.cutOff = filter.fc;
    s.filter.resonance = filter.res;
    s.filter.voices = filter.filt;
    s.filter.mode = filter.mode >> 4;

    for (unsigned v = 0; v < 3; v++) {
        auto& sv = s.voices[v];
        auto& _sidV = voice[v];
        auto& _sidE = envelope[v];

        sv.wave = _sidV.waveform;
        sv.frequency = _sidV.freq;
        sv.pulseWidth = _sidV.pw;
        sv.attack = _sidE.attack;
        sv.delay = _sidE.delay;
        sv.sustain = _sidE.sustain;
        sv.release = _sidE.release;
        sv.control = _sidV.contr;
    }
}
    
}
