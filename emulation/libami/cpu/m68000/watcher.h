
#pragma once

#include <cstdint>
#include <vector>

namespace M68FAMILY {

class M68000;

struct Watcher {
    uint32_t addr;
    bool enabled;
};

struct WatchPoints {
    M68000& cpu;
    std::vector<Watcher> watchers;
    int controlFlag;

    WatchPoints(M68000& cpu, int controlFlag);

    virtual ~WatchPoints() = default;

    auto add(uint32_t addr) -> void;

    auto remove(uint32_t addr) -> void;

    auto check(uint32_t addr, unsigned Size = 1) -> bool;

    auto checkRange(uint32_t addr, unsigned Size) -> bool;

    auto find(uint32_t addr) -> Watcher*;

    auto isEnabled(uint32_t addr) -> bool;

    auto isDisabled(uint32_t addr) -> bool;

    auto enable(uint32_t addr) -> void;

    auto disable(uint32_t addr) -> void;

    auto disableAll() -> void;

    auto flagWhenNeeded() -> void;

    auto flag(bool enable) -> void;
};

struct ModifiedCodes {
    M68000& cpu;
    int controlFlag;

    ModifiedCodes(M68000& cpu, int controlFlag);

    uint32_t addrFrom;
    uint32_t addrTo;
    bool alarm;

    auto add(uint32_t addr, uint32_t addrTo) -> void;
    auto checkAndSet(uint32_t addr, unsigned Size) -> void;
    auto getAndForget() -> bool;
    auto disable() -> void;
};

struct HistoryEntry {
    uint32_t addr;
    uint16_t mem[5] = {0};
    uint16_t flags;
};

struct HistoryHandler {
    M68000& cpu;
    int controlFlag;

    HistoryHandler(M68000& cpu, int controlFlag);

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
