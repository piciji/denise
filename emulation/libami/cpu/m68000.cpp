
#include "m68000.h"
#include "../agnus/agnus.h"
#include "../../tools/serializer.h"

namespace LIBAMI {

Cpu::Cpu(Agnus& agnus) : M68FAMILY::M68000(agnus) {

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
