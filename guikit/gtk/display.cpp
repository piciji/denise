
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

    if (!connect())
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

            devices.push_back({crc32.value(), devName, i, outInfo, crtcInfo->mode, ~0u, screens->outputs[i]});
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

        const XRRModeInfo* mode = getModeInfo( screens, device->outInfo->modes[j] );

        if (!mode || (mode->modeFlags & RR_Interlace) )
            continue;

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

    XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screens, activeDevice->outInfo->crtc);

    Status status = XRRSetCrtcConfig(display, screens, activeDevice->outInfo->crtc,
        CurrentTime, crtcInfo->x, crtcInfo->y, setting->rrMode, crtcInfo->rotation,
        &screens->outputs[activeDevice->pos], 1);

    activeDevice->activeMode = setting->rrMode;

    return status == RRSetConfigSuccess;
}

auto pMonitor::resetSetting() -> bool {

    if (!activeDevice)
        return false;

    XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screens, activeDevice->outInfo->crtc);

    Status status = XRRSetCrtcConfig(display, screens, activeDevice->outInfo->crtc,
        CurrentTime, crtcInfo->x, crtcInfo->y, activeDevice->originalMode, crtcInfo->rotation,
        &screens->outputs[activeDevice->pos], 1);

    activeDevice = nullptr;

    return status == RRSetConfigSuccess;
}
