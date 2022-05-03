
Display* pMonitor::display = nullptr;
XRRScreenResources* pMonitor::screens = nullptr;
std::vector<pMonitor::Device> pMonitor::devices;
std::vector<pMonitor::Setting> pMonitor::settings;
pMonitor::Device* pMonitor::activeDevice = nullptr;

static const XRRModeInfo* getModeInfo(const XRRScreenResources* sr, RRMode id) {
    for (int i = 0;  i < sr->nmode;  i++) {
        if (sr->modes[i].id == id)
            return sr->modes + i;
    }

    return nullptr;
}

static double modeRefresh(const XRRModeInfo *mode_info) {
    double rate;
    double vTotal = mode_info->vTotal;

    if (mode_info->modeFlags & RR_DoubleScan)
        vTotal *= 2;

    if (mode_info->modeFlags & RR_Interlace)
        vTotal /= 2;

    if (mode_info->hTotal && vTotal)
        rate = ((double) mode_info->dotClock /
                ((double) mode_info->hTotal * (double) vTotal));
    else
        rate = 0;

    return rate;
}

auto pMonitor::getCurrentRefreshRate() -> float {
    const XRRModeInfo* mode;

    if (!devices.size())
        fetchDisplays();

    if (!display)
        return 0.0;

    if (activeDevice) {
        mode = getModeInfo(screens, activeDevice->activeMode);
        return (float) modeRefresh(mode);
    }

    for(auto& device : devices) {

        if (XRRGetOutputPrimary(display, DefaultRootWindow(display)) == device.xid) {

            for (unsigned j = 0; j < device.outInfo->nmode; j++) {

                if (device.outInfo->modes[j] == device.originalMode) {
                    mode = getModeInfo(screens, device.outInfo->modes[j]);
                    return (float) modeRefresh(mode);
                }
            }
        }
    }

    // fallback (not working reliable)
    auto screen = DefaultScreen(display);
    ::Window root = RootWindow(display, screen);

    XRRScreenConfiguration* conf = XRRGetScreenInfo(display, root);
    return (float)XRRConfigCurrentRate(conf);
}

auto pMonitor::getCurrentResolution() -> Size {
    const XRRModeInfo* mode;

    if (!devices.size())
        fetchDisplays();

    if (!display)
        return {0,0};

    if (activeDevice) {
        mode = getModeInfo(screens, activeDevice->activeMode);
        return { mode->width, mode->height };
    }

    return {0,0};
}

auto pMonitor::connect() -> bool {
    if (!display) {
        display = XOpenDisplay(NULL);

        if (!display)
            return false;
    }

    if (!screens)
        screens = XRRGetScreenResources(display, DefaultRootWindow(display));

    return screens != nullptr;
}

auto pMonitor::disconnect() -> void {
    if (screens)
        XRRFreeScreenResources(screens);

    if (display)
        XCloseDisplay(display);

    screens = nullptr;
    display = nullptr;
}

auto pMonitor::fetchDisplays() -> void {

    if (activeDevice || !connect())
        return;

    devices.clear();
    CRC32 crc32;

    for (unsigned i = 0; i < screens->noutput; i++) {
        XRROutputInfo* outInfo = XRRGetOutputInfo(display, screens, screens->outputs[i]);

        if (outInfo && outInfo->connection == RR_Connected) {

            crc32.init();

            crc32.calc((uint8_t*) outInfo->name, outInfo->nameLen);

            std::string devName = outInfo->name;

            XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screens, outInfo->crtc);

            if (crtcInfo)
                devices.push_back({crc32.value(), devName, i, outInfo,
                                   crtcInfo->mode, crtcInfo->mode, screens->outputs[i], crtcInfo->x, crtcInfo->y});
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

    if (!connect())
        return;

    CRC32 crc32;

    settings.clear();

    settings.push_back({ 0, "-", device, 0 });

    for (unsigned j = 0; j < device->outInfo->nmode; j++) {

        RRMode rrModeId = device->outInfo->modes[j];

        const XRRModeInfo* mode = getModeInfo( screens, rrModeId );

        if (!mode || (mode->modeFlags & RR_Interlace) )
            continue;

        if (rrModeId == device->xid) {
      //      device->activeMode = mode->id;
        }

        double refresh = modeRefresh(mode);

        std::string name = mode->name;

        name += "@" + String::convertDoubleToString( refresh, 2 ) + "Hz";

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

        settings.push_back({ crc32.value(), name, device, mode->id });
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

    if (!connect())
        return false;

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

    auto screen = DefaultScreen (display);

    activeDevice->activeMode = setting->rrMode;

    unsigned screenWidth = 0;
    unsigned screenHeight = 0;
    int screenWidthMM;
    int screenHeightMM;

    for(auto& device : devices) {
        const XRRModeInfo* _mode = getModeInfo( screens, device.activeMode  );

        if ((device.x + _mode->width) > screenWidth)
            screenWidth = device.x + _mode->width;

        if((device.y + _mode->height) > screenHeight)
            screenHeight = device.y + _mode->height;
    }

    if (screenWidth == 0 || screenHeight == 0) {
        activeDevice->activeMode = activeDevice->originalMode;
        return false;
    }

    if (screenWidth != DisplayWidth (display, screen) || screenHeight != DisplayHeight (display, screen)) {
        double dpi = (25.4 * (double)DisplayHeight (display, screen)) / (double)DisplayHeightMM(display, screen);

        screenWidthMM = (int)((25.4 * (double)screenWidth) / dpi);
        screenHeightMM = (int)((25.4 * (double)screenHeight) / dpi);
    } else {
        screenWidthMM = DisplayWidthMM (display, screen);
        screenHeightMM = DisplayHeightMM (display, screen);
    }

    //printf ("screen %d: %dx%d %dx%d mm \n", screen, screenWidth, screenHeight, screenWidthMM, screenHeightMM);

    Status status;

    for(auto& device : devices) { // disable
        status = XRRSetCrtcConfig (display, screens, device.outInfo->crtc, CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);

        if (status != RRSetConfigSuccess) {
            resetSetting();
            return false;
        }
    }

    // set overall screen size
    XRRSetScreenSize(display, DefaultRootWindow(display), screenWidth, screenHeight, screenWidthMM, screenHeightMM);

    for(auto& device : devices) {
        XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screens, device.outInfo->crtc);

        status = XRRSetCrtcConfig(display, screens, device.outInfo->crtc,
                                         CurrentTime, device.x, device.y, device.activeMode, crtcInfo->rotation,
                                         &screens->outputs[device.pos], 1);

        if (status != RRSetConfigSuccess) {
            resetSetting();
            return false;
        }
    }
    return true;
}

auto pMonitor::resetSetting() -> bool {

    if (!activeDevice)
        return false;

    activeDevice->activeMode = activeDevice->originalMode;

    activeDevice = nullptr;

    auto screen = DefaultScreen (display);

    Status status;

    for(auto& device : devices) { // disable
        status = XRRSetCrtcConfig (display, screens, device.outInfo->crtc, CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
    }

    XRRSetScreenSize (display, DefaultRootWindow(display),
                      DisplayWidth (display, screen),
                      DisplayHeight (display, screen),
                      DisplayWidthMM (display, screen),
                      DisplayHeightMM (display, screen));

    for(auto& device : devices) {
        XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screens, device.outInfo->crtc);

        status = XRRSetCrtcConfig(display, screens, device.outInfo->crtc,
                                         CurrentTime, device.x, device.y, device.originalMode,
                                         crtcInfo->rotation,
                                         &screens->outputs[device.pos], 1);
    }

    return true;
}
