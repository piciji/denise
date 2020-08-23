
#include "manager.h"
#include "../tools/status.h"
#include "../cmd/cmd.h"
#include "dsp/bass.h"
#include "dsp/reverb.h"

AudioManager* audioManager = nullptr;

AudioManager::AudioManager() {
    
    floatConversion = 1.0 / 32768.0;
    
    rData.out = new float[4096];
    
    cosine.setData( &rData );
}

AudioManager::~AudioManager() {
    
    delete[] rData.out;
}

auto AudioManager::setLatency() -> void {
    
    unsigned latency = settings->get<unsigned>("audio_latency", 64u, {1u, 120u});
    audioDriver->setLatency( latency );
}

auto AudioManager::setFrequency() -> void {    
    
    unsigned frequency = settings->get<unsigned>("audio_frequency_v2", 48000u, {0u, 48000u});
    audioDriver->setFrequency( frequency );
    
    this->outputFrequency = (double)frequency;
    
    setResampler();
}

auto AudioManager::setSynchronize() -> void {
    
    auto synchronize = settings->get<bool>("audio_sync", true);
    audioDriver->synchronize(synchronize);
    
    setBufferSize();
}

auto AudioManager::setResampler() -> void {    
    if (!activeEmulator)
        return;
    
    stat = activeEmulator->getStatsForSelectedRegion();
    
    inputFrequency = stat.sampleRate;
    
    double monitorRatio = 1.0;
    
    bool adjustToMonitorFrequency = settings->get<bool>("video_override_exact", true);        
                        
    if (adjustToMonitorFrequency) {
        double monitorFrequency;
        
        if (stat.isPal())
            monitorFrequency = settings->get<double>("video_pal", 50.0, {25.0, 100.0});
        else
            monitorFrequency = settings->get<double>("video_ntsc", 60.0, {30.0, 120.0});
        
        inputFrequency = (inputFrequency * monitorFrequency) / stat.fps;   
        
        monitorRatio = monitorFrequency / stat.fps;
    }        
    
    ratio = outputFrequency / inputFrequency;
    
    cosine.reset( ratio, stat.stereoSound ? 2 : 1 );   
    
    activeEmulator->setMonitorFpsRatio( monitorRatio );
}

auto AudioManager::setBufferSize() -> void {
    if (!activeEmulator)
        return;

    stat = activeEmulator->getStatsForSelectedRegion();
    auto synchronize = settings->get<bool>("audio_sync", true);
    
    bufferSize = 2048;
    
    if (synchronize || dynamicRateControl)
        bufferSize = 512;
    
    if (!stat.stereoSound)
        bufferSize >>= 1;
    
    bufferPos = 0;
}

auto AudioManager::setVolume() -> void {
    
    unsigned volume = settings->get<unsigned>("audio_volume", 100u,{0u, 100u});
    bool mute = settings->get<bool>("audio_mute", false);
        
    floatConversion = 0.0;
    
    if (!mute)
        floatConversion =  ( (float)volume * 0.01 ) / 32768.0;   
}

auto AudioManager::setAudioDsp() -> void {
    
    if (!activeEmulator)
        return;
    
    for(auto dsp : dsps)
        delete dsp;
    
    dsps.clear();
    
    stat = activeEmulator->getStatsForSelectedRegion();

    bool useBass = settings->get<bool>("audio_bass", false );
    
    if (useBass) {
        
        DSP::Bass* bass = new DSP::Bass;                        
        
        bass->setMono( !stat.stereoSound );
        bass->init( 
            (float)outputFrequency,
            (float)settings->get<unsigned>("audio_bass_freq", 200, {20, 200} ),
            (float)settings->get<unsigned>("audio_bass_gain", 10, {0, 40} ),
            settings->get<float>("audio_bass_clipping", 0.4, {0.0, 1.0} )
        );
        
        dsps.push_back( (DSP::Base*)bass );
    }    
    
    bool useReverb = settings->get<bool>("audio_reverb", false );
    
    if (useReverb) {
        
        DSP::Reverb* reverb = new DSP::Reverb;
        
        reverb->setMono( !stat.stereoSound );
        reverb->init( 
            (float)outputFrequency,
            settings->get<float>("audio_reverb_drytime", 0.43, {0.0, 1.0} ),
            settings->get<float>("audio_reverb_wettime", 0.4, {0.0, 1.0} ),
            settings->get<float>("audio_reverb_damping", 0.8, {0.0, 1.0} ),
            settings->get<float>("audio_reverb_roomwidth", 0.56, {0.0, 1.0} ),
            settings->get<float>("audio_reverb_roomsize", 0.56, {0.0, 1.0} )
        );
        
        dsps.push_back( (DSP::Base*)reverb );
    }  
}

