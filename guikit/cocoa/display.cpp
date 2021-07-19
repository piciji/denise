
#include <IOKit/graphics/IOGraphicsLib.h>
#include <ApplicationServices/ApplicationServices.h>


#define UUID_SIZE 37

#ifdef UNDOCUMENTED_RETINA_SUPPORT

typedef union {
    uint8_t rawData[0xDC];
    struct {
        uint32_t mode;
        uint32_t flags;        // 0x4
        uint32_t width;        // 0x8
        uint32_t height;    // 0xC
        uint32_t depth;        // 0x10
        uint32_t dc2[42];
        uint16_t dc3;
        uint16_t freq;        // 0xBC
        uint32_t dc4[4];
        float density;        // 0xD0
    } derived;
} modes_D4;

extern "C"
{
    void CGSGetCurrentDisplayMode(CGDirectDisplayID display, int* modeNum);
    void CGSConfigureDisplayMode(CGDisplayConfigRef config, CGDirectDisplayID display, int modeNum);
    void CGSGetNumberOfDisplayModes(CGDirectDisplayID display, int* nModes);
    void CGSGetDisplayModeDescriptionOfLength(CGDirectDisplayID display, int idx, modes_D4* mode, int length);
};

void CopyAllDisplayModes(CGDirectDisplayID display, modes_D4** modes, int* cnt) {
    int nModes;
    CGSGetNumberOfDisplayModes(display, &nModes);
    
    if(nModes)
        *cnt = nModes;
    
    if(!modes)
        return;
    
    *modes = new modes_D4[nModes];
    
    for(int i=0; i < nModes; i++)
        CGSGetDisplayModeDescriptionOfLength(display, i, &(*modes)[i], 0xD4);
}
#endif

namespace GUIKIT {
    
std::vector<pMonitor::Device> pMonitor::devices;
std::vector<pMonitor::Setting> pMonitor::settings;
pMonitor::Device* pMonitor::activeDevice = nullptr;

auto pMonitor::fetchDisplays() -> void {

    devices.clear();
    CRC32 crc32;

    CGDisplayCount count;
    CGGetOnlineDisplayList(INT_MAX, NULL, &count);

    CGDirectDisplayID screens[count];
    CGGetOnlineDisplayList(INT_MAX, screens, &count);

    for (int i = 0; i < count; i++) {
        CGDirectDisplayID screen = screens[i];
        
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(screen);

        char screenUUID[UUID_SIZE];
        CFStringGetCString(CFUUIDCreateString(kCFAllocatorDefault, CGDisplayCreateUUIDFromDisplayID(screen)), screenUUID, sizeof(screenUUID), kCFStringEncodingUTF8);

        std::string name( screenUUID );
        
        NSDictionary* deviceInfo = (__bridge NSDictionary *)IODisplayCreateInfoDictionary(CGDisplayIOServicePort(screen), kIODisplayOnlyPreferredName);
        
        NSDictionary* localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];
        
        if([localizedNames count] > 0) {
            auto _title = [localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]];
            
            name = [_title UTF8String];
        }
        
        crc32.init();
        crc32.calc( (uint8_t*)&screenUUID[0], UUID_SIZE );
        
        for(auto& device : devices) {
            if (device.ident == name) {
                name += "_" + std::to_string(i);
                break;
            }
        }

        devices.push_back({crc32.value(), name, screen, mode});
    }
}

auto pMonitor::getDisplays() -> std::vector<Monitor::Property> {

    if (!devices.size())
        fetchDisplays();

    std::vector<Monitor::Property> results;

    for(auto& device : devices)
        results.push_back({device.id, device.ident});

    return results;
}

