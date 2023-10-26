
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
    append( cropAutoAspect, {0u, 0u}, 10 );
    append( cropAuto, {0u, 0u} );

    setAlignment(0.5);
}

CropLayout::Type2::Type2() {
    append( cropAllSidesAspect, {0u, 0u}, 10 );
    append( cropAllSides, {0u, 0u} );

    setAlignment(0.5);
}

CropLayout::Type3::Type3() {
    append( cropFree1, {0u, 0u}, 10 );
    append( cropFree2, {0u, 0u}, 10 );
    append( cropFree3, {0u, 0u} );

    setAlignment(0.5);
}

CropLayout::Type4::Type4() {
    append( cropFree4, {0u, 0u}, 10 );
    append( cropFree5, {0u, 0u}, 10 );
    append( cropFree6, {0u, 0u} );

    setAlignment(0.5);
}

CropLayout::Hotkey::Hotkey() {
    append( label, {0u, 0u}, 10 );
    for(int i = 0; i < 12; i++) {
        auto checkBox = new GUIKIT::CheckBox;
        append( *checkBox, {0u, 0u}, i < 11 ? 10 : 0 );
        boxes.push_back(checkBox);
    }
    append(spacer, {~0u, 0u});
    append(reset, {0u, 0u});

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
    append( type4, {0u, 0u}, 5 );
    append( cropLeft, {~0u, 0u}, 5 );
    append( cropRight, {~0u, 0u}, 5 );
    append( cropTop, {~0u, 0u}, 5 );
    append( cropBottom, {~0u, 0u}, 10 );

    cropLeft.slider.setLength(101);
    cropRight.slider.setLength(101);
    cropTop.slider.setLength(101);
    cropBottom.slider.setLength(101);

    GUIKIT::RadioBox::setGroup( type1.cropOff, type1.cropMonitor, type1.cropAutoAspect, type1.cropAuto,
                                type2.cropAllSidesAspect, type2.cropAllSides,
                                type3.cropFree1, type3.cropFree2, type3.cropFree3,
                                type4.cropFree4, type4.cropFree5, type4.cropFree6);

    append( hotkey, {~0u, 0u} );

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

    cropLayout.type1.cropAutoAspect.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::AutoRatio);

        updateVisibillity();
    };

    cropLayout.type1.cropAuto.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Auto);

		updateVisibillity();
	};

    cropLayout.type2.cropAllSidesAspect.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::AllSidesRatio);

        updateBorderSlider();
		updateVisibillity();
	};

    cropLayout.type2.cropAllSides.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::AllSides);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type3.cropFree1.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free);

        updateBorderSlider();
		updateVisibillity();
	};

    cropLayout.type3.cropFree2.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 1);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type3.cropFree3.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 2);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type4.cropFree4.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 3);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type4.cropFree5.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 4);

        updateBorderSlider();
        updateVisibillity();
    };

    cropLayout.type4.cropFree6.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free + 5);

        updateBorderSlider();
        updateVisibillity();
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

    for(int i = 0; i < 12; i++) {
        cropLayout.hotkey.boxes[i]->onToggle = [this, i](bool checked) {
            updateBorderHotkeyUsage(i, checked);
        };
    }

    cropLayout.hotkey.reset.onActivate = [this]() {
        int type = _settings->get<int>("crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 11u});
        if (type < (int)CropType::Free)
            return;
        int offset = type - (int)CropType::Free;

        program->setCrop(emulator, "crop_left", program->getCropDefault(offset, 0));
        program->setCrop(emulator, "crop_right", program->getCropDefault(offset, 1));
        program->setCrop(emulator, "crop_top", program->getCropDefault(offset, 2));
        program->setCrop(emulator, "crop_bottom", program->getCropDefault(offset, 3));
        updateBorderSlider();
        updateCrop("");
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
    unsigned state = _settings->get<unsigned>( "border_hotkey", program->getCropHotkeyDefault() );

    if (checked)
        state |= 1 << bit;
    else
        state &= ~(1 << bit);

    _settings->set<unsigned>( "border_hotkey", state );
}

auto GeometryLayout::updateCrop(std::string property, unsigned value) -> void {
    if (!property.empty())
        program->setCrop(emulator, property, value);

    emuThread->lockVideo();
    if (emuThread->enabled && (activeEmulator == emulator) )
        emuThread->updateBorder = true;
    else
        program->updateCrop( emulator );

    emuThread->unlockVideo();
}

auto GeometryLayout::updateVisibillity() -> void {
	auto val = _settings->get<unsigned>( "crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 11u});

    cropLayout.cropLeft.setEnabled( val >= 4 );
    cropLayout.cropRight.setEnabled( val >= 6 );
    cropLayout.cropTop.setEnabled( val >= 6 );
    cropLayout.cropBottom.setEnabled( val >= 6 );
    cropLayout.hotkey.reset.setEnabled( dynamic_cast<LIBAMI::Interface*>(emulator) && (val == 7 || val == 8 || val == 9) );
}

auto GeometryLayout::translate() -> void {

    cropLayout.cropLeft.name.setText( trans->get("left", {},true) );
    cropLayout.cropRight.name.setText( trans->get("right", {},true) );
    cropLayout.cropTop.name.setText( trans->get("up", {},true) );
    cropLayout.cropBottom.name.setText( trans->get("down", {},true) );

    cropLayout.type1.cropOff.setText( trans->get("disabled") + " (0)" );
    cropLayout.type1.cropMonitor.setText( trans->get("monitor") + " (1)" );
    cropLayout.type1.cropAutoAspect.setText( trans->get("crop complete ratio") + " (2)" );
    cropLayout.type1.cropAutoAspect.setTooltip( trans->get("crop complete ratio tooltip") );
    cropLayout.type1.cropAuto.setText( trans->get("crop complete") + " (3)" );

    cropLayout.type2.cropAllSidesAspect.setText( trans->get("crop all sides equally ratio") + " (4)" );
    cropLayout.type2.cropAllSides.setText( trans->get("crop all sides equally") + " (5)" );

    cropLayout.type3.cropFree1.setText( trans->get("crop each side manually") + " (6)" );
    cropLayout.type3.cropFree1.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type3.cropFree2.setText( trans->get("crop each side manually") + " (7)" );
    cropLayout.type3.cropFree2.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type3.cropFree3.setText( trans->get("crop each side manually") + " (8)" );
    cropLayout.type3.cropFree3.setTooltip( trans->get("crop free tooltip") );

    cropLayout.type4.cropFree4.setText( trans->get("crop each side manually") + " (9)" );
    cropLayout.type4.cropFree4.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type4.cropFree5.setText( trans->get("crop each side manually") + " (10)" );
    cropLayout.type4.cropFree5.setTooltip( trans->get("crop free tooltip") );
    cropLayout.type4.cropFree6.setText( trans->get("crop each side manually") + " (11)" );
    cropLayout.type4.cropFree6.setTooltip( trans->get("crop free tooltip") );

    cropLayout.hotkey.label.setText( trans->get("switchable by Hotkey", {}, true) );
    cropLayout.hotkey.reset.setText( trans->getA("reset") );

    for(int i = 0; i < 12; i++) {
        cropLayout.hotkey.boxes[i]->setText( std::to_string(i) );
    }

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
    auto valCropType = _settings->get<unsigned>("crop_type", (unsigned) CropType::Monitor, {0u, 11u});

    if (valCropType == 1) cropLayout.type1.cropMonitor.setChecked();
    else if (valCropType == 2) cropLayout.type1.cropAutoAspect.setChecked();
    else if (valCropType == 3) cropLayout.type1.cropAuto.setChecked();

    else if (valCropType == 4) cropLayout.type2.cropAllSidesAspect.setChecked();
    else if (valCropType == 5) cropLayout.type2.cropAllSides.setChecked();

    else if (valCropType == 6) cropLayout.type3.cropFree1.setChecked();
    else if (valCropType == 7) cropLayout.type3.cropFree2.setChecked();
    else if (valCropType == 8) cropLayout.type3.cropFree3.setChecked();
    else if (valCropType == 9) cropLayout.type4.cropFree4.setChecked();
    else if (valCropType == 10) cropLayout.type4.cropFree5.setChecked();
    else if (valCropType == 11) cropLayout.type4.cropFree6.setChecked();
    else cropLayout.type1.cropOff.setChecked();

    updateBorderSlider();

    unsigned hotkeyState = _settings->get<unsigned>( "border_hotkey", program->getCropHotkeyDefault() );
    for(int i = 0; i < 12; i++) {
        cropLayout.hotkey.boxes[i]->setChecked( hotkeyState & ( 1 << i ) );
    }

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