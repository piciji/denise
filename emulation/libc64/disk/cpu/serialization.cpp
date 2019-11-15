
#include "m6502custom.h"

namespace LIBC64 {
    
auto M6502Custom::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( step );
    s.integer( adrTemp );
    s.integer( zeroAdrTemp );
    s.integer( dataTemp );
    s.integer( displacement );
    s.integer( readNext );    
}
    
}