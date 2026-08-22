
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
        activeVideoManager->rebuildShader = true;
    }
        
    for( auto emulator : emulators ) {
        checkShaderSupport(emulator);
        auto videoManager =  VideoManager::getInstance(emulator);

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
            std::string absPath = FileHelper::generatedFolder(subPath, FileHelper::FLAG_CREATE) + cacheFile;
            GUIKIT::File f(absPath, true);

            if (f.open(GUIKIT::File::Mode::Write))
                f.write(diskFile.data, diskFile.size);
        } else {
            std::string absPath = FileHelper::generatedFolder(subPath, 0) + cacheFile;
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

    videoDriver->useShaderCache(true );

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

    auto settings = Program::getSettings( activeEmulator );

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
        auto settings = Program::getSettings(emulator);
        settings->set<unsigned>("video_crt", state ? (unsigned)VideoManager::CrtMode::Gpu : (unsigned)VideoManager::CrtMode::None);
        vManager->reloadSettings(true);
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
        activeVideoManager->renderFrame<uint8_t, 0x10>(cropData, activeEmulator->cropWidth(), activeEmulator->cropHeight(), activeEmulator->cropPitch());
    } else {
        auto cropData16 = activeEmulator->cropData16();

        if (cropData16) {
            unsigned _width = activeEmulator->cropWidth();
            unsigned _height = activeEmulator->cropHeight();
            unsigned _pitch = activeEmulator->cropPitch();
            unsigned _options = activeEmulator->cropOptions();

            switch(_options) {
                case 0: activeVideoManager->renderFrame<uint16_t, 0x10 | 0>(cropData16, _width, _height, _pitch); break;
                case 1: activeVideoManager->renderFrame<uint16_t, 0x10 | 1>(cropData16, _width, _height, _pitch); break;
                case 2: activeVideoManager->renderFrame<uint16_t, 0x10 | 2>(cropData16, _width, _height, _pitch); break;

                case 4: activeVideoManager->renderFrame<uint16_t, 0x10 | 4>(cropData16, _width, _height, _pitch); break;
                case 5: activeVideoManager->renderFrame<uint16_t, 0x10 | 5>(cropData16, _width, _height, _pitch); break;
                case 6: activeVideoManager->renderFrame<uint16_t, 0x10 | 6>(cropData16, _width, _height, _pitch); break;

                case 8: activeVideoManager->renderFrame<uint16_t, 0x10 | 8>(cropData16, _width, _height, _pitch); break;
                case 9: activeVideoManager->renderFrame<uint16_t, 0x10 | 9>(cropData16, _width, _height, _pitch); break;
                case 10: activeVideoManager->renderFrame<uint16_t, 0x10 | 10>(cropData16, _width, _height, _pitch); break;
            }
        }
    }
    if (VideoManager::takeScreenShots && view && isPause) {
        view->screenshot.pause = isPause;
        isPause = 0;
    }
}

auto Program::canExclusiveFullscreen() -> bool {
    // FSE fullscreen switch hangs sometimes, FSO replaces FSE and is more stable
    return false;

    bool result = videoDriver->canExclusiveFullscreen()
        && globalSettings->get<bool>("exclusive_fullscreen", false);

    return result;
}

auto Program::hintExclusiveFullscreen() -> void {

    if (canExclusiveFullscreen())
        videoDriver->hintExclusiveFullscreen( true, view->getCustomFullscreenRefreshRate() );
    else
        videoDriver->hintExclusiveFullscreen( false );
}

