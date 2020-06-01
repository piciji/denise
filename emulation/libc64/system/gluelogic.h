
#pragma once

#include "system.h"

namespace LIBC64 {
    
struct GlueLogic {
    
    enum Type : uint8_t { Discrete = 0, CustomIC = 1 } type = Type::Discrete;
    
    using Callback = std::function<void ()>;
    
    Callback updateVbank;
    
    uint8_t vbankBefore;
    
    Emulator::Events* events;
    
    GlueLogic( Emulator::Events* events ) {
        
        this->events = events;
        
        updateVbank = [this]() {
            
            system->vicBank = vbankBefore;
        };    
        
        this->events->registerCallback( {&updateVbank, 1} );
    }
    
    auto serialize(Emulator::Serializer& s) -> void {        
        
        s.integer( vbankBefore );   
        
        s.integer( (uint8_t&) type );
    }
    
    auto setType( Type type ) -> void {
        
        this->type = type;
        // in case of on the fly switching during runtime
        events->remove( &updateVbank );
    }
    
    auto setVBank( uint8_t vbank, bool ddrChange ) -> void {
                        
        if ( type == Type::Discrete ) {
            
            system->vicBank = vbank;
            
            goto end; // lets update vbankBefore in case of runtime switch of glue logic
        }
        // custom ic for C64C

        // both vbank bits have changed and new vbank is 1 or 2
        if (((vbankBefore ^ vbank) == 3) &&  (vbank == 1 || vbank == 2) ) {        
            system->vicBank = 3; // force to 3 for the following cycle only
            events->add( &updateVbank, 2, Emulator::Events::UpdateExisting );
            
        } else if (ddrChange && (vbank < vbankBefore) && ((vbankBefore ^ vbank) != 3)) {
            events->add( &updateVbank, 2, Emulator::Events::UpdateExisting );
            
        } else
            system->vicBank = vbank;
                    
        end:
            vbankBefore = vbank;
    }
    
    auto reset() -> void {
        
        vbankBefore = 0;
    }
};
    
}
