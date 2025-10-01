
#include "manager.h"
#include "../cmd/cmd.h"
#include "dsp/bass.h"
#include "dsp/echo.h"
#include "dsp/reverb.h"
#include "dsp/panning.h"
#include "../view/status.h"
#include "../view/view.h"
#include "../tools/chronos.h"

AudioManager* audioManager = nullptr;

AudioManager::AudioManager() : drive(*this) {
    
    volumeAdjust = 1.0 / 32768.0;
    
    rData.out = new float[65536];
    
    cosine.setData( &rData );

    muteTimer.setInterval(200);

    // prevent RESID crack on power up
    muteTimer.onFinished = [this]() {
        muteTimer.setEnabled(false);
        setVolume();
    };

    bass = new DSP::Bass;
    echo = new DSP::Echo;
    reverb = new DSP::Reverb;
    panning = new DSP::Panning;
}

AudioManager::~AudioManager() {
    
    delete[] rData.out;
    delete bass;
    delete echo;
    delete reverb;
    delete panning;
}

auto AudioManager::setLatency() -> void {

    unsigned latency = globalSettings->get<unsigned>("audio_latency", 30u, {1u, 120u});

    audioDriver->setLatency( latency );
}

auto AudioManager::setFrequency() -> void {
    unsigned frequency = globalSettings->get<unsigned>("audio_frequency_v2", 48000u, {0u, 48000u});
    audioDriver->setFrequency( frequency );
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

auto AudioManager::setResampler(float overrideRate) -> void {
    if (!activeEmulator)
        return;
    
    stat = activeEmulator->getStatsForSelectedRegion();
    
    double inputFrequency = stat.sampleRate;
    inputFPS = stat.fps;

    measureUiUpdate.enable = false;
    
    double monitorRatio = 1.0;

    float speed;
    bool percent;
    unsigned speedProfile;

    if (overrideRate == 0.0f)
        speedProfile = view->getSpeedBySelectedProfile(speed, percent);
    else {
        speedProfile = ~0;
        percent = false;
        speed = overrideRate;
    }

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
    
    ratio = (double)audioDriver->getFrequency() / inputFrequency;

    cosine.reset( ratio, stat.stereoSound ? 2 : 1 );   
    
    activeEmulator->setMonitorFpsRatio( monitorRatio );

    // check changed FPS for adaptive sync
    VideoManager::setSynchronize();
}

auto AudioManager::setBufferSize() -> void {
    if (!activeEmulator)
        return;

    stat = activeEmulator->getStatsForSelectedRegion();

    if (rewind) {
        bufferSize = (stat.sampleRate / stat.fps) * 2.0 * 1.06;
        if (bufferSize > MAX_AUDIO_BUF_SIZE) {
            bufferSize = MAX_AUDIO_BUF_SIZE;
            //fprintf(stderr, "audio buf limited\n");
        }
    } else {
        bufferSize = 2048;
    
        if (audioDriver->hasSynchronized() || dynamicRateControl)
            bufferSize = 512;
    }

    if (!stat.stereoSound)
        bufferSize >>= 1;
    
    bufferPos = 0;
}

auto AudioManager::setVolume() -> void {
    if (!activeEmulator)
        return;

    auto settings = program->getSettings(activeEmulator);
    unsigned volume = settings->get<unsigned>("audio_volume", 100u, {0u, 100u});
    bool mute = muteTimer.enabled() || globalSettings->get<bool>("audio_mute", false);
        
    volumeAdjust = 0.0;
    
    if (!mute)
        volumeAdjust = ( (float)volume * 0.01 ) / 32768.0;
}

auto AudioManager::setTapeNoise( ) -> void {
    if (!activeEmulator)
        return;

    auto settings = program->getSettings( activeEmulator );

    bool active = settings->get<bool>( "audio_tape_noise", false);

    unsigned volume = settings->get<unsigned>("audio_tape_noise_volume", 100u, {0u, 300u});

    activeEmulator->setTapeLoadingNoise( active ? volume : 0 );
}

auto AudioManager::resetDriveSounds() -> void {
    drive.unload();
    setDriveSounds(false);
}

auto AudioManager::setDriveSounds( bool init ) -> void {
    if (!activeEmulator)
        return;

    auto settings = program->getSettings( activeEmulator );

    bool mixFloppySounds = settings->get<bool>("audio_floppy", false);
    bool mixTapeSounds = settings->get<bool>("audio_tape", false);

    mixDriveSounds = mixFloppySounds | mixTapeSounds;

    Emulator::Interface::MediaGroup* group = activeEmulator->getDiskMediaGroup();

    if (group) {
        if (mixFloppySounds) {
            unsigned volume = settings->get<unsigned>("audio_floppy_volume", 200u, {0u, 300u});
            unsigned volumeExt = settings->get<unsigned>("audio_floppy_volume_external", 200u, {0u, 300u});
            bool extIsSame = drive.getFloppyFolder(activeEmulator, false) == drive.getFloppyFolder(activeEmulator, true);

            if (!drive.loaded(activeEmulator, group))
                drive.readPack(activeEmulator, group);

            if (extIsSame)
                drive.unload(activeEmulator, group, true);
            else {
                if (!drive.loaded(activeEmulator, group, true))
                    drive.readPack(activeEmulator, group, true);
            }

            drive.setVolume(activeEmulator, group, (float) volume * 0.01, false);
            drive.setVolume(activeEmulator, group, (float) volumeExt * 0.01, true);
        } else
            drive.reset(group);
    }

    if (init)
        activeEmulator->enableFloppySounds(mixFloppySounds);

    group = activeEmulator->getTapeMediaGroup();

    if (group) { // amiga has no tape
        if (mixTapeSounds) {
            unsigned volume = settings->get<unsigned>("audio_tape_volume", 100u, {0u, 300u});

            if (!drive.loaded(activeEmulator, group))
                drive.readPack(activeEmulator, group);

            drive.setVolume(activeEmulator, group, (float) volume * 0.01, false);
        } else
            drive.reset(group);
    }

    if (init)
        activeEmulator->enableTapeSounds(mixTapeSounds);
}

auto AudioManager::setAudioDsp() -> void {
    
    if (!activeEmulator)
        return;

    dsps.clear();
    
    stat = activeEmulator->getStatsForSelectedRegion();
    
    auto settings = program->getSettings( activeEmulator );

    bool useBass = settings->get<bool>("audio_bass", false );
    
    if (useBass) {
        bass->setMono( !stat.stereoSound );
        bass->init( 
            (float)audioDriver->getFrequency(),
            (float)settings->get<unsigned>("audio_bass_freq", 200, {20, 200} ),
            (float)settings->get<unsigned>("audio_bass_gain", 10, {0, 40} ),
            settings->get<float>("audio_bass_clipping", 0.4, {0.0, 1.0} )
        );
        
        dsps.push_back( (DSP::Base*)bass );
    }

    bool useEcho = settings->get<bool>("audio_echo", false );

    if (useEcho) {
        echo->setMono( !stat.stereoSound );
        echo->init(
            (float)audioDriver->getFrequency(),
            settings->get<unsigned>("audio_echo_delay", 200, {0, 1000} ),
            settings->get<float>("audio_echo_feedback", 0.5, {0.0, 1.0} ),
            settings->get<float>("audio_echo_amp", 0.2, {0.0, 1.0} )
        );

        dsps.push_back( (DSP::Base*)echo );
    }

    bool useReverb = settings->get<bool>("audio_reverb", false );
    
    if (useReverb) {
        reverb->setMono( !stat.stereoSound );
        reverb->init( 
            (float)audioDriver->getFrequency(),
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

auto AudioManager::power() -> void {
    rewind = false;
    setSynchronize();
    setResampler();
    setBufferSize();
    setAudioDsp();
    setVolume();
    setDriveSounds();
    setTapeNoise();

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
    
    buffer[bufferPos++] = sampleLeft * volumeAdjust;
    
    if (stat.stereoSound)
        buffer[bufferPos++] = sampleRight * volumeAdjust;

    if (bufferPos < bufferSize)
        return;

    //if (rewind)
      //  fprintf(stderr, "rewind audio buf too small ");

    flush();
}

auto AudioManager::setRewind(bool state) -> void {
    rewind = state;
    reverseAndFlushBuffer();
    setBufferSize();
}

auto AudioManager::reverseAndFlushBuffer() -> void {
    if (!bufferPos)
        return;

    if (stat.stereoSound) {
        for (unsigned int i = 0; i < (bufferPos >> 1); i++) {
            if (i & 1)
                std::swap( buffer[i], buffer[bufferPos - i] );
            else
                std::swap( buffer[i], buffer[bufferPos - 2 - i] );
        }
    } else {
        for (unsigned int i = 0; i < (bufferPos >> 1); i++) {
            std::swap( buffer[i], buffer[bufferPos - 1 - i] );
        }
    }

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

    if (mixDriveSounds)
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