auto AudioManager::setRateControl() -> void {
    
    dynamicRateControl = settings->get<bool>("dynamic_rate_control", false);
    
    rateDelta = settings->get<float>("rate_control_delta", 0.005, {0.0, 0.010});        
    
    setBufferSize();
}

auto AudioManager::setStatistics() -> void {
    
    statistics.enable = settings->get<bool>("show_audio_buffer", false);
}

auto AudioManager::power() -> void {
    audioManager->setResampler();
    audioManager->setBufferSize();
    audioManager->setAudioDsp();

    statistics.average = 0;
    statistics.sum = 0;
    statistics.count = 0;
    statistics.min = -1;
    statistics.max = 1;
    statistics.current = 0;
}

auto AudioManager::applyDsp() -> void {

    DSP::Data dspInput;
    DSP::Data dspOutput;
    
    dspOutput.samples = nullptr;
    dspOutput.frames = 0;

    dspInput.samples = rData.out;
    dspInput.frames = rData.outputFrames;
    
    for( auto dsp : dsps ) {
                
        dsp->process( &dspOutput, &dspInput );

        dspInput.samples = dspOutput.samples;
        dspInput.frames = dspOutput.frames;
    }

    if (dspOutput.frames) {
        rData.out = dspOutput.samples;
        rData.outputFrames = dspOutput.frames;
    }
}

auto AudioManager::process( int16_t sampleLeft, int16_t sampleRight ) -> void {
    
    buffer[bufferPos++] = sampleLeft * floatConversion;
    
    if (stat.stereoSound)
        buffer[bufferPos++] = sampleRight * floatConversion;    

    if (bufferPos < bufferSize)
        return;   

    bufferPos = 0;
                            
    rData.in = &buffer[0];
    rData.inputFrames = stat.stereoSound ? bufferSize >> 1 : bufferSize;
    
    if (dynamicRateControl || statistics.enable) {
        double deviation = audioDriver->getCenterBufferDeviation();
        
        if (statistics.enable)
            calcStatistics( (float)deviation );
        
        if (dynamicRateControl)
            rData.ratio = ratio * (1.0 + rateDelta * deviation);
    }   
    
    cosine.process();

    if (dsps.size())
        applyDsp();

    if ( audioDriver->expectFloatingPoint() ) {
        
        for (unsigned i = 0; i < (rData.outputFrames << 1); i++ )
            outBufferFloat[i] = *(rData.out + i);        
        
        // 4 byte per channel, 8 byte per audio frame
        audioDriver->addSamples( (uint8_t*) outBufferFloat, rData.outputFrames << 3);
        
    } else {
        
        for (unsigned i = 0; i < (rData.outputFrames << 1); i++)
            outBuffer[i] = sclamp<16>( *(rData.out + i) * 32767.0 );        

        // 2 byte per channel, 4 byte per audio frame
        audioDriver->addSamples( (uint8_t*) outBuffer, rData.outputFrames << 2);
    }
}

auto AudioManager::calcStatistics( float adjust ) -> void {
    
    statistics.sum += adjust;
    statistics.minRaw = std::max( statistics.minRaw, adjust );
    statistics.maxRaw = std::min( statistics.maxRaw, adjust );

    if (++statistics.count == 100) {
        statistics.count = 1; // average value is the first entry
        statistics.sum = statistics.sum / 100.0;
        statistics.average = 50.0 - (statistics.sum * 50.0);
        statistics.current = 50.0 - (adjust * 50.0);
        statistics.min = 50.0 - (statistics.minRaw * 50.0);
        statistics.max = 50.0 - (statistics.maxRaw * 50.0);
        statistics.minRaw = -1;
        statistics.maxRaw = 1;
        status->update = 1;
    }
}
