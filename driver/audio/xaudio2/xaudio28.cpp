
#ifdef DRV_XAUDIO28

#if defined(DRV_XAUDIO27) || defined(DRV_XAUDIO29)

#define _WIN32_WINNT 0x0602
#define XA_IDENT XAudio28
#define XA_IDENT_CORE XAudio28Core

#include "xaudio28.h"
#include "header29.h" // includes 2.8 too

#include "core.cpp"

namespace DRIVER {
    
auto XAudio28::create() -> XAudio28* {
    
    return new XAudio28Core();
}

}

#endif

#endif

