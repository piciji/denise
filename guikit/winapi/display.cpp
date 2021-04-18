
std::vector<pDisplay::Device> pDisplay::devices;
std::vector<pDisplay::Setting> pDisplay::settings;
pDisplay::Device* pDisplay::activeDevice = nullptr;

auto pDisplay::fetchDisplays() -> void {
    unsigned i = 0;
    CRC32 crc32;
    devices.clear();

    DISPLAY_DEVICE device;
    ZeroMemory(&device, sizeof(device));
    device.cb = sizeof(DISPLAY_DEVICE);

    while( EnumDisplayDevices(NULL, i++, &device, 0 ) ) {

        if (device.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            continue;

        std::string devStr = utf8_t(device.DeviceString);

        std::string devName = utf8_t(device.DeviceName);

        devStr += " " + devName;

        crc32.init();

        crc32.calc( (uint8_t*)devName.c_str(), devName.size() );

        DEVMODE originalSetting;
        ZeroMemory(&originalSetting, sizeof(originalSetting));
        originalSetting.dmSize = sizeof(DEVMODE);

        EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &originalSetting);

        devices.push_back({device, originalSetting, crc32.value(), devStr});
    }
}

auto pDisplay::getDisplays() -> std::vector<Display::Property> {

    if (!devices.size())
        fetchDisplays();

    std::vector<Display::Property> results;

    for(auto& device : devices)
        results.push_back({device.id, device.ident});

    return results;
}

auto pDisplay::fetchSettings( Device* device ) -> void {

    unsigned i = 0;
    DEVMODE devSetting;
    settings.clear();

    ZeroMemory(&devSetting, sizeof(devSetting));
    devSetting.dmSize = sizeof(DEVMODE);
    CRC32 crc32;

    settings.push_back({ device, devSetting, 0, "-" });

    while( EnumDisplaySettingsEx( device->displayDevice.DeviceName, i++, &devSetting, 0 ) ) {

        if (devSetting.dmDisplayFlags & DM_INTERLACED)
            continue;
		
		if (devSetting.dmBitsPerPel != 32)
			continue;

        std::string width = std::to_string( devSetting.dmPelsWidth );
        std::string height = std::to_string( devSetting.dmPelsHeight );
        std::string frequency = std::to_string( devSetting.dmDisplayFrequency );

        std::string name = width + "  x  " + height;

        name += "@" + frequency + "Hz";		
		
        crc32.init();

        crc32.calc( (uint8_t*)name.c_str(), name.size() );

        settings.push_back({ device, devSetting, crc32.value(), name });
    }
}

auto pDisplay::getSettings( unsigned displayId ) -> std::vector<Display::Property> {

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

    std::vector<Display::Property> results;

    for(auto& setting : settings)
        results.push_back({setting.id, setting.ident});

    return results;
}

auto pDisplay::setSetting( unsigned displayId, unsigned settingId ) -> bool {

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

    auto result = ChangeDisplaySettingsEx(
        activeDevice->displayDevice.DeviceName,
        &setting->devMode,
        nullptr,
        CDS_FULLSCREEN,
        0
    );

    return result == DISP_CHANGE_SUCCESSFUL;
}

auto pDisplay::resetSetting() -> void {

    if (!activeDevice)
        return;

    ChangeDisplaySettingsEx(
        activeDevice->displayDevice.DeviceName,
        &activeDevice->originalSetting,
        nullptr,
        0,
        0
    );

    activeDevice = nullptr;
}

