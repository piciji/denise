
#ifdef DRV_DINPUT5

#define DIRECTINPUT_VERSION 0x0500
#define DI_IDENT DInput5
#define DI_IDENT_CORE DInput5Core

#include "v5.h"

#include "core.cpp"

namespace DRIVER {
    
auto DInput5::create() -> DInput5* {
    
    return new DInput5Core();
}

}

#endif
