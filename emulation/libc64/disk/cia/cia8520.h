
#pragma once

#include "../../../cia/m8520.h"

namespace LIBC64 {

struct Cia8520 : CIA::M8520 {

    Cia8520( uint8_t model ) : CIA::M8520( model, new Emulator::SystemTimer ) {}

    auto clock() -> void {

        events->process();

        CIA::M8520::clock();
    }

    auto reset() -> void {

        events->clear();

        CIA::M8520::reset();
    }

    auto serialize(Emulator::Serializer& s) -> void {

        events->serialize( s );

        CIA::M8520::serialize( s );
    }
};

}
