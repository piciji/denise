
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
	    
    if ( !videoDriver->init( view->getViewportHandle() ) ) {
        delete videoDriver;
        videoDriver = new DRIVER::Video;
    }
	
	videoDriver->setFilter( DRIVER::Video::Filter::Linear );

    if (activeVideoManager)
        activeVideoManager->reinitThread(true);    
        
    // opengl crt shader only at the moment
    for( auto emuConfigView : emuConfigViews )
        emuConfigView->videoLayout->updateVisibillity();
        
    for( auto emulator : emulators )        
        VideoManager::getInstance( emulator )->reloadSettings();
	
	VideoManager::setShaderInputPrecision( globalSettings->get<bool>("shader_input_precision", false) );
	VideoManager::setThreaded( globalSettings->get<bool>("crt_threaded", true) );
	
	if (!cmd->debug) {
		loadPlaceholder();
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
    
    status->countFrames();
	
    if (frame)
        activeVideoManager->renderFrame(frame, width, height, linePitch);
}

auto Program::loadPlaceholder() -> void {

	GUIKIT::File file( imgFolder() + "startscreen.png" );
	
	if (!file.open())
		return;
	
	uint8_t* data = file.read();
	
	if (!data)
		return;	
	
	if (!placeholder.loadPng( data, file.getSize() ))
		return;			
}

auto Program::renderPlaceholder(bool blackScreen) -> void {
		
	if (cmd->debug || isRunning)
		return;
	
	unsigned gpu_pitch;
    unsigned* gpu_data = 0;
    unsigned _w, _h;
	uint8_t* data = placeholder.data;	
	
	if (!blackScreen && !placeholder.empty()) {
		if (videoDriver->lock(gpu_data, gpu_pitch, placeholder.width, placeholder.height)) {

			for (_h = 0; _h < placeholder.height; _h++) {
				for (_w = 0; _w < placeholder.width; _w++) {
					*gpu_data++ = data[0] << 16 | data[1] << 8 | data[2];
					data += 4;
				}
				gpu_data += gpu_pitch - (placeholder.width );
			}
		}
	} else { // blackscreen
		if (videoDriver->lock(gpu_data, gpu_pitch, 256, 256)) {

			for (_h = 0; _h < 256; _h++) {
				for (_w = 0; _w < 256; _w++) {
					*gpu_data++ = 0;
				}
				gpu_data += gpu_pitch - 256;
			}
		}
	}
	
	videoDriver->unlock();
	videoDriver->redraw(true);
}

auto Program::setVideoSynchronize() -> void {
    videoDriver->synchronize( globalSettings->get<bool>("video_sync", false) );
}

auto Program::setVideoHardSync() -> void {
    videoDriver->hardSync( globalSettings->get<bool>("gl_hardsync", false) );
}

auto Program::hintExclusiveFullscreen() -> void {
	videoDriver->hintExclusiveFullscreen( globalSettings->get("exclusive_fullscreen", false) );
}

auto Program::setFpsLimit() -> void {
	VideoManager::setFpsLimit( globalSettings->get("fps_limit", false) );
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

auto Program::fastForward( bool activate, bool aggressive ) -> void {
    if (!activeEmulator)
        return;
    
    auto settings = getSettings( activeEmulator );
    
    unsigned forward = 0;
    auto vSync = globalSettings->get<bool>("video_sync", false);
    auto fpsLimit = globalSettings->get("fps_limit", false);
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});

    if (activate) {                        
        if (vSync)
            view->videoSyncItem.toggle();

        if (fpsLimit)
            view->fpsLimitItem.toggle();
        
        globalSettings->set<bool>("video_sync_temp", vSync, false); // remember vsync
        globalSettings->set<bool>("fps_limit_temp", fpsLimit, false); // remember fps limit

        if (crtMode != VideoManager::CrtMode::None)
            EmuConfigView::TabWindow::getView(activeEmulator)->videoLayout->base.mode.crtNone.activate();

        globalSettings->set<unsigned>("video_crt_temp", (unsigned)crtMode, false); // remember crt mode

        forward = (unsigned)Emulator::Interface::FastForward::NoAudioOut | (unsigned)Emulator::Interface::FastForward::ReduceVideoOutput;
        if (aggressive)
            forward |= (unsigned)Emulator::Interface::FastForward::NoVideoSequencer;

        if (aggressive)
            globalSettings->set<bool>("fast_forward_aggressive", activate, false);
        else
            globalSettings->set<bool>("fast_forward", activate, false);

    } else {
        auto vSyncTemp = globalSettings->get<bool>("video_sync_temp", false);
        auto fpsLimitTemp = globalSettings->get<bool>("fps_limit_temp", false);
        VideoManager::CrtMode crtModeTemp = (VideoManager::CrtMode)globalSettings->get<unsigned>("video_crt_temp", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
        
        if (vSyncTemp && !vSync)
            view->videoSyncItem.toggle();
        
        if (fpsLimitTemp && !fpsLimit)
            view->fpsLimitItem.toggle();

        if (crtMode == VideoManager::CrtMode::None) {
            if (crtModeTemp == VideoManager::CrtMode::Cpu)
                EmuConfigView::TabWindow::getView(activeEmulator)->videoLayout->base.mode.crtCpu.activate();
            else if (crtModeTemp == VideoManager::CrtMode::Gpu)
                EmuConfigView::TabWindow::getView(activeEmulator)->videoLayout->base.mode.crtGpu.activate();
        }
        
        globalSettings->set<bool>("video_sync_temp", false, false);
        globalSettings->set<bool>("fps_limit_temp", false, false);
        globalSettings->set<unsigned>("video_crt_temp", (unsigned)VideoManager::CrtMode::None, false);
        globalSettings->set<bool>("fast_forward_aggressive", false, false);
        globalSettings->set<bool>("fast_forward", false, false);
    }

    activeEmulator->fastForward( forward );
}

auto Program::saveExitScreenshot() -> void {        
       
    if (!activeEmulator)
        return;
    
    auto pitch = activeEmulator->cropPitch();
    auto data = activeEmulator->cropData();
    auto width = activeEmulator->cropWidth();
    auto height = activeEmulator->cropHeight();    
    
    if (!data)
        return;
   
    std::string palIdent = "Pepto PAL";
    uint32_t* colorTable = nullptr;
    
    for(auto& palette : activeEmulator->palettes) {
        
        if (palette.name == palIdent) {
            colorTable = new uint32_t[ palette.paletteColors.size() ];
            unsigned i = 0;
            
            for(auto& col : palette.paletteColors)
                colorTable[i++] = col.rgb;
            
            break;
        }
    }
    
    if (!colorTable)
        return;
    
    uint8_t* screen = new uint8_t[width * height * 3];
    uint8_t* ptr = screen;
    uint32_t color;
    
	for(unsigned h = 0; h < height; h++) {
		for(unsigned w = 0; w < width; w++) {
            
            color = colorTable[*data++ & 0xf];
            
            *ptr++ = (color >> 16) & 0xff;
            *ptr++ = (color >> 8) & 0xff;
            *ptr++ = (color >> 0) & 0xff;
        }			

		data += pitch;		
	}
    
    unsigned pngSize = 0;
    GUIKIT::Image png;
    uint8_t* pngData = png.generatePng( screen, width, height, pngSize );
    
    GUIKIT::File file;
    file.setFile( cmd->screenshotPath );
    file.open(GUIKIT::File::Mode::Write);
    file.write( pngData, pngSize );
    
    delete[] screen;
    delete[] pngData;
    delete[] colorTable;
}
