
#include "watcher.h"
#include "m6510.h"
#include <cstring>

#define INSTRUCTION_HISTORY_SIZE 512
#define INSTRUCTION_HISTORY_MASK (INSTRUCTION_HISTORY_SIZE - 1)

namespace LIBC64 {

WatchPoints::WatchPoints(M6510& cpu, int controlFlag) : cpu(cpu) {
    this->controlFlag = controlFlag;
    watchers.reserve( 10 );
}

auto WatchPoints::add(uint16_t addr) -> void {
    auto w = find( addr );

    if (!w)
        watchers.push_back( {addr, true} );
    else {
        w->enabled = true;
    }

    flag( true );
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
        flag( true );
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

    flag(false);
}

auto WatchPoints::flagWhenNeeded() -> void {
    for ( auto& w : watchers ) {
        if (w.enabled) {
            flag(true);
            return;
        }
    }
    flag(false);
}

auto WatchPoints::flag(bool enable) -> void {
    cpu.flagDebugAction( controlFlag, enable );
}

ModifiedCodes::ModifiedCodes(M6510& cpu, int controlFlag) : cpu(cpu) {
    this->controlFlag = controlFlag;
    alarm = false;
}

auto ModifiedCodes::add(uint16_t addr, uint16_t addrTo) -> void {
    this->addrFrom = addr;
    this->addrTo = addrTo;
    cpu.flagDebugAction( controlFlag, true );
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
    cpu.flagDebugAction( controlFlag, false );
}

HistoryHandler::HistoryHandler(M6510& cpu, int controlFlag) : cpu(cpu) {
    this->controlFlag = controlFlag;
    this->pos = 0;
    this->_enable = false;
    traces.resize( INSTRUCTION_HISTORY_SIZE );
}

auto HistoryHandler::add() -> void {
    auto& trace = traces[pos++];
    pos &= INSTRUCTION_HISTORY_MASK;
    if (pos == 0)
        _overflow = true;

    uint16_t addr = cpu.pc;
    trace.addr = addr;
    trace.flags = cpu.getFlags();
    for (uint8_t& m : trace.mem)
        m = cpu.memory.peek( addr++ );
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
    cpu.flagDebugAction( controlFlag, _enable );
}

}
