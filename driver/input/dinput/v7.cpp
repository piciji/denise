
#ifdef DRV_DINPUT7

#define DIRECTINPUT_VERSION 0x0700
#define DI_IDENT DInput7
#define DI_IDENT_CORE DInput7Core

#include "v7.h"

#include "core.cpp"

namespace DRIVER {
    
auto DInput7::create() -> DInput7* {
    
    return new DInput7Core();
}

}

#endif
