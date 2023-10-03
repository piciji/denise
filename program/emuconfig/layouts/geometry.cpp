
MonitorResolutionLayout::MonitorResolutionLayout() : displaySettings(true) {
    append(active, {0u, 0u}, 10 );
    append(display, {0u, 0u}, 10 );
    append(displaySettings, {0u, 0u} );
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
    active.setForegroundColor( 0xff4500 );
}

CropLayout::Type1::Type1() {
    append( cropOff, {0u, 0u}, 10 );
    append( cropMonitor, {0u, 0u}, 10 );
    append( cropAuto, {0u, 0u}, 10 );
    append( cropSemiAuto, {0u, 0u} );
}

CropLayout::Type2::Type2() {
    append( cropFree1, {0u, 0u}, 10 );
    append( cropFree2, {0u, 0u}, 10 );
    append( cropFree3, {0u, 0u} );
}

CropLayout::Type3::Type3() {
    append( cropFree4, {0u, 0u}, 10 );
    append( cropFree5, {0u, 0u}, 10 );
    append( cropFree6, {0u, 0u} );
}

CropLayout::Hotkey::Hotkey() {
    append( label, {0u, 0u}, 10 );
    append( cropOff, {0u, 0u}, 10 );
    append( cropMonitor, {0u, 0u}, 10 );
    append( cropAuto, {0u, 0u}, 10 );
    append( cropSemiAuto, {0u, 0u}, 10 );
    append( cropFree1, {0u, 0u}, 10 );
    append( cropFree2, {0u, 0u}, 10 );
    append( cropFree3, {0u, 0u}, 10 );
    append( cropFree4, {0u, 0u}, 10 );
    append( cropFree5, {0u, 0u}, 10 );
    append( cropFree6, {0u, 0u} );

    setAlignment(0.5);
}

