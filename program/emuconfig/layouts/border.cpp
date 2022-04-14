
BorderLayout::BorderLayout(TabWindow* tabWindow) : 
cropLeft("px"),
cropRight("px"),
cropTop("px"),
cropBottom("px")
{
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);
    
	cropLayout.append( cropType, {0u, 0u}, 5 );
    cropLayout.append( cropType2, {0u, 0u}, 5 );
	cropLayout.append( cropAspectCorrect, {0u, 0u}, 5 );
	cropLayout.append( cropLeft, {~0u, 0u}, 5 );
	cropLayout.append( cropRight, {~0u, 0u}, 5 );
	cropLayout.append( cropTop, {~0u, 0u}, 5 );
	cropLayout.append( cropBottom, {~0u, 0u} );	
	
	cropLayout.setPadding(10);
	
	cropLayout.setFont(GUIKIT::Font::system("bold"));    
	append(cropLayout, {~0u, 0u}, 10);
	
	cropLeft.slider.setLength(101);
	cropRight.slider.setLength(101);
	cropTop.slider.setLength(101);
	cropBottom.slider.setLength(101);
	
	cropType.append( cropOff, {0u, 0u}, 10 );
	cropType.append( cropMonitor, {0u, 0u}, 10 );
	cropType.append( cropAuto, {0u, 0u}, 10 );
	cropType2.append( cropSemiAuto, {0u, 0u}, 10 );
	cropType2.append( cropFree, {0u, 0u} );	
	
	GUIKIT::RadioBox::setGroup( cropOff, cropMonitor, cropAuto, cropSemiAuto, cropFree );
	
    typedef Emulator::Interface::CropType CropType;
    
	cropOff.onActivate = [this]() {
        updateCrop("crop_type", (unsigned)CropType::Off);

		updateVisibillity();
	};
    
	cropMonitor.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Monitor);

		updateVisibillity();
	};

	cropAuto.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Auto);

		updateVisibillity();
	};

	cropSemiAuto.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::SemiAuto);

		updateVisibillity();
	};

	cropFree.onActivate = [this]( ) {
        updateCrop("crop_type", (unsigned)CropType::Free);

		updateVisibillity();
	};
	
	cropAspectCorrect.onToggle = [this](bool checked) {
        updateCrop("crop_aspect_correct", checked);
	};
	
	cropLeft.slider.onChange = [this]() {
        auto value = cropLeft.slider.position();
        updateCrop("crop_left", value);

		cropLeft.value.setText( std::to_string( value ) + " px" );
	};

	cropRight.slider.onChange = [this]( ) {
		auto value = cropRight.slider.position( );
        updateCrop("crop_right", value);

		cropRight.value.setText( std::to_string( value ) + " px" );
	};

	cropTop.slider.onChange = [this]( ) {
		auto value = cropTop.slider.position( );
        updateCrop("crop_top", value);

		cropTop.value.setText( std::to_string( value ) + " px" );
	};

	cropBottom.slider.onChange = [this]( ) {
		auto value = cropBottom.slider.position( );
        updateCrop("crop_bottom", value);

		cropBottom.value.setText( std::to_string( value ) + " px" );
	};
    	
    loadSettings();
}

auto BorderLayout::updateCrop(std::string property, unsigned value) -> void {
    emuThread->lockVideo();
    _settings->set<unsigned>( property, value );
    if (emuThread->enabled && activeEmulator)
        emuThread->updateBorder = true;
    else
        program->updateCrop( emulator );

    emuThread->unlockVideo();
}

auto BorderLayout::updateVisibillity() -> void {
	auto val = _settings->get<unsigned>( "crop_type", (unsigned)Emulator::Interface::CropType::Monitor, {0u,4u});
	
	cropLeft.setEnabled( val == 3 || val == 4 );
	cropRight.setEnabled( val == 4 );
	cropTop.setEnabled( val == 4 );
	cropBottom.setEnabled( val == 4 );
	
	cropAspectCorrect.setEnabled( val == 2 || val == 3 );
}

auto BorderLayout::translate() -> void {
	
	cropLeft.name.setText( trans->get("left", {},true) );
	cropRight.name.setText( trans->get("right", {},true) );
	cropTop.name.setText( trans->get("up", {},true) );
	cropBottom.name.setText( trans->get("down", {},true) );
	
	cropOff.setText( trans->get("disabled") );
	cropMonitor.setText( trans->get("monitor") );
	cropAuto.setText( trans->get("crop complete") );
	cropSemiAuto.setText( trans->get("crop all sides equally") );
	cropFree.setText( trans->get("crop each side manually") );
	
	cropAspectCorrect.setText( trans->get("maintain display ratio") );
	cropLayout.setText( trans->get("crop border") );
    
    SliderLayout::scale({&cropLeft, &cropRight, &cropTop, &cropBottom}, "100 px");
}

auto BorderLayout::loadSettings() -> void {
    typedef Emulator::Interface::CropType CropType;
    
    auto valLeft = _settings->get<unsigned>("crop_left", 0,{0u, 100u});
    cropLeft.slider.setPosition(valLeft);
    cropLeft.value.setText(std::to_string(valLeft) + " px");

    auto valRight = _settings->get<unsigned>("crop_right", 0,{0u, 100u});
    cropRight.slider.setPosition(valRight);
    cropRight.value.setText(std::to_string(valRight) + " px");

    auto valTop = _settings->get<unsigned>("crop_top", 0,{0u, 100u});
    cropTop.slider.setPosition(valTop);
    cropTop.value.setText(std::to_string(valTop) + " px");

    auto valBottom = _settings->get<unsigned>("crop_bottom", 0,{0u, 100u});
    cropBottom.slider.setPosition(valBottom);
    cropBottom.value.setText(std::to_string(valBottom) + " px");

    auto valCropType = _settings->get<unsigned>("crop_type", (unsigned) CropType::Monitor,{0u, 4u});
    if (valCropType == 1) cropMonitor.setChecked();
    else if (valCropType == 2) cropAuto.setChecked();
    else if (valCropType == 3) cropSemiAuto.setChecked();
    else if (valCropType == 4) cropFree.setChecked();
    else cropOff.setChecked();

    auto valCropAC = _settings->get<bool>("crop_aspect_correct", 0);
    cropAspectCorrect.setChecked(valCropAC);
    
    updateVisibillity();
}