
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
    enabled = false;
    updateFastForward = -1;
}

EmuThread::~EmuThread() {
    enable(false);
}

auto EmuThread::enable(bool state) -> void {

    if (state == enabled)
        return;

    enabled = state;

    if (state) {
        while (kill) {
            std::this_thread::yield();
        }
        initWorker();
    } else {
        kill = true;
        while (kill) {
            std::this_thread::yield();
        }
    }
}

auto EmuThread::lock(bool freeDriverContext) -> bool {
    //if (freeDriverContext)
        freeContext = true;

    if  (!enabled || acknowledged /* check for nesting */ )
        return false;

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

        //if (GUIKIT::ThreadPriority::setPriority( GUIKIT::ThreadPriority::Mode::Realtime, 3.0, 5.0 )) {
          //   logger->log("increased render thread prio");
        //}

        kill = false;
        attention = false;
        acknowledged = false;
        freeContext = false;

        while (1) {

            if (kill) {
                kill = false;
                videoDriver->freeContext();
                return;
            }

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

    if (updateFastForward != -1) {
        program->fastForward( updateFastForward, program->warp.aggressive );
        updateFastForward = -1;
    }

    if (pollHotkeys) {
        pollHotkeys = false;
        InputManager::pollHotkeys();
    }
}

auto EmuThread::clearEvents() -> void {
    statusUpdates.clear();
    finishAudioRecord = false;
    updateFastForward = -1;
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