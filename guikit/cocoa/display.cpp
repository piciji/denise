
#include <IOKit/graphics/IOGraphicsLib.h>

#define UUID_SIZE 37

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

    for(auto& setting : settings)
        CGDisplayModeRelease( setting.mode );
    
    settings.clear();
    //CRC32 crc32;

    settings.push_back({ 0, "-", 0, device });
    
    CFArrayRef modes = CGDisplayCopyAllDisplayModes( device->displayId, NULL );

    for (CFIndex i = 0; i < CFArrayGetCount(modes); i++) {
        
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
        int32_t modeId = CGDisplayModeGetIODisplayModeID(mode);
        
        size_t _width = CGDisplayModeGetWidth(mode);
        size_t _height = CGDisplayModeGetHeight(mode);
        uint32_t _flags = CGDisplayModeGetIOFlags(mode);
        
        if (_flags & kDisplayModeInterlacedFlag)
            continue;
        
        std::string width = std::to_string(_width);
        std::string height = std::to_string(_height);
        std::string freq = "";
        std::string flags = "";

        double _freq = CGDisplayModeGetRefreshRate(mode);
        if (_freq)
            freq = String::convertDoubleToString(_freq, 2) + " Hz";
            
       // if (_flags)
         //   flags = std::to_string(_flags);
            
        std::string name = width + "x" + height;

        if (freq != "")
            name += " @" + freq;
            
        //if (flags != "")
          //  name += " f:" + flags;

        //crc32.init();
        //crc32.calc( (uint8_t*)name.c_str(), name.size() );

        settings.push_back( {(uint32_t)modeId, name, mode, device} );
    }

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

    if (!devices.size())
        fetchDisplays();

    activeDevice = nullptr;
    for(auto& _device : devices) {
        if (_device.id == displayId) {
            activeDevice = &_device;
            break;
        }
    }

    if (!activeDevice)
        return false;

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
    
    CGConfigureDisplayWithDisplayMode(configRef, activeDevice->displayId, setting->mode, NULL);
    
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
