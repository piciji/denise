
#pragma once

#include "../../../cia/m6526.h"

namespace LIBC64 {

struct Cia8520 : CIA::M6526 {

    Cia8520( uint8_t model ) : CIA::M6526( model, new Emulator::SystemTimer ) {}

    auto clock() -> void {

        events->process();

        CIA::M6526::clock();
    }

    auto reset() -> void {

        events->clear();

        CIA::M6526::reset();
    }

    auto serialize(Emulator::Serializer& s) -> void {

        events->serialize( s );

        CIA::M6526::serialize( s );
    }
};

}
