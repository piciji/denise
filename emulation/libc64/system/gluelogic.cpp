
#include "gluelogic.h"
#include "system.h"
#include "../../tools/serializer.h"

namespace LIBC64 {

GlueLogic::GlueLogic(System* system, Emulator::SystemTimer& sysTimer) : system(system), sysTimer(system->sysTimer) {

    updateVbank = [this]() {

        this->system->vicBank = vbankBefore << 14;
    };

    sysTimer.registerCallback( {&updateVbank, 1} );
}

auto GlueLogic::serialize(Emulator::Serializer& s) -> void {

    s.integer( vbankBefore );

    s.integer( (uint8_t&) type );
}

auto GlueLogic::setType( Type type ) -> void {

    this->type = type;
    // in case of on the fly switching during runtime
    sysTimer.remove( &updateVbank );
}

auto GlueLogic::setVBank( uint8_t vbank, bool ddrChange ) -> void {

    if ( type == Type::Discrete ) {

        system->vicBank = vbank << 14;

        goto end; // lets update vbankBefore in case of runtime switch of glue logic
    }
    // custom ic for C64C

    // both vbank bits have changed and new vbank is 1 or 2
    if (((vbankBefore ^ vbank) == 3) &&  (vbank == 1 || vbank == 2) ) {
        system->vicBank = 3 << 14; // force to 3 for the following cycle only
        sysTimer.add( &updateVbank, 2, Emulator::SystemTimer::Action::UpdateExisting );

    } else if (ddrChange && (vbank < vbankBefore) && ((vbankBefore ^ vbank) != 3)) {
        sysTimer.add( &updateVbank, 2, Emulator::SystemTimer::Action::UpdateExisting );

    } else
        system->vicBank = vbank << 14;

    end:
    vbankBefore = vbank;
}

auto GlueLogic::reset() -> void {

    vbankBefore = 0;
}

}
