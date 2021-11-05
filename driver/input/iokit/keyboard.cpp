
struct IokitKeyboard {
    
    Hid::Keyboard* hidKeyboard = nullptr;
    bool keysPressed[256] = {0};
    OsxKeyNames keyNames;
    
    auto init() -> bool {
        term();
        
        hidKeyboard = new Hid::Keyboard;
        hidKeyboard->id = 0;
        
        if (!initHID())
            return false;
        
        for(unsigned keycode = 0; keycode <= 0xff; keycode++) {
            if (keycode < 3 || keycode > 231) {
                hidKeyboard->buttons().append( "" );
                continue;
            }

            std::string ident = keyNames.translate(keycode);
            
            hidKeyboard->buttons().append( ident, getKeyCode( ident, (uint8_t)keycode ) );
            
            //printf("%s %d \n", ident.c_str(), keycode);
        }
        
        return true;
    }
    
    auto getKeyCode(std::string ident, uint8_t keycode ) -> Hid::Key {
        // keycodes are device indepandant and describe the position on keyboard.
        // for easier use we assign the keycodes to human readable enums of the uk layout.
        auto key = Hid::Input::getKeyCode( ident );
        
        if (key != Hid::Key::Unknown)
            return key;
        
        if (keycode == 40) return Hid::Key::Return;
        if (keycode == 44) return Hid::Key::Space;
        if (keycode == 42) return Hid::Key::Backspace;
        if (keycode == 43) return Hid::Key::Tab;
        if (keycode == 41) return Hid::Key::Esc;
        if (keycode == 57) return Hid::Key::CapsLock;
        
        if (keycode == 100) return Hid::Key::Grave;
        if (keycode == 45) return Hid::Key::Minus;
        if (keycode == 46) return Hid::Key::Equal;
        if (keycode == 48) return Hid::Key::ClosedSquareBracket;
        if (keycode == 49) return Hid::Key::NumberSign;
        if (keycode == 53) return Hid::Key::Backslash;
        if (keycode == 54) return Hid::Key::Comma;
        if (keycode == 55) return Hid::Key::Period;
        if (keycode == 56) return Hid::Key::Slash;
        //if (keycode == 135) return Hid::Key::Menu;
        if (keycode == 52) return Hid::Key::Apostrophe;
        if (keycode == 51) return Hid::Key::Semicolon;
        if (keycode == 47) return Hid::Key::OpenSquareBracket;
        if (keycode == 3) return Hid::Key::Fn;
        
        if (keycode == 73) return Hid::Key::Insert;
        if (keycode == 74) return Hid::Key::Home;
        if (keycode == 75) return Hid::Key::Prior;
        if (keycode == 76) return Hid::Key::Delete;
        if (keycode == 77) return Hid::Key::End;
        if (keycode == 78) return Hid::Key::Next;
        //if (keycode == 107) return Hid::Key::Print;
        //if (keycode == 78) return Hid::Key::ScrollLock;
        //if (keycode == 127) return Hid::Key::Pause;
        
        if (keycode == 81) return Hid::Key::CursorDown;
        if (keycode == 80) return Hid::Key::CursorLeft;
        if (keycode == 79) return Hid::Key::CursorRight;
        if (keycode == 82) return Hid::Key::CursorUp;
        
        if (keycode == 225) return Hid::Key::ShiftLeft;
        if (keycode == 229) return Hid::Key::ShiftRight;
        if (keycode == 226) return Hid::Key::AltLeft;
        if (keycode == 230) return Hid::Key::AltRight;
        if (keycode == 224) return Hid::Key::ControlLeft;
        if (keycode == 228) return Hid::Key::ControlRight;
        if (keycode == 227) return Hid::Key::SuperLeft;
        if (keycode == 231) return Hid::Key::SuperRight;
        
        if (keycode == 98) return Hid::Key::NumPad0;
        if (keycode == 89) return Hid::Key::NumPad1;
        if (keycode == 90) return Hid::Key::NumPad2;
        if (keycode == 91) return Hid::Key::NumPad3;
        if (keycode == 92) return Hid::Key::NumPad4;
        if (keycode == 93) return Hid::Key::NumPad5;
        if (keycode == 94) return Hid::Key::NumPad6;
        if (keycode == 95) return Hid::Key::NumPad7;
        if (keycode == 96) return Hid::Key::NumPad8;
        if (keycode == 97) return Hid::Key::NumPad9;
        if (keycode == 99) return Hid::Key::NumComma;
        if (keycode == 84) return Hid::Key::NumDivide;
        if (keycode == 85) return Hid::Key::NumMultiply;
        if (keycode == 86) return Hid::Key::NumSubtract;
        if (keycode == 87) return Hid::Key::NumAdd;
        if (keycode == 88) return Hid::Key::NumEnter;
        if (keycode == 83) return Hid::Key::NumLock;
        if (keycode == 134) return Hid::Key::NumEqual;
        
        // on french keyboard M <> ;
        if (keycode == 16) return Hid::Key::Semicolon;

        if (keycode == 30) return Hid::Key::D1;
        if (keycode == 31) return Hid::Key::D2;
        if (keycode == 32) return Hid::Key::D3;
        if (keycode == 33) return Hid::Key::D4;
        if (keycode == 34) return Hid::Key::D5;
        if (keycode == 35) return Hid::Key::D6;
        if (keycode == 36) return Hid::Key::D7;
        if (keycode == 37) return Hid::Key::D8;
        if (keycode == 38) return Hid::Key::D9;
        if (keycode == 39) return Hid::Key::D0;

        
        return Hid::Key::Unknown;
    }
    
