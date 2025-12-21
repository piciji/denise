
#pragma once

#include <cstdint>
#include <vector>
#include <functional>

namespace Emulator {

using WatcherCallback = std::function<void ( bool state )>;

struct Watcher {
    uint32_t addr;
    bool enabled;
};

struct WatchPoints {
    std::vector<Watcher> watchers;
    WatcherCallback callback;

    WatchPoints();

    virtual ~WatchPoints() = default;

    auto add(uint32_t addr) -> void;

    auto remove(uint32_t addr) -> void;

    auto check(uint32_t addr) -> bool;

    auto find(uint32_t addr) -> Watcher*;

    auto isEnabled(uint32_t addr) -> bool;

    auto isDisabled(uint32_t addr) -> bool;

    auto enable(uint32_t addr) -> void;

    auto disable(uint32_t addr) -> void;

    auto disableAll() -> void;

    auto flagWhenNeeded() -> void;
};

struct ModifiedCodes {
    WatcherCallback callback;

    ModifiedCodes();

    uint32_t addrFrom;
    uint32_t addrTo;
    bool alarm;

    auto add(uint32_t addr, uint32_t addrTo) -> void;
    auto checkAndSet(uint32_t addr) -> void;
    auto getAndForget() -> bool;
    auto disable() -> void;
};

struct HistoryEntry {
    uint32_t addr;
    uint8_t mem[4] = {0}; // we should remember the actual values, because of potentially modified code.
    uint8_t flags;
};

struct HistoryHandler {
    WatcherCallback callback;

    HistoryHandler();

    bool _enable = false;
    bool _overflow = false;
    uint16_t pos;
    std::vector<HistoryEntry> traces;

    auto getNext() -> HistoryEntry&;
    auto get(unsigned i) -> HistoryEntry*;
    auto flagWhenNeeded() -> void;
    auto enable() -> void;
    auto disable() -> void;
};

}
