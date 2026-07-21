#pragma once

#include "debugger.h"
#include "../../guikit/api.h"
#include "../../emulation/interface.h"

struct DbgWatcher {
    unsigned addr;
    unsigned endAddr;
    unsigned ident;
    std::string desc;
    DebuggerAction action;
    bool enabled;

    bool useHitCount = false;
    unsigned hitCount = 0;
    unsigned hitCountCompare = 0;

    bool useExpression = false;
    std::string expression;
    unsigned expressionCompare = 0;
};

struct WatcherHelper {
    GUIKIT::ListView* watcherList;
    Debugger* debugger;
    std::vector<DbgWatcher> watchers;

    auto updateList() -> void;
    auto updateBreakpointVisuals(unsigned row, DbgWatcher* watcher, bool preventColumResizing = false) -> void;
    auto addToList(unsigned addr, unsigned endAddr, DebuggerAction action, const std::string& desc = "") -> DbgWatcher*;
    auto removeFromList(unsigned ident) -> void;
    auto findBy(unsigned ident) -> DbgWatcher*;
    auto findBy(unsigned addr, DebuggerAction action) -> std::vector<DbgWatcher*>;
    auto findEnabled(unsigned addr, DebuggerAction action) -> std::vector<DbgWatcher*>;
    auto findRowBy(unsigned ident) -> std::optional<unsigned>;
    auto elements() const -> unsigned { return watchers.size(); }
    auto getWatcher(unsigned pos) -> DbgWatcher& { return watchers[pos]; }
};