
#include "cmd/cmd.h"
#include <cstring>

auto Program::initVideo() -> void {
    
	if (videoDriver)
        delete videoDriver;
    
    if (cmd->noDriver) {
        videoDriver = new DRIVER::Video;
        return;
    }

    videoDriver = DRIVER::Video::create( getVideoDriver() );
    
    setVideoSynchronize();
    setVideoHardSync();
    setFpsLimit();
    //setVideoFilter();
    updateFullscreenSetting();
	    
    if ( !videoDriver->init( view->getViewportHandle() ) ) {
        delete videoDriver;
        videoDriver = new DRIVER::Video;
    }
	
	videoDriver->setFilter( DRIVER::Video::Filter::Linear );

    if (activeVideoManager)
        activeVideoManager->reinitThread(true);    
        
    // opengl crt shader only at the moment
    for( auto emuView : emuConfigViews )
        emuView->videoLayout->updateVisibillity();
        
    for( auto emulator : emulators )        
        VideoManager::getInstance( emulator )->reloadSettings();
	
	VideoManager::setShaderInputPrecision( globalSettings->get<bool>("shader_input_precision", false) );
	VideoManager::setThreaded( globalSettings->get<bool>("crt_threaded", true) );
	
	if (!cmd->debug) {
		view->loadPlaceholder();
        view->renderPlaceholder();
	}
}

auto Program::setVideoManagerGlobals() -> void {
	VideoManager::setAspectCorrect( globalSettings->get<bool>("aspect_correct", true) );
	VideoManager::setIntegerScaling( globalSettings->get<bool>("integer_scaling", false) );
}

auto Program::getVideoDriver() -> std::string {
	auto curDriver = globalSettings->get<std::string>("video_driver", "");
	auto drivers = DRIVER::Video::available();
	
	for(auto& driver : drivers) {
		if(curDriver == driver) return driver;
	}
	return DRIVER::Video::preferred();
}

auto Program::finishVBlank() -> void {
    
    activeVideoManager->waitForRenderer();

    videoDriver->unlock();
    
    if (VideoManager::fpsLimit)
        activeVideoManager->applyFpsLimit();
    
    videoDriver->redraw();
}

auto Program::midScreenCallback() -> void {
    
    activeVideoManager->renderMidScreen();
}

