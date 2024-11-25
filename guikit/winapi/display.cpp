
std::vector<pMonitor::Device> pMonitor::devices;
std::vector<pMonitor::Setting> pMonitor::settings;
pMonitor::Device* pMonitor::activeDevice = nullptr;
pMonitor::Setting* pMonitor::activeSetting = nullptr;

auto pMonitor::getTimingInfo() -> DwmGetCompositionTimingInfo_t {
    static DwmGetCompositionTimingInfo_t dwmGetCompositionTimingInfo = nullptr;
    static bool loaded = false;

    if (loaded)
        return dwmGetCompositionTimingInfo;
    loaded = true;

    HMODULE hMod = LoadLibraryA("dwmapi.dll");
    if(!hMod)
        return nullptr;

    dwmGetCompositionTimingInfo = (DwmGetCompositionTimingInfo_t)( GetProcAddress(hMod, "DwmGetCompositionTimingInfo"));
    return dwmGetCompositionTimingInfo;
}

auto pMonitor::getCurrentRefreshRate() -> float {
    HRESULT result = S_FALSE;
    float rateInterval = 0.0;

    if (activeDevice && activeSetting) {
        return (float)activeSetting->devMode.dmDisplayFrequency;
    }

    if ((getVersionNew() > Windows7) && (getVersionNew() < Windows11)) {
        DWM_TIMING_INFO timingInfo;
        ZeroMemory(&timingInfo, sizeof(timingInfo));
        timingInfo.cbSize = sizeof(timingInfo);

        DwmGetCompositionTimingInfo_t dwmGetCompositionTimingInfo = getTimingInfo();

        if (dwmGetCompositionTimingInfo)
            result = dwmGetCompositionTimingInfo(NULL, &timingInfo);

        if (result == S_OK) {

            if (timingInfo.rateRefresh.uiDenominator > 0 &&
                timingInfo.rateRefresh.uiNumerator > 0) {

                rateInterval = ((float)timingInfo.rateRefresh.uiDenominator / 1000000.0)
                               * (float)timingInfo.rateRefresh.uiNumerator;

                if (rateInterval < 1.0)
                    rateInterval = rateInterval * 1000000.0;
            }
        }

        if (rateInterval > 0.0)
            return rateInterval;
    }

    DEVMODE devSetting;
    ZeroMemory(&devSetting, sizeof(devSetting));
    devSetting.dmSize = sizeof(DEVMODE);

    EnumDisplaySettingsEx( NULL, ENUM_CURRENT_SETTINGS, &devSetting, 0 );

    auto rate = devSetting.dmDisplayFrequency;

    if (rate == 59)
        rate = 60;
    else if (rate == 49)
        rate = 50;

    return (float)rate;
}

auto pMonitor::fetchDisplays() -> void {

    if (activeDevice)
        return;

    unsigned i = 0;
    CRC32 crc32;
    devices.clear();

    DISPLAY_DEVICE device;
    ZeroMemory(&device, sizeof(device));
    device.cb = sizeof(DISPLAY_DEVICE);

    while( EnumDisplayDevices(NULL, i++, &device, 0 ) ) {

        if (/*!device.StateFlags ||*/ (device.StateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER /*| DISPLAY_DEVICE_MODESPRUNED*/)))
            continue;

        std::string devStr = utf8_t(device.DeviceString);

        std::string devName = utf8_t(device.DeviceName);

        devStr += " " + devName;

        crc32.init();

        crc32.calc( (uint8_t*)devName.c_str(), devName.size() );

        DEVMODE devSetting;
        ZeroMemory(&devSetting, sizeof(devSetting));
        devSetting.dmSize = sizeof(DEVMODE);
        int i = 0;
        while( EnumDisplaySettingsEx( device.DeviceName, i++, &devSetting, 0 ) ) {
            if ((devSetting.dmDisplayFlags & DM_INTERLACED) || (devSetting.dmBitsPerPel != 32))
               continue;

            DEVMODE originalSetting;
            ZeroMemory(&originalSetting, sizeof(originalSetting));
            originalSetting.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &originalSetting);

            devices.push_back({crc32.value(), devStr, device, originalSetting});

            break;
        }
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

    unsigned i = 0;
    DEVMODE devSetting;
    settings.clear();

    ZeroMemory(&devSetting, sizeof(devSetting));
    devSetting.dmSize = sizeof(DEVMODE);
    CRC32 crc32;

    settings.push_back({ 0, "-", device, devSetting });

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

        bool found = false;
        for(auto& setting : settings) {
            if (setting.id == crc32.value()) {
                found = true;
                break;
            }
        }

        if (found)
            continue;

        settings.push_back({ crc32.value(), name, device, devSetting, (float)devSetting.dmDisplayFrequency });
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

auto pMonitor::getRefreshRate( unsigned displayId, unsigned settingId ) -> float {

    if (!devices.size()) {
        fetchDisplays();

        if (!devices.size())
            return 0.0;
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
        return 0.0;

    return (float)setting->devMode.dmDisplayFrequency;
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

    activeSetting = nullptr;
    for(auto& _setting : settings) {
        if (_setting.id == settingId) {
            activeSetting = &_setting;
            break;
        }
    }

    if (!activeSetting)
        return false;

    auto result = ChangeDisplaySettingsEx(
        activeDevice->displayDevice.DeviceName,
        &activeSetting->devMode,
        nullptr,
        CDS_FULLSCREEN,
        0
    );

    bool success = result == DISP_CHANGE_SUCCESSFUL;

    if (success && (activeSetting->rate != 0.0f) && Monitor::onFullscreenRefreshChange)
        Monitor::onFullscreenRefreshChange(activeSetting->rate);

    return success;
}

auto pMonitor::resetSetting() -> void {

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
    activeSetting = nullptr;
}

