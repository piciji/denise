
#ifdef DRV_XAUDIO27

#if defined(DRV_XAUDIO28) || defined(DRV_XAUDIO29)

#define _WIN32_WINNT 0x0501
#define XA_IDENT XAudio27
#define XA_IDENT_CORE XAudio27Core

#include "xaudio27.h"
#include "header27.h"

#include "core.cpp"

namespace DRIVER {
    
auto XAudio27::create() -> XAudio27* {
    
    return new XAudio27Core();
}

}

#endif

#endif
