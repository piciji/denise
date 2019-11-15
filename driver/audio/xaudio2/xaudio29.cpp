
#ifdef DRV_XAUDIO29

#if defined(DRV_XAUDIO27) || defined(DRV_XAUDIO28)

#define _WIN32_WINNT 0x0a00
#define XA_IDENT XAudio29
#define XA_IDENT_CORE XAudio29Core

#include "xaudio29.h"
#include "header29.h"

#include "core.cpp"

namespace DRIVER {
    
auto XAudio29::create() -> XAudio29* {
    
    return new XAudio29Core();
}

}

#endif

#endif

