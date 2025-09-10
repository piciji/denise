
#include "cmd/cmd.h"
#include <cstring>

auto Program::initVideo(bool driverChange) -> void {

    DRIVER::Rotation rotation = DRIVER::ROT_0;

	if (videoDriver) {
        rotation = videoDriver->getRotation();
        delete videoDriver;
    }
    
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
    updateHDR();
    updateBFI();
	    
    if ( !videoDriver->init( view->getViewportHandle(driverChange) ) ) {
        delete videoDriver;
        videoDriver = new DRIVER::Video;
    }

    if (driverChange)
        videoDriver->setRotation(rotation);

    if (activeVideoManager) {
        activeVideoManager->reinitCrtThread(true);
        activeVideoManager->rebuildShader = true;
    }
        
    for( auto emulator : emulators ) {
        checkShaderSupport(emulator);
        auto videoManager =  VideoManager::getInstance(emulator);
        auto settings = program->getSettings( emulator );

        videoManager->setCrtThreaded( settings->get<bool>("cpu_filter_threaded", true) );

        auto emuView = EmuConfigView::TabWindow::getView(emulator);
        if (emuView && emuView->presentationLayout) {
            emuView->presentationLayout->updatePresets(true, true);
            emuView->presentationLayout->checkHDR();
        } else
            videoManager->reloadSettings(true);
    }

    view->buildShader();
    view->loadDragnDropOverlay();
    videoDriver->setShaderProgressCallback( [](int pass, bool hasErrors) {
        if (program->quitInProgress)
            return;
        if (pass < 0) {
            auto manager = VideoManager::getInstance(activeEmulator);
            if (manager) {
                if (emuThread->enabled)
                    emuThread->events |= EmuThread::EVT_SHADER_ERROR;
                else
                    manager->finishPreset();
            }
        }

        if (statusHandler) {
            emuThread->lockStatus();
            if (pass < 0) {
                statusHandler->setMessage(trans->get(hasErrors ? "shader has errors" : "shader activated"), hasErrors);
            } else {
                statusHandler->setMessage(trans->get(hasErrors ? "pass error" : "pass success",
                    {{"%pass%", std::to_string(pass)}}), hasErrors);
            }
            emuThread->unlockStatus();
        }
    } );

    videoDriver->setShaderCacheCallback( [this](DRIVER::DiskFile& diskFile) {
        if (diskFile.isLUT) {
            GUIKIT::File file(diskFile.path);
            if (!file.open())
                return;

            GUIKIT::Image img;
            if (!img.load(file.read(), file.getSize(), true ))
                return;

            diskFile.data = img.data;
            diskFile.width = img.width;
            diskFile.height = img.height;

            return;
        }

        std::string subPath = GUIKIT::File::getPath(diskFile.path);
        std::string cacheFile = GUIKIT::String::getFileName(diskFile.path);

        if (diskFile.data && diskFile.size) {
            std::string absPath = program->generatedFolder(subPath, true) + cacheFile;
            GUIKIT::File f(absPath, true);

            if (f.open(GUIKIT::File::Mode::Write))
                f.write(diskFile.data, diskFile.size);
        } else {
            std::string absPath = program->generatedFolder(subPath, false) + cacheFile;
            GUIKIT::File f(absPath, true);

            if (f.open()) {
                diskFile.data = f.read();
                diskFile.size = f.getSize();
            }
        }
    } );

    videoDriver->setScreenshotCallback([this](uint8_t* _data, unsigned _width, unsigned _height) {
        takeScreenshot(_data, _width, _height);
    } );
    
    if (activeEmulator) {
        videoDriver->useShaderCache( getSettings( activeEmulator )->get<bool>("shader_cache", true) );
    }

    updateOnScreenText();

    loadProgress();
}

