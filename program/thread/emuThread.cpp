
#include "../program.h"
#include "../tools/chronos.h"
#include "../view/view.h"
#include "emuThread.h"

EmuThread* emuThread = nullptr;

EmuThread::EmuThread() {
    kill = false;
    attention = false;
    acknowledged = false;
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

auto EmuThread::lock() -> bool {
    if  (!enabled || acknowledged)
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

  //      if (GUIKIT::ThreadPriority::setPriority( GUIKIT::ThreadPriority::Mode::High, 3.0, 5.0 )) {
        //     logger->log("increased render thread prio");
   //     }

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
