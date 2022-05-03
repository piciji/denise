
#include "manager.h"
#include "../audio/manager.h"

#define VIDEO_SKEW 0.0015

auto VideoManager::setSynchronize() -> void {
    bool vsync = globalSettings->get<bool>("video_sync", true);
    bool threadedRenderer = globalSettings->get("threaded_renderer", true);
    bool adaptive = globalSettings->get<bool>("adaptive_sync", true);
    bool vrr = globalSettings->get<bool>("vrr_sync", false);

    unsigned frameRenderEach = 1;
    float skew = 0.0;

    if (!activeEmulator)
        return;

    if (audioDriver->hasSynchronized()) {
        if (vsync && adaptive) {
            if (!threadedRenderer) {
                float monitorFrequency = GUIKIT::Monitor::getCurrentRefreshRate();
                vrr = false;
                float ratio = (float)audioManager->inputFPS / monitorFrequency;
                float intpart;
                float fractpart = std::modf (ratio, &intpart);

                if ((uint8_t)intpart <= 1) {
                    skew = std::abs(1.0 - ratio );
                    if ((skew > VIDEO_SKEW) /*&& ((float) audioManager->inputFPS > monitorFrequency)*/)
                        threadedRenderer = true;
                } else {
                    if (fractpart > VIDEO_SKEW)
                        threadedRenderer = true;
                    else
                        frameRenderEach = (uint8_t)intpart;
                }
            }
        }
    } else if (!threadedRenderer) {
        vrr = false;

        if (adaptive)
            threadedRenderer = true;
        else
            vsync = false;
    }

	if (activeVideoManager) {
		activeVideoManager->waitForCrtRenderer();
		activeVideoManager->reinitCrtThread();
	}
		
    if (videoDriver->hasThreaded() != threadedRenderer)
        videoDriver->setThreaded( threadedRenderer );

    if (videoDriver->hasSynchronized() != vsync)
        videoDriver->synchronize( vsync );

    setFrameRender( frameRenderEach );

    if (audioManager) {
        videoDriver->setVRR( vrr, (float)audioManager->inputFPS );
        audioManager->allowDrc = (vsync || vrr) && !threadedRenderer && (frameRenderEach == 1) && (skew <= VIDEO_SKEW);
        audioManager->setRateControl();
    }
    program->updateOverallSynchronize();
}

auto VideoManager::setHardSync() -> void {
    videoDriver->hardSync( globalSettings->get<bool>("gl_hardsync", true) );
}

#undef VIDEO_SKEW
