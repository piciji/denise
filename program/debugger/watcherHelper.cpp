
#include "watcherHelper.h"

auto WatcherHelper::updateList() -> void {
    watcherList->lockRedraw();
    watcherList->reset();

    for (auto& w : watchers) {
        if (w.ident.empty()) {
            int format = debugger->isC64() ? 4 : 6;
            watcherList->append( {"", GUIKIT::String::convertToHex(w.addr, format),"", ""}, true );
        } else {
            watcherList->append( {"", w.ident, "", ""}, true );
        }

        unsigned row = watcherList->rowCount() - 1;

        updateBreakpointVisuals( row, &w, true );

        if (w.action == DebuggerAction::Watchpoint)
            watcherList->setImage( row, 2, debugger->memoryImg, true );
        else if (w.action == DebuggerAction::WatchpointWrite)
            watcherList->setImage( row, 2, debugger->memoryBorderImg, true );
        else if (w.action == DebuggerAction::ExceptionPoint)
            watcherList->setImage( row, 2, debugger->exceptionImg, true );
        else
            watcherList->setImage( row, 2, debugger->processorImg, true );

        watcherList->setImage( row, 3, debugger->trashImg, true );
    }

    watcherList->autoSizeColumns();
    watcherList->unlockRedraw();
}

auto WatcherHelper::updateBreakpointVisuals(unsigned row, DbgWatcher* watcher, bool preventColumResizing) -> void {
    if (watcher->enabled) {
        if (watcher->useHitCount || watcher->useExpression)
            watcherList->setImage( row, 0, debugger->breakCondEnableSmallImg, preventColumResizing );
        else {
            watcherList->setImage( row, 0, debugger->breakEnableSmallImg, preventColumResizing );
        }

    } else {
        watcherList->setImage( row, 0, debugger->breakDisableSmallImg, preventColumResizing);
    }
}

auto WatcherHelper::addToList(unsigned addr, DebuggerAction action, const std::string& ident) -> void {
    watchers.push_back( {addr, ident, action, true} );

    std::sort(watchers.begin(), watchers.end(), [](DbgWatcher& a, DbgWatcher& b) -> bool {
        if (a.action < b.action)
            return true;
        if (a.action > b.action)
            return false;

        return a.addr < b.addr;
    });
}

auto WatcherHelper::removeFromList(unsigned addr, DebuggerAction action) -> void {
    for (auto it = watchers.begin(); it != watchers.end();) {
        if (it->addr == addr && it->action == action) {
            watchers.erase(it);
            break;
        }
        ++it;
    }
}

auto WatcherHelper::findBy(unsigned addr, DebuggerAction action) -> DbgWatcher* {
    for (auto& watcher : watchers) {
        if (watcher.addr == addr && watcher.action == action)
            return &watcher;
    }
    return nullptr;
}

auto WatcherHelper::findRowBy(unsigned addr, DebuggerAction action) -> std::optional<unsigned> {
    for (unsigned i = 0; i < watchers.size(); i++) {
        DbgWatcher& watcher = watchers[i];
        if (watcher.addr == addr && watcher.action == action)
            return i;
    }
    return std::nullopt;
}
