
MonitorResolutionLayout::MonitorResolutionLayout() : displaySettings(true) {
    append(active, {0u, 0u}, 10 );
    append(display, {0u, 0u}, 10 );
    append(displaySettings, {0u, 0u}, 20 );
    append(adjustEmuSpeed, {0u, 0u} );
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

RotationLayout::RotationLayout() {
    append(rotation, {0u, 0u}, 20 );
    append(degree0, {0u, 0u}, 10 );
    append(degree90, {0u, 0u}, 10 );
    append(degree180, {0u, 0u}, 10 );
    append(degree270, {0u, 0u} );
    setAlignment(0.5);
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));

    GUIKIT::RadioBox::setGroup(degree0, degree90, degree180, degree270);
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

CropLayout::CropHorizontal::CropHorizontal() :
cropLeft("px"),
cropRight("px") {
    append( cropLeft, {~0u, 0u}, 10 );
    append( cropRight, {~0u, 0u} );

    cropLeft.slider.setLength(101);
    cropRight.slider.setLength(101);
}

CropLayout::CropVertical::CropVertical() :
cropTop("px"),
cropBottom("px") {
    append( cropTop, {~0u, 0u}, 10 );
    append( cropBottom, {~0u, 0u} );

    cropTop.slider.setLength(101);
    cropBottom.slider.setLength(101);
}

