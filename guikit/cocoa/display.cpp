
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

        int modeId;
        CGSGetCurrentDisplayMode(screen, &modeId);
        modes_D4 mode;
        CGSGetDisplayModeDescriptionOfLength(curScreen, modeId, &mode, 0xD4);

        char screenUUID[UUID_SIZE];
        CFStringGetCString(CFUUIDCreateString(kCFAllocatorDefault, CGDisplayCreateUUIDFromDisplayID(screen)), screenUUID, sizeof(screenUUID), kCFStringEncodingUTF8);

        std::string name( screenUUID );

        io_service_t serv = [self IOServicePortFromCGDisplayID: displayID];
        if (serv != 0) {
            CFDictionaryRef info = IODisplayCreateInfoDictionary(serv, kIODisplayOnlyPreferredName);
            IOObjectRelease(serv);

            CFStringRef _displayName;
            CFDictionaryRef names = CFDictionaryGetValue(info, CFSTR(kDisplayProductName));

            if ( names && CFDictionaryGetValueIfPresent(names, CFSTR("en_US"), (const void**) &_displayName) ) {
                NSString* displayname = [NSString stringWithString: (__bridge NSString *) _displayName];

                name = [foo displayname];
            }

            CFRelease(info);
        }

        crc32.init();
        crc32.calc( &screenUUID[0], UUID_SIZE );

        device.push_back({crc32.value(), name, screen, modeId});
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

    settings.clear();
    CRC32 crc32;

    settings.push_back({ 0, "-", device });

    CGDirectDisplayID screen = device->screen;

    int count;
    modes_D4* modes;
    CopyAllDisplayModes(screen, &modes, &count);

    for (int i = 0; i < count; i++) {
        modes_D4 mode = modes[i];

        std::string width = std::to_string(mode.derived.width);
        std::string height = std::to_string(mode.derived.height);
        std::string freq = "";
        std::string depth = "";
        std::string scaling = "";

        if (mode.derived.freq)
            freq = std::to_string(mode.derived.freq);

        if (mode.derived.depth != 4)
            depth = std::to_string( mode.derived.depth ) + " bpp";


        if (mode.derived.density == 2.0) {
            scaling = "scaled";
        }

        std::string name = width + "x" + height;

        if (depth != "")
            name += " " + depth;

        if (scaling != "")
            name += " " + scaling;

        if (freq)
            name += " @" + freq + "Hz";

        crc32.init();
        crc32.calc( (uint8_t*)name.c_str(), name.size() );

        settings.push_back( {crc32.value(), name, i, device} );
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

    CGSConfigureDisplayMode(configRef, activeDevice->displayId, setting->mode);
    return true;
}

auto pMonitor::resetSetting() -> bool {

    if (!activeDevice)
        return false;

    CGDisplayConfigRef configRef;
    CGBeginDisplayConfiguration(&configRef);

    CGSConfigureDisplayMode(configRef, activeDevice->displayId, activeDevice->originalMode);

    activeDevice = nullptr;

    return true;
}