auto pMonitor::fetchSettings( Device* device ) -> void {

#ifndef UNDOCUMENTED_RETINA_SUPPORT
    for(auto& setting : settings)
        if (setting.mode != 0)
            CGDisplayModeRelease( setting.mode );
#endif
        
    settings.clear();
    CRC32 crc32;

    settings.push_back({ 0, "-", 0, device });

#ifndef UNDOCUMENTED_RETINA_SUPPORT
  //  CFStringRef keys[1] = { kCGDisplayShowDuplicateLowResolutionModes };
  //  CFBooleanRef values[1] = { kCFBooleanTrue };
        
  //  CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, (const void**) keys, (const void**) values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );

    CFArrayRef modes = CGDisplayCopyAllDisplayModes( device->displayId, NULL );
    
    for (CFIndex i = 0; i < CFArrayGetCount(modes); i++) {
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
        int32_t modeId = CGDisplayModeGetIODisplayModeID(mode);
        
        size_t _width = CGDisplayModeGetWidth(mode);
        size_t _height = CGDisplayModeGetHeight(mode);
        size_t _pixelwidth = CGDisplayModeGetPixelWidth(mode);
        size_t _pixelheight = CGDisplayModeGetPixelHeight(mode);
        uint32_t _depth = 4;
        
        //CFStringRef format = CGDisplayModeCopyPixelEncoding(mode);
        //if (CFStringCompare(format, CFSTR(IO16BitDirectPixels), 0) == 0)
        //    continue;
        
        bool scaled = (_width != _pixelwidth) || (_height != _pixelheight);

        uint32_t _flags = CGDisplayModeGetIOFlags(mode);
            
        if (_flags & kDisplayModeInterlacedFlag) {
            CGDisplayModeRelease(mode);
            continue;
        }
        
        std::string freq = "";
        double _freq = CGDisplayModeGetRefreshRate(mode);
        if (_freq)
            freq = String::convertDoubleToString(_freq, 2) + " Hz";
            
#else
    int modeCount;
    modes_D4* modes;
    CopyAllDisplayModes(device->displayId, &modes, &modeCount);
        
    for (int i = 0; i < modeCount; i++) {
        modes_D4 mode = modes[i];
        
        if (mode.derived.flags & kDisplayModeInterlacedFlag)
            continue;
        
        uint32_t _width = mode.derived.width;
        uint32_t _height = mode.derived.height;
        uint16_t _freq = mode.derived.freq;
        uint32_t _depth = mode.derived.depth;
        bool scaled = mode.derived.density == 2.0;

        std::string freq = "";
        if (_freq)
            freq = std::to_string(_freq) + " Hz";
#endif
            
        if (_width == 1 && _height == 1) {
#ifndef UNDOCUMENTED_RETINA_SUPPORT
            CGDisplayModeRelease(mode);
#endif
            continue;
        }
            
        std::string width = std::to_string(_width);
        std::string height = std::to_string(_height);
        
        std::string name = width + "x" + height;
            
        if(_depth != 4)
            name += " " + std::to_string(_depth) + "bpp ";
            
        if (scaled)
            name += " scaled";
            
        if (freq != "")
            name += " @" + freq;
            
        crc32.init();
        crc32.calc( (uint8_t*)name.c_str(), name.size() );

        bool found = false;
        for(auto& setting : settings) {
            if (setting.ident == name) {
                found = true;
                break;
            }
        }
        
        if (found) {
#ifndef UNDOCUMENTED_RETINA_SUPPORT
            CGDisplayModeRelease(mode);
#endif
            continue;
        }

#ifdef UNDOCUMENTED_RETINA_SUPPORT
    settings.push_back( {crc32.value(), name, i, device} );
#else
    settings.push_back( {(uint32_t)modeId, name, mode, device} );
#endif
        
    }
    #ifdef UNDOCUMENTED_RETINA_SUPPORT
        delete[] modes;
    #endif
}

auto pMonitor::getSettings( unsigned displayId ) -> std::vector<Monitor::Property> {
    if (!devices.size()) {
        fetchDisplays();

        if (!devices.size())
            return {};
    }

    Device* device = &devices[0];
    for(auto& _device : devices) {
        if (_device.id == displayId) {
            device = &_device;
            break;
        }
    }

    if (!settings.size() || (settings[0].parentDevice != device) )
        fetchSettings( device );

    std::vector<Monitor::Property> results;

    for(auto& setting : settings)
        results.push_back({setting.id, setting.ident});

    return results;
}

auto pMonitor::setSetting( unsigned displayId, unsigned settingId ) -> bool {

    if (!devices.size()) {
        fetchDisplays();

        if (!devices.size())
            return false;
    }

    activeDevice = &devices[0];
    for(auto& _device : devices) {
        if (_device.id == displayId) {
            activeDevice = &_device;
            break;
        }
    }

    if (!settings.size() || (settings[0].parentDevice != activeDevice) )
        fetchSettings( activeDevice );

    Setting* setting = nullptr;
    for(auto& _setting : settings) {
        if (_setting.id == settingId) {
            setting = &_setting;
            break;
        }
    }

    if (!setting)
        return false;

    CGDisplayConfigRef configRef;
    CGBeginDisplayConfiguration(&configRef);
    
#ifdef UNDOCUMENTED_RETINA_SUPPORT
    CGSConfigureDisplayMode(configRef, activeDevice->displayId, setting->mode);
#else
    CGConfigureDisplayWithDisplayMode(configRef, activeDevice->displayId, setting->mode, NULL);
#endif
    
    CGCompleteDisplayConfiguration(configRef, kCGConfigureForAppOnly);
    return true;
}

auto pMonitor::resetSetting() -> bool {

    if (!activeDevice)
        return false;

    CGDisplayConfigRef configRef;
    CGBeginDisplayConfiguration(&configRef);
    
    CGConfigureDisplayWithDisplayMode(configRef, activeDevice->displayId, activeDevice->originalMode, NULL);

    CGCompleteDisplayConfiguration(configRef, kCGConfigurePermanently);
    
    activeDevice = nullptr;

    return true;
}

}