auto Program::bufferScreenshot(uint8_t* _data, unsigned _size) -> void {
    auto& screenShot = view->screenshot;
    unsigned useSize = _size * 3;

    if (!screenShot.bufferSize)
        screenShot.bufferSize = useSize;
    else if (screenShot.bufferSize != useSize) {
        view->clearScreenshotBuffer();
        screenShot.bufferSize = useSize;
    }

    uint8_t* ptr = new uint8_t[useSize];
    screenShot.buffer.push_back(ptr);
    std::memcpy(ptr, _data, useSize);
}

auto Program::takeScreenshot(uint8_t* _data, unsigned _width, unsigned _height) -> void {
    auto& screenShot = view->screenshot;
    screenShot.sharedMutex.lock();
    GUIKIT::Image::Encoded encoded;
    encoded.data = nullptr;
    GUIKIT::Image image;
    std::vector<uint32_t> colorTable;

    if (screenShot.saveState) {
        GUIKIT::Image image(_width, _height, _data, GUIKIT::Image::Format::RGB);
        image.scaleLinear(200, 150);
        encoded = image.generate(GUIKIT::Image::Type::PNG);

    } else if (screenShot.animatedGif) {
        bufferScreenshot(_data, _width * _height);

        if (!--screenShot.gun) {            
            for (auto& pal : activeVideoManager->palette->paletteColors)
                colorTable.push_back(pal.rgb);

            encoded = image.generate(screenShot.type, colorTable, screenShot.buffer, _width, _height, screenShot.interval);
        }

    } else if (screenShot.twoFrames && !screenShot.buffer.size()) {
        bufferScreenshot(_data, _width * _height);

    } else {
        if (screenShot.writePalette) {
            for (auto& pal : activeVideoManager->palette->paletteColors)
                colorTable.push_back(pal.rgb);
            
            encoded = image.generate(screenShot.type, colorTable, {_data}, _width, _height);

        } else {
            if (screenShot.buffer.size()) {
                auto screenBefore = screenShot.buffer[0];

                if (screenShot.bufferSize == (_width * _height * 3)) {
                    uint8_t* src = screenBefore;
                    uint8_t* desc = _data;

                    for (unsigned y = 0; y < _height; y++) {
                        for (unsigned x = 0; x < (_width * 3); x++)
                            *desc++ = ((unsigned)*desc + (unsigned)*src++) >> 1;
                    }
                }
                view->clearScreenshotBuffer();
            }

            encoded = image.generate(screenShot.type, _data, _width, _height);
        }
    }

    if (encoded.data) {
        GUIKIT::File file;
        std::string _replace = std::to_string(Chronos::getTimestampInSecondsReal());
        if (screenShot.gun && !screenShot.animatedGif)
            _replace += "_" + std::to_string(screenShot.gun++);

        std::string _path = screenShot.path;

        GUIKIT::String::replace(_path, "#ident#", _replace);

        if ((encoded.type == GUIKIT::Image::Type::PNG) && (encoded.type != screenShot.type)) {
            // request GIF, but image has more than 256 colors -> write as PNG
            GUIKIT::String::replace(_path, ".gif", ".png");
        }
        file.setFile(_path);
        file.open(GUIKIT::File::Mode::Write);
        file.write(encoded.data, encoded.size);
        file.unload();
        delete[] encoded.data;
    }

    screenShot.sharedMutex.unlock();
}

auto Program::loadProgress() -> void {
    static GUIKIT::Image* mediaImage = nullptr;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;
        GUIKIT::File file(program->imgFolder() + "progress.png");

        if (!file.open())
            return;

        uint8_t* data = file.read();

        if (!data)
            return;

        mediaImage = new GUIKIT::Image;
        if (!mediaImage->loadPng(data, file.getSize()))
            return;
    }

    if (mediaImage && mediaImage->data)
        videoDriver->setProgressAnimation(mediaImage->data, mediaImage->width, mediaImage->height);
}

auto Program::setVideoDimension(Emulator::Interface* emulator) -> void {
    if (!activeEmulator || (emulator && (emulator != activeEmulator)))
        return;

    auto settings = program->getSettings( activeEmulator );

    int aspectMode = settings->get<int>("aspect_mode", 1, {0, 3});
    bool integerScaling = settings->get<bool>("integer_scaling", false);

    videoDriver->setAspectRatio( aspectMode, integerScaling );
}

