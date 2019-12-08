
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
		settings->set<unsigned>( this->tabWindow->ident("crop_type"), (unsigned)CropType::Off );
		updateVisibillity();
		program->updateCrop( emulator );
	};
	
	cropMonitor.onActivate = [this]( ) {
		settings->set<unsigned>( this->tabWindow->ident( "crop_type" ), (unsigned)CropType::Monitor );
		updateVisibillity();
		program->updateCrop( emulator );
	};

	cropAuto.onActivate = [this]( ) {
		settings->set<unsigned>( this->tabWindow->ident( "crop_type" ), (unsigned)CropType::Auto );
		updateVisibillity();
		program->updateCrop( emulator );
	};

	cropSemiAuto.onActivate = [this]( ) {
		settings->set<unsigned>( this->tabWindow->ident( "crop_type" ), (unsigned)CropType::SemiAuto );
		updateVisibillity();
		program->updateCrop( emulator );
	};

	cropFree.onActivate = [this]( ) {
		settings->set<unsigned>( this->tabWindow->ident( "crop_type" ), (unsigned)CropType::Free );
		updateVisibillity();
		program->updateCrop( emulator );
	};
	
	cropAspectCorrect.onToggle = [this]() {
		settings->set<unsigned>( this->tabWindow->ident( "crop_aspect_correct" ), cropAspectCorrect.checked() );
		program->updateCrop( emulator );
	};
	
	cropLeft.slider.onChange = [this]() {
		auto value = cropLeft.slider.position();
		settings->set<unsigned>( this->tabWindow->ident("crop_left"), value);
		cropLeft.value.setText( std::to_string( value ) + " px" );
		program->updateCrop( emulator );
	};

	cropRight.slider.onChange = [this]( ) {
		auto value = cropRight.slider.position( );
		settings->set<unsigned>( this->tabWindow->ident( "crop_right" ), value );
		cropRight.value.setText( std::to_string( value ) + " px" );
		program->updateCrop( emulator );
	};

	cropTop.slider.onChange = [this]( ) {
		auto value = cropTop.slider.position( );
		settings->set<unsigned>( this->tabWindow->ident( "crop_top" ), value );
		cropTop.value.setText( std::to_string( value ) + " px" );
		program->updateCrop( emulator );
	};

	cropBottom.slider.onChange = [this]( ) {
		auto value = cropBottom.slider.position( );
		settings->set<unsigned>( this->tabWindow->ident( "crop_bottom" ), value );
		cropBottom.value.setText( std::to_string( value ) + " px" );
		program->updateCrop( emulator );
	};
    	
	auto valLeft = settings->get<unsigned>(tabWindow->ident("crop_left"), 0, {0u,100u});
    cropLeft.slider.setPosition(valLeft);
    cropLeft.value.setText( std::to_string( valLeft ) + " px" );
	
	auto valRight = settings->get<unsigned>(tabWindow->ident("crop_right"), 0, {0u,100u});
    cropRight.slider.setPosition(valRight);
    cropRight.value.setText( std::to_string( valRight ) + " px" );
	
	auto valTop = settings->get<unsigned>(tabWindow->ident("crop_top"), 0, {0u,100u});
    cropTop.slider.setPosition(valTop);
    cropTop.value.setText( std::to_string( valTop ) + " px" );
	
	auto valBottom = settings->get<unsigned>(tabWindow->ident("crop_bottom"), 0, {0u,100u});
    cropBottom.slider.setPosition(valBottom);
    cropBottom.value.setText( std::to_string( valBottom ) + " px" );

	auto valCropType = settings->get<unsigned>(tabWindow->ident("crop_type"), 0, {0u,4u});
	if (valCropType == 1) cropMonitor.setChecked();
	else if (valCropType == 2) cropAuto.setChecked();
	else if (valCropType == 3) cropSemiAuto.setChecked();
	else if (valCropType == 4) cropFree.setChecked();	
	else cropOff.setChecked();
	
	auto valCropAC = settings->get<bool>(tabWindow->ident("crop_aspect_correct"), 0);
	cropAspectCorrect.setChecked( valCropAC );
	
	updateVisibillity();
}

auto BorderLayout::updateVisibillity() -> void {
	auto val = settings->get<unsigned>(tabWindow->ident("crop_type"), 0);
	
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
	cropMonitor.setText( trans->get("crop_monitor") );
	cropAuto.setText( trans->get("crop_auto") );
	cropSemiAuto.setText( trans->get("crop_semi_auto") );
	cropFree.setText( trans->get("crop_free") );
	
	cropAspectCorrect.setText( trans->get("crop_aspect_correct") );
	cropLayout.setText( trans->get("crop") );
    
    SliderLayout::scale({&cropLeft, &cropRight, &cropTop, &cropBottom}, "100 px");
}
