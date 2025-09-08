
#include "../../tools/crc32.h"
#include "../../tools/tools.h"

struct IokitJoypad {
    
    IOHIDManagerRef hidman = NULL;
    
    static auto getInstance() -> IokitJoypad* {
        
        if (instance == nullptr) {
            instance = new IokitJoypad;
        }
        return instance;
    }
    
    struct JoypadInput {
        IOHIDElementCookie cookie;
        IOHIDElementRef elementRef;

        int32_t min;
        int32_t max;

        int32_t minReport;
        int32_t maxReport;
    };
    
    struct Joypad {
        Hid::Joypad* hid = nullptr;
        
        IOHIDDeviceRef deviceRef;
        uint32_t usage;
        uint32_t usagePage;
                
        std::vector<JoypadInput> axes;
        std::vector<JoypadInput> hats;
        std::vector<JoypadInput> buttons;
    };
    
    std::vector<Joypad> joypads;
    
    auto init() -> bool {

        bool error = false;
        CFArrayRef array = NULL;
        
        const void* vals[] = {
            (void*) createDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick, error),
            (void*) createDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad, error),
            (void*) createDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController, error),
        };
        
        const auto countElements = std::extent< decltype(vals) >::value;        
        
        if(!error) {
            array = CFArrayCreate(kCFAllocatorDefault, vals, countElements, &kCFTypeArrayCallBacks);
        }

        for (unsigned i = 0; i < countElements; i++) {            
            if (vals[i]) {
                CFRelease((CFTypeRef) vals[i]);
            }
        }
        
        if (!array)
            return false;

        hidman = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        
        if (hidman != NULL) {
            error = !configHIDManager(array);
        }
        CFRelease(array);

        return !error;
    }
    
    auto createDictionary(const uint32_t page, const uint32_t usage, bool& error ) -> CFDictionaryRef {
        
        CFDictionaryRef retval = NULL;
        CFNumberRef pageNumRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
        CFNumberRef usageNumRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
        
        const void* keys[2] = {(void*) CFSTR(kIOHIDDeviceUsagePageKey), (void*) CFSTR(kIOHIDDeviceUsageKey)};
        const void* vals[2] = {(void*) pageNumRef, (void*) usageNumRef};

        if (pageNumRef && usageNumRef) {
            retval = CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        }

        if (pageNumRef)
            CFRelease(pageNumRef);

        if (usageNumRef)
            CFRelease(usageNumRef);

        if (!retval)
            error = true;

        return retval;
    }
    
    auto configHIDManager(CFArrayRef matchingArray) -> bool {
        
        CFRunLoopRef runloop = CFRunLoopGetCurrent();

        if (IOHIDManagerOpen(hidman, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
            return false;
        }

        IOHIDManagerSetDeviceMatchingMultiple(hidman, matchingArray);
        IOHIDManagerRegisterDeviceMatchingCallback(hidman, IokitJoypad::addCallback, NULL );
        IOHIDManagerScheduleWithRunLoop(hidman, runloop, CFSTR("DRV_JOYPAD"));

        while (CFRunLoopRunInMode(CFSTR("DRV_JOYPAD"), 0, TRUE) == kCFRunLoopRunHandledSource) {}

        return true;
    }
    
    auto _add(void* ctx, IOReturn res, void* sender, IOHIDDeviceRef ioHIDDeviceObject) -> void {
        
        Joypad jp;

        if (res != kIOReturnSuccess)
            return;

        if (alreadyAdded(ioHIDDeviceObject))
            return;                     

        if (!getDeviceInfo(ioHIDDeviceObject, jp)) {
            return;
        }

        joypads.emplace_back( jp );
        
        IOHIDDeviceRegisterRemovalCallback(ioHIDDeviceObject, IokitJoypad::removeCallback, (void*)jp.hid);
        IOHIDDeviceScheduleWithRunLoop(ioHIDDeviceObject, CFRunLoopGetCurrent(), CFSTR("DRV_JOYPAD"));
    }

    auto alreadyAdded(IOHIDDeviceRef deviceRef) -> bool {
        
        for(auto& joypad : joypads) {
            if (joypad.deviceRef == deviceRef)
                return true;
        }
        
        return false;
    }

    auto getDeviceInfo(IOHIDDeviceRef hidDevice, Joypad& jp) -> bool {
        
        CFTypeRef refCF = NULL;
        CFArrayRef array = NULL;

        refCF = IOHIDDeviceGetProperty(hidDevice, CFSTR(kIOHIDPrimaryUsagePageKey));
        if (refCF) {
            CFNumberGetValue((CFNumberRef)refCF, kCFNumberSInt32Type, &jp.usagePage);
        }
        if (jp.usagePage != kHIDPage_GenericDesktop) {
            return false;
        }

        refCF = IOHIDDeviceGetProperty(hidDevice, CFSTR(kIOHIDPrimaryUsageKey));
        if (refCF) {
            CFNumberGetValue((CFNumberRef)refCF, kCFNumberSInt32Type, &jp.usage);
        }

        if ((jp.usage != kHIDUsage_GD_Joystick &&
                jp.usage != kHIDUsage_GD_GamePad &&
                jp.usage != kHIDUsage_GD_MultiAxisController)) {
            return false;
        }

        jp.deviceRef = hidDevice;
        jp.hid = new Hid::Joypad;
        std::string displayname = "";

        refCF = IOHIDDeviceGetProperty(hidDevice, CFSTR(kIOHIDProductKey));
        if (!refCF) {
            refCF = IOHIDDeviceGetProperty(hidDevice, CFSTR(kIOHIDManufacturerKey));
        }
        
        if(refCF) {
            displayname = [(id)refCF UTF8String];
        }
        
        if(displayname.empty()) displayname = "Joypad";			
        jp.hid->name = uniqueDeviceName(joypads, displayname);

        
        std::string serial = jp.hid->name;

        refCF = IOHIDDeviceGetProperty(hidDevice, CFSTR(kIOHIDSerialNumberKey));
        if (refCF) {
            serial = [(id)refCF UTF8String];
        }

        CRC32 crc32((uint8_t*)serial.c_str(), serial.size());

        jp.hid->id = uniqueDeviceId( joypads, crc32.value() );
        
        array = IOHIDDeviceCopyMatchingElements(hidDevice, NULL, kIOHIDOptionsTypeNone);
        if (array) {            
            const CFRange range = { 0, CFArrayGetCount(array) };
            CFArrayApplyFunction(array, range, IokitJoypad::addElementCallback, &jp);
            
            CFRelease(array);
        }

        return true;
    }
    
    auto _remove(Joypad& jp) -> void {
                
        if (jp.deviceRef) {
            IOHIDDeviceUnscheduleFromRunLoop(jp.deviceRef, CFRunLoopGetCurrent(), CFSTR("DRV_JOYPAD"));
            jp.deviceRef = NULL;
        }        
        
        if (jp.hid) {
            delete jp.hid;
            jp.hid = nullptr;
        }
    }
    
    auto _addElement(const void* value, void* ctx) -> void {
        
        Joypad* jp = (Joypad*) ctx;
        JoypadInput* input = nullptr;
        
        unsigned axes = jp->hid->axes().inputs.size();
        unsigned buttons = jp->hid->buttons().inputs.size();
		unsigned hats = jp->hid->hats().inputs.size();
        
        IOHIDElementRef elementRef = (IOHIDElementRef) value;
        const CFTypeID elementTypeID = elementRef ? CFGetTypeID(elementRef) : 0;
        
        if (!elementRef || (elementTypeID != IOHIDElementGetTypeID()) )
            return;

        const IOHIDElementCookie cookie = IOHIDElementGetCookie(elementRef);
        const uint32_t usagePage = IOHIDElementGetUsagePage(elementRef);
        const uint32_t usage = IOHIDElementGetUsage(elementRef);
        
        std::function<void ()> addAxis;
        
        addAxis = [&]() -> void {
            if (!elementAlreadyAdded(cookie, jp->axes)) {
                jp->axes.push_back({});
                input = &jp->axes.back();
                std::string axis = std::to_string(axes);
                
                switch(axes) {
                    case 0: axis = "X"; break;
                    case 1: axis = "Y"; break;
                    case 2: axis = "Z"; break;
                    case 3: axis = "Z|Rot"; break;
                    case 4: axis = "X|Rot"; break;
                    case 5: axis = "Y|Rot"; break;
                }				

                jp->hid->axes().append(axis);
                axes++;
            }
        };

        switch (IOHIDElementGetType(elementRef)) {
            
            case kIOHIDElementTypeInput_Misc:
            case kIOHIDElementTypeInput_Button:
            case kIOHIDElementTypeInput_Axis: {
                
                switch (usagePage) {
                    case kHIDPage_GenericDesktop:
                        switch (usage) {
                            case kHIDUsage_GD_X:
                            case kHIDUsage_GD_Y:
                            case kHIDUsage_GD_Z:
                            case kHIDUsage_GD_Rx:
                            case kHIDUsage_GD_Ry:
                            case kHIDUsage_GD_Rz:
                            case kHIDUsage_GD_Slider:
                            case kHIDUsage_GD_Dial:
                            case kHIDUsage_GD_Wheel:
                                addAxis(); break;

                            case kHIDUsage_GD_Hatswitch:
                                if (!elementAlreadyAdded(cookie, jp->hats)) {
                                    jp->hats.push_back({});
                                    input = &jp->hats.back();
                                    auto hat = hats / 2;
                                    jp->hid->hats().append(std::to_string(hat) + ".X");
                                    jp->hid->hats().append(std::to_string(hat) + ".Y");
                                    hats += 2;
                                }
                                break;
                            case kHIDUsage_GD_DPadUp:
                            case kHIDUsage_GD_DPadDown:
                            case kHIDUsage_GD_DPadRight:
                            case kHIDUsage_GD_DPadLeft:
                            case kHIDUsage_GD_Start:
                            case kHIDUsage_GD_Select:
                            case kHIDUsage_GD_SystemMainMenu:
                                if (!elementAlreadyAdded(cookie, jp->buttons)) {
                                    jp->buttons.push_back({});
                                    input = &jp->buttons.back();
                                    jp->hid->buttons().append( std::to_string(buttons) );
                                    buttons++;
                                }
                                break;
                        }
                        break;

                    case kHIDPage_Simulation:
                        switch (usage) {
                            case kHIDUsage_Sim_Rudder:
                            case kHIDUsage_Sim_Throttle:
                                addAxis(); break;
                            default:
                                break;
                        }
                        break;

                    case kHIDPage_Button:
                    case kHIDPage_Consumer:
                        if (!elementAlreadyAdded(cookie, jp->buttons)) {
                            jp->buttons.push_back({});
                            input = &jp->buttons.back();
                            jp->hid->buttons().append( std::to_string(buttons) );
                            buttons++;
                        }
                        break;

                    default:
                        break;
                }
            } break;

            case kIOHIDElementTypeCollection: {
                CFArrayRef array = IOHIDElementGetChildren(elementRef);
                
                if (array) {
                    CFArrayApplyFunction(array, { 0, CFArrayGetCount(array) }, IokitJoypad::addElementCallback, jp);
                }
            } break;

            default:
                break;
        }
        
        if (input) {
            input->cookie = cookie;
            input->elementRef = elementRef;
            input->minReport = input->min = (SInt32) IOHIDElementGetLogicalMin(elementRef);
            input->maxReport = input->max = (SInt32) IOHIDElementGetLogicalMax(elementRef);
        }
    }

    auto elementAlreadyAdded(const IOHIDElementCookie cookie, std::vector<JoypadInput>& inputs) -> bool {
        
        for(auto& input : inputs ) {
            if (input.cookie == cookie)
                return true;
        }
        return false;
    }
    
    auto term() -> void {
        for(auto& jp : joypads) {
            _remove(jp);
        }
        joypads.clear();
        joypads.shrink_to_fit();

        if (hidman) {
            IOHIDManagerUnscheduleFromRunLoop(hidman, CFRunLoopGetCurrent(), CFSTR("DRV_JOYPAD"));
            IOHIDManagerClose(hidman, kIOHIDOptionsTypeNone);
            CFRelease(hidman);
            hidman = NULL;
        }
    }
    
    auto poll(std::vector<Hid::Device*>& devices) -> void {

        while (CFRunLoopRunInMode(CFSTR("DRV_JOYPAD"), 0, TRUE) == kCFRunLoopRunHandledSource) {}

        for (auto& jp : joypads) {

            for(auto& input : jp.hid->buttons().inputs) {
                auto value = (unsigned) getElementState(jp, jp.buttons[input.id] );
                input.setValue( value > 1 ? 1 : value );
            }
            
            for(auto& input : jp.hid->axes().inputs) {
                JoypadInput& element = jp.axes[input.id];
                int32_t value = getElementState(jp, element );
                        
                signed range = element.maxReport - element.minReport;
                value = (value - element.minReport) * 65535ll / range - 32767;
                
                input.setValue( sclamp<16>( value) );
            }
            
            auto& hats = jp.hid->hats();

            for (unsigned hat = 0; hat < hats.inputs.size() / 2; hat++ ) {
                JoypadInput& element = jp.hats[hat];
                unsigned x = hat * 2;

                unsigned range = (element.max - element.min + 1);
                signed value = getElementState(jp, element) - element.min;
                
                if (range == 4) {
                    value *= 2;
                } else if (range != 8) {
                    value = -1;
                }
                
                switch (value) {
                    case 0: //up
                        hats.inputs[x + 0].setValue(0);
                        hats.inputs[x + 1].setValue(-32768);                                
                        break;
                    case 1: //right + up
                        hats.inputs[x + 0].setValue(32767);
                        hats.inputs[x + 1].setValue(-32768);
                        break;
                    case 2: //right
                        hats.inputs[x + 0].setValue(32767);
                        hats.inputs[x + 1].setValue(0);
                        break;
                    case 3: //right + down
                        hats.inputs[x + 0].setValue(32767);
                        hats.inputs[x + 1].setValue(32767);
                        break;
                    case 4: //down
                        hats.inputs[x + 0].setValue(0);
                        hats.inputs[x + 1].setValue(32767);
                        break;
                    case 5: //left + down
                        hats.inputs[x + 0].setValue(-32768);
                        hats.inputs[x + 1].setValue(32767);
                        break;
                    case 6: //left
                        hats.inputs[x + 0].setValue(-32768);
                        hats.inputs[x + 1].setValue(0);
                        break;
                    case 7: //left + up
                        hats.inputs[x + 0].setValue(-32768);
                        hats.inputs[x + 1].setValue(-32768);
                        break;
                    default: //center
                        hats.inputs[x + 0].setValue(0);
                        hats.inputs[x + 1].setValue(0);
                        break;
                }
            }
            devices.push_back(jp.hid);
        }
    }
    
    auto getElementState(Joypad& jp, JoypadInput& input) -> int32_t {
        int32_t value = 0;

        IOHIDValueRef valueRef;
        
        if (IOHIDDeviceGetValue(jp.deviceRef, input.elementRef, &valueRef) == kIOReturnSuccess) {
            value = (SInt32) IOHIDValueGetIntegerValue(valueRef);

            if (value < input.minReport)
                input.minReport = value;
            
            if (value > input.maxReport)
                input.maxReport = value;
        }
        return value;
    }

private:
    
    static auto addCallback(void* ctx, IOReturn res, void* sender, IOHIDDeviceRef ioHIDDeviceObject) -> void {
        
        instance->_add(ctx, res, sender, ioHIDDeviceObject);
    }
    
    static auto removeCallback(void* ctx, IOReturn res, void* sender) -> void {
        
        Hid::Joypad* hid = (Hid::Joypad*)ctx;
        
        unsigned pos = 0;
        for(auto& joypad : instance->joypads) {
            
            if (joypad.hid == hid) {
                joypad.deviceRef = NULL;
                
                instance->_remove( joypad );

                instance->joypads.erase(instance->joypads.begin() + pos);
                instance->joypads.shrink_to_fit();
                break;
            }
            pos++;
        }
    }
    
    static auto addElementCallback(const void* value, void* ctx) -> void {
        
        instance->_addElement(value, ctx);
    }
    
    //singleton only
    static IokitJoypad* instance;

    IokitJoypad() {}

    IokitJoypad(const IokitJoypad& source) {}

    IokitJoypad(IokitJoypad&& source) {}
};

IokitJoypad* IokitJoypad::instance = nullptr;
