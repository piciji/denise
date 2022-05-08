
#pragma once

#include "resampler/data.h"
#include "resampler/cosine.h"
#include "dsp/base.h"
#include "../program.h"
#include "../../emulation/interface.h"
#include "record/handler.h"
#include "mixer/drive.h"

struct AudioManager {
    
    AudioManager();
    ~AudioManager();
    
    unsigned bufferPos = 0;
    unsigned bufferSize = 0;
    float buffer[2048];        
    float floatConversion;
    std::vector<DSP::Base*> dsps;
    
    int16_t outBuffer[65536];
    float outBufferFloat[65536];
    Resampler::Cosine cosine;
    Resampler::Data rData;
    AudioRecord::Handler record;
    Mixer::Drive drive;

    bool mixDriveSounds = false;

    struct {
        bool enable = false;
        unsigned lastTS;
    } measureUiUpdate;
    
    double ratio;    
    bool dynamicRateControl;
    double rateDelta = 0.005;
    bool allowDrc = false;
    
    struct {
        float sum = 0;
        unsigned count = 0;
        float average = 0;
        bool enable = false;
        float current;
        float min;
        float max;
        float minRaw;
        float maxRaw;

    } statistics;
        
    double outputFrequency;
    double inputFrequency;
    double inputFPS;

    Emulator::Interface::Stats stat;

    template<unsigned bits>
    static auto sclamp(const signed x) -> signed {
        const signed b = 1U << (bits - 1);
        const signed m = b - 1;
        return (x > m) ? m : (x < -b) ? -b : x;
    }     

    auto process( int16_t sampleLeft, int16_t sampleRight ) -> void;
    auto flush() -> void;
    
    auto setAudioDsp() -> void;
        
    auto setLatency() -> void;
    auto setFrequency() -> void;     
    auto setSynchronize() -> void;
    auto setVolume() -> void;
    auto setRateControl() -> void;
    auto setDriveSounds(bool init = true) -> void;
    auto setPriority() -> void;
    
    auto setBufferSize() -> void;
    auto setResampler() -> void;
    auto setStatistics() -> void;
    auto calcStatistics( float adjust ) -> void;
    auto power() -> void;
    auto powerOff() -> void;
    auto applyDsp() -> void;

    auto checkIfUINeedsAnUpdate() -> void;
};

extern AudioManager* audioManager;