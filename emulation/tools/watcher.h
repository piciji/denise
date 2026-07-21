
#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include "expressionParser.h"

namespace Emulator {

using WatcherCallback = std::function<void ( bool state )>;
using ExpressionCallback = std::function< uint32_t (const std::string& input, int& pos)>;

struct Watcher {
    unsigned ident;
    uint32_t addr;
    uint32_t endAddr;
    unsigned hitCount = 0;
    unsigned curHitCount = 0;
    unsigned hitCountMode = 0;
    bool useExpression = false;
    ExpressionParser expressionParser;
    unsigned expressionMode = 0;
    bool expressionResultBefore = false;
    bool hit = false;
};

struct WatchPoints {
    std::vector<Watcher> watchers;
    WatcherCallback callback;
    ExpressionCallback expressionCallback;

    WatchPoints();

    virtual ~WatchPoints() = default;

    auto add(unsigned ident, uint32_t addr, uint32_t endAddr) -> void;

    auto remove(unsigned ident) -> void;

    auto setBreakpointCondition(unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> void;

    auto check(uint32_t addr, bool withConditions = true) -> bool;

    auto check(uint32_t addr, unsigned Size, bool withConditions = true) -> bool;

    auto getAndResetHitIdents(std::vector<unsigned>& idents) -> void;

    auto checkConditions(Watcher& w) -> bool;

    auto find(unsigned ident) -> Watcher*;

    auto removeAll() -> void;

    auto reset() -> void;

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
    auto checkAndSet(uint32_t addr, unsigned Size) -> void;
    auto getAndForget() -> bool;
    auto disable() -> void;
};

template<typename T>
struct HistoryEntry {
    uint32_t addr;
    T mem[5] = {0}; // we should remember the actual values, because of potentially modified code.
    uint16_t flags;
};

template<typename T>
struct HistoryHandler {
    WatcherCallback callback;

    HistoryHandler();

    bool _enable = false;
    bool _overflow = false;
    uint16_t pos;
    std::vector<HistoryEntry<T>> traces;

    auto getNext() -> HistoryEntry<T>&;
    auto get(unsigned i) -> HistoryEntry<T>*;
    auto flagWhenNeeded() -> void;
    auto enable() -> void;
    auto disable() -> void;
};

}
