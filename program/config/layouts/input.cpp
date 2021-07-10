
InputControl::InputControl() {
    mapper.setEnabled(false);
    erase.setEnabled(false);
    linker.setEnabled(false);
    mapperAlt.setEnabled(false);
    eraseAlt.setEnabled(false);
    linkerAlt.setEnabled(false);

    append(mapper, {0u, 0u}, 10);     
	append(linker, {0u, 0u}, 10);     
    append(erase, {0u, 0u});
    append(spacer, {~0u, 0u});    
    append(alternate, {0u, 0u}, 5);  
    append(mapperAlt, {0u, 0u}, 10);     
	append(linkerAlt, {0u, 0u}, 10);     
    append(eraseAlt, {0u, 0u});   
    
    setAlignment( 0.5 );
}

InputAssign::InputAssign() {    
    append(infoLabel, {~0u, 0u}, 10);
    append(assignLabel, {0u, 0u}, 10);
    append(appendRadio, {0u, 0u}, 3);
    append(overwriteRadio, {0u, 0u});    
    GUIKIT::RadioBox::setGroup( overwriteRadio, appendRadio );
    appendRadio.setChecked();
    setAlignment(0.5);
}

InputLayout::InputLayout() {
    setMargin(10);

    inputList.setHeaderText( { "", "", "" } );
    inputList.setHeaderVisible();

    append(assigner, {~0u, 0u}, 10);
    append(inputList, {~0u, ~0u}, 10);
    append(control, {~0u, 0u}, 10);
    append(driverWrapper, {~0u, 0u}, 10);
    driverWrapper.append(reset, {0u, 0u});

	auto selectedDriver = program->getInputDriver();
	unsigned i = 0;
	for (auto& driver : inputDriver->available()) {
		driverLayout.combo.append(driver);
		if (driver == selectedDriver) {
			driverLayout.combo.setSelection(i);
		}
		i++;
	}
    if (driverLayout.combo.rows() > 0) driverWrapper.append(driverLayout, {~0u, 0u});
    if (driverLayout.combo.rows() == 1) driverLayout.setEnabled(false);

    captureTimer.setInterval(3500);
    pollTimer.setInterval(50);

    reset.onActivate = [&]() {
        if (!configView->message->question( trans->get("reset_device_question") ))
			return;
        
        stopCapture();

		InputManager::unmapHotkeys();
    
		loadInputList();
    };
    
    assigner.overwriteRadio.onActivate = [this]() {
        globalSettings->set<unsigned>("input_assigner", 1);
    };
    
    assigner.appendRadio.onActivate = [this]() {
        globalSettings->set<unsigned>("input_assigner", 0);
    };

    if (globalSettings->get<unsigned>("input_assigner", 1) == 1)
        assigner.overwriteRadio.setChecked();
    else
        assigner.appendRadio.setChecked( );

    control.erase.onActivate = [this]() {   
        eraseSelected( );
    };
    
    control.eraseAlt.onActivate = [this]() {   
        eraseSelected( true );
    };
	
	control.linker.onActivate = [this]() {
		linkSelected();	
	};   
    
    control.linkerAlt.onActivate = [this]() {
		linkSelected( true );	
	}; 

    control.mapper.onActivate = [this]() {
        mapSelected();
    };

    control.mapperAlt.onActivate = [this]() {
        mapSelected(true);
    };

    inputList.onActivate = [this]() {
        mapSelected();
    };

    inputList.onChange = [&]() {
        control.erase.setEnabled( inputList.selected() );
		control.linker.setEnabled( inputList.selected() );
        control.mapper.setEnabled( inputList.selected() );
        control.eraseAlt.setEnabled( inputList.selected() );
		control.linkerAlt.setEnabled( inputList.selected() );
        control.mapperAlt.setEnabled( inputList.selected() );
    };

	driverLayout.combo.onChange = [this]() {
		globalSettings->set<std::string>("input_driver", driverLayout.combo.text());
        InputManager::rememberLastDeviceState();
		program->initInput();
	};    
    
    stopCapture();
    loadInputList();
}

auto InputLayout::loadInputList() -> void {
    inputList.reset();
    control.erase.setEnabled(false);
	control.linker.setEnabled(false);
    control.mapper.setEnabled(false);
    control.eraseAlt.setEnabled(false);
	control.linkerAlt.setEnabled(false);
    control.mapperAlt.setEnabled(false);

    for (auto& hotkey : InputManager::hotkeys) {
        appendListEntry( hotkey.name, (InputMapping*)hotkey.guid );
    }
}

