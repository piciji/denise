
#include "watcherHelper.h"

auto WatcherHelper::updateList() -> void {
    watcherList->lockRedraw();
    watcherList->reset();

    for (auto& w : watchers) {
        if (w.desc.empty()) {
            int format = debugger->isC64() ? 4 : 6;

            std::string _adr = GUIKIT::String::convertToHex(w.addr, format);

            std::string _endAdr;
            if (w.addr != w.endAddr)
                _endAdr = GUIKIT::String::convertToHex(w.endAddr, format);

            watcherList->append( {"", _adr,_endAdr, "", ""}, true );

        } else {
            watcherList->append( {"", w.desc, "", "", ""}, true );
        }

        unsigned row = watcherList->rowCount() - 1;

        updateBreakpointVisuals( row, &w, true );

        if (w.action == DebuggerAction::Watchpoint)
            watcherList->setImage( row, 3, debugger->memoryImg, true );
        else if (w.action == DebuggerAction::WatchpointWrite)
            watcherList->setImage( row, 3, debugger->memoryBorderImg, true );
        else if (w.action == DebuggerAction::ExceptionPoint)
            watcherList->setImage( row, 3, debugger->exceptionImg, true );
        else
            watcherList->setImage( row, 3, debugger->processorImg, true );

        watcherList->setImage( row, 4, debugger->trashImg, true );
    }

    watcherList->autoSizeColumns();
    watcherList->unlockRedraw();
}

auto WatcherHelper::updateBreakpointVisuals(unsigned row, DbgWatcher* watcher, bool preventColumResizing) -> void {
    if (watcher->enabled) {
        if (watcher->useHitCount || watcher->useExpression)
            watcherList->setImage( row, 0, debugger->breakCondEnableSmallImg, preventColumResizing );
        else
            watcherList->setImage( row, 0, debugger->breakEnableSmallImg, preventColumResizing );

    } else
        watcherList->setImage( row, 0, debugger->breakDisableSmallImg, preventColumResizing);
}

auto WatcherHelper::addToList(unsigned addr, unsigned endAddr, DebuggerAction action, const std::string& desc) -> DbgWatcher* {
    static unsigned ident = 1;

    watchers.push_back( {addr, endAddr, ident, desc, action, true} );

    std::sort(watchers.begin(), watchers.end(), [](DbgWatcher& a, DbgWatcher& b) -> bool {
        if (a.action < b.action)
            return true;
        if (a.action > b.action)
            return false;

        return a.addr < b.addr;
    });

    return findBy(ident++);
}

auto WatcherHelper::removeFromList(unsigned ident) -> void {
    for (auto it = watchers.begin(); it != watchers.end();) {
        if (it->ident == ident) {
            watchers.erase(it);
            break;
        }
        ++it;
    }
}

auto WatcherHelper::findEnabled(unsigned addr, DebuggerAction action) -> std::vector<DbgWatcher*> {
    std::vector<DbgWatcher*> result;

    for (auto& watcher : watchers) {
        if (watcher.enabled && watcher.addr == addr && watcher.endAddr == addr && watcher.action == action)
            result.push_back(&watcher);
    }
    return result;
}

auto WatcherHelper::findBy(unsigned addr, DebuggerAction action) -> std::vector<DbgWatcher*> {
    std::vector<DbgWatcher*> result;

    for (auto& watcher : watchers) {
        if (watcher.addr == addr && watcher.endAddr == addr && watcher.action == action)
            result.push_back(&watcher);
    }
    return result;
}

auto WatcherHelper::findBy(unsigned ident) -> DbgWatcher* {
    for (auto& watcher : watchers) {
        if (watcher.ident == ident)
            return &watcher;
    }
    return nullptr;
}

auto WatcherHelper::findRowBy(unsigned ident) -> std::optional<unsigned> {
    for (unsigned i = 0; i < watchers.size(); i++) {
        DbgWatcher& watcher = watchers[i];
        if (watcher.ident == ident)
            return i;
    }
    return std::nullopt;
}
