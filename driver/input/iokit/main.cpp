
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <functional>

#include "../../tools/hid.h"
#include "../../tools/chronos.h"

namespace DRIVER {

#include "keyNames.cpp"
#include "keyboard.cpp"
#include "mouse.cpp"
#include "joypad.cpp"
    
#ifdef DRV_SDLINPUT
    SdlInput* sdl;
#endif

    
struct Iokit : Input {
    CocoaMouse mouse;
    IokitKeyboard keyboard;
    
    auto init( uintptr_t handle ) -> bool {
        term();
        
        if (!keyboard.init())
            return false;
        
        mouse.init();
                
        if(!joypad()->init()) {}
        
        return true;
    }
    
    auto term() -> void {

        joypad()->term();
    }
    
    auto poll() -> std::vector<Hid::Device*> {
        std::vector<Hid::Device*> devices;
        
        keyboard.poll(devices);
        mouse.poll(devices);
        
        joypad()->poll(devices);
        
        return devices;
    }
    
    auto mAcquire() -> void {
        mouse.mAcquire();
    }
    
    auto mUnacquire() -> void {
        mouse.mUnacquire();
    }
    
    auto mIsAcquired() -> bool {
        return mouse.mIsAcquired();
    }
        
    auto joypad() -> IokitJoypad* {
        return IokitJoypad::getInstance();
    }
    
    ~Iokit() {
        term();
    }
};
    
}