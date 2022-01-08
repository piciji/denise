
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

    std::atomic<bool> ready;
    std::atomic<bool> kill;
    std::atomic<bool> freeContext;

    std::mutex statusMutex;

    auto lock() -> bool;
    auto unlock() -> void;

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
};

extern EmuThread* emuThread;