auto Program::setVideoFilter() -> void {
	if (activeEmulator)
		videoDriver->setLinearFilter( Program::getSettings( activeEmulator )->get<bool>("video_filter", true) );
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
        auto settings = Program::getSettings( emulator );

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
    auto settings = Program::getSettings( emulator );
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
    auto settings = Program::getSettings( emulator );

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
    auto settings = Program::getSettings( emulator );
    int type = settings->get<int>("crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 6u});
    Emulator::Interface::Crop crop = {0};
    getCrop(emulator, crop);

	emulator->cropFrame( (Emulator::Interface::CropType) type, crop );
}

auto Program::toggleWarp( Warp::Mode mode) -> void {
    if (warp.manuellEndsAutoWarp && warp.motorControlled)
        warp.autoMode = Warp::Off;

    if (mode == warp.mode)
        mode = Warp::Mode::Off;

    setWarp(mode, true);
}

auto Program::setWarp( Warp::Mode mode, bool manuell ) -> void {
    unsigned forward = 0;
    bool resetFrameCounter = !(warp.mode != Warp::Off && mode != Warp::Mode::Off);

    switch (mode) {
        case Warp::Mode::Off:
            warp.manuell = false;
            loopFrames = 0;
            if (warp.mode == Warp::Mode::FastForward) {
                warp.mode = Warp::Off;
                audioManager->setSynchronize();
                audioManager->setResampler();
                audioManager->setVolume();
            } else {
                warp.mode = Warp::Off;
                VideoManager::setSynchronize();
            }

            if (audioManager)
                audioManager->drive.reset();

            break;
        case Warp::Mode::Normal:
        case Warp::Mode::Aggressive: {
            warp.manuell = manuell;

            if (warp.mode == Warp::Mode::FastForward) {
                warp.mode = mode;
                audioManager->setSynchronize();
                audioManager->setResampler();
                audioManager->setVolume();
            } else {
                warp.mode = mode;
                VideoManager::setSynchronize();
            }

            forward = (unsigned)Emulator::Interface::WarpMode::NoAudioOut | (unsigned)Emulator::Interface::WarpMode::ReduceVideoOutputEach16th;
            if (mode == Warp::Mode::Aggressive)
                forward |= (unsigned)Emulator::Interface::WarpMode::NoVideoSequencer;
        } break;

        case Warp::FastForward: {
            warp.mode = mode;
            warp.manuell = manuell;
            audioManager->setSynchronize();
            audioManager->setResampler();
            audioManager->volumeAdjust = 0.0;
            if (warp.ff_each == 1) forward = (unsigned)Emulator::Interface::WarpMode::ReduceVideoOutputEach4th;
            else if (warp.ff_each == 2) forward = (unsigned)Emulator::Interface::WarpMode::ReduceVideoOutputEach8th;
            else if (warp.ff_each == 3) forward = (unsigned)Emulator::Interface::WarpMode::ReduceVideoOutputEach16th;
        } break;
    }

    if (statusHandler && resetFrameCounter)
        statusHandler->resetFrameCounter();

    activeEmulator->setWarpMode( forward );

    if (view)
        view->updateWarpCheck();
}

auto Program::updateFullscreenSetting() -> void {
    if (!activeEmulator)
        return;
    auto _settings = Program::getSettings(activeEmulator);

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
    auto _settings = Program::getSettings(activeEmulator);

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

    auto settings = Program::getSettings( emulator );
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

    auto _settings = Program::getSettings(activeEmulator);
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
                screenTextFontPath = FileHelper::generatedFolder("fonts") + _fontFile;
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
    auto _settings = Program::getSettings(_emu);

    bool enable = _settings->get<bool>("hdr_enable", false);
    bool gamut = _settings->get<bool>("hdr_gamut", true);
    unsigned maxNits = _settings->get<unsigned>("hdr_nits", 1000, {0, 10000});
    unsigned pwNits = _settings->get<unsigned>("hdr_pw_nits", 200, { 0, 2000 });
    float contrast = _settings->get<float>("hdr_contrast", 5.0, {0.0f, 10.0f});

    videoDriver->setHDR( enable, maxNits, pwNits, contrast, gamut );
}

auto Program::updateBFI() -> void {
    auto _emu = activeEmulator ? activeEmulator : getLastUsedEmu();
    auto _settings = Program::getSettings(_emu);

    unsigned bfiFrames = _settings->get<unsigned>("bfi_frames", 0, { 0, 6 });
    unsigned darkFrames = _settings->get<unsigned>("dark_frames", 0, { 0, 6 });
    bool strobeShader = _settings->get<bool>("strobe_shader", false);

    videoDriver->setBFI(bfiFrames, strobeShader ? 0 : darkFrames);
}
