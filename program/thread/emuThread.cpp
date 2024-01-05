
#include "../program.h"
#include "../tools/chronos.h"
#include "../view/view.h"
#include "emuThread.h"
#include "../emuconfig/config.h"
#include "../input/manager.h"

EmuThread* emuThread = nullptr;

EmuThread::EmuThread() {
    kill = false;
    attention = false;
    acknowledged = false;
    finishAudioRecord = false;
    pollHotkeys = false;
    updateBorder = false;
    updateFps = false;
    dismissPlaceholder = false;
    enabled = false;
}

EmuThread::~EmuThread() {
    enable(false);
}

auto EmuThread::enable(bool state) -> void {

    if (state == enabled)
        return;  

    if (state) {
        while (kill) {
            std::this_thread::yield();
        }
        initWorker();
    } else {
        kill = true;
        unlock();
        while (kill) {
            std::this_thread::yield();
        }
    }
	
	enabled = state;
}

auto EmuThread::lock() -> bool {

    if  (!enabled || acknowledged /* check for nesting */ )
        return false;

    freeContext = true;
    attention = true;
    while(attention) {
        std::this_thread::yield();
    }

    return true;
}

auto EmuThread::unlock() -> void {
    acknowledged = false;
}

auto EmuThread::initWorker() -> void {

    std::thread worker([this] {
#ifdef __APPLE__
        if (GUIKIT::ThreadPriority::setPriority( GUIKIT::ThreadPriority::Mode::High, 3.0, 5.0 )) {
          // logger->log("increased emu thread prio");
        }
#endif
        kill = false;
        attention = false;
        acknowledged = false;
        freeContext = false;

        while (1) {

            if (freeContext) {
                freeContext = false;
                videoDriver->freeContext();
            }

            if (attention) {
                attention = false;
                acknowledged = true;

                while(acknowledged) {
                    std::this_thread::yield();
                }
            }

            if (kill) {
                videoDriver->freeContext();
                kill = false;
                return;
            }

            if (updateBorder && activeEmulator) {
                videoMutex.lock();
                program->updateCrop(activeEmulator);
                updateBorder = false;
                videoMutex.unlock();
            }

            program->loop();

        }
    });

    worker.detach();
}

auto EmuThread::addStatusUpdate(unsigned id, int visible, GUIKIT::Image* image, std::string text, bool alignRight, int overrideForegroundColor  ) -> void {

    statusUpdates.push_back({id, visible, image, text, alignRight, overrideForegroundColor});
}

auto EmuThread::handleStatusUpdate( ) -> void {

    statusMutex.lock();

    if (!statusUpdates.size()) {
        statusMutex.unlock();
        return;
    }

    auto copy = statusUpdates;

    statusUpdates.clear();

    statusMutex.unlock();

    auto& statusBar = view->statusBar;

    for(auto& sU : copy) {
        if (sU.visible != -1) {
            statusBar.updateVisible(sU.id, (bool)sU.visible);
        } else if (sU.image) {
            statusBar.updateImage(sU.id, sU.image);
        } else
            statusBar.updateText(sU.id, sU.text, sU.alignRight, sU.overrideForegroundColor);
    }

    statusBar.update();
}

auto EmuThread::handleUIEvents() -> void {

    if (finishAudioRecord) {
        finishAudioRecord = false;
        auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
        if (emuView && emuView->audioLayout)
            emuView->audioLayout->stopRecord();
    }

    if (pollHotkeys) {
        pollHotkeys = false;
        InputManager::pollHotkeys();
    }

    if (updatePaletteForSoftwareView) {
        updatePaletteForSoftwareView = false;
        auto emulator = program->getEmulator("C64");
        auto emuView = EmuConfigView::TabWindow::getView( emulator );
        auto vManager = VideoManager::getInstance( emulator );

        if (emuView && emuView->mediaLayout)
            emuView->mediaLayout->colorListing( vManager->getForegroundColor(), vManager->getBackgroundColor() );
    }

    if (updateFps) {
        program->fpsChangeTimer.setEnabled();
        updateFps = false;
    }

    if (dismissPlaceholder) {
        program->setVideoFilter();
        view->setDefaultCursor();
        dismissPlaceholder = false;
    }
}

auto EmuThread::clearEvents() -> void {
    statusUpdates.clear();
    finishAudioRecord = false;
    updatePaletteForSoftwareView = false;
    updateBorder = false;
    pollHotkeys = false;
}

auto EmuThread::lockHotkeys() -> void {
    if (enabled)
        hotkeyMutex.lock();
}

auto EmuThread::unlockHotkeys() -> void {
    if (enabled)
        hotkeyMutex.unlock();
}

auto EmuThread::lockStatus() -> void {
    if (enabled)
        statusMutex.lock();
}

auto EmuThread::unlockStatus() -> void {
    if (enabled)
        statusMutex.unlock();
}

auto EmuThread::lockVideo() -> void {
    if (enabled)
        videoMutex.lock();
}

auto EmuThread::unlockVideo() -> void {
    if (enabled)
        videoMutex.unlock();
}

auto EmuThread::lockPaletteForSoftwareView() -> void {
    if (enabled)
        paletteForSoftwareView.lock();
}

auto EmuThread::unlockPaletteForSoftwareView() -> void {
    if (enabled)
        paletteForSoftwareView.unlock();
}
