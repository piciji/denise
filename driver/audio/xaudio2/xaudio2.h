
#include "xaudio27.h"
#include "xaudio28.h"
#include "xaudio29.h"
#include "../../driver.h"
#include "../../tools/win.h"

#if defined (DRV_XAUDIO27) && !defined(DRV_XAUDIO28) && !defined(DRV_XAUDIO29)
    #define _WIN32_WINNT 0x0501
    #define XA_IDENT Audio
    #define XA_IDENT_CORE XAudio2
    #include "header27.h"
    #include "core.cpp"

#elif defined (DRV_XAUDIO28) && !defined(DRV_XAUDIO27) && !defined(DRV_XAUDIO29)
    #define _WIN32_WINNT 0x0602
    #define XA_IDENT Audio
    #define XA_IDENT_CORE XAudio2
    #include "header29.h"
    #include "core.cpp"

#elif defined (DRV_XAUDIO29) && !defined(DRV_XAUDIO27) && !defined(DRV_XAUDIO28)
    #define _WIN32_WINNT 0x0a00
    #define XA_IDENT Audio
    #define XA_IDENT_CORE XAudio2
    #include "header29.h"
    #include "core.cpp"
#else   

namespace DRIVER {
    
struct XAudio2 : Audio {
    
    unsigned version;
    XAudio27* xAudio27 = nullptr;
    XAudio28* xAudio28 = nullptr;
    XAudio29* xAudio29 = nullptr;
    
    XAudio2( unsigned version ) {
        
        this->version = version;
        
        switch( version ) {
            case 27: xAudio27 = XAudio27::create(); break;
            case 28: xAudio28 = XAudio28::create(); break;
            case 29: xAudio29 = XAudio29::create(); break;
        }
    }
    
    ~XAudio2() {
        if (xAudio27)
            delete xAudio27;
        
        if (xAudio28)
            delete xAudio28;
        
        if (xAudio29)
            delete xAudio29;

        xAudio27 = nullptr, xAudio28 = nullptr, xAudio29 = nullptr;
    }    
    
    auto init(uintptr_t handle) -> bool {
        
        switch( version ) {          
            case 27: return xAudio27->init( handle );
            case 28: return xAudio28->init( handle );
            case 29: return xAudio29->init( handle );
        }
        return false;
    }
    
    auto term() -> void {
        switch( version ) {          
            case 27: xAudio27->term( ); break;
            case 28: xAudio28->term( ); break;
            case 29: xAudio29->term( ); break;
        }    
    }
    
    auto clear() -> void {
        switch( version ) {          
            case 27: xAudio27->clear( ); break;
            case 28: xAudio28->clear( ); break;
            case 29: xAudio29->clear( ); break;
        }            
    }
    
    auto setFrequency(unsigned value) -> void {
        switch( version ) {          
            case 27: xAudio27->setFrequency(value); break;
            case 28: xAudio28->setFrequency(value); break;
            case 29: xAudio29->setFrequency(value); break;
        } 
    }
    
    auto setLatency(unsigned value) -> void { 
        switch( version ) {          
            case 27: xAudio27->setLatency(value); break;
            case 28: xAudio28->setLatency(value); break;
            case 29: xAudio29->setLatency(value); break;
        }        
    }
    
    auto synchronize(bool state) -> void { 
        switch( version ) {          
            case 27: xAudio27->synchronize(state); break;
            case 28: xAudio28->synchronize(state); break;
            case 29: xAudio29->synchronize(state); break;
        }        
    }
    
    auto hasSynchronized() -> bool {
        switch( version ) {          
            case 27: return xAudio27->hasSynchronized();
            case 28: return xAudio28->hasSynchronized();
            case 29: return xAudio29->hasSynchronized();
        } 
        
        return hasSynchronized();
    }
    
    auto addSamples(const uint8_t* buffer, unsigned size) -> void { 
        switch( version ) {          
            case 27: xAudio27->addSamples(buffer, size); break;
            case 28: xAudio28->addSamples(buffer, size); break;
            case 29: xAudio29->addSamples(buffer, size); break;
        }        
    }
    
    auto getCenterBufferDeviation() -> double { 
        switch( version ) {          
            case 27: return xAudio27->getCenterBufferDeviation();
            case 28: return xAudio28->getCenterBufferDeviation();
            case 29: return xAudio29->getCenterBufferDeviation();
        }     
        return 0.0;
    }
    
    auto expectFloatingPoint() -> bool {
 
        return true;
    }
    
    auto getMinimumLatency() -> unsigned {
        
        switch( version ) {          
            case 27: return xAudio27->getMinimumLatency();
            case 28: return xAudio28->getMinimumLatency();
            case 29: return xAudio29->getMinimumLatency();
        }   
        
        return 1;
    }
};
    
}

#endif