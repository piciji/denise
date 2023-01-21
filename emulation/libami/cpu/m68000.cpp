
#include "m68000.h"
#include "../agnus/agnus.h"
#include "../../tools/serializer.h"
#include "../interface.h"

#define CPU_LOG_START 2000000
#define CPU_LOG_COUNT 1000000

namespace LIBAMI {

Cpu::Cpu(Agnus& agnus) : M68FAMILY::M68000(agnus) {

}

auto Cpu::getA(uint8_t pos) -> uint32_t {
    return regsA[pos & 7];
}

auto Cpu::getD(uint8_t pos) -> uint32_t {
    return regsD[pos & 7];
}

auto Cpu::getStatus() -> uint16_t {
    return getSR();
}

auto Cpu::getIrd() -> uint16_t {
    return ird;
}

auto Cpu::getPC() -> uint32_t {
    return pc;
}

auto Cpu::logState() -> void {
    if (logCounter == (CPU_LOG_COUNT + CPU_LOG_START) )
        return;

    if (logCounter++ < CPU_LOG_START)
        return;

    ref.interface->log("H", true);
    ref.interface->log(ref.hPos, false, true);
    ref.interface->log("IRD", false);
    ref.interface->log(ird, false, true);
    ref.interface->log("PC", false);
    ref.interface->log(pc, false, true);
    ref.interface->log("S", false);
    ref.interface->log(getSR(), false, true);
    ref.interface->log("D", false);
    for(unsigned i = 0; i < 8; i++)
        ref.interface->log(regsD[i], false, true);
    ref.interface->log("A", false);
    for(unsigned i = 0; i < 8; i++)
        ref.interface->log(regsA[i], false, true);
//    ref.interface->log("USP", false);
//    ref.interface->log(usp, false, true);
//    ref.interface->log("SSP", false);
//    ref.interface->log(ssp, false, true);
}

auto Cpu::serialize(Emulator::Serializer& s) -> void {
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

#undef CPU_LOG_START
#undef CPU_LOG_END