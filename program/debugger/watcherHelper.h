#pragma once

#include "debugger.h"
#include "../../guikit/api.h"
#include "../../emulation/interface.h"

struct DbgWatcher {
    unsigned addr;
    std::string ident;
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
    auto addToList(unsigned addr, DebuggerAction action, const std::string& ident = "") -> void;
    auto removeFromList(unsigned addr, DebuggerAction action) -> void;
    auto findBy(unsigned addr, DebuggerAction action) -> DbgWatcher*;
    auto findRowBy(unsigned addr, DebuggerAction action) -> std::optional<unsigned>;
    auto elements() const -> unsigned { return watchers.size(); }
    auto getWatcher(unsigned pos) -> DbgWatcher& { return watchers[pos]; }
};