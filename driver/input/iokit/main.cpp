
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
    std::string joypadDriver = "";
    CocoaMouse mouse;
    IokitKeyboard keyboard;
    
    auto init( uintptr_t handle ) -> bool {
        term();
        
        if (!keyboard.init())
            return false;
        
        mouse.init();
        
#ifdef DRV_SDLINPUT
        if (joypadDriver == "sdl")
            if (!sdl->init()) {}
                
#endif 

        if (joypadDriver == "")
            if(!joypad()->init()) {}
        
        return true;
    }
    
    auto term() -> void {

        if(joypadDriver == "")
            joypad()->term();
    }
    
    auto poll() -> std::vector<Hid::Device*> {
        std::vector<Hid::Device*> devices;
        
        keyboard.poll(devices);
        mouse.poll(devices);
        
#ifdef DRV_SDLINPUT
        if (joypadDriver == "sdl") sdl->pollJoypad(devices);
#endif

        if (joypadDriver == "")
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
    
    Iokit(std::string joypadDriver = "") {
        this->joypadDriver = joypadDriver;
        
#ifdef DRV_SDLINPUT
        if (this->joypadDriver == "sdl") sdl = new SdlInput();
#endif

    }
    
    auto joypad() -> IokitJoypad* {
        return IokitJoypad::getInstance();
    }
    
    ~Iokit() {
#ifdef DRV_SDLINPUT
        if (joypadDriver == "sdl") if(sdl) delete sdl, sdl = nullptr;
#endif

        term();
    }
};
    
}