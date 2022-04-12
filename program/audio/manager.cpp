
#include "manager.h"
#include "../cmd/cmd.h"
#include "dsp/bass.h"
#include "dsp/reverb.h"
#include "dsp/panning.h"
#include "../view/status.h"
#include "../view/view.h"
#include "../tools/chronos.h"

AudioManager* audioManager = nullptr;

AudioManager::AudioManager() {
    
    floatConversion = 1.0 / 32768.0;
    
    rData.out = new float[65536];
    
    cosine.setData( &rData );    
}

AudioManager::~AudioManager() {
    
    delete[] rData.out;
}

auto AudioManager::setLatency() -> void {
    
    unsigned latency = globalSettings->get<unsigned>("audio_latency", 30u, {1u, 120u});
    audioDriver->setLatency( latency );
}

auto AudioManager::setPriority() -> void {
    
    bool priority = globalSettings->get<bool>("audio_priority", false);
    audioDriver->setHighPriority( priority );
}

auto AudioManager::setFrequency() -> void {    
    
    unsigned frequency = globalSettings->get<unsigned>("audio_frequency_v2", 48000u, {0u, 48000u});
    audioDriver->setFrequency( frequency );
    
    this->outputFrequency = (double)frequency;
    
    setResampler();

    drive.unload( );
    setDriveSounds( false );
}

auto AudioManager::setSynchronize() -> void {

    bool synchronize = false;
    if (cmd->debug)
        synchronize = false;
    else if (view)
        synchronize = !view->isMaximumSpeed();

    if (audioDriver->hasSynchronized() == synchronize)
        return;

    audioDriver->synchronize(synchronize);
    program->updateOverallSynchronize();
    setBufferSize();
}

auto AudioManager::setResampler() -> void {
    if (!activeEmulator)
        return;
    
    stat = activeEmulator->getStatsForSelectedRegion();
    
    inputFrequency = stat.sampleRate;
    inputFPS = stat.fps;

    measureUiUpdate.enable = false;
    
    double monitorRatio = 1.0;

    float speed;
    bool percent;
    unsigned speedProfile = view->getSpeedBySelectedProfile(speed, percent);

    if (speedProfile != 0) {
        if (percent) {
            inputFrequency = (inputFrequency * (double) speed) / 100.0;

            inputFPS = (inputFPS * (double) speed) / 100.0;

            monitorRatio = (double) speed / 100.0;

        } else {
            inputFrequency = (inputFrequency * (double) speed) / inputFPS;

            monitorRatio = (double) speed / inputFPS;

            inputFPS = speed;
        }

        measureUiUpdate.enable = monitorRatio < 0.5;

        if (measureUiUpdate.enable) {
            measureUiUpdate.lastTS = Chronos::getTimestampInMilliseconds();
        }
    }
    
    ratio = outputFrequency / inputFrequency;

    cosine.reset( ratio, stat.stereoSound ? 2 : 1 );   
    
    activeEmulator->setMonitorFpsRatio( monitorRatio );

    // check changed FPS for adaptive sync
    VideoManager::setSynchronize();
}

auto AudioManager::setBufferSize() -> void {
    if (!activeEmulator)
        return;

    stat = activeEmulator->getStatsForSelectedRegion();
    auto synchronize = audioDriver->hasSynchronized();
    
    bufferSize = 2048;
    
    if (synchronize || dynamicRateControl)
        bufferSize = 512;
    
    if (!stat.stereoSound)
        bufferSize >>= 1;
    
    bufferPos = 0;
}

auto AudioManager::setVolume() -> void {
    
    unsigned volume = globalSettings->get<unsigned>("audio_volume", 100u, {0u, 100u});
    bool mute = globalSettings->get<bool>("audio_mute", false);
        
    floatConversion = 0.0;    
    
    if (!mute)
        floatConversion = ( (float)volume * 0.01 ) / 32768.0;
}

