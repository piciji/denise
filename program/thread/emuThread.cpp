
#include "../program.h"
#include "../tools/chronos.h"
#include "../view/view.h"
#include "emuThread.h"

#include "../debugger/debugger.h"
#include "../emuconfig/config.h"
#include "../input/manager.h"
#include "../media/autoloader.h"
#include "../media/fileloader.h"
#include "../view/status.h"
#include "../emuconfig/layouts/audio.h"
#include "../monitor/binaryMonitor.h"

EmuThread* emuThread = nullptr;

EmuThread::EmuThread() {
    kill = false;
    attention = false;
    acknowledged = false;
    updateBorder = false;
    debugging = false;
    enabled = false;
    events = 0;
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

auto EmuThread::lockDebugger() -> void {
    debugging = true;
    while (debugging) {

        if (freeContext) {
            freeContext = false;
            videoDriver->freeContext();
        }

        if (attention && debugging) {
            attention = false;
            acknowledged = true;

            while(acknowledged) {
                std::this_thread::yield();
            }

            if (!debugging) {
                break;
            }
        }

        program->loopDebugging();
    }
}

auto EmuThread::unlockDebugger() -> void {
    if (debugging) {
        debugging = false;
        acknowledged = false;
        if (program->binaryMonitor.clientConnected())
            program->binaryMonitor.sendResume();
    }
}

auto EmuThread::lock(bool unlockDebugging) -> bool {
    if  (!enabled)
        return false;

    if (unlockDebugging) {
        Debugger::lock = true;
        unlockDebugger();
    }

    if (acknowledged /* check for nesting */ )
        return false;

    freeContext = true;
    attention = true;
    while(attention) {
        std::this_thread::yield();
    }

    return true;
}

auto EmuThread::unlock() -> void {
    Debugger::lock = false;
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

        while (true) {

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
#if defined(_WIN32) || defined(__APPLE__)
#else
                // linux hack to prevent slowdown when killing thread
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
#endif
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

    if (statusUpdates.empty()) {
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

    if (events) {
        unsigned _events = events;
        events = 0;

        if (_events & EVT_FINISH_AUDIO_RECORD) {
            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            if (emuView && emuView->audioLayout)
                emuView->audioLayout->stopRecord();
            if (view)
                view->setAudioRecordText();
        }

        if (_events & EVT_POLL_HOTKEYS)
            InputManager::pollHotkeys();

        if (_events & EVT_UPDATE_PALETTE_SOFTWARE) {
            auto emulator = program->getEmulator("C64");
            auto emuView = EmuConfigView::TabWindow::getView(emulator);
            auto vManager = VideoManager::getInstance(emulator);

            if (emuView && emuView->mediaLayout)
                emuView->mediaLayout->colorListing(vManager->getForegroundColor(), vManager->getBackgroundColor());
        }

        if (_events & EVT_SHADER_ERROR) {
            auto manager = VideoManager::getInstance(activeEmulator);
            if (manager)
                manager->finishPreset();
        }

        if(_events & EVT_UPDATE_FPS)
            program->fpsChangeTimer.setEnabled();

        if (_events & EVT_AUTO_LOAD_NO_TRAPS) {
            lock(true);
            fileloader->autoload(activeEmulator, autoloader->getLatestDrive(activeEmulator), 0, false, true);
            unlock();
        }

        if (_events & EVT_DEBUGGER) {
            if (program->hasActiveDebugger())
                Debugger::Callback();
        }

        if (_events & EVT_INSERT_MEDIA) {
            if (insertMedia.file && insertMedia.emulator) {
                auto emuView = EmuConfigView::TabWindow::getView(insertMedia.emulator);
                if (emuView && emuView->mediaLayout) {
                    auto items = insertMedia.file->scanArchive();
                    lock();
                    emuView->mediaLayout->insertImage(insertMedia.media, insertMedia.file, &items[0]);
                    unlock();
                }
            }
        }
    }
}

auto EmuThread::clearEvents() -> void {
    statusUpdates.clear();
    updateBorder = false;
    events = 0;
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
