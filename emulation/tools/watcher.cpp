
#include "watcher.h"

#include <utility>

#define INSTRUCTION_HISTORY_SIZE 512
#define INSTRUCTION_HISTORY_MASK (INSTRUCTION_HISTORY_SIZE - 1)

namespace Emulator {

WatchPoints::WatchPoints() {
    watchers.reserve( 10 );
    callback = [](bool state) {};
}

auto WatchPoints::add(uint32_t addr) -> void {
    if (!find( addr ))
        watchers.push_back( {addr} );

    callback(true);
}

auto WatchPoints::remove(uint32_t addr) -> void {
    for (auto it = watchers.begin(); it != watchers.end();) {
        if (it->addr == addr) {
            watchers.erase(it);
            flagWhenNeeded();
            break;
        }
        ++it;
    }
}

auto WatchPoints::find(uint32_t addr) -> Watcher* {
    for ( auto& w : watchers ) {
        if (w.addr == addr)
            return &w;
    }
    return nullptr;
}

auto WatchPoints::check(uint32_t addr, bool withConditions) -> bool {
    for ( auto& w : watchers ) {
        if (w.addr == addr)
            return withConditions ? checkConditions(w) : true;
    }
    return false;
}

auto WatchPoints::check( uint32_t addr, unsigned Size, bool withConditions ) -> Watcher* {
    for (auto& w: watchers) {
        if ((w.addr >= addr) && (w.addr < addr + Size)) {
            if (withConditions)
                return checkConditions(w) ? &w : nullptr;
            return &w;
        }
    }
    return nullptr;
}

auto WatchPoints::removeAll() -> void {
    watchers.clear();
    callback(false);
}

auto WatchPoints::reset() -> void {
    for ( auto& w : watchers ) {
        w.curHitCount = 0;
    }
    flagWhenNeeded();
}

auto WatchPoints::flagWhenNeeded() -> void {
    callback(!watchers.empty());
}

auto WatchPoints::setBreakpointCondition( unsigned addr, unsigned hitCount, unsigned hitCountMode,
                                          const std::string& expression, unsigned expressionMode ) -> void {
    auto* w = find( addr );
    if (!w)
        return;

    w->hitCount = hitCount;
    w->curHitCount = 0;
    w->hitCountMode = hitCountMode;
    w->useExpression = !expression.empty();
    w->expressionParser.setExpression( expression );
    w->expressionMode = expressionMode;
    w->expressionResultBefore = false;
    w->expressionParser.callback = expressionCallback;
}

auto WatchPoints::checkConditions( Watcher& w ) -> bool {
    if (w.useExpression) {
        if (w.expressionMode == 0) {
            if (!w.expressionParser.parseSilent())
                return false;
        } else if (w.expressionMode == 1) {
            bool result = w.expressionParser.parseSilent();
            if (result != w.expressionResultBefore) {
                w.expressionResultBefore = result;
            } else {
                return false;
            }
        }
    }

    if (w.hitCount) {
        if (++w.curHitCount == 0)
            w.curHitCount = w.hitCount + 1;

        if ((w.hitCountMode == 0) && (w.curHitCount != w.hitCount)) {
            return false;
        }
        if ((w.hitCountMode == 1) && (w.curHitCount < w.hitCount)) {
            return false;
        }
    }

    return true;
}

ModifiedCodes::ModifiedCodes() {
    alarm = false;
    callback = [](bool state) {};
}

auto ModifiedCodes::add(uint32_t addr, uint32_t addrTo) -> void {
    this->addrFrom = addr;
    this->addrTo = addrTo;
    callback(true);
}

auto ModifiedCodes::checkAndSet(uint32_t addr) -> void {
    if ((addrFrom <= addr) && (addrTo >= addr))
        alarm = true;
}

auto ModifiedCodes::checkAndSet( uint32_t addr, unsigned Size ) -> void {
    if ((addrFrom >= addr) && (addrFrom < addr + Size) && (addrTo >= addr))
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

template <typename T>
HistoryHandler<T>::HistoryHandler() {
    this->pos = 0;
    this->_enable = false;
    traces.resize( INSTRUCTION_HISTORY_SIZE );
    callback = [](bool state) {};
}

template <typename T>
auto HistoryHandler<T>::getNext() -> HistoryEntry<T>& {
    auto& trace = traces[pos++];
    pos &= INSTRUCTION_HISTORY_MASK;
    if (pos == 0)
        _overflow = true;
    return trace;
}

template <typename T>
auto HistoryHandler<T>::get(unsigned i) -> HistoryEntry<T>* {
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

template <typename T>
auto HistoryHandler<T>::enable() -> void {
    _enable = true;
    pos = 0;
    _overflow = false;
    flagWhenNeeded();
}

template <typename T>
auto HistoryHandler<T>::disable() -> void {
    _enable = false;
    flagWhenNeeded();
}

template <typename T>
auto HistoryHandler<T>::flagWhenNeeded() -> void {
    callback(_enable);
}

template struct HistoryHandler<uint8_t>;
template struct HistoryHandler<uint16_t>;

}