CropLayout::CropLayout() :
cropLeft("px"),
cropRight("px"),
cropTop("px"),
cropBottom("px")
{
    append( type1, {0u, 0u}, 5 );
    append( type2, {0u, 0u}, 5 );
    append( type3, {0u, 0u}, 5 );
    append( aspectCorrect, {0u, 0u}, 5 );
    append( cropLeft, {~0u, 0u}, 5 );
    append( cropRight, {~0u, 0u}, 5 );
    append( cropTop, {~0u, 0u}, 5 );
    append( cropBottom, {~0u, 0u}, 10 );

    cropLeft.slider.setLength(101);
    cropRight.slider.setLength(101);
    cropTop.slider.setLength(101);
    cropBottom.slider.setLength(101);

    GUIKIT::RadioBox::setGroup( type1.cropOff, type1.cropMonitor, type1.cropAuto, type1.cropSemiAuto,
                                type2.cropFree1, type2.cropFree2, type2.cropFree3,
                                type3.cropFree4, type3.cropFree5, type3.cropFree6);

    append( hotkey, {0u, 0u} );

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

RatioLayout::RatioLayout() {
    append(label, {0u, 0u}, 10);
    append(window, {0u, 0u}, 5);
    append(tv, {0u, 0u}, 5);
    append(native, {0u, 0u}, 20);
    append(integerScaling, {0u, 0u});

    GUIKIT::RadioBox::setGroup( window, tv, native );

    setAlignment(0.5);
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

GeometryLayout::GeometryLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);

	append(cropLayout, {~0u, 0u}, 10);
    append(ratioLayout, {~0u, 0u}, 10);
    append(monitorResolutionLayout, {~0u, 0u});

    typedef Emulator::Interface::CropType CropType;

    cropLayout.type1.cropOff.onActivate = [this]() {
        updateCrop("crop_type", (unsigned)CropType::Off);

		updateVisibillity();
	};

    cropLayout.type1.cropMonitor.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Monitor);

		updateVisibillity();
	};

    cropLayout.type1.cropAuto.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Auto);

		updateVisibillity();
	};

    cropLayout.type1.cropSemiAuto.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::SemiAuto);

        updateBorderSlider();
		updateVisibillity();
	};

    cropLayout.type2.cropFree1.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free);

        updateBorderSlider();
		updateVisibillity();
	};

    cropLayout.type2.cropFree2.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 1);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type2.cropFree3.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 2);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type3.cropFree4.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 3);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type3.cropFree5.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 4);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type3.cropFree6.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 5);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.aspectCorrect.onToggle = [this](bool checked) {
        updateCrop("crop_aspect_correct", checked);
	};

    cropLayout.cropLeft.slider.onChange = [this](unsigned position) {
        updateCrop("crop_left", position);

        cropLayout.cropLeft.value.setText( std::to_string( position ) + " px" );
	};

    cropLayout.cropRight.slider.onChange = [this](unsigned position) {
        updateCrop("crop_right", position);

        cropLayout.cropRight.value.setText( std::to_string( position ) + " px" );
	};

    cropLayout.cropTop.slider.onChange = [this](unsigned position) {
        updateCrop("crop_top", position);

        cropLayout.cropTop.value.setText( std::to_string( position ) + " px" );
	};

    cropLayout.cropBottom.slider.onChange = [this](unsigned position) {
        updateCrop("crop_bottom", position);

        cropLayout.cropBottom.value.setText( std::to_string( position ) + " px" );
	};

    cropLayout.hotkey.cropOff.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(0, checked);
    };
    cropLayout.hotkey.cropMonitor.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(1, checked);
    };
    cropLayout.hotkey.cropAuto.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(2, checked);
    };
    cropLayout.hotkey.cropSemiAuto.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(3, checked);
    };
    cropLayout.hotkey.cropFree1.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(4, checked);
    };
    cropLayout.hotkey.cropFree2.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(5, checked);
    };
    cropLayout.hotkey.cropFree3.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(6, checked);
    };
    cropLayout.hotkey.cropFree4.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(7, checked);
    };
    cropLayout.hotkey.cropFree5.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(8, checked);
    };
    cropLayout.hotkey.cropFree6.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(9, checked);
    };

    ratioLayout.window.onActivate = [this]() {
        emuThread->lock();
        _settings->set<int>("aspect_mode", 0);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.tv.onActivate = [this]() {
        emuThread->lock();
        _settings->set<int>("aspect_mode", 1);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.native.onActivate = [this]() {
        emuThread->lock();
        _settings->set<int>("aspect_mode", 2);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.integerScaling.onToggle = [this](bool checked) {
        emuThread->lock();
        _settings->set<bool>("integer_scaling", checked);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    monitorResolutionLayout.display.onChange = [this]() {
        emuThread->lock();
        auto displayId = monitorResolutionLayout.display.userData();

        monitorResolutionLayout.displaySettings.reset();

        for( auto& resolution : GUIKIT::Monitor::getSettings( displayId ) )
            monitorResolutionLayout.displaySettings.append( resolution.name, resolution.id );

        _settings->set<unsigned>("fullscreen_display", (unsigned)monitorResolutionLayout.display.userData() );

        _settings->set<unsigned>("fullscreen_setting", 0 );

        program->updateFullscreenSetting();

        emuThread->unlock();
        monitorResolutionLayout.synchronizeLayout();
    };

    monitorResolutionLayout.displaySettings.onChange = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("fullscreen_setting", monitorResolutionLayout.displaySettings.userData() );

        program->updateFullscreenSetting();
        emuThread->unlock();
    };

    monitorResolutionLayout.active.onToggle = [this](bool checked) {
        emuThread->lock();
        _settings->set<bool>("fullscreen_setting_active", checked);

        program->updateFullscreenSetting();

        monitorResolutionLayout.display.setEnabled( checked );
        monitorResolutionLayout.displaySettings.setEnabled( checked );
        emuThread->unlock();
    };

    for( auto& display : GUIKIT::Monitor::getDisplays() )
        monitorResolutionLayout.display.append(display.name, display.id);

    loadSettings();
}

auto GeometryLayout::updateBorderHotkeyUsage(unsigned bit, bool checked) -> void {
    unsigned state = _settings->get<unsigned>( "border_hotkey", 1 | 2 | 4 | 8 | 0x10 );

    if (checked)
        state |= 1 << bit;
    else
        state &= ~(1 << bit);

    _settings->set<unsigned>( "border_hotkey", state );
}

auto GeometryLayout::updateCrop(std::string property, unsigned value) -> void {
    program->setCrop(emulator, property, value);

    emuThread->lockVideo();
    if (emuThread->enabled && (activeEmulator == emulator) )
        emuThread->updateBorder = true;
    else
        program->updateCrop( emulator );

    emuThread->unlockVideo();
}

auto GeometryLayout::updateVisibillity() -> void {
	auto val = _settings->get<unsigned>( "crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 9u});

    cropLayout.cropLeft.setEnabled( val >= 3 );
    cropLayout.cropRight.setEnabled( val >= 4 );
    cropLayout.cropTop.setEnabled( val >= 4 );
    cropLayout.cropBottom.setEnabled( val >= 4 );

    cropLayout.aspectCorrect.setEnabled( val == 2 || val == 3 );
}

auto GeometryLayout::translate() -> void {

    cropLayout.cropLeft.name.setText( trans->get("left", {},true) );
    cropLayout.cropRight.name.setText( trans->get("right", {},true) );
    cropLayout.cropTop.name.setText( trans->get("up", {},true) );
    cropLayout.cropBottom.name.setText( trans->get("down", {},true) );

    cropLayout.type1.cropOff.setText( trans->get("disabled") + " (0)" );
    cropLayout.type1.cropMonitor.setText( trans->get("monitor") + " (1)" );
    cropLayout.type1.cropAuto.setText( trans->get("crop complete") + " (2)" );
    cropLayout.type1.cropSemiAuto.setText( trans->get("crop all sides equally") + " (3)" );
    cropLayout.type2.cropFree1.setText( trans->get("crop each side manually") + " (4)" );
    cropLayout.type2.cropFree1.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type2.cropFree2.setText( trans->get("crop each side manually") + " (5)" );
    cropLayout.type2.cropFree2.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type2.cropFree3.setText( trans->get("crop each side manually") + " (6)" );
    cropLayout.type2.cropFree3.setTooltip( trans->get("crop free tooltip") );

    cropLayout.type3.cropFree4.setText( trans->get("crop each side manually") + " (7)" );
    cropLayout.type3.cropFree4.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type3.cropFree5.setText( trans->get("crop each side manually") + " (8)" );
    cropLayout.type3.cropFree5.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type3.cropFree6.setText( trans->get("crop each side manually") + " (9)" );
    cropLayout.type3.cropFree6.setTooltip( trans->get("crop free tooltip") );

    cropLayout.hotkey.label.setText( trans->get("switchable by Hotkey", {}, true) );
    cropLayout.hotkey.cropOff.setText( "0" );
    cropLayout.hotkey.cropMonitor.setText( "1" );
    cropLayout.hotkey.cropAuto.setText( "2" );
    cropLayout.hotkey.cropSemiAuto.setText( "3" );
    cropLayout.hotkey.cropFree1.setText( "4" );
    cropLayout.hotkey.cropFree2.setText( "5" );
    cropLayout.hotkey.cropFree3.setText( "6" );

    cropLayout.hotkey.cropFree4.setText( "7" );
    cropLayout.hotkey.cropFree5.setText( "8" );
    cropLayout.hotkey.cropFree6.setText( "9" );

    cropLayout.aspectCorrect.setText( trans->get("maintain display ratio") );
	cropLayout.setText( trans->get("crop border") );

    ratioLayout.setText( trans->getA("scaling") );
    ratioLayout.label.setText( trans->getA("aspect ratio", true) );
    ratioLayout.window.setText( trans->getA("window") );
    ratioLayout.tv.setText( trans->getA("CRT TV") );
    ratioLayout.native.setText( trans->getA("Native") );
    ratioLayout.native.setTooltip( trans->getA("Native tooltip") );
    ratioLayout.integerScaling.setText( trans->getA("integer_scaling") );
    monitorResolutionLayout.active.setTooltip( trans->get("fullscreen switch tooltip") );
    monitorResolutionLayout.setText( trans->get("preselect fullscreen resolution") );
    monitorResolutionLayout.active.setText( trans->get("enable") );
    
    SliderLayout::scale({&cropLayout.cropLeft, &cropLayout.cropRight, &cropLayout.cropTop, &cropLayout.cropBottom}, "100 px");
}

auto GeometryLayout::updateBorderSlider() -> void {
    Emulator::Interface::Crop crop = {0};
    if (program->getCrop(emulator, crop)) {
        cropLayout.cropLeft.slider.setPosition(crop.left);
        cropLayout.cropLeft.value.setText(std::to_string(crop.left) + " px");
        cropLayout.cropRight.slider.setPosition(crop.right);
        cropLayout.cropRight.value.setText(std::to_string(crop.right) + " px");
        cropLayout.cropTop.slider.setPosition(crop.top);
        cropLayout.cropTop.value.setText(std::to_string(crop.top) + " px");
        cropLayout.cropBottom.slider.setPosition(crop.bottom);
        cropLayout.cropBottom.value.setText(std::to_string(crop.bottom) + " px");
    }
}

auto GeometryLayout::loadSettings() -> void {
    typedef Emulator::Interface::CropType CropType;
    auto valCropType = _settings->get<unsigned>("crop_type", (unsigned) CropType::Monitor, {0u, 9u});

    if (valCropType == 1) cropLayout.type1.cropMonitor.setChecked();
    else if (valCropType == 2) cropLayout.type1.cropAuto.setChecked();
    else if (valCropType == 3) cropLayout.type1.cropSemiAuto.setChecked();
    else if (valCropType == 4) cropLayout.type2.cropFree1.setChecked();
    else if (valCropType == 5) cropLayout.type2.cropFree2.setChecked();
    else if (valCropType == 6) cropLayout.type2.cropFree3.setChecked();
    else if (valCropType == 7) cropLayout.type3.cropFree4.setChecked();
    else if (valCropType == 8) cropLayout.type3.cropFree5.setChecked();
    else if (valCropType == 9) cropLayout.type3.cropFree6.setChecked();
    else cropLayout.type1.cropOff.setChecked();

    auto valCropAC = _settings->get<bool>("crop_aspect_correct", 0);
    cropLayout.aspectCorrect.setChecked(valCropAC);
    updateBorderSlider();

    unsigned hotkeyState = _settings->get<unsigned>( "border_hotkey", 1 | 2 | 4 | 8 | 0x10 );
    cropLayout.hotkey.cropOff.setChecked( hotkeyState & 1 );
    cropLayout.hotkey.cropMonitor.setChecked( hotkeyState & 2 );
    cropLayout.hotkey.cropAuto.setChecked( hotkeyState & 4 );
    cropLayout.hotkey.cropSemiAuto.setChecked( hotkeyState & 8 );
    cropLayout.hotkey.cropFree1.setChecked( hotkeyState & 0x10 );
    cropLayout.hotkey.cropFree2.setChecked( hotkeyState & 0x20 );
    cropLayout.hotkey.cropFree3.setChecked( hotkeyState & 0x40 );
    cropLayout.hotkey.cropFree4.setChecked( hotkeyState & 0x80 );
    cropLayout.hotkey.cropFree5.setChecked( hotkeyState & 0x100 );
    cropLayout.hotkey.cropFree6.setChecked( hotkeyState & 0x200 );

    bool integerScaling = _settings->get<bool>("integer_scaling", false);
    ratioLayout.integerScaling.setChecked(integerScaling);

    int aspectMode = _settings->get<int>("aspect_mode", 1, {0, 2});
    switch(aspectMode) {
        case 0: ratioLayout.window.setChecked(); break;
        case 1: ratioLayout.tv.setChecked(); break;
        case 2: ratioLayout.native.setChecked(); break;
    }

    monitorResolutionLayout.active.setChecked( _settings->get<bool>("fullscreen_setting_active", false) );

    auto displayId = _settings->get<unsigned>("fullscreen_display", 0 );

    monitorResolutionLayout.display.setSelectionByUserId( displayId );

    monitorResolutionLayout.displaySettings.reset();
    for( auto& resolution : GUIKIT::Monitor::getSettings( displayId ) )
        monitorResolutionLayout.displaySettings.append( resolution.name, resolution.id );

    monitorResolutionLayout.displaySettings.setSelectionByUserId( _settings->get<unsigned>("fullscreen_setting", 0 ) );

    monitorResolutionLayout.display.setEnabled( monitorResolutionLayout.active.checked() );
    monitorResolutionLayout.displaySettings.setEnabled( monitorResolutionLayout.active.checked() );

    updateVisibillity();
}