auto InputLayout::appendListEntry(std::string& name, InputMapping* mapping) -> void {
    inputList.append({ trans->get( name ), mapping->getDescription(), mapping->alternate->getDescription() });
}

auto InputLayout::updateListEntry(unsigned selection, InputMapping* mapping) -> void {
    unsigned column = !mapping->parent ? 1 : 2;
    inputList.setText(selection, column, mapping->getDescription() );
    inputList.setSelection(selection);
}

auto InputLayout::translate() -> void {
    stopCapture();
    inputList.setHeaderText({trans->get("input"), trans->get("map"), trans->get("alternate_map")});
    reset.setText( trans->get( "reset" ) );
    reset.setTooltip( trans->get( "reset_device_info" ) );
    
    control.erase.setText( trans->get( "erase" ) );
    control.erase.setTooltip( trans->get( "erase_device_info" ) );
    control.eraseAlt.setText( trans->get( "erase" ) );
    control.eraseAlt.setTooltip( trans->get( "erase_device_info" ) );
	control.linker.setText( trans->get( "and_or_connection" ) );   
    control.linkerAlt.setText( trans->get( "and_or_connection" ) );       
    control.mapper.setText( trans->get( "assign" ) );
    control.mapperAlt.setText( trans->get( "assign" ) );
    control.alternate.setText( trans->get( "alternate", {}, true ) );
    
    assigner.overwriteRadio.setText( trans->get("overwrite") );
    assigner.appendRadio.setText( trans->get("append") );
    assigner.assignLabel.setText( trans->get("assignment", {}, true) );

	driverLayout.name.setText( trans->get("driver", {}, true) );
}

auto InputLayout::displayInputCall() -> void {
    assigner.infoLabel.setFont( GUIKIT::Font::system() );
    assigner.infoLabel.setText( trans->get("register_input") );
}

auto InputLayout::inputId() -> unsigned {
    return inputList.selection();
}

auto InputLayout::getMappingOfSelected(std::string& inputIdent) -> InputMapping* {
	
	auto input = InputManager::hotkeys[ inputId() ];
	inputIdent = input.name;
	
	return (InputMapping*)input.guid;
}

auto InputLayout::eraseSelected( bool alternate ) -> void {

    if (!inputList.selected())
        return;

    stopCapture();

    std::string inputIdent;
    auto mapping = getMappingOfSelected(inputIdent);
    if (alternate)
        mapping = mapping->alternate;

    mapping->init();

	InputManager::updateAllMappingsInUse();
        
    updateListEntry(inputId(), mapping);
}

auto InputLayout::linkSelected( bool alternate ) -> void {
        
    if (!inputList.selected())
        return;

    stopCapture();

    std::string inputIdent;
    auto mapping = getMappingOfSelected(inputIdent);

    if (alternate)
        mapping = mapping->alternate;

    mapping->swapLinker();
    updateListEntry(inputId(), mapping);

    for (auto manager : inputManagers)
        manager->sort();
}

auto InputLayout::mapSelected( bool alternate ) -> void {
    if (InputManager::captureObject)
        return;
          
    std::string inputIdent;
    auto mapping = getMappingOfSelected(inputIdent);
    
    if (alternate)
        mapping = mapping->alternate;
    
    assigner.infoLabel.setFont(GUIKIT::Font::system("Bold"));
    assigner.infoLabel.setText(trans->get("press_key",{
        {"%trigger%", trans->get(inputIdent)}}));

    unsigned selection = inputId();
    InputManager::capture(mapping);

    captureTimer.setEnabled();
    pollTimer.setEnabled();    
    bool overwriteMode = globalSettings->get<unsigned>("input_assigner", 1) == 1;

    pollTimer.onFinished = [&, mapping, selection, overwriteMode]() {
        if (InputManager::capture(overwriteMode)) {
            stopCapture();
            updateListEntry(selection, mapping);
        }
    };
    captureTimer.onFinished = [&]() {
        stopCapture();
    };
}

auto InputLayout::stopCapture() -> void {
    pollTimer.setEnabled(false);
    captureTimer.setEnabled(false);
    InputManager::captureObject = nullptr;
    displayInputCall();
}