CropLayout::CropLayout() {
    append( type1, {0u, 0u}, 5 );
    append( type2, {0u, 0u}, 5 );
    append( type3, {0u, 0u}, 5 );
    append( type4, {0u, 0u}, 5 );

    append( cropHorizontal, {~0u, 0u}, 5 );
    append( cropVertical, {~0u, 0u}, 10 );

    GUIKIT::RadioBox::setGroup( type1.cropOff, type1.cropMonitor, type1.cropAutoAspect, type1.cropAuto,
                                type2.cropAllSidesAspect, type2.cropAllSides,
                                type3.cropFree1, type3.cropFree2, type3.cropFree3,
                                type4.cropFree4, type4.cropFree5, type4.cropFree6);

    append( hotkey, {~0u, 0u} );

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

RatioLayout::Control::Control() {
    append(label, {0u, 0u}, 10);
    append(window, {0u, 0u}, 5);
    append(tv, {0u, 0u}, 5);
    append(native, {0u, 0u}, 5);
    append(nativeFree, {0u, 0u}, 20);
    append(integerScaling, {0u, 0u});

    GUIKIT::RadioBox::setGroup( window, tv, native, nativeFree );

    setAlignment(0.5);

}

RatioLayout::Hotkey::Hotkey() {
    append(label, {0u, 0u}, 10);

    for(int i = 0; i < 4; i++) {
        auto checkBox = new GUIKIT::CheckBox;
        append( *checkBox, {0u, 0u}, i < 3 ? 10 : 0 );
        boxes.push_back(checkBox);
    }

    setAlignment(0.5);
}

RatioLayout::Dimension::Dimension() {
    append(label, {0u, 0u}, 10);
    append(width, {0u, 0u}, 10);
    append(height, {0u, 0u}, 10);
    append(refresh, {0u, 0u}, 10);
    append(apply, {0u, 0u}, 10);
    append(scaleOnEmuSwitch, { 0u, 0u });
    append(spacer, { ~0u, 0u });
    append(cropWindow, {0u, 0u}, 10);    
    append(aspectCorrectResizing, {0u, 0u});

    setAlignment(0.5);
}

RatioLayout::RatioLayout() {
    append(control, {~0u, 0u}, 10);
    append(hotkey, {~0u, 0u}, 10);
    append(dimension, {~0u, 0u});

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

GeometryLayout::GeometryLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);

	append(cropLayout, {~0u, 0u}, 10);
    append(ratioLayout, {~0u, 0u}, 10);
    append(monitorResolutionLayout, {~0u, 0u}, 10);
    append(rotationLayout, {~0u, 0u});

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

    cropLayout.cropHorizontal.cropLeft.slider.onChange = [this](unsigned position) {
        updateCrop("crop_left", position);

        cropLayout.cropHorizontal.cropLeft.value.setText( std::to_string( position ) + " px" );
	};

    cropLayout.cropHorizontal.cropRight.slider.onChange = [this](unsigned position) {
        updateCrop("crop_right", position);

        cropLayout.cropHorizontal.cropRight.value.setText( std::to_string( position ) + " px" );
	};

    cropLayout.cropVertical.cropTop.slider.onChange = [this](unsigned position) {
        updateCrop("crop_top", position);

        cropLayout.cropVertical.cropTop.value.setText( std::to_string( position ) + " px" );
	};

    cropLayout.cropVertical.cropBottom.slider.onChange = [this](unsigned position) {
        updateCrop("crop_bottom", position);

        cropLayout.cropVertical.cropBottom.value.setText( std::to_string( position ) + " px" );
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

        program->setCrop(emulator, "crop_left", program->getCropDefault(emulator, offset, 0));
        program->setCrop(emulator, "crop_right", program->getCropDefault(emulator, offset, 1));
        program->setCrop(emulator, "crop_top", program->getCropDefault(emulator, offset, 2));
        program->setCrop(emulator, "crop_bottom", program->getCropDefault(emulator, offset, 3));
        updateBorderSlider();
        updateCrop("");
    };

    ratioLayout.control.window.onActivate = [this]() {
        emuThread->lock();
        _settings->set<int>("aspect_mode", 0);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.control.tv.onActivate = [this]() {
        emuThread->lock();
        _settings->set<int>("aspect_mode", 1);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.control.native.onActivate = [this]() {
        emuThread->lock();
        _settings->set<int>("aspect_mode", 2);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.control.nativeFree.onActivate = [this]() {
        emuThread->lock();
        _settings->set<int>("aspect_mode", 3);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.control.integerScaling.onToggle = [this](bool checked) {
        emuThread->lock();
        _settings->set<bool>("integer_scaling", checked);
        program->setVideoDimension(this->emulator);
        view->updateViewport();
        emuThread->unlock();
    };

    ratioLayout.dimension.cropWindow.onActivate = [this]() {
        view->adjustToEmu(true);
    };

    ratioLayout.dimension.scaleOnEmuSwitch.onToggle = [&](bool checked) {
        _settings->set<bool>("scale_emu_switch", checked);
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

    for(int i = 0; i < 4; i++) {
        ratioLayout.hotkey.boxes[i]->onToggle = [this, i](bool checked) {
            updateScaleHotkeyUsage(i, checked);
        };
    }

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
        monitorResolutionLayout.adjustEmuSpeed.setEnabled( checked );
        emuThread->unlock();
    };

    monitorResolutionLayout.adjustEmuSpeed.onToggle = [this](bool checked) {
        emuThread->lock();
        _settings->set<bool>("fullscreen_setting_adjust_emu_speed", checked);
        emuThread->unlock();
    };

    rotationLayout.degree0.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("rotation", (unsigned)DRIVER::ROT_0);
        if (activeEmulator == emulator)
            videoDriver->setRotation(DRIVER::ROT_0);
        emuThread->unlock();
    };

    rotationLayout.degree90.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("rotation", (unsigned)DRIVER::ROT_90 );
        if (activeEmulator == emulator)
            videoDriver->setRotation(DRIVER::ROT_90);
        emuThread->unlock();
    };

    rotationLayout.degree180.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("rotation", (unsigned)DRIVER::ROT_180);
        if (activeEmulator == emulator)
            videoDriver->setRotation(DRIVER::ROT_180);
        emuThread->unlock();
    };

    rotationLayout.degree270.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned>("rotation", (unsigned)DRIVER::ROT_270);
        if (activeEmulator == emulator)
            videoDriver->setRotation(DRIVER::ROT_270);
        emuThread->unlock();
    };

    ratioLayout.dimension.width.onChange = [this]() {
        int val = ratioLayout.dimension.width.value();
        if (val < 100) val = 100;
        _settings->set<unsigned>("view_hold_width", val);
    };


    ratioLayout.dimension.height.onChange = [this]() {
        int val = ratioLayout.dimension.height.value();
        if (val < 100) val = 100;
        _settings->set<unsigned>("view_hold_height", val);
    };

    ratioLayout.dimension.refresh.onActivate = [this]() {
        if (view->fullScreen())
            return;

        emuThread->lock();
        auto w = globalSettings->get<unsigned>("screen_width", 800);
        auto h = globalSettings->get<unsigned>("screen_height", 600);

        _settings->set<unsigned>("view_hold_width", w);
        _settings->set<unsigned>("view_hold_height", h);

        ratioLayout.dimension.width.setValue(w);
        ratioLayout.dimension.height.setValue(h);
        emuThread->unlock();
    };

    ratioLayout.dimension.apply.onActivate = [this]() {
        if ((emulator == activeEmulator) && !view->fullScreen()) {
            emuThread->lock();
            view->updateToHoldDimension();
            emuThread->unlock();
        }
    };

    ratioLayout.dimension.aspectCorrectResizing.onToggle = [&](bool checked) {
        emuThread->lock();
        _settings->set<bool>("aspect_correct_resizing", checked);
        if (emulator == activeEmulator)
            view->updateViewport();
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

auto GeometryLayout::updateScaleHotkeyUsage(unsigned bit, bool checked) -> void {
    unsigned state = _settings->get<unsigned>( "scale_hotkey", program->getScaleHotkeyDefault() );

    if (checked)
        state |= 1 << bit;
    else
        state &= ~(1 << bit);

    _settings->set<unsigned>( "scale_hotkey", state );
}

auto GeometryLayout::updateCrop(std::string property, unsigned value) -> void {
    if (!property.empty())
        program->setCrop(emulator, property, value);

    emuThread->lockVideo();
    if (emuThread->enabled && (activeEmulator == emulator) )
        emuThread->updateBorder = true;
    else
        program->updateCrop(emulator);

    emuThread->unlockVideo();
}

auto GeometryLayout::updateVisibillity() -> void {
	auto val = _settings->get<unsigned>( "crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u, 11u});

    cropLayout.cropHorizontal.cropLeft.setEnabled( val >= 4 );
    cropLayout.cropHorizontal.cropRight.setEnabled( val >= 6 );
    cropLayout.cropVertical.cropTop.setEnabled( val >= 6 );
    cropLayout.cropVertical.cropBottom.setEnabled( val >= 6 );
    cropLayout.hotkey.reset.setEnabled( val >= 6 );
}

auto GeometryLayout::translate() -> void {

    rotationLayout.rotation.setText( trans->getA("rotation angle", true) );
    rotationLayout.degree0.setText("0 °");
    rotationLayout.degree90.setText("90 °");
    rotationLayout.degree180.setText("180 °");
    rotationLayout.degree270.setText("270 °");
    rotationLayout.setText( trans->getA("image rotation") );

    cropLayout.cropHorizontal.cropLeft.name.setText( trans->get("left", {},true) );
    cropLayout.cropHorizontal.cropRight.name.setText( trans->get("right", {},true) );
    cropLayout.cropVertical.cropTop.name.setText( trans->get("up", {},true) );
    cropLayout.cropVertical.cropBottom.name.setText( trans->get("down", {},true) );

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
    if (dynamic_cast<LIBAMI::Interface*>(emulator))
        cropLayout.hotkey.reset.setTooltip( trans->getA("reset free border tooltip") );

    for(int i = 0; i < 12; i++) {
        cropLayout.hotkey.boxes[i]->setText( std::to_string(i) );
    }

    for(int i = 0; i < 4; i++) {
        ratioLayout.hotkey.boxes[i]->setText( std::to_string(i) );
    }

	cropLayout.setText( trans->get("crop border") );

    ratioLayout.setText( trans->getA("scaling") );
    ratioLayout.control.label.setText( trans->getA("aspect ratio", true) );
    ratioLayout.control.window.setText( trans->getA("window") + " (0)" );
    ratioLayout.control.tv.setText( trans->getA("CRT TV") + " (1)" );
    ratioLayout.control.native.setText( trans->getA("Native") + " (2)" );
    ratioLayout.control.native.setTooltip( trans->getA("Native tooltip") );
    ratioLayout.control.nativeFree.setText( trans->getA("Native free") + " (3)" );
    ratioLayout.control.integerScaling.setText( trans->getA("integer_scaling") );
    ratioLayout.hotkey.label.setText( trans->getA("switchable by Hotkey", true) );
    ratioLayout.dimension.aspectCorrectResizing.setText(trans->get("lock aspect ratio"));
    ratioLayout.dimension.label.setText(trans->getA("resolution", true));
    ratioLayout.dimension.refresh.setText(trans->getA("refresh"));
    ratioLayout.dimension.apply.setText(trans->getA("apply"));
    ratioLayout.dimension.cropWindow.setText( trans->getA("crop window") );
    ratioLayout.dimension.scaleOnEmuSwitch.setText(trans->getA("changing emulator"));

    monitorResolutionLayout.active.setTooltip( trans->get("fullscreen switch tooltip") );
    monitorResolutionLayout.setText( trans->get("preselect fullscreen resolution") );
    monitorResolutionLayout.active.setText( trans->get("enable") );
    monitorResolutionLayout.adjustEmuSpeed.setText( trans->get("adjust speed to monitor") );
    monitorResolutionLayout.adjustEmuSpeed.setTooltip( trans->get("adjust speed to monitor tooltip") );
    
    SliderLayout::scale({&cropLayout.cropHorizontal.cropLeft, &cropLayout.cropVertical.cropTop}, "100 px");
    SliderLayout::scale({&cropLayout.cropHorizontal.cropRight, &cropLayout.cropVertical.cropBottom}, "100 px");
}

auto GeometryLayout::updateBorderSlider() -> void {
    Emulator::Interface::Crop crop = {0};
    if (program->getCrop(emulator, crop)) {
        cropLayout.cropHorizontal.cropLeft.slider.setPosition(crop.left);
        cropLayout.cropHorizontal.cropLeft.value.setText(std::to_string(crop.left) + " px");
        cropLayout.cropHorizontal.cropRight.slider.setPosition(crop.right);
        cropLayout.cropHorizontal.cropRight.value.setText(std::to_string(crop.right) + " px");
        cropLayout.cropVertical.cropTop.slider.setPosition(crop.top);
        cropLayout.cropVertical.cropTop.value.setText(std::to_string(crop.top) + " px");
        cropLayout.cropVertical.cropBottom.slider.setPosition(crop.bottom);
        cropLayout.cropVertical.cropBottom.value.setText(std::to_string(crop.bottom) + " px");
    }
}

auto GeometryLayout::loadSettings() -> void {
    typedef Emulator::Interface::CropType CropType;

    DRIVER::Rotation rotation = (DRIVER::Rotation)_settings->get<unsigned>("rotation", (unsigned)DRIVER::ROT_0, {0u, 3u});
    setRotation(rotation);

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
    ratioLayout.control.integerScaling.setChecked(integerScaling);

    int aspectMode = _settings->get<int>("aspect_mode", 1, {0, 3});
    switch(aspectMode) {
        case 0: ratioLayout.control.window.setChecked(); break;
        case 1: ratioLayout.control.tv.setChecked(); break;
        case 2: ratioLayout.control.native.setChecked(); break;
        case 3: ratioLayout.control.nativeFree.setChecked(); break;
    }

    hotkeyState = _settings->get<unsigned>( "scale_hotkey", program->getScaleHotkeyDefault() );
    for(int i = 0; i < 4; i++) {
        ratioLayout.hotkey.boxes[i]->setChecked( hotkeyState & ( 1 << i ) );
    }

    monitorResolutionLayout.active.setChecked( _settings->get<bool>("fullscreen_setting_active", false) );
    monitorResolutionLayout.adjustEmuSpeed.setChecked( _settings->get<bool>("fullscreen_setting_adjust_emu_speed", false) );

    auto displayId = _settings->get<unsigned>("fullscreen_display", 0 );

    monitorResolutionLayout.display.setSelectionByUserId( displayId );

    monitorResolutionLayout.displaySettings.reset();
    for( auto& resolution : GUIKIT::Monitor::getSettings( displayId ) )
        monitorResolutionLayout.displaySettings.append( resolution.name, resolution.id );

    monitorResolutionLayout.displaySettings.setSelectionByUserId( _settings->get<unsigned>("fullscreen_setting", 0 ) );

    monitorResolutionLayout.display.setEnabled( monitorResolutionLayout.active.checked() );
    monitorResolutionLayout.displaySettings.setEnabled( monitorResolutionLayout.active.checked() );
    monitorResolutionLayout.adjustEmuSpeed.setEnabled( monitorResolutionLayout.active.checked() );

    ratioLayout.dimension.width.setValue( _settings->get<unsigned>("view_hold_width", 800) );
    ratioLayout.dimension.height.setValue( _settings->get<unsigned>("view_hold_height", 600) );
    ratioLayout.dimension.aspectCorrectResizing.setChecked( _settings->get<bool>("aspect_correct_resizing", false) );

    ratioLayout.dimension.scaleOnEmuSwitch.setChecked(_settings->get<bool>("scale_emu_switch", false));

    updateVisibillity();
}

auto GeometryLayout::setRotation(DRIVER::Rotation rotation) -> void {
    switch(rotation) {
        default:
        case DRIVER::ROT_0: rotationLayout.degree0.setChecked(); break;
        case DRIVER::ROT_90: rotationLayout.degree90.setChecked(); break;
        case DRIVER::ROT_180: rotationLayout.degree180.setChecked(); break;
        case DRIVER::ROT_270: rotationLayout.degree270.setChecked(); break;
    }
}