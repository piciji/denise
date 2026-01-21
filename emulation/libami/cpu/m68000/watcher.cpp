
#include "watcher.h"
#include "m68000.h"
#include <cstring>

#define INSTRUCTION_HISTORY_SIZE 512
#define INSTRUCTION_HISTORY_MASK (INSTRUCTION_HISTORY_SIZE - 1)

namespace M68FAMILY {

WatchPoints::WatchPoints(M68000& cpu, int controlFlag) : cpu(cpu) {
    this->controlFlag = controlFlag;
    watchers.reserve( 10 );
}

auto WatchPoints::add(uint32_t addr) -> void {
    if (!find( addr ))
        watchers.push_back( {addr} );

    flag( true );
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

auto WatchPoints::check(uint32_t addr, unsigned Size) -> bool {
    for ( auto& w : watchers ) {
        if ((w.addr >= addr) && (w.addr < addr + Size))
            return true;
    }
    return false;
}

auto WatchPoints::removeAll() -> void {
    watchers.clear();
    flag(false);
}

auto WatchPoints::flagWhenNeeded() -> void {
    flag( !watchers.empty() );
}

auto WatchPoints::flag(bool enable) -> void {
    cpu.flagDebugAction( controlFlag, enable );
}

ModifiedCodes::ModifiedCodes(M68000& cpu, int controlFlag) : cpu(cpu) {
    this->controlFlag = controlFlag;
    alarm = false;
}

auto ModifiedCodes::add(uint32_t addr, uint32_t addrTo) -> void {
    this->addrFrom = addr;
    this->addrTo = addrTo;
    cpu.flagDebugAction( controlFlag, true );
}

auto ModifiedCodes::checkAndSet(uint32_t addr, unsigned Size) -> void {
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
    cpu.flagDebugAction( controlFlag, false );
}

HistoryHandler::HistoryHandler(M68000& cpu, int controlFlag) : cpu(cpu) {
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

    uint32_t addr = cpu.pcEdge();
    trace.addr = addr;
    trace.flags = cpu.getSR();
    for (uint16_t& m : trace.mem) {
        m = cpu.peek( addr );
        addr += 2;
    }
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