    auto term() -> void {
        if(hidKeyboard) delete hidKeyboard, hidKeyboard = nullptr;
    }
    
    auto poll(std::vector<Hid::Device*>& devices) -> void {

        for (auto& input : hidKeyboard->buttons().inputs)
            input.setValue( keysPressed[input.id] );
        
        devices.push_back(hidKeyboard);
    }
    
    auto initHID() -> bool {
        IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault,kIOHIDOptionsTypeNone);
        
        if (hidManager == NULL)
            return false;
        
        CFMutableDictionaryRef keyboard = deviceMatchingDictionary(0x01, 6);
        CFMutableDictionaryRef keypad = deviceMatchingDictionary(0x01, 7);
        
        if (!keyboard || !keypad)
            return false;
        
        CFMutableDictionaryRef matchesList[] = { keyboard, keypad };
        
        CFArrayRef matches = CFArrayCreate(kCFAllocatorDefault, (const void **)matchesList, 2, NULL);
        IOHIDManagerSetDeviceMatchingMultiple(hidManager, matches);
        
        IOHIDManagerRegisterInputValueCallback(hidManager, IokitKeyboard::HIDKeyboardCallback, this);
        
        IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
        
        if (IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
            #define V10_15 1700
            
            #if(__MAC_OS_X_VERSION_MAX_ALLOWED >= 101500)
            if (NSAppKitVersionNumber >= V10_15) {
                if (kIOHIDAccessTypeGranted != IOHIDCheckAccess(kIOHIDRequestTypeListenEvent)) {
                    
                    NSURL* uri = [NSURL URLWithString:[NSString stringWithFormat:@"x-apple.systempreferences:com.apple.preference.security?%@", @"Privacy_ListenEvent"]];
                
                    [[NSWorkspace sharedWorkspace] openURL:uri];
                }
            }
            #endif
        }
        
        return true;
    }
    
    auto deviceMatchingDictionary(UInt32 usagePage, UInt32 usage) -> CFMutableDictionaryRef {
        CFMutableDictionaryRef ret = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                               &kCFTypeDictionaryKeyCallBacks,
                                                               &kCFTypeDictionaryValueCallBacks);
        if (!ret) return NULL;
        
        CFNumberRef pageNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage );
        
        if (!pageNumberRef) {
            CFRelease(ret);
            return NULL;
        }
        
        CFDictionarySetValue(ret, CFSTR(kIOHIDDeviceUsagePageKey), pageNumberRef);
        CFRelease(pageNumberRef);
        
        CFNumberRef usageNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
        
        if (!usageNumberRef) {
            CFRelease(ret);
            return NULL;
        }
        
        CFDictionarySetValue(ret, CFSTR(kIOHIDDeviceUsageKey), usageNumberRef);
        CFRelease(usageNumberRef);
        
        return ret;
    }
    
    auto _HIDKeyboardCallback(IOReturn result, void* sender, IOHIDValueRef value) -> void {
        IOHIDElementRef elem = IOHIDValueGetElement(value);
        
        //if (IOHIDElementGetUsagePage(elem) != 0x07)
         //   return;
        
        uint32_t scancode = IOHIDElementGetUsage(elem);
        
        if (scancode < 3 || scancode > 231) return;
        
        long pressed = IOHIDValueGetIntegerValue(value);
        
        keysPressed[scancode] = pressed ? true : false;
    }

    static auto HIDKeyboardCallback(void* context, IOReturn result, void* sender, IOHIDValueRef value) -> void {
        auto instance = (IokitKeyboard*)context;
        
        if(instance)
            instance->_HIDKeyboardCallback(result, sender, value);
    }
    
    ~IokitKeyboard() {
        term();
    }

};
