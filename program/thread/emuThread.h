
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
    std::atomic<bool> finishAudioRecord;
    std::atomic<bool> pollHotkeys;
    std::atomic<bool> updatePaletteForSoftwareView;
    std::atomic<bool> updateBorder;
    std::atomic<bool> updateFps;
    std::atomic<bool> dismissPlaceholder;
    std::atomic<bool> presentShaderError;
    std::atomic<bool> disableTraps;

    std::atomic<bool> ready;
    std::atomic<bool> kill;
    std::atomic<bool> freeContext;

    std::mutex statusMutex;
    std::mutex hotkeyMutex;
    std::mutex videoMutex;
    std::mutex paletteForSoftwareView;

    auto lock() -> bool;
    auto unlock() -> void;
    auto locked() -> bool { return attention || acknowledged; }

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