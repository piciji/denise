
#include <Cocoa/Cocoa.h>
#include <IOKit/hid/IOHIDManager.h>
#if(__MAC_OS_X_VERSION_MAX_ALLOWED >= 101500)
    #import <IOKit/hidsystem/IOHIDLib.h>
#endif

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
    
struct Iokit : Input {
    CocoaMouse mouse;
    IokitKeyboard keyboard;
    bool useCocoa = false;
    
    auto init( uintptr_t handle ) -> bool {
        term();
        
        if (!keyboard.init(useCocoa))
            return false;
        
        mouse.init( handle, useCocoa );
                
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
    
    auto setKeyboardCallback( KeyCallback* callback ) -> void {
        keyboard.keyCallback = callback;
    }
    
    auto mIsAcquired() -> bool {
        return mouse.mIsAcquired();
    }
        
    auto joypad() -> IokitJoypad* {
        return IokitJoypad::getInstance();
    }
    
    auto sentUIKeyPresses(bool keyDown, uint16_t keyCode) -> void {
        if (useCocoa)
            keyboard.sentUIKeyPresses(keyDown, keyCode);
    }
    
    auto sentUIMousePresses(bool keyDown, Button button) -> void {
        if (useCocoa)
            mouse.sentUIPresses(keyDown, button);
    }
    
    auto sentUIMouseMovement(int deltaX, int deltaY) -> void {
        if (useCocoa)
            mouse.sentUIMovement(deltaX, deltaY);
    }
    
    Iokit(bool useCocoa = false) {
        this->useCocoa = useCocoa;
    }
    
    ~Iokit() {
        term();
    }
};
    
}
