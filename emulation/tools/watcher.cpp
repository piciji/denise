
#include "watcher.h"

#include <utility>

#define INSTRUCTION_HISTORY_SIZE 512
#define INSTRUCTION_HISTORY_MASK (INSTRUCTION_HISTORY_SIZE - 1)

namespace Emulator {

WatchPoints::WatchPoints() {
    watchers.reserve( 10 );
    callback = [](bool state) {};
}

auto WatchPoints::add(uint16_t addr) -> void {
    auto w = find( addr );

    if (!w)
        watchers.push_back( {addr, true} );
    else {
        w->enabled = true;
    }

    callback(true);
}

auto WatchPoints::remove(uint16_t addr) -> void {
    for (auto it = watchers.begin(); it != watchers.end();) {
        if (it->addr == addr) {
            watchers.erase(it);
            flagWhenNeeded();
            break;
        }
        ++it;
    }
}

auto WatchPoints::find(uint16_t addr) -> Watcher* {
    for ( auto& w : watchers ) {
        if (w.addr == addr)
            return &w;
    }
    return nullptr;
}

auto WatchPoints::check(uint16_t addr) -> bool {
    for ( auto& w : watchers ) {
        if ((w.addr == addr) && w.enabled)
            return true;
    }
    return false;
}

auto WatchPoints::isEnabled(uint16_t addr) -> bool {
    auto w = find( addr );
    return w != nullptr && w->enabled;
}

auto WatchPoints::isDisabled(uint16_t addr) -> bool {
    auto w = find( addr );
    return w == nullptr || !w->enabled;
}

auto WatchPoints::enable(uint16_t addr) -> void {
    if (auto w = find( addr )) {
        w->enabled = true;
        callback(true);
    }
}

auto WatchPoints::disable(uint16_t addr) -> void {
    if (auto w = find( addr )) {
        w->enabled = false;
        flagWhenNeeded();
    }
}

auto WatchPoints::disableAll() -> void {
    for ( auto& w : watchers )
        w.enabled = false;

    callback(false);
}

auto WatchPoints::flagWhenNeeded() -> void {
    for ( auto& w : watchers ) {
        if (w.enabled) {
            callback(true);
            return;
        }
    }
    callback(false);
}

ModifiedCodes::ModifiedCodes() {
    alarm = false;
    callback = [](bool state) {};
}

auto ModifiedCodes::add(uint16_t addr, uint16_t addrTo) -> void {
    this->addrFrom = addr;
    this->addrTo = addrTo;
    callback(true);
}

auto ModifiedCodes::checkAndSet(uint16_t addr) -> void {
    if ((addrFrom <= addr) && (addrTo >= addr))
        alarm = true;
}

auto ModifiedCodes::getAndForget() -> bool {
    auto _alarm = alarm;
    alarm = false;
    return _alarm;
}

auto ModifiedCodes::disable() -> void {
    alarm = false;
    callback(false);
}

HistoryHandler::HistoryHandler() {
    this->pos = 0;
    this->_enable = false;
    traces.resize( INSTRUCTION_HISTORY_SIZE );
    callback = [](bool state) {};
}

auto HistoryHandler::getNext() -> HistoryEntry& {
    auto& trace = traces[pos++];
    pos &= INSTRUCTION_HISTORY_MASK;
    if (pos == 0)
        _overflow = true;
    return trace;
}

auto HistoryHandler::get(unsigned i) -> HistoryEntry* {
    if (i >= INSTRUCTION_HISTORY_SIZE)
        return nullptr;

    unsigned _p = pos & INSTRUCTION_HISTORY_MASK;

    if (_overflow) {
        if (_p == 0)
            _p = INSTRUCTION_HISTORY_MASK;
        else
            _p -= 1;

        if (_p >= i)
            return &traces[_p - i];

        return &traces[INSTRUCTION_HISTORY_SIZE - (i - _p)];
    }

    return (_p == 0 || i >= _p) ? nullptr : &traces[_p - i - 1];
}

auto HistoryHandler::enable() -> void {
    _enable = true;
    pos = 0;
    _overflow = false;
    flagWhenNeeded();
}

auto HistoryHandler::disable() -> void {
    _enable = false;
    flagWhenNeeded();
}

auto HistoryHandler::flagWhenNeeded() -> void {
    callback(_enable);
}

}
