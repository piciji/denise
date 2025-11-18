
#include "m68000.h"
#include "../agnus/agnus.h"
#include "../../tools/serializer.h"
#include "../system/snapShots.h"

namespace LIBAMI {

Cpu::Cpu(Agnus& agnus) : M68FAMILY::M68000(agnus) {

}

auto Cpu::getSnapshot() -> CpuSnapshot {
    CpuSnapshot snap{};
    std::copy(std::begin(regsD), std::end(regsD), std::begin(snap.regsD));
    std::copy(std::begin(regsA), std::end(regsA), std::begin(snap.regsA));
    snap.pc = pc;
    snap.pcOpEdge = pcEdge;
    snap.irc = irc;
    snap.ird = ird;
    snap.usp = usp;
    snap.ssp = ssp;
    snap.flags = getSR();

    snap.ipl = iplPins;
    snap.stp = control & Stop;
    snap.hlt = control & Halt;
    return snap;
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
