
#pragma once

#include "resampler/data.h"
#include "resampler/cosine.h"
#include "dsp/reverb.h"
#include "dsp/dsp.h"
#include "../program.h"
#include "../../emulation/interface.h"

struct AudioManager {
    
    AudioManager();
    ~AudioManager();
    
    unsigned bufferPos = 0;
    unsigned bufferSize = 0;
    double buffer[2048];    
    
    int16_t outBuffer[4096];
    float outBufferFloat[4096];
    Resampler::Cosine cosine;
    Resampler::Data rData;
    
    double ratio;
    bool dynamicRateControl;
    double rateDelta = 0.005;
    
    struct {
        double sum = 0;
        unsigned count = 0;
        double average = 0;
        bool enable = false;
        double current;
        double min;
        double max;
        double minRaw;
        double maxRaw;

    } statistics;
        
    double outputFrequency;

    Emulator::Interface::Stats stat;
    DSP::Base* dsp = nullptr;

    template<unsigned bits>
    static auto sclamp(const signed x) -> signed {
        const signed b = 1U << (bits - 1);
        const signed m = b - 1;
        return (x > m) ? m : (x < -b) ? -b : x;
    }     

    auto process( int16_t sampleLeft, int16_t sampleRight ) -> void;   
    
    auto setAudioDsp() -> void;
    
    auto setLatency() -> void;
    auto setFrequency() -> void;     
    auto setSynchronize() -> void;
    auto setVolume() -> void;
    auto setRateControl() -> void;
    
    auto setBufferSize() -> void;
    auto setResampler() -> void;
    auto setStatistics() -> void;
    auto calcStatistics( double adjust ) -> void;
    auto power() -> void;
};

extern AudioManager* audioManager;