auto Program::videoRefresh(const uint16_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void {

	if (cmd->noGui)
		return;
    
    statusHandler->updateFrameCounter();
	
    if (frame)
        activeVideoManager->renderFrame<uint16_t>(frame, width, height, linePitch);
}

auto Program::videoRefresh8(const uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void {
    
	if (cmd->noGui)
		return;
	
    statusHandler->updateFrameCounter();
	
    if (frame)
        activeVideoManager->renderFrame<uint8_t>(frame, width, height, linePitch);
}

auto Program::setVideoSynchronize() -> void {
    bool vsync = false;
    float skew = 0.0;

    if (!activeEmulator)
        return;

    if (audioDriver->hasSynchronized()) {
        vsync = globalSettings->get<bool>("video_sync", true);
        bool adaptive = globalSettings->get<bool>("adaptive_sync", true);

        if (vsync && adaptive) {
            float monitorFrequency = GUIKIT::Monitor::getCurrentRefreshRate();
            skew = std::abs(1.0 - (float)audioManager->inputFPS / monitorFrequency );

            if ((skew > 0.0015) && ((float)audioManager->inputFPS > monitorFrequency)) {
                vsync = false;
            }
        }
    }

    if (videoDriver->hasSynchronized() != vsync)
        videoDriver->synchronize( vsync );

    audioManager->skew = skew;
    audioManager->setRateControl();
	updateOverallSynchronize();
}

auto Program::setVideoHardSync() -> void {
    videoDriver->hardSync( globalSettings->get<bool>("gl_hardsync", false) );
}

auto Program::hintExclusiveFullscreen() -> void {
	videoDriver->hintExclusiveFullscreen( globalSettings->get("exclusive_fullscreen", false), view->getCustomFullscreenRefreshRate() );
}

auto Program::setFpsLimit() -> void {
//	VideoManager::setFpsLimit( globalSettings->get("fps_limit", false) );
//    audioManager->setRateControl();
//	updateOverallSynchronize();
}

auto Program::setVideoFilter() -> void {
	if (activeEmulator)			
		videoDriver->setFilter( (DRIVER::Video::Filter)getSettings( activeEmulator )->get<unsigned>("video_filter", 1u, {0u, 1u}) );
	
	if (activeVideoManager)
        activeVideoManager->shader.recreate = true;
}

auto Program::setPalette( Emulator::Interface* emulator ) -> void {
    
    auto paletteManager = PaletteManager::getInstance( emulator );
    
    if (!paletteManager)
        return;
    
    auto videoManager = VideoManager::getInstance( emulator );
    
    videoManager->setPalette( paletteManager->getCurrentPalette() );
}

auto Program::updateCrop( Emulator::Interface* emulator ) -> void {
	
    auto settings = getSettings( emulator );
    
	auto left = settings->get<unsigned>("crop_left", 0, {0u,100u});
	auto right = settings->get<unsigned>("crop_right", 0, {0u,100u});
	auto top = settings->get<unsigned>("crop_top", 0, {0u,100u});
	auto bottom = settings->get<unsigned>("crop_bottom", 0, {0u,100u});
	
	auto type = settings->get<unsigned>("crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u,4u});
	auto aspectCorrect = settings->get<bool>("crop_aspect_correct", 0);
	
	emulator->crop( (Emulator::Interface::CropType) type, aspectCorrect, left, right, top, bottom );
    
    if (activeVideoManager) {
        activeVideoManager->reinitThread();
        activeVideoManager->shader.recreate = true;        
    }
}

auto Program::toggleFastForward(bool aggressive) -> void {
    if (!activeEmulator)
        return;

    bool ff = warp.active && !warp.aggressive;
    bool ffa = warp.active && warp.aggressive;

    if ( (!ff && !ffa) || (ff && !aggressive) || (ffa && aggressive) )
        if (warp.motorControlled)
            warp.enableAutoWarp = false;

    if ( (!aggressive && ffa) || (aggressive && ff) ) {
        // switch modes (already active)
        unsigned val = (unsigned)Emulator::Interface::FastForward::NoAudioOut | (unsigned)Emulator::Interface::FastForward::ReduceVideoOutput;
        if (aggressive)
            val |= (unsigned)Emulator::Interface::FastForward::NoVideoSequencer;

        activeEmulator->fastForward( val );
        warp.aggressive = aggressive;

        if (view)
            view->updateFastforwardCheck();
    } else
        fastForward( !ff && !ffa, aggressive);
}

auto Program::fastForward( bool activate, bool aggressive ) -> void {
    if (!activeEmulator)
        return;
    
    auto settings = getSettings( activeEmulator );
    
    unsigned forward = 0;
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});

    if (activate) {
        if (videoDriver->hasSynchronized())
            videoDriver->synchronize( false );

        if (crtMode != VideoManager::CrtMode::None) {
            settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);
            VideoManager::getInstance( activeEmulator )->reloadSettings();
        }

        globalSettings->set<unsigned>("video_crt_temp", (unsigned)crtMode, false); // remember crt mode

        forward = (unsigned)Emulator::Interface::FastForward::NoAudioOut | (unsigned)Emulator::Interface::FastForward::ReduceVideoOutput;
        if (aggressive)
            forward |= (unsigned)Emulator::Interface::FastForward::NoVideoSequencer;

    } else {
        VideoManager::CrtMode crtModeTemp = (VideoManager::CrtMode)globalSettings->get<unsigned>("video_crt_temp", (unsigned)VideoManager::CrtMode::None, {0u, 2u});

        program->setVideoSynchronize();

        if (crtMode == VideoManager::CrtMode::None) {
            if (crtModeTemp == VideoManager::CrtMode::Cpu) {
                settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
                VideoManager::getInstance( activeEmulator )->reloadSettings();

            } else if (crtModeTemp == VideoManager::CrtMode::Gpu) {
                settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Gpu);
                VideoManager::getInstance( activeEmulator )->reloadSettings();
            }
        }

        globalSettings->set<unsigned>("video_crt_temp", (unsigned)VideoManager::CrtMode::None, false);

        if (audioManager)
            audioManager->drive.reset();
    }

    if (activate)
        warp.aggressive = aggressive;

    warp.active = activate;

    if (statusHandler)
        statusHandler->resetFrameCounter();

    activeEmulator->fastForward( forward );

    if (view)
        view->updateFastforwardCheck();
	
	updateOverallSynchronize();
}

auto Program::updateOverallSynchronize() -> void {
	VideoManager::synchronized = false;
	
	bool fastForward = activeEmulator && activeEmulator->getForward();
	
	if (fastForward)
		return;	
	
	bool vSync = videoDriver->hasSynchronized();
	
	bool aSync = audioDriver->hasSynchronized();
	
	bool fpsLimit = VideoManager::fpsLimit;	
	
	if ( vSync || fpsLimit || aSync )
		VideoManager::synchronized = true;
}

auto Program::updateFullscreenSetting() -> void {

    if (!view)
        return;

    bool _active = globalSettings->get<bool>("fullscreen_setting_active", false);
    unsigned _display = globalSettings->get<unsigned>("fullscreen_display", 0 );
    unsigned _setting = globalSettings->get<unsigned>("fullscreen_setting", 0 );

    if (!_active || (_setting == 0) )
        view->setFullscreenSetting( false );
    else
        view->setFullscreenSetting( true, _display, _setting );

    program->hintExclusiveFullscreen();
}