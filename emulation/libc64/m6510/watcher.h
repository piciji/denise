
#pragma once

#include <cstdint>
#include <vector>

namespace LIBC64 {

class M6510;

struct Watcher {
    uint16_t addr;
    bool enabled;
};

struct WatchPoints {
    M6510& cpu;
    std::vector<Watcher> watchers;
    int controlFlag;

    WatchPoints(M6510& cpu, int controlFlag);

    virtual ~WatchPoints() = default;

    auto add(uint16_t addr) -> void;

    auto remove(uint16_t addr) -> void;

    auto check(uint16_t addr) -> bool;

    auto find(uint16_t addr) -> Watcher*;

    auto isEnabled(uint16_t addr) -> bool;

    auto isDisabled(uint16_t addr) -> bool;

    auto enable(uint16_t addr) -> void;

    auto disable(uint16_t addr) -> void;

    auto disableAll() -> void;

    auto flagWhenNeeded() -> void;

    auto flag(bool enable) -> void;
};

struct ModifiedCodes {
    M6510& cpu;
    int controlFlag;

    ModifiedCodes(M6510& cpu, int controlFlag);

    uint16_t addrFrom;
    uint16_t addrTo;
    bool alarm;

    auto add(uint16_t addr, uint16_t addrTo) -> void;
    auto checkAndSet(uint16_t addr) -> void;
    auto getAndForget() -> bool;
    auto disable() -> void;
};

struct HistoryEntry {
    uint16_t addr;
    uint8_t mem[3] = {0}; // we should remember the actual values, because of potentially modified code.
    uint8_t flags;
};

struct HistoryHandler {
    M6510& cpu;
    int controlFlag;

    HistoryHandler(M6510& cpu, int controlFlag);

    bool _enable = false;
    bool _overflow = false;
    uint16_t pos;
    std::vector<HistoryEntry> traces;

    auto add() -> void;
    auto get(unsigned i) -> HistoryEntry*;
    auto flagWhenNeeded() -> void;
    auto enable() -> void;
    auto disable() -> void;
};

}
