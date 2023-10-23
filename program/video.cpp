
#include "cmd/cmd.h"
#include <cstring>

auto Program::initVideo(bool driverChange) -> void {
    
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
	    
    if ( !videoDriver->init( view->getViewportHandle(driverChange) ) ) {
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
    view->updateShaderVisibility();
    view->loadDragnDropOverlay();
}

auto Program::setVideoDimension(Emulator::Interface* emulator) -> void {
    if (!activeEmulator || (emulator && (emulator != activeEmulator)))
        return;

    auto settings = program->getSettings( activeEmulator );

    int aspectMode = settings->get<int>("aspect_mode", 1, {0, 2});
    bool integerScaling = settings->get<bool>("integer_scaling", false);

    videoDriver->setRatio( aspectMode, integerScaling );
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

    return !isPause && videoDriver->canExclusiveFullscreen()
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

auto Program::getCropHotkeyDefault() -> unsigned {
    return 1 | 2 | 4 | 8 | 0x80 | 0x100 | 0x200;
}

auto Program::getCropDefault(int pos, int direction) -> unsigned {
    if (pos > 5 || direction > 3)
        return 0;

    static int Adjustments[6][4] = {
        {0, 0, 0, 0},
        {45, 19, 11, 37},
        {45, 19, 16, 10},
        {39, 15, 16, 11},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    return Adjustments[pos][direction];
}

auto Program::upgradeCropSettings() -> void {
    for(auto emulator : emulators) {
        auto settings = getSettings( emulator );

        if (!settings->get<bool>("upd_crop", false)) {
            settings->set<bool>("upd_crop", true);

            auto cropType = settings->get<int>("crop_type", (unsigned) Emulator::Interface::CropType::Monitor, {0u, 11u});
            auto valCropAC = settings->get<bool>("crop_aspect_correct", false);
            settings->remove("crop_aspect_correct");

            switch(cropType) {
                case 2: cropType = valCropAC ? 2 : 3; break;
                case 3: cropType = valCropAC ? 4 : 5; break;
                case 4: cropType = 6; break;
                default:
                    continue;
            }

            settings->set<unsigned>("crop_type", cropType);
            if (dynamic_cast<LIBAMI::Interface*>(emulator))
                settings->set<unsigned>("border_hotkey", program->getCropHotkeyDefault() );
        }
    }
}

auto Program::getCrop(Emulator::Interface* emulator, Emulator::Interface::Crop& crop) -> bool {
    typedef Emulator::Interface::CropType CropType;
    auto settings = getSettings( emulator );
    int type = settings->get<int>("crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 11u});
    bool hasAmiga = dynamic_cast<LIBAMI::Interface*>(emulator);

    if ((CropType)type == CropType::AllSidesRatio || (CropType)type == CropType::AllSides) {
        crop.right = crop.top = crop.bottom = crop.left = settings->get<unsigned>("crop_all", 0, {0u, 100u});
    } else if (type >= (int)Emulator::Interface::CropType::Free) {
        int offset = type - (int)Emulator::Interface::CropType::Free;
        std::string _offset = offset ? std::to_string(offset) : "";

        crop.left = settings->get<unsigned>("crop_left" + _offset, hasAmiga ? getCropDefault(offset, 0) : 0, {0u, 100u});
        crop.right = settings->get<unsigned>("crop_right" + _offset, hasAmiga ? getCropDefault(offset, 1) : 0, {0u, 100u});
        crop.top = settings->get<unsigned>("crop_top" + _offset, hasAmiga ? getCropDefault(offset, 2) : 0, {0u, 100u});
        crop.bottom = settings->get<unsigned>("crop_bottom" + _offset, hasAmiga ? getCropDefault(offset, 3) : 0, {0u, 100u});
    } else
        return false;

    return true;
}

auto Program::setCrop(Emulator::Interface* emulator, std::string ident, int value) -> void {
    typedef Emulator::Interface::CropType CropType;
    bool isDimension = ident == "crop_left" || ident == "crop_right" || ident == "crop_bottom" || ident == "crop_top";
    auto settings = getSettings( emulator );

    if (isDimension) {
        int type = settings->get<int>("crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 11u});
        if ((CropType) type == CropType::AllSidesRatio || (CropType) type == CropType::AllSides) {
            ident = "crop_all";

        } else if (type >= (int) Emulator::Interface::CropType::Free) {
            int offset = type - (int) Emulator::Interface::CropType::Free;
            std::string _offset = offset ? std::to_string(offset) : "";

            ident += _offset;
        }
    }

    settings->set<unsigned>( ident, value );
}

auto Program::getCropMessage( Emulator::Interface* emulator, Emulator::Interface::CropType cropType) -> std::string {
    typedef Emulator::Interface::CropType CropType;
    std::string out;

    switch(cropType) {
        case CropType::Off: out = "disabled"; break;
        case CropType::Monitor: out = "monitor"; break;
        case CropType::AutoRatio: out = "crop complete ratio"; break;
        case CropType::Auto: out = "crop complete"; break;
        case CropType::AllSidesRatio: out = "crop all sides equally ratio"; break;
        case CropType::AllSides: out = "crop all sides equally"; break;
        default:
        case CropType::Free: out = "crop each side manually"; break;
    }

    return trans->getA(out) + " (" + std::to_string(int(cropType)) + ")";
}

auto Program::updateCrop( Emulator::Interface* emulator ) -> void {
    auto settings = getSettings( emulator );
    int type = settings->get<int>("crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 6u});
    Emulator::Interface::Crop crop = {0};
    getCrop(emulator, crop);

	emulator->cropFrame( (Emulator::Interface::CropType) type, crop );
    
    if (activeVideoManager && (activeVideoManager->emulator == activeEmulator)) {
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
    if (!activeEmulator)
        return;
    auto _settings = getSettings(activeEmulator);

    if (!view)
        return;

    bool _active = _settings->get<bool>("fullscreen_setting_active", false);
    unsigned _display = _settings->get<unsigned>("fullscreen_display", 0 );
    unsigned _setting = _settings->get<unsigned>("fullscreen_setting", 0 );

    if (!_active || (_setting == 0) )
        view->setFullscreenSetting( false );
    else
        view->setFullscreenSetting( true, _display, _setting );

    hintExclusiveFullscreen();
}

auto Program::fpsChanged() -> void {
    if (quitInProgress)
        return;

    if (emuThread->enabled)
        emuThread->updateFps = true;
    else
        fpsChangeTimer.setEnabled();
}