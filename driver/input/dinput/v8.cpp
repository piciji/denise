
#ifdef DRV_DINPUT8

#define DIRECTINPUT_VERSION 0x0800
#define DI_IDENT DInput8
#define DI_IDENT_CORE DInput8Core

#include "v8.h"

#include "core.cpp"

namespace DRIVER {
    
auto DInput8::create() -> DInput8* {
    
    return new DInput8Core();
}

}

#endif
