
#include "manager.h"
#include "../tools/status.h"
#include "../cmd/cmd.h"

AudioManager* audioManager = nullptr;

AudioManager::AudioManager() {
    
    rData.out = new double[4096];
    
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
    
    double inputFrequency = stat.sampleRate;
    
    bool adjustToMonitorFrequency = settings->get<bool>("video_override_exact", true);        
                        
    if (adjustToMonitorFrequency) {
        double monitorFrequency;
        
        if (stat.isPal())
            monitorFrequency = settings->get<double>("video_pal", 50.0, {25.0, 100.0});
        else
            monitorFrequency = settings->get<double>("video_ntsc", 60.0, {30.0, 120.0});
        
        inputFrequency = (inputFrequency * monitorFrequency) / stat.fps;          
    }        
    
    ratio = outputFrequency / inputFrequency;
    
    cosine.reset( ratio, stat.stereoSound ? 2 : 1 );      
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
    if (!dsp)
        return;
    
    unsigned volume = settings->get<unsigned>("audio_volume", 100u,{0u, 100u});
    bool mute = settings->get<bool>("audio_mute", false);
    
    dsp->setVolume( mute ? 0.0 : ( volume * 0.01 ) );
}

auto AudioManager::setAudioDsp() -> void {
    
    if (!activeEmulator)
        return;
    
    stat = activeEmulator->getStatsForSelectedRegion();
    
    bool reverb = settings->get<bool>("audio_reverb", false );
    
    if (dsp)        
        delete dsp;
    
    if (reverb)
        dsp = new DSP::Reverb;
    else
        dsp = new DSP::Base;
    
    dsp->init( 2, stat.stereoSound ? 2 : 1 );    
    
    setVolume();
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

auto AudioManager::process( int16_t sampleLeft, int16_t sampleRight ) -> void {
    
    buffer[bufferPos++] = (sampleLeft / 32768.0) + 1e-25;
    
    if (stat.stereoSound)
        buffer[bufferPos++] = (sampleRight / 32768.0) + 1e-25;    

    if (bufferPos < bufferSize)
        return;   

    bufferPos = 0;
    
    if (cmd->noDriver)
        return;
    
    rData.in = &buffer[0];
    rData.inputFrames = stat.stereoSound ? bufferSize >> 1 : bufferSize;
    
    if (dynamicRateControl || statistics.enable) {
        double deviation = audioDriver->getCenterBufferDeviation();
        
        if (statistics.enable)
            calcStatistics( deviation );
        
        if (dynamicRateControl)
            rData.ratio = ratio * (1.0 + rateDelta * deviation);
    }   
    
    cosine.process();

    dsp->process( rData.out, rData.outputFrames );

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

auto AudioManager::calcStatistics( double adjust ) -> void {
    
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
