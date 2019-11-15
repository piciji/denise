
struct CocoaMouse {
    
    Hid::Mouse* hidMouse = nullptr;
    
    auto init() -> void {
        term();
        
        hidMouse = new Hid::Mouse;
        hidMouse->id = 1;
        
        hidMouse->axes().append("X");
        hidMouse->axes().append("Y");
        
        hidMouse->buttons().append("Left");
        hidMouse->buttons().append("Right");
        hidMouse->buttons().append("Middle");
        hidMouse->buttons().append("Up");
        hidMouse->buttons().append("Down");
    }
    
    auto term() -> void {
        if(hidMouse) delete hidMouse, hidMouse = nullptr;
    }
    
    auto poll(std::vector<Hid::Device*>& devices) -> void {
        
        int32_t deltaX;
        int32_t deltaY;
        CGGetLastMouseDelta(&deltaX, &deltaY);
        
        hidMouse->axes().inputs[0].setValue( deltaX );
        hidMouse->axes().inputs[1].setValue( deltaY );
        
        hidMouse->axes().timeStamp = Chronos::getTimestampInMicroseconds();
        
        NSUInteger buttonState = [NSEvent pressedMouseButtons];
        
        for (auto& input : hidMouse->buttons().inputs) {
            input.setValue( (buttonState & (1 << input.id)) != 0 );
        }
        
        devices.push_back(hidMouse);
    }
    
    auto mAcquire() -> void {
        if(mIsAcquired()) return;
        CGAssociateMouseAndMouseCursorPosition(false);
        CGDisplayHideCursor(0);
    }
    
    auto mUnacquire() -> void {
        if(mIsAcquired()) {
            CGAssociateMouseAndMouseCursorPosition(true);
            CGDisplayShowCursor(0);
        }
    }
    
    auto mIsAcquired() -> bool {
        return !CGCursorIsVisible();
    }
    
    ~CocoaMouse() {
        term();
    }
};