auto AudioManager::setDriveSounds( bool init ) -> void {
    if (!activeEmulator)
        return;

    auto settings = program->getSettings( activeEmulator );

    mixFloppySounds = settings->get<bool>("audio_floppy", false);

    if (init)
        activeEmulator->enableFloppySounds( mixFloppySounds );

    if (mixFloppySounds) {
        unsigned volume = settings->get<unsigned>("audio_floppy_volume", 100u, {0u, 300u});

        if (!drive.loaded( activeEmulator, activeEmulator->getDiskMediaGroup() )) {
            drive.readPack( activeEmulator, activeEmulator->getDiskMediaGroup() );
        }

        drive.setVolume( activeEmulator, activeEmulator->getDiskMediaGroup(), (float)volume * 0.01 );
    } else
        drive.reset();
}

auto AudioManager::setAudioDsp() -> void {
    
    if (!activeEmulator)
        return;
    
    for(auto dsp : dsps)
        delete dsp;
    
    dsps.clear();
    
    stat = activeEmulator->getStatsForSelectedRegion();
    
    auto settings = program->getSettings( activeEmulator );

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
    
    bool usePanning = settings->get<bool>("audio_panning", false );
    
    if (usePanning) {
        
        DSP::Panning* panning = new DSP::Panning;
        
        panning->init(
            settings->get<float>("audio_panning_left0", 1.0, {0.0, 1.0} ),
            settings->get<float>("audio_panning_left1", 0.0, {0.0, 1.0} ),
            settings->get<float>("audio_panning_right0", 0.0, {0.0, 1.0} ),
            settings->get<float>("audio_panning_right1", 1.0, {0.0, 1.0} )
        );
            
        dsps.push_back( (DSP::Base*)panning );
    }
}

auto AudioManager::setRateControl() -> void {

    dynamicRateControl = allowDrc && globalSettings->get<bool>("dynamic_rate_control", false);

    rateDelta = globalSettings->get<float>("rate_control_delta", 0.005, {0.0, 0.010});
    
    setBufferSize();

    if (!dynamicRateControl)
        rData.ratio = ratio;
}

auto AudioManager::setStatistics() -> void {
    
    statistics.enable = globalSettings->get<bool>("show_audio_buffer", false);
}

auto AudioManager::power() -> void {
    setSynchronize();
    setResampler();
    setBufferSize();
    setAudioDsp();
    setVolume();
    setDriveSounds();

    statistics.average = 0;
    statistics.sum = 0;
    statistics.count = 0;
    statistics.min = -1;
    statistics.max = 1;
    statistics.current = 0;
}

auto AudioManager::powerOff() -> void {
    record.finish();
    drive.reset();
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

    flush();
}

auto AudioManager::flush( ) -> void {

    rData.in = &buffer[0];
    rData.inputFrames = stat.stereoSound ? bufferPos >> 1 : bufferPos;

    bufferPos = 0;

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

    if (mixFloppySounds)
        drive.mixSound(rData.out, rData.outputFrames << 1);

    if ( audioDriver->expectFloatingPoint() ) {

        for (unsigned i = 0; i < (rData.outputFrames << 1); i++ )
            outBufferFloat[i] = *(rData.out + i);

        record.write( (uint8_t*) outBufferFloat, rData.outputFrames );

        // 4 byte per channel, 8 byte per audio frame
        audioDriver->addSamples( (uint8_t*) outBufferFloat, rData.outputFrames << 3);

    } else {

        for (unsigned i = 0; i < (rData.outputFrames << 1); i++)
            outBuffer[i] = sclamp<16>( *(rData.out + i) * 32767.0 );

        record.write( (uint8_t*) outBuffer, rData.outputFrames );

        // 2 byte per channel, 4 byte per audio frame
        audioDriver->addSamples( (uint8_t*) outBuffer, rData.outputFrames << 2);
    }

    if (measureUiUpdate.enable)
        checkIfUINeedsAnUpdate();
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
        statusHandler->setDrcBufferUpdate();
    }
}

auto AudioManager::checkIfUINeedsAnUpdate() -> void {
    unsigned ts = Chronos::getTimestampInMilliseconds();

    if ((ts - measureUiUpdate.lastTS) > 20) {
        activeEmulator->requestImmediateReturn();
        measureUiUpdate.lastTS = ts;
    }
}
