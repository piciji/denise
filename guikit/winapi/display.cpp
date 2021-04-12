
std::vector<DISPLAY_DEVICE> pDisplay::devices;
std::vector<DEVMODE> pDisplay::deviceSettings;

auto pDisplay::getDisplays() -> std::vector<Display::Property> {

    unsigned i = 0;
    std::vector<Display::Property> results;

    DISPLAY_DEVICE device;
    ZeroMemory(&device, sizeof(device));
    device.cb = sizeof(DISPLAY_DEVICE);

    while( EnumDisplayDevices(NULL, i, &device, 0 ) ) {
        devices.push_back( device );

        std::string devStr = utf8_t(device.DeviceString);

        std::string devName = utf8_t(device.DeviceName);

        devStr += " " + devName;

        results.push_back({i++, devStr});
    }

    return results;
}

auto pDisplay::getResolutions( unsigned displayId ) -> std::vector<Display::Property> {

    unsigned i = 0;
    std::vector<Display::Property> results;
    DEVMODE devSetting;

    if (displayId >= devices.size())
        return {};

    DISPLAY_DEVICE* device = &devices[displayId];

    ZeroMemory(&devSetting, sizeof(devSetting));
    devSetting.dmSize = sizeof(DEVMODE);

    while( EnumDisplaySettings( device->DeviceName, i, &devSetting ) ) {
        deviceSettings.push_back( devSetting );

        std::string name = std::to_string( devSetting.dmPelsWidth );

        name += " x " + std::to_string( devSetting.dmPelsHeight );

        name += " " + std::to_string( devSetting.dmDisplayFrequency ) + "Hz ";

        //name += " " + std::to_string( devSetting.dmBitsPerPel );

        if (devSetting.dmDisplayFlags & DM_INTERLACED)
            name += " i";
        else
            name += " p";

        results.push_back({i, name});

        i++;
    }

    return results;
}

auto pDisplay::setResolution( unsigned displayId, unsigned resolutionId ) -> bool {

    return false;
}
