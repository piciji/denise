
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
    
    VideoManager::setSynchronize();
    VideoManager::setHardSync();
    setVideoFilter();
    setVideoDimension();
    updateFullscreenSetting();
	    
    if ( !videoDriver->init( view->getViewportHandle() ) ) {
        delete videoDriver;
        videoDriver = new DRIVER::Video;
    }

    if (activeVideoManager)
        activeVideoManager->reinitCrtThread(true);
        
    // opengl crt shader only at the moment
    for( auto emuView : emuConfigViews ) {
        if (emuView->videoLayout)
            emuView->videoLayout->updateVisibillity();
    }
        
    for( auto emulator : emulators )        
        VideoManager::getInstance( emulator )->reloadSettings();
	
	VideoManager::setShaderInputPrecision( globalSettings->get<bool>("shader_input_precision", false) );
	VideoManager::setCrtThreaded( globalSettings->get<bool>("crt_threaded", true) );
}

auto Program::setVideoDimension() -> void {
    bool integerScaling = globalSettings->get<bool>("integer_scaling", false);

    if (globalSettings->get<bool>("aspect_correct", true)) {
        videoDriver->setAspectCorrection( 4.0, 3.0, integerScaling);
    } else
        videoDriver->setAspectCorrection( 1.0, 1.0, integerScaling);
}

auto Program::getVideoDriver() -> std::string {
	auto curDriver = globalSettings->get<std::string>("video_driver", "");
	auto drivers = DRIVER::Video::available();
	
	for(auto& driver : drivers) {
		if(curDriver == driver) return driver;
	}
	return DRIVER::Video::preferred();
}

auto Program::midScreenCallback(uint8_t options) -> void {

    switch(options) {
        case 0: activeVideoManager->renderMidScreen<0>(); break;
        case 1: activeVideoManager->renderMidScreen<1>(); break;
        case 2: activeVideoManager->renderMidScreen<2>(); break;

        case 4: activeVideoManager->renderMidScreen<4>(); break;
        case 5: activeVideoManager->renderMidScreen<5>(); break;
        case 6: activeVideoManager->renderMidScreen<6>(); break;
    }
}

auto Program::videoRefresh(const uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t options) -> void {

	if (cmd->noGui)
		return;
    
    statusHandler->updateFrameCounter();
	
    if (frame) {
        switch(options) {
            case 0: activeVideoManager->renderFrame<uint16_t, 0>(frame, width, height, linePitch); break;
            case 1: activeVideoManager->renderFrame<uint16_t, 1>(frame, width, height, linePitch); break;
            case 2: activeVideoManager->renderFrame<uint16_t, 2>(frame, width, height, linePitch); break;

            case 4: activeVideoManager->renderFrame<uint16_t, 4>(frame, width, height, linePitch); break;
            case 5: activeVideoManager->renderFrame<uint16_t, 5>(frame, width, height, linePitch); break;
            case 6: activeVideoManager->renderFrame<uint16_t, 6>(frame, width, height, linePitch); break;
        }
    }
}

auto Program::videoRefresh8(const uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void {
    
	if (cmd->noGui)
		return;
	
    statusHandler->updateFrameCounter();
	
    if (frame)
        activeVideoManager->renderFrame<uint8_t>(frame, width, height, linePitch);
}

auto Program::canExclusiveFullscreen() -> bool {

    return !isPause
        && globalSettings->get<bool>("exclusive_fullscreen", false)
        && !globalSettings->get<bool>("threaded_emu", false);
}

auto Program::hintExclusiveFullscreen() -> void {

    if (canExclusiveFullscreen())
        videoDriver->hintExclusiveFullscreen( true, view->getCustomFullscreenRefreshRate() );
    else
        videoDriver->hintExclusiveFullscreen( false );
}

auto Program::setVideoFilter(bool driverOnly) -> void {
	if (activeEmulator)
		videoDriver->setFilter( (DRIVER::Video::Filter)getSettings( activeEmulator )->get<unsigned>("video_filter", 1u, {0u, 1u}) );
	
	if (!driverOnly && activeVideoManager)
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
        activeVideoManager->reinitCrtThread();
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
    activeVideoManager = VideoManager::getInstance( activeEmulator );
    
    unsigned forward = 0;
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});

    if (activate) {
        VideoManager::hidePlaceHolder();
        warp.active = true;
        warp.aggressive = aggressive;
        VideoManager::setFrameRender(1);

        if (videoDriver->hasSynchronized())
            videoDriver->synchronize( false );

        if (videoDriver->hasVRR())
            videoDriver->setVRR(false);

        if (crtMode != VideoManager::CrtMode::None) {
            settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);
            if (activeVideoManager)
                activeVideoManager->reloadSettings();
            settings->set<unsigned>("video_crt", (unsigned)crtMode);
        }

        forward = (unsigned)Emulator::Interface::FastForward::NoAudioOut | (unsigned)Emulator::Interface::FastForward::ReduceVideoOutput;
        if (aggressive)
            forward |= (unsigned)Emulator::Interface::FastForward::NoVideoSequencer;

    } else {
        warp.active = false;

        VideoManager::setSynchronize();

        if (activeVideoManager) {
            if (crtMode != VideoManager::CrtMode::None)
                activeVideoManager->reloadSettings();

            activeVideoManager->resetTempData();
        }

        if (audioManager)
            audioManager->drive.reset();
    }

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

    bool vrr = videoDriver->hasVRR();
	
	bool aSync = audioDriver->hasSynchronized();

	if ( vSync || vrr || aSync )
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

auto Program::fpsChanged() -> void {
    if (quitInProgress)
        return;

    if (emuThread->enabled)
        emuThread->updateFps = true;
    else
        fpsChangeTimer.setEnabled();
}