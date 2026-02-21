
#pragma once
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

struct EmuThread {
    EmuThread();
    ~EmuThread();

    bool enabled = false;
    std::atomic<bool> attention;
    std::atomic<bool> acknowledged;
    std::atomic<bool> debugging;
    std::atomic<bool> updateBorder;

    enum {  EVT_AUTO_LOAD_NO_TRAPS = 4, EVT_DISMISS_PLACEHOLDER = 8,
            EVT_UPDATE_FPS = 0x10, EVT_SHADER_ERROR = 0x20, EVT_UPDATE_PALETTE_SOFTWARE = 0x40,
            EVT_POLL_HOTKEYS = 0x80, EVT_FINISH_AUDIO_RECORD = 0x100, EVT_DEBUGGER = 0x200 };

    std::atomic<unsigned> events;

    std::atomic<bool> ready;
    std::atomic<bool> kill;
    std::atomic<bool> freeContext;

    std::mutex statusMutex;
    std::mutex hotkeyMutex;
    std::mutex videoMutex;
    std::mutex paletteForSoftwareView;

    auto lock(bool unlockDebugging = false) -> bool;
    auto unlock() -> void;
    auto locked() -> bool { return attention || acknowledged; }

    auto lockDebugger() -> void;
    auto unlockDebugger() -> void;

    auto lockHotkeys() -> void;
    auto unlockHotkeys() -> void;
    auto lockStatus() -> void;
    auto unlockStatus() -> void;
    auto lockVideo() -> void;
    auto unlockVideo() -> void;
    auto lockPaletteForSoftwareView() -> void;
    auto unlockPaletteForSoftwareView() -> void;

    auto enable(bool state) -> void;

    auto initWorker() -> void;

    struct StatusUpdates {
        unsigned id;
        int visible;
        GUIKIT::Image* image;
        std::string text;
        bool alignRight;
        int overrideForegroundColor;
    };
    std::vector<StatusUpdates> statusUpdates;

    auto addStatusUpdate(unsigned id, int visible = -1, GUIKIT::Image* image = nullptr, std::string text = "", bool alignRight = false, int overrideForegroundColor = -1 ) -> void;
    auto handleStatusUpdate( ) -> void;
    auto handleUIEvents() -> void;
    auto clearEvents() -> void;
};

extern EmuThread* emuThread;