auto Program::getVideoDriver() -> std::string {
	auto curDriver = globalSettings->get<std::string>("video_driver", "");
	auto drivers = DRIVER::Video::available();
	
	for(auto& driver : drivers) {
		if(curDriver == driver) return driver;
	}
	return DRIVER::Video::preferred();
}

auto Program::activateGPU(Emulator::Interface* emulator, bool state) -> void {
    auto vManager = VideoManager::getInstance( emulator );
    bool shaderActive = vManager->crtMode == VideoManager::CrtMode::Gpu;

    if (state != shaderActive) {
        auto settings = program->getSettings(emulator);
        settings->set<unsigned>("video_crt", state ? (unsigned)VideoManager::CrtMode::Gpu : (unsigned)VideoManager::CrtMode::None);
        vManager->reloadSettings(true);
    }
}

auto Program::midScreenCallback(uint8_t options) -> void {

    switch(options) {
        case 0: activeVideoManager->renderMidScreen<0>(); break;
        case 1: activeVideoManager->renderMidScreen<1>(); break;
        case 2: activeVideoManager->renderMidScreen<2>(); break;

        case 4: activeVideoManager->renderMidScreen<4>(); break;
        case 5: activeVideoManager->renderMidScreen<5>(); break;
        case 6: activeVideoManager->renderMidScreen<6>(); break;

        case 8: activeVideoManager->renderMidScreen<8>(); break;
        case 9: activeVideoManager->renderMidScreen<9>(); break;
        case 10: activeVideoManager->renderMidScreen<10>(); break;
    }
}

