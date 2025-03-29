
struct CocoaMouse {
    
    Hid::Mouse* hidMouse = nullptr;
    bool mouseAcquired;
    NSWindow* window = nullptr;
    std::atomic<int> deltaX = 0;
    std::atomic<int> deltaY = 0;
    std::atomic<unsigned> buttonState = 0;
    bool useCocoa = false;
    
    auto init(uintptr_t handle, bool useCocoa) -> void {
        this->useCocoa = useCocoa;
        window = (NSWindow*)handle;
        term();
        
        hidMouse = new Hid::Mouse;
        hidMouse->id = 1;
        mouseAcquired = false;
        
        hidMouse->axes().append("X");
        hidMouse->axes().append("Y");
        
        hidMouse->buttons().append("Left");
        hidMouse->buttons().append("Right");
        hidMouse->buttons().append("Middle");
        hidMouse->buttons().append("Back");
        hidMouse->buttons().append("Forward");
    }
    
    auto term() -> void {
        if(hidMouse) {
            delete hidMouse;
            hidMouse = nullptr;
        }
    }
    
    auto poll(std::vector<Hid::Device*>& devices) -> void {
        int _deltaX, _deltaY;
        
        if (!useCocoa) {
            CGGetLastMouseDelta(&_deltaX, &_deltaY);
            buttonState = [NSEvent pressedMouseButtons];
            
            hidMouse->axes().inputs[0].setValue( _deltaX );
            hidMouse->axes().inputs[1].setValue( _deltaY );
        } else {
            _deltaX = deltaX;
            deltaX = 0;
            _deltaY = deltaY;
            deltaY = 0;
            hidMouse->axes().inputs[0].setValue( _deltaX );
            hidMouse->axes().inputs[1].setValue( _deltaY );
        }
        
        unsigned _bState = buttonState;
        for (auto& input : hidMouse->buttons().inputs) {
            input.setValue( (_bState & (1 << input.id)) != 0 );
        }
        
        if (_bState & ((unsigned)Input::Button::Forward | (unsigned)Input::Button::Back))
            buttonState &= ~((unsigned)Input::Button::Forward | (unsigned)Input::Button::Back);
        
        devices.push_back(hidMouse);
    }
    
    auto sentUIPresses(bool keyDown, Input::Button button) -> void {
        if (keyDown)    buttonState |= (unsigned)button;
        else {
            if ((unsigned)button & ((unsigned)Input::Button::Forward | (unsigned)Input::Button::Back));
            else
                buttonState &= ~(unsigned)button;
        }
    }
    
    auto sentUIMovement(int _deltaX, int _deltaY) -> void {
        deltaX += _deltaX;
        deltaY += _deltaY;
    }
    
    auto mAcquire() -> void {
        if(mIsAcquired()) return;
        CGAssociateMouseAndMouseCursorPosition(false);
        CGDisplayHideCursor(0);
        
        NSRect frame = [window frame];
        NSRect screen = [[NSScreen mainScreen] frame];
        CGWarpMouseCursorPosition( {CGRectGetMidX(frame), (screen.size.height - frame.origin.y) - frame.size.height * 0.5 } );

        mouseAcquired = true;
    }
    
    auto mUnacquire() -> void {
        if(mIsAcquired()) {
            CGAssociateMouseAndMouseCursorPosition(true);
            CGDisplayShowCursor(0);
            [NSCursor unhide];
            mouseAcquired = false;
        }
    }
    
    auto mIsAcquired() -> bool {
        return mouseAcquired /*!CGCursorIsVisible()*/; // deprecated and not working reliable
    }
    
    ~CocoaMouse() {
        term();
    }
};
