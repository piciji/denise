
#include "m68000.h"


auto M68FAMILY::M68000::sync(uint16_t cycles) -> void {
    LIBAMI::system->doDMA( cycles );
}

auto M68FAMILY::M68000::readByte(uint32_t adr) -> uint8_t {
    return LIBAMI::system->readMemory(adr);
}

auto M68FAMILY::M68000::readWord(uint32_t adr) -> uint16_t {
    return LIBAMI::system->readMemory16(adr);
}

auto M68FAMILY::M68000::writeByte(uint32_t adr, uint8_t data) -> void {
    LIBAMI::system->writeMemory(adr, data);
}

auto M68FAMILY::M68000::writeWord(uint32_t adr, uint16_t data) -> void {
    LIBAMI::system->writeMemory16(adr, data);
}

namespace LIBAMI {

M68000* cpu = nullptr;

M68000::M68000() : M68FAMILY::M68000() {

}



auto M68000::IackCycle(uint8_t level, uint8_t& vector) -> uint8_t {
    vector = 24 + level;
    return USER_VECTOR;
}

auto M68000::serialize(Emulator::Serializer& s) -> void {
    s.array(regsD);
    s.array(regsA);
    s.integer(pc);
    s.integer(usp);
    s.integer(ssp);
    s.integer(irc);
    s.integer(ird);
    s.integer(c);
    s.integer(v);
    s.integer(z);
    s.integer(n);
    s.integer(x);
    s.integer(i);
    s.integer(this->s);
    s.integer(iplPins);
    s.integer(iplSample);
    s.integer(control);
}

}