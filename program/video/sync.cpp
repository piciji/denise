
#include "manager.h"
#include "../audio/manager.h"

#define VIDEO_SKEW 0.0015

auto VideoManager::setSynchronize() -> void {
    bool vsync = globalSettings->get<bool>("video_sync", true);
    bool threadedRenderer = globalSettings->get("threaded_renderer", false);
    bool adaptive = globalSettings->get<bool>("adaptive_sync", true);
    unsigned frameRenderEach = 1;
    float skew = 0.0;

    if (!activeEmulator)
        return;

    if (audioDriver->hasSynchronized()) {
        if (vsync && adaptive) {
            float monitorFrequency = GUIKIT::Monitor::getCurrentRefreshRate();

            if (!threadedRenderer) {
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
        if (adaptive)
            threadedRenderer = true;
        else
            vsync = false;
    }

    if (videoDriver->hasThreaded() != threadedRenderer)
        videoDriver->setThreaded( threadedRenderer );

    if (videoDriver->hasSynchronized() != vsync)
        videoDriver->synchronize( vsync );

    setFrameRender( frameRenderEach );

    if (audioManager) {
        audioManager->allowDrc = vsync && !threadedRenderer && (frameRenderEach == 1) && (skew <= VIDEO_SKEW);
        audioManager->setRateControl();
    }
    program->updateOverallSynchronize();
}

auto VideoManager::setHardSync() -> void {
    videoDriver->hardSync( globalSettings->get<bool>("gl_hardsync", false) );
}

#undef VIDEO_SKEW
