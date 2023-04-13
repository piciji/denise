
CropLayout::Type1::Type1() {
    append( cropOff, {0u, 0u}, 10 );
    append( cropMonitor, {0u, 0u}, 10 );
    append( cropAuto, {0u, 0u}, 10 );
}

CropLayout::Type2::Type2() {
    append( cropSemiAuto, {0u, 0u}, 10 );
    append( cropFree, {0u, 0u} );
}

CropLayout::Hotkey::Hotkey() {
    append( label, {0u, 0u}, 10 );
    append( cropOff, {0u, 0u}, 10 );
    append( cropMonitor, {0u, 0u}, 10 );
    append( cropAuto, {0u, 0u}, 10 );
    append( cropSemiAuto, {0u, 0u}, 10 );
    append( cropFree, {0u, 0u} );

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
    append( aspectCorrect, {0u, 0u}, 5 );
    append( cropLeft, {~0u, 0u}, 5 );
    append( cropRight, {~0u, 0u}, 5 );
    append( cropTop, {~0u, 0u}, 5 );
    append( cropBottom, {~0u, 0u}, 10 );

    cropLeft.slider.setLength(101);
    cropRight.slider.setLength(101);
    cropTop.slider.setLength(101);
    cropBottom.slider.setLength(101);

    GUIKIT::RadioBox::setGroup( type1.cropOff, type1.cropMonitor, type1.cropAuto, type2.cropSemiAuto, type2.cropFree );

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
    append(ratioLayout, {~0u, 0u});

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

    cropLayout.type2.cropSemiAuto.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::SemiAuto);

		updateVisibillity();
	};

    cropLayout.type2.cropFree.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free);

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
    cropLayout.hotkey.cropFree.onToggle = [this](bool checked) {
        updateBorderHotkeyUsage(4, checked);
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
    	
    loadSettings();
}

auto GeometryLayout::updateBorderHotkeyUsage(unsigned bit, bool checked) -> void {
    unsigned state = _settings->get<unsigned>( "border_hotkey", ~0 );

    if (checked)
        state |= 1 << bit;
    else
        state &= ~(1 << bit);

    _settings->set<unsigned>( "border_hotkey", state );
}

auto GeometryLayout::updateCrop(std::string property, unsigned value) -> void {
    emuThread->lockVideo();
    _settings->set<unsigned>( property, value );
    if (emuThread->enabled && activeEmulator)
        emuThread->updateBorder = true;
    else
        program->updateCrop( emulator );

    emuThread->unlockVideo();
}

auto GeometryLayout::updateVisibillity() -> void {
	auto val = _settings->get<unsigned>( "crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u,4u});

    cropLayout.cropLeft.setEnabled( val == 3 || val == 4 );
    cropLayout.cropRight.setEnabled( val == 4 );
    cropLayout.cropTop.setEnabled( val == 4 );
    cropLayout.cropBottom.setEnabled( val == 4 );

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
    cropLayout.type2.cropSemiAuto.setText( trans->get("crop all sides equally") + " (3)" );
    cropLayout.type2.cropFree.setText( trans->get("crop each side manually") + " (4)" );

    cropLayout.hotkey.label.setText( trans->get("switchable by Hotkey", {}, true) );
    cropLayout.hotkey.cropOff.setText( "0" );
    cropLayout.hotkey.cropMonitor.setText( "1" );
    cropLayout.hotkey.cropAuto.setText( "2" );
    cropLayout.hotkey.cropSemiAuto.setText( "3" );
    cropLayout.hotkey.cropFree.setText( "4" );

    cropLayout.aspectCorrect.setText( trans->get("maintain display ratio") );
	cropLayout.setText( trans->get("crop border") );

    ratioLayout.setText( trans->getA("scaling") );
    ratioLayout.label.setText( trans->getA("aspect ratio", true) );
    ratioLayout.window.setText( trans->getA("window") );
    ratioLayout.tv.setText( trans->getA("CRT TV") );
    ratioLayout.native.setText( trans->getA("Native") );
    ratioLayout.native.setTooltip( trans->getA("Native tooltip") );
    ratioLayout.integerScaling.setText( trans->getA("integer_scaling") );
    
    SliderLayout::scale({&cropLayout.cropLeft, &cropLayout.cropRight, &cropLayout.cropTop, &cropLayout.cropBottom}, "100 px");
}

auto GeometryLayout::loadSettings() -> void {
    typedef Emulator::Interface::CropType CropType;
    
    auto valLeft = _settings->get<unsigned>("crop_left", 0,{0u, 100u});
    cropLayout.cropLeft.slider.setPosition(valLeft);
    cropLayout.cropLeft.value.setText(std::to_string(valLeft) + " px");

    auto valRight = _settings->get<unsigned>("crop_right", 0,{0u, 100u});
    cropLayout.cropRight.slider.setPosition(valRight);
    cropLayout.cropRight.value.setText(std::to_string(valRight) + " px");

    auto valTop = _settings->get<unsigned>("crop_top", 0,{0u, 100u});
    cropLayout.cropTop.slider.setPosition(valTop);
    cropLayout.cropTop.value.setText(std::to_string(valTop) + " px");

    auto valBottom = _settings->get<unsigned>("crop_bottom", 0,{0u, 100u});
    cropLayout.cropBottom.slider.setPosition(valBottom);
    cropLayout.cropBottom.value.setText(std::to_string(valBottom) + " px");

    auto valCropType = _settings->get<unsigned>("crop_type", (unsigned) CropType::Monitor,{0u, 4u});
    if (valCropType == 1) cropLayout.type1.cropMonitor.setChecked();
    else if (valCropType == 2) cropLayout.type1.cropAuto.setChecked();
    else if (valCropType == 3) cropLayout.type2.cropSemiAuto.setChecked();
    else if (valCropType == 4) cropLayout.type2.cropFree.setChecked();
    else cropLayout.type1.cropOff.setChecked();

    auto valCropAC = _settings->get<bool>("crop_aspect_correct", 0);
    cropLayout.aspectCorrect.setChecked(valCropAC);

    unsigned hotkeyState = _settings->get<unsigned>( "border_hotkey", ~0 );
    cropLayout.hotkey.cropOff.setChecked( hotkeyState & 1 );
    cropLayout.hotkey.cropMonitor.setChecked( hotkeyState & 2 );
    cropLayout.hotkey.cropAuto.setChecked( hotkeyState & 4 );
    cropLayout.hotkey.cropSemiAuto.setChecked( hotkeyState & 8 );
    cropLayout.hotkey.cropFree.setChecked( hotkeyState & 16 );

    bool integerScaling = _settings->get<bool>("integer_scaling", false);
    ratioLayout.integerScaling.setChecked(integerScaling);

    int aspectMode = _settings->get<int>("aspect_mode", 1, {0, 2});
    switch(aspectMode) {
        case 0: ratioLayout.window.setChecked(); break;
        case 1: ratioLayout.tv.setChecked(); break;
        case 2: ratioLayout.native.setChecked(); break;
    }

    updateVisibillity();
}