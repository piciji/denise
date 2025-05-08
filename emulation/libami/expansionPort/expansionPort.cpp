
#include "expansionPort.h"
#include "../agnus/agnus.h"
#include "../../tools/macros.h"

namespace LIBAMI {

ExpansionPort::ExpansionPort(Agnus& agnus) : agnus(agnus) {}

auto ExpansionPort::reset(bool softReset) -> void {    
    baseAdr = 0;
    boardState = BoardState::ShutUp;
}

auto ExpansionPort::writeAutoConf(uint32_t addr, uint8_t data) -> void {
    switch (addr & 0xffff) {
        case 0x48: {
            baseAdr |= (data & 0xf0) << 16;
            boardState = BoardState::Configured;
            add();            
        } break;

        case 0x4a:
            baseAdr |= (data & 0xf0) << 12;
            break;

        case 0x4c:
            if (canBeShutUp())
                boardState = BoardState::ShutUp;
            break;
    }    
}

auto ExpansionPort::writeAutoConfW(uint32_t addr, uint16_t data) -> void {
    switch (addr & 0xffff) {
        case 0x48: {
            baseAdr = ((data >> 8) & 0xff) << 16;
            boardState = BoardState::Configured;
            add();
        } break;

        case 0x4c: {
            for(auto expansion : agnus.expansions)
                if (expansion->boardState == ExpansionPort::BoardState::AutoConf)
                    boardState = BoardState::ShutUp;
        } break;
    }
}

auto ExpansionPort::add() -> void {
    int _firstPage = firstPage();
    if (_firstPage == 0)
        return;

    bool memoryExpansion = dynamic_cast<FastMemExpansion*>(this);
    int _pages = pages();
    for (int i = _firstPage; i < (_firstPage + _pages); i++) {
        agnus.mapper[i] = memoryExpansion ? Agnus::FAST_MEM : Agnus::EXPANSION;
        agnus.expansionsConfigured[i] = this;
    }
}

auto ExpansionPort::getSizeBits() -> uint8_t {
    switch(pages()) {
        case 1: return 1;
        case 2: return 2;
        case 4: return 3;
        case 8: return 4;
        case 16: return 5;
        case 32: return 6;
        case 64: return 7;
        case 128: return 0;
    }
    return 1;
}


}
