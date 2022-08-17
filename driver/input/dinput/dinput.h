
#include "v5.h"
#include "v7.h"
#include "v8.h"
#include "../../driver.h"

#if defined (DRV_DINPUT5) && !defined(DRV_DINPUT7) && !defined(DRV_DINPUT8)
    #define DIRECTINPUT_VERSION 0x0500
    #define DI_IDENT Input
    #define DI_IDENT_CORE DInput
    #include "core.cpp"

#elif defined (DRV_DINPUT7) && !defined(DRV_DINPUT5) && !defined(DRV_DINPUT8)
    #define DIRECTINPUT_VERSION 0x0700
    #define DI_IDENT Input
    #define DI_IDENT_CORE DInput
    #include "core.cpp"

#elif defined (DRV_DINPUT8) && !defined(DRV_DINPUT5) && !defined(DRV_DINPUT7)
    #define DIRECTINPUT_VERSION 0x0800
    #define DI_IDENT Input
    #define DI_IDENT_CORE DInput
    #include "core.cpp"
#else   

namespace DRIVER {
    
struct DInput : Input {
    
    unsigned version;
    DInput5* dinput5 = nullptr;
    DInput7* dinput7 = nullptr;
    DInput8* dinput8 = nullptr;
    
    DInput( unsigned version ) {
        
        this->version = version;
        
        switch( version ) {
            case 0x0500: dinput5 = DInput5::create(); break;
            case 0x0700: dinput7 = DInput7::create(); break;
            case 0x0800: dinput8 = DInput8::create(); break;
        }
    }
    
    ~DInput() {
        if (dinput5)
            delete dinput5;
        
        if (dinput7)
            delete dinput7;
        
        if (dinput8)
            delete dinput8;

        dinput5 = nullptr, dinput7 = nullptr, dinput8 = nullptr;
    }
    
    auto init(uintptr_t handle) -> bool {
        
        switch( version ) {          
            case 0x0500: return dinput5->init( handle );
            case 0x0700: return dinput7->init( handle );
            case 0x0800: return dinput8->init( handle );
        }
        return false;
    }
    
    auto term() -> void {
        switch( version ) {          
            case 0x0500: dinput5->term( ); break;
            case 0x0700: dinput7->term( ); break;
            case 0x0800: dinput8->term( ); break;
        }    
    }
    
    auto mAcquire() -> void {
        switch( version ) {          
            case 0x0500: dinput5->mAcquire( ); break;
            case 0x0700: dinput7->mAcquire( ); break;
            case 0x0800: dinput8->mAcquire( ); break;
        }            
    }
    
    auto mUnacquire() -> void {
        switch( version ) {          
            case 0x0500: dinput5->mUnacquire( ); break;
            case 0x0700: dinput7->mUnacquire( ); break;
            case 0x0800: dinput8->mUnacquire( ); break;
        } 
    }
    
    auto mIsAcquired() -> bool { 
        switch( version ) {          
            case 0x0500: return dinput5->mIsAcquired();
            case 0x0700: return dinput7->mIsAcquired();
            case 0x0800: return dinput8->mIsAcquired();
        } 
        
        return false;
    }
    
	auto poll() -> std::vector<Hid::Device*> { 
        switch( version ) {          
            case 0x0500: return dinput5->poll();
            case 0x0700: return dinput7->poll();
            case 0x0800: return dinput8->poll();
        } 
        
        return {};
    }

    auto setKeyboardCallback( KeyCallback* callback ) -> void {
        switch( version ) {
            case 0x0500: dinput5->setKeyboardCallback(callback); break;
            case 0x0700: dinput7->setKeyboardCallback(callback); break;
            case 0x0800: dinput8->setKeyboardCallback(callback); break;
        }
    }
    
};
    
}

#endif