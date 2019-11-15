
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
        view->message->error("shader error");
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
    
    if (cmd->noDriver)
        return;
	
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
