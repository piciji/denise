
#include "manager.h"
#include "../audio/manager.h"

#define VIDEO_SKEW 0.0015

auto VideoManager::setSynchronize() -> void {
    if (!activeEmulator)
        return;

    bool vsync = false;
    bool vrr = false;

    if (program->warp.mode == Program::Warp::Off) {
        vsync = globalSettings->get<bool>("video_sync", true);
        vrr = globalSettings->get<bool>("vrr_sync", false);
    }

    auto _settings = Program::getSettings( activeEmulator );
    unsigned tr = _settings->get<unsigned>("threaded_renderer", 0);

    bool threadedRenderer = tr == 1;
    bool adaptive = tr == 2;

    unsigned frameRenderEach = 1;
    float skew = 0.0;

    if (audioDriver->hasSynchronized()) {
        if (vsync && adaptive) {
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
    } else if (!threadedRenderer) {
        vrr = false;

        if (adaptive)
            threadedRenderer = true;
        else
            vsync = false;
    }

    if (program->warp.mode == Program::Warp::Off) {
        // don't change when toggling warp mode
        if (videoDriver->hasThreaded() != threadedRenderer)
            videoDriver->setThreaded( threadedRenderer );
    }

    if (videoDriver->hasSynchronized() != vsync)
        videoDriver->synchronize( vsync );

    setFrameRender( frameRenderEach );

    if (audioManager) {
        videoDriver->setVRR( vrr, (float)audioManager->inputFPS );
        audioManager->allowDrc = (vsync || vrr) && !threadedRenderer && (frameRenderEach == 1) && (skew <= VIDEO_SKEW);
        audioManager->setRateControl();
    }
}

auto VideoManager::setHardSync() -> void {
    videoDriver->hardSync( globalSettings->get<bool>("hardsync", false) );
}

#undef VIDEO_SKEW