auto Program::videoRefresh(const uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t options) -> void {

	if (cmd->noGui)
		return;
    
    statusHandler->updateFrameCounter();
	
    if (frame) {
        switch(options) {
            //lores
            case 0: activeVideoManager->renderFrame<uint16_t, 0>(frame, width, height, linePitch); break; // non lace
            case 1: activeVideoManager->renderFrame<uint16_t, 1>(frame, width, height, linePitch); break; // lace odd
            case 2: activeVideoManager->renderFrame<uint16_t, 2>(frame, width, height, linePitch); break; // lace even
            // hires
            case 4: activeVideoManager->renderFrame<uint16_t, 4>(frame, width, height, linePitch); break;
            case 5: activeVideoManager->renderFrame<uint16_t, 5>(frame, width, height, linePitch); break;
            case 6: activeVideoManager->renderFrame<uint16_t, 6>(frame, width, height, linePitch); break;
            // shres
            case 8: activeVideoManager->renderFrame<uint16_t, 8>(frame, width, height, linePitch); break;
            case 9: activeVideoManager->renderFrame<uint16_t, 9>(frame, width, height, linePitch); break;
            case 10: activeVideoManager->renderFrame<uint16_t, 10>(frame, width, height, linePitch); break;
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

auto Program::repeatLastFrame() -> void {
    if (cmd->noGui)
        return;

    auto cropData = activeEmulator->cropData();

    if (cropData) {
        activeVideoManager->renderFrame<uint8_t>(cropData, activeEmulator->cropWidth(), activeEmulator->cropHeight(), activeEmulator->cropPitch());
    } else {
        auto cropData16 = activeEmulator->cropData16();

        if (cropData16) {
            unsigned _width = activeEmulator->cropWidth();
            unsigned _height = activeEmulator->cropHeight();
            unsigned _pitch = activeEmulator->cropPitch();
            unsigned _options = activeEmulator->cropOptions();

            switch(_options) {
                case 0: activeVideoManager->renderFrame<uint16_t, 0>(cropData16, _width, _height, _pitch); break;
                case 1: activeVideoManager->renderFrame<uint16_t, 1>(cropData16, _width, _height, _pitch); break;
                case 2: activeVideoManager->renderFrame<uint16_t, 2>(cropData16, _width, _height, _pitch); break;

                case 4: activeVideoManager->renderFrame<uint16_t, 4>(cropData16, _width, _height, _pitch); break;
                case 5: activeVideoManager->renderFrame<uint16_t, 5>(cropData16, _width, _height, _pitch); break;
                case 6: activeVideoManager->renderFrame<uint16_t, 6>(cropData16, _width, _height, _pitch); break;

                case 8: activeVideoManager->renderFrame<uint16_t, 8>(cropData16, _width, _height, _pitch); break;
                case 9: activeVideoManager->renderFrame<uint16_t, 9>(cropData16, _width, _height, _pitch); break;
                case 10: activeVideoManager->renderFrame<uint16_t, 10>(cropData16, _width, _height, _pitch); break;
            }
        }
    }
    if (VideoManager::takeScreenShots && view && isPause) {
        view->screenshot.pause = isPause;
        isPause = 0;
    }
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

auto Program::setVideoFilter() -> void {
	if (activeEmulator)
		videoDriver->setLinearFilter( getSettings( activeEmulator )->get<bool>("video_filter", true) );
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

auto Program::getScaleHotkeyDefault() -> unsigned {
    return 1 | 2 | 4 | 8;
}

auto Program::getCropDefault(Emulator::Interface* emulator, int pos, int direction) -> unsigned {
    if (!dynamic_cast<LIBAMI::Interface*>(emulator))
        return 0;

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

    if ((CropType)type == CropType::AllSidesRatio || (CropType)type == CropType::AllSides) {
        crop.right = crop.top = crop.bottom = crop.left = settings->get<unsigned>("crop_all", 0, {0u, 100u});
    } else if (type >= (int)Emulator::Interface::CropType::Free) {
        int offset = type - (int)Emulator::Interface::CropType::Free;
        std::string _offset = offset ? std::to_string(offset) : "";

        crop.left = settings->get<unsigned>("crop_left" + _offset, getCropDefault(emulator, offset, 0), {0u, 100u});
        crop.right = settings->get<unsigned>("crop_right" + _offset, getCropDefault(emulator, offset, 1), {0u, 100u});
        crop.top = settings->get<unsigned>("crop_top" + _offset, getCropDefault(emulator, offset, 2), {0u, 100u});
        crop.bottom = settings->get<unsigned>("crop_bottom" + _offset, getCropDefault(emulator, offset, 3), {0u, 100u});
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

auto Program::getScaleMessage(Emulator::Interface* emulator, int aspectMode ) -> std::string {
    std::string out;

    switch(aspectMode) {
        case 0: out = "window"; break;
        default:
        case 1: out = "CRT TV"; break;
        case 2: out = "Native"; break;
        case 3: out = "Native free"; break;
    }

    return trans->getA(out) + " (" + std::to_string(aspectMode) + ")";
}

auto Program::updateCrop( Emulator::Interface* emulator ) -> void {
    auto settings = getSettings( emulator );
    int type = settings->get<int>("crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 6u});
    Emulator::Interface::Crop crop = {0};
    getCrop(emulator, crop);

	emulator->cropFrame( (Emulator::Interface::CropType) type, crop );
    
    if (activeVideoManager && (activeVideoManager->emulator == activeEmulator)) {
        activeVideoManager->reinitCrtThread();
    }
}

auto Program::toggleWarp(bool aggressive) -> void {
    if (!activeEmulator)
        return;

    bool ff = warp.active && !warp.aggressive;
    bool ffa = warp.active && warp.aggressive;

    if (warp.manuellEndsAutoWarp && warp.motorControlled)
        warp.enableAutoWarp = false;

    if ( (!aggressive && ffa) || (aggressive && ff) ) {
        // switch modes (already active)
        unsigned val = (unsigned)Emulator::Interface::WarpMode::NoAudioOut | (unsigned)Emulator::Interface::WarpMode::ReduceVideoOutput;
        if (aggressive)
            val |= (unsigned)Emulator::Interface::WarpMode::NoVideoSequencer;

        activeEmulator->setWarpMode( val );
        warp.aggressive = aggressive;

        if (view)
            view->updateWarpCheck();
    } else {
        bool toggleOn = !ff && !ffa;
        setWarp( toggleOn, aggressive);
        warp.manuell = toggleOn;
    }
}

auto Program::setWarp( bool activate, bool aggressive ) -> void {
    if (!activeEmulator)
        return;

    activeVideoManager = VideoManager::getInstance( activeEmulator );
    unsigned forward = 0;

    if (activate) {
        activeEmulator->setLineCallback(false);
        if (activeVideoManager)
            activeVideoManager->waitForCrtRenderer();

        VideoManager::hidePlaceHolder();
        warp.active = true;
        warp.aggressive = aggressive;
        VideoManager::setFrameRender(1);

        if (videoDriver->hasSynchronized())
            videoDriver->synchronize( false );

        if (videoDriver->hasVRR())
            videoDriver->setVRR(false);

        forward = (unsigned)Emulator::Interface::WarpMode::NoAudioOut | (unsigned)Emulator::Interface::WarpMode::ReduceVideoOutput;
        if (aggressive)
            forward |= (unsigned)Emulator::Interface::WarpMode::NoVideoSequencer;

    } else {
        warp.active = false;

        VideoManager::setSynchronize();

        if (activeVideoManager) {
            activeVideoManager->resetTempData();
        }

        if (audioManager)
            audioManager->drive.reset();
    }

    if (statusHandler)
        statusHandler->resetFrameCounter();

    activeEmulator->setWarpMode( forward );

    if (view)
        view->updateWarpCheck();
	
	updateOverallSynchronize();
}

auto Program::updateOverallSynchronize() -> void {
	VideoManager::synchronized = false;

	if (activeEmulator && activeEmulator->getWarpMode())
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
        emuThread->events |= EmuThread::EVT_UPDATE_FPS;
    else
        fpsChangeTimer.setEnabled();
}

auto Program::setRotation() -> void {
    if (!activeEmulator)
        return;
    auto _settings = getSettings(activeEmulator);

    DRIVER::Rotation rotation = (DRIVER::Rotation)_settings->get<unsigned>("rotation", (unsigned)DRIVER::ROT_0, {0u, 3u});
    videoDriver->setRotation(rotation);
}

auto Program::checkShaderSupport(Emulator::Interface* emulator) -> void {
    auto emuView = EmuConfigView::TabWindow::getView(emulator);

    if (videoDriver->shaderSupport()) {
        if (emuView && emuView->presentationLayout)
            emuView->presentationLayout->addShaderUI();
        return;
    }

    auto settings = program->getSettings( emulator );
    auto crtMode = settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});

    if ((VideoManager::CrtMode)crtMode == VideoManager::CrtMode::Gpu)
        settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);

    auto vManager = VideoManager::getInstance(emulator);

    if (emuView && emuView->presentationLayout)
        emuView->presentationLayout->unloadShader();
    else if (vManager)
        vManager->clearPreset();
}

auto Program::updateOnScreenText(bool keepFontPath) -> void {
    static std::string systemFont = "inv";

    if (!activeEmulator)
        return;

    auto _settings = getSettings(activeEmulator);
    unsigned screenTextColor = _settings->get<unsigned>("screen_text_color", ~0);
    unsigned screenTextBgColor = _settings->get<unsigned>("screen_text_bgcolor", (255 << 24) | (69 << 16) | (128 << 8) | (116 << 0));
    unsigned screenWarnColor = _settings->get<unsigned>("screen_warn_color", (255 << 24) | (177 << 16) | (3 << 8) | (23 << 0));
    unsigned screenWarnBgColor = _settings->get<unsigned>("screen_warn_bgcolor", (255 << 24) | (95 << 16) | (169 << 8) | (132 << 0));
    unsigned screenTextFontSize = _settings->get<unsigned>("screen_text_fontsize", 18, {8, 36});
    unsigned screenTextPosition = _settings->get<unsigned>("screen_text_position", 0);
    unsigned screenTextPaddingHorizontal = _settings->get<unsigned>("screen_text_padding_horizontal", 10, {0, 60});
    unsigned screenTextPaddingVertical = _settings->get<unsigned>("screen_text_padding_vertical", 8, {0, 30});
    unsigned screenTextMarginHorizontal = _settings->get<unsigned>("screen_text_margin_horizontal", 10, {0, 100});
    unsigned screenTextMarginVertical = _settings->get<unsigned>("screen_text_margin_vertical", 12, {0, 100});
    bool paddingSeparate = _settings->get<bool>("screen_text_padding_separate", true);
    bool marginSeparate = _settings->get<bool>("screen_text_margin_separate", false);

    std::string screenTextFontPath = "";
    unsigned fIndex = 0;

    if (!keepFontPath) {
        std::string _fontFile = _settings->get<std::string>("screen_text_font", "");
        fIndex = _settings->get<unsigned>("screen_text_findex", 0);
        bool found = false;

        if (!_fontFile.empty()) {
            screenTextFontPath = fontFolder() + _fontFile;
            GUIKIT::File file(screenTextFontPath);
            found = file.exists();
            file.unload();

            if (!found) {
                screenTextFontPath = generatedFolder("fonts") + _fontFile;
                file.setFile(screenTextFontPath);
                found = file.exists();
                file.unload();
            }
        }

        if (!found) {
            fIndex = 0;
            if (systemFont == "inv") // one time only
                systemFont = GUIKIT::Font::systemFontFile();

            if (!systemFont.empty()) {
                screenTextFontPath = systemFont;
                GUIKIT::File file(screenTextFontPath);
                found = file.exists();
                file.unload();
            }

            if (!found) {
                screenTextFontPath = fontFolder() + "/SourceCodePro-Regular.ttf";
            }
        }
    }

    DRIVER::ScreenTextDescription desc;
    desc.position = static_cast<DRIVER::ScreenTextDescription::Position>(screenTextPosition);
    desc.fontSize = screenTextFontSize;
    desc.fontColor = screenTextColor;
    desc.backgroundColor = screenTextBgColor;
    desc.warnColor = screenWarnColor;
    desc.warnBackgroundColor = screenWarnBgColor;
    desc.fontPath = screenTextFontPath;
    desc.fontIndex = fIndex;
    desc.paddingHorizontal = screenTextPaddingHorizontal;
    desc.paddingVertical = paddingSeparate ? (int)screenTextPaddingVertical : -1;
    desc.marginHorizontal = screenTextMarginHorizontal;
    desc.marginVertical = marginSeparate ? (int)screenTextMarginVertical : -1;

    videoDriver->setScreenTextDescription(desc);
}

auto Program::updateHDR() -> void {
    auto _emu = activeEmulator ? activeEmulator : getLastUsedEmu();
    auto _settings = getSettings(_emu);

    bool enable = _settings->get<bool>("hdr_enable", false);
    bool gamut = _settings->get<bool>("hdr_gamut", true);
    unsigned maxNits = _settings->get<unsigned>("hdr_nits", 1000, {0, 10000});
    unsigned pwNits = _settings->get<unsigned>("hdr_pw_nits", 200, { 0, 2000 });
    float contrast = _settings->get<float>("hdr_contrast", 5.0, {0.0f, 10.0f});

    videoDriver->setHDR( enable, maxNits, pwNits, contrast, gamut );
}

auto Program::updateBFI() -> void {
    auto _emu = activeEmulator ? activeEmulator : getLastUsedEmu();
    auto _settings = getSettings(_emu);

    unsigned bfiFrames = _settings->get<unsigned>("bfi_frames", 0, { 0, 6 });
    unsigned darkFrames = _settings->get<unsigned>("dark_frames", 0, { 0, 6 });

    videoDriver->setBFI(bfiFrames, darkFrames);
}
