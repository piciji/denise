
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
    setVideoFilter();
    
    if ( !videoDriver->init( view->getViewportHandle() ) ) {
        delete videoDriver;
        videoDriver = new DRIVER::Video;
    }

    if (activeVideoManager)
        activeVideoManager->reinitThread(true);    
        
    // opengl crt shader only at the moment
    for( auto emuConfigView : emuConfigViews )
        emuConfigView->videoLayout->updateVisibillity();
        
    for( auto emulator : emulators )        
        VideoManager::getInstance( emulator )->reloadSettings();
}

auto Program::getVideoDriver() -> std::string {
	auto curDriver = settings->get<std::string>("video_driver", "");
	auto drivers = DRIVER::Video::available();
	
	for(auto& driver : drivers) {
		if(curDriver == driver) return driver;
	}
	return DRIVER::Video::preferred();
}

auto Program::finishVBlank() -> void {
    
    activeVideoManager->waitForRenderer();

    videoDriver->unlock();
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

auto Program::blackScreen() -> void {
    unsigned gpu_pitch;
    unsigned* gpu_data = 0;
    unsigned _w, _h;

    if (videoDriver->lock(gpu_data, gpu_pitch, 512, 512)) {

        for (_h = 0; _h < 512; _h++) {
            for (_w = 0; _w < 512; _w++) {
                *gpu_data++ = 0;
            }
            gpu_data += gpu_pitch - 512;
        }

        videoDriver->unlock();
        videoDriver->redraw();
    }
}

auto Program::setVideoSynchronize() -> void {
    videoDriver->synchronize( settings->get<bool>("video_sync", false) );
}

auto Program::setVideoHardSync() -> void {
    videoDriver->hardSync( settings->get<bool>("gl_hardsync", false) );
}

auto Program::hintExclusiveFullscreen() -> void {
	videoDriver->hintExclusiveFullscreen( settings->get("exclusive_fullscreen", false) );
}

auto Program::setVideoFilter() -> void {
    videoDriver->setFilter( (DRIVER::Video::Filter)settings->get<unsigned>("video_filter", 1u, {0u, 1u}) );
	
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
	
	auto left = settings->get<unsigned>(ident(emulator, "crop_left"), 0, {0u,100u});
	auto right = settings->get<unsigned>(ident(emulator, "crop_right"), 0, {0u,100u});
	auto top = settings->get<unsigned>(ident(emulator, "crop_top"), 0, {0u,100u});
	auto bottom = settings->get<unsigned>(ident(emulator, "crop_bottom"), 0, {0u,100u});
	
	auto type = settings->get<unsigned>(ident(emulator, "crop_type"), 0, {0u,4u});
	auto aspectCorrect = settings->get<bool>(ident(emulator, "crop_aspect_correct"), 0);
	
	emulator->crop( (Emulator::Interface::CropType) type, aspectCorrect, left, right, top, bottom );
    
    if (activeVideoManager) {
        activeVideoManager->reinitThread();
        activeVideoManager->shader.recreate = true;        
    }
}

auto Program::fastForward( bool activate, bool aggressive ) -> void {
    if (!activeEmulator)
        return;
    
    unsigned forward = 0;
    auto vSync = settings->get<bool>("video_sync", false);
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)settings->get<unsigned>(program->ident(activeEmulator, "video_crt"), (unsigned)VideoManager::CrtMode::None, {0u, 2u});

    if (activate) {                        
        if (vSync)
            view->videoSyncItem.toggle();

        settings->set<bool>("video_sync_temp", vSync, false); // remember vsync

        if (crtMode != VideoManager::CrtMode::None)
            EmuConfigView::TabWindow::getView(activeEmulator)->videoLayout->base.mode.crtNone.activate();

        settings->set<unsigned>("video_crt_temp", (unsigned)crtMode, false); // remember crt mode

        forward = (unsigned)Emulator::Interface::FastForward::NoAudioOut | (unsigned)Emulator::Interface::FastForward::ReduceVideoOutput;
        if (aggressive)
            forward |= (unsigned)Emulator::Interface::FastForward::NoVideoSequencer;

        if (aggressive)
            settings->set<bool>("fast_forward_aggressive", activate, false);
        else
            settings->set<bool>("fast_forward", activate, false);

    } else {
        auto vSyncTemp = settings->get<bool>("video_sync_temp", false);
        VideoManager::CrtMode crtModeTemp = (VideoManager::CrtMode)settings->get<unsigned>("video_crt_temp", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
        
        if (vSyncTemp && !vSync)
            view->videoSyncItem.toggle();

        if (crtMode == VideoManager::CrtMode::None) {
            if (crtModeTemp == VideoManager::CrtMode::Cpu)
                EmuConfigView::TabWindow::getView(activeEmulator)->videoLayout->base.mode.crtCpu.activate();
            else if (crtModeTemp == VideoManager::CrtMode::Gpu)
                EmuConfigView::TabWindow::getView(activeEmulator)->videoLayout->base.mode.crtGpu.activate();
        }
        
        settings->set<bool>("video_sync_temp", false, false);
        settings->set<unsigned>("video_crt_temp", (unsigned)VideoManager::CrtMode::None, false);
        settings->set<bool>("fast_forward_aggressive", false, false);
        settings->set<bool>("fast_forward", false, false);
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
