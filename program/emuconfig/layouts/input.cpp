
InputSelector::InputSelector() {
    append(device, {~0u, 0u}, 20);
	append(hotkeys, {0u, 0u});
	append(spacer, {~0u, 0u});
    append(plugin, {0u, 0u}, 10);
    setAlignment(0.5);
}

InputControl::InputControl() {
    mapper.setEnabled(false);
    erase.setEnabled(false);
    linker.setEnabled(false);
    mapperAlt.setEnabled(false);
    eraseAlt.setEnabled(false);
    linkerAlt.setEnabled(false);

    append(mapper, {0u, 0u}, 10);     
	append(linker, {0u, 0u}, 10);     
    append(erase, {0u, 0u}, 10);
    append(autofireSlider, {~0u, 0u}, 10);
    append(alternate, {0u, 0u}, 5);  
    append(mapperAlt, {0u, 0u}, 10);     
	append(linkerAlt, {0u, 0u}, 10);     
    append(eraseAlt, {0u, 0u});

    autofireSlider.slider.setLength( 100 );
    autofireSlider.updateValueWidth( "100" );
    
    setAlignment( 0.5 );
}

InputMapControl::InputMapControl() : analogSensitivity("%") {
    append(keyLayoutLabel, {0u, 0u}, 5);
    append(keyLayout, {0u, 0u}, 10);
    append(automap, {0u, 0u}, 10);
    append(analogSensitivity, {~0u, 0u}, 10);
    append(reset, {0u, 0u});    
    
    setAlignment( 0.5 );
    
    analogSensitivity.slider.setLength(101);    
    
    automap.setEnabled( false );   
    keyLayout.setEnabled( false );    
    keyLayout.append( "", -1 );
    
    for ( auto& keyboardLayout : InputManager::keyboardLayouts ) {     
        keyLayout.append( "", (unsigned)keyboardLayout.type ); 
    }
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

InputLayout::InputLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setMargin(10);

    inputList.setHeaderText( { "", "", "", "" } );
    inputList.setHeaderVisible();
    
    controllerImage.loadPng((uint8_t*)Icons::controller, sizeof(Icons::controller));
    mouseImage.loadPng((uint8_t*)Icons::mouse, sizeof(Icons::mouse));
    keyboardImage.loadPng((uint8_t*)Icons::keyboard, sizeof(Icons::keyboard));
    virtualKeyImage.loadPng((uint8_t*)Icons::virtualKey, sizeof(Icons::virtualKey));
    lightgunImage.loadPng((uint8_t*)Icons::lightgun, sizeof(Icons::lightgun));
    lightpenImage.loadPng((uint8_t*)Icons::lightpen, sizeof(Icons::lightpen));
    
    for(auto& connector : emulator->connectors) {
        auto pluginConnector = new GUIKIT::CheckButton;
        pluginConnector->setText( trans->get( connector.name ) );
        
        Emulator::Interface::Connector* connectorPtr = &connector;
        
        pluginConnector->onToggle = [&, pluginConnector, connectorPtr]() {

            emuThread->lock();
            auto device = emulator->getUnplugDevice();
            
            if (pluginConnector->checked())
                device = emulator->getDevice( deviceId() );
            
            this->emulator->connect( connectorPtr, device );

            auto manager = InputManager::getManager( this->emulator );
            manager->updateMappingsInUse();

            emuThread->unlock();
            _settings->set<unsigned>( _underscore(connectorPtr->name), device->id);
            
            view->checkInputDevice( emulator, connectorPtr, device );
            
            view->setCursor( emulator );
        };
        
        selector.append( *pluginConnector, {0u, 0u}, &emulator->connectors.back() == &connector ? 0 : 20 );
        
        selector.connectorButtons.push_back( { pluginConnector, &connector } );
    }

    append(selector, {~0u, 0u}, 10);
    append(assigner, {~0u, 0u}, 10);
    append(inputList, {~0u, ~0u}, 10);
    append(control, {~0u, 0u}, 10);
    append(mapControl, {~0u, 0u}, 10);

    captureTimer.setInterval(3500);
    pollTimer.setInterval(50);

    control.autofireSlider.slider.onChange = [this]() {
        auto manager = InputManager::getManager( this->emulator );
        unsigned position = control.autofireSlider.slider.position();
        position += 1;
        _settings->set<unsigned>( "autofire_frequency", position );
        control.autofireSlider.value.setText( std::to_string(position) );

        emuThread->lock();
        manager->updateAutofireFrequency();
        emuThread->unlock();
    };

    mapControl.reset.onActivate = [this]() {
        if (!mes->question( trans->get("reset_device_question") ))
			return;

        emuThread->lock();
        stopCapture();
                
		if (hotkeyMode())
			InputManager::getManager( this->emulator )->unmapCustomHotkeys();
		else
			InputManager::getManager( this->emulator )->unmapDevice( deviceId() );        
    
		update();
        emuThread->unlock();
    };

    control.erase.onActivate = [this]() {
        emuThread->lock();
        eraseSelected();
        emuThread->unlock();
    };

    control.eraseAlt.onActivate = [this]() {
        emuThread->lock();
        eraseSelected( true );
        emuThread->unlock();
    };
	
	control.linker.onActivate = [this]() {
        emuThread->lock();
        linkSelected();
        emuThread->unlock();
	};

    control.linkerAlt.onActivate = [this]() {
        emuThread->lock();
        linkSelected( true );
        emuThread->unlock();
    };
    
    control.mapper.onActivate = [this]() {
        emuThread->lock();
        mapSelected();
        emuThread->unlock();
    };
    
    control.mapperAlt.onActivate = [this]() {
        emuThread->lock();
        mapSelected( true );
        emuThread->unlock();
    };
    
    inputList.onActivate = [this]() {
        emuThread->lock();
		mapSelected();
        emuThread->unlock();
    };

    mapControl.automap.onActivate = [&]() {
        
        auto selection = mapControl.keyLayout.selection();
        
        if (hotkeyMode() || selection == 0)
            return;
        
        if ( mes->question( trans->get("layout_map_question") ) ) {

            emuThread->lock();
            InputManager::getManager( this->emulator )->unmapDevice( deviceId() );                
            
            auto type = mapControl.keyLayout.userData( selection );
            
            InputManager::getManager( this->emulator )->autoAssign( (KeyboardLayout::Type)type, true );
            
            InputManager::getManager( this->emulator )->updateMappingsInUse();
            
            update();
            emuThread->unlock();
        }
    };
    
    mapControl.keyLayout.onChange = [&]() {
        
		if (hotkeyMode())
			return;
		
        _settings->set<int>( "keyboard_layout", mapControl.keyLayout.userData() );
        
        update();        
    };

    inputList.onChange = [&]() {
        displayInputCall();
        std::string inputIdent;
        auto mapping = getMappingOfSelected(inputIdent);
        
        control.erase.setEnabled( inputList.selected() );
		control.linker.setEnabled( !mapping->isAnalog() && inputList.selected() );
        control.mapper.setEnabled( inputList.selected() );
        control.eraseAlt.setEnabled(!mapping->isAnalog() && inputList.selected());
        control.linkerAlt.setEnabled(!mapping->isAnalog() && inputList.selected());
        control.mapperAlt.setEnabled( !mapping->isAnalog() && inputList.selected() );

        updateSliderVisibillity( mapping->emuDevice );
    };

    mapControl.analogSensitivity.slider.onChange = [this]() {
        
		if(hotkeyMode())
			return;

        emuThread->lock();
        auto& device = emulator->devices[ deviceId() ];
        
        unsigned position = mapControl.analogSensitivity.slider.position();
        
        mapControl.analogSensitivity.value.setText( std::to_string(position) + " " + mapControl.analogSensitivity.unit );    
    
        _settings->set<unsigned>("analog_sensitivity_" + _underscore(device.name), position);
        
        InputManager::getManager( this->emulator )->updateAnalogSensitivity( &device );
        emuThread->unlock();
    };    
	
	selector.hotkeys.onToggle = [this]() {
        emuThread->lock();
		stopCapture();
		update();
        emuThread->unlock();
	};

    assigner.overwriteRadio.onActivate = [this]() {
        _settings->set<unsigned>("input_assigner", 1);
    };

    assigner.appendRadio.onActivate = [this]() {
        _settings->set<unsigned>("input_assigner", 0);
    };

    updateAssigner();

    updateKeyLayout();

    updateAutofireFrequency();

    loadDeviceList();
}

auto InputLayout::updateAssigner() -> void {
    if (_settings->get<unsigned>("input_assigner", 1) == 1)
        assigner.overwriteRadio.setChecked();
    else
        assigner.appendRadio.setChecked();
}

auto InputLayout::stopCapture() -> void {
    pollTimer.setEnabled(false);
    captureTimer.setEnabled(false);
    InputManager::captureObject = nullptr;
    displayInputCall();
}

auto InputLayout::loadDeviceList() -> void {
	
	unsigned selection = selector.device.selection();
	
    selector.device.reset();
    
    for (auto& device : emulator->devices) {
        if (device.inputs.size() == 0)
            continue;
        
        selector.device.append( trans->get( device.name ), device.id );
    }
	
	selector.device.setSelection( selection );

    selector.device.onChange = [&]() {
		selector.hotkeys.setChecked(false);
        stopCapture();
        update();
    };
	
    stopCapture();
	
	update();
}

auto InputLayout::loadInputList(unsigned deviceId) -> void {
    inputList.reset();
    control.erase.setEnabled(false);
	control.linker.setEnabled(false);
    control.mapper.setEnabled(false);
    control.eraseAlt.setEnabled(false);
	control.linkerAlt.setEnabled(false);
    control.mapperAlt.setEnabled(false);

    auto& device = emulator->devices[deviceId];
    GUIKIT::Image* image = nullptr;
    
    if (device.isJoypad())
        image = &controllerImage;
    else if (device.isMouse() || device.isPaddles())
        image = &mouseImage;
    else if (device.isLightGun())
        image = &lightgunImage;
    else if (device.isLightPen())
        image = &lightpenImage;
    else 
        image = &keyboardImage;
    
    auto layoutType = _settings->get<int>( "keyboard_layout", InputManager::assumeLayoutType() );
    
    inputList.lockRedraw();
    
    unsigned counter = device.isKeyboard() ? (emulator->devices[deviceId].inputs.size() >> 1) : 0;
    
    for (auto& input : emulator->devices[deviceId].inputs) {
        
        std::string useName = input.name;
        
        for(auto& layout : input.layouts) {
            if ( (KeyboardLayout::Type) layout.type == layoutType ) {
                useName = layout.name;
                break;
            }
        }
                    
        appendListEntry( useName, (InputMapping*)input.guid, image );
        
        if (counter) {
            if (--counter == 0) {
                inputList.unlockRedraw();
            }                
        }
    }
    
    inputList.unlockRedraw();
    
    mapControl.keyLayout.setEnabled( device.isKeyboard() );    
    mapControl.automap.setEnabled( device.isKeyboard() && mapControl.keyLayout.selection() != 0 );
    
    updateConnectorButtons();
    enableConnectorButtons();
    updateAnalogSensitivity();
    updateSliderVisibillity( &device );
}

auto InputLayout::loadHotkeyList() -> void {
	inputList.reset();
    control.erase.setEnabled(false);
	control.linker.setEnabled(false);
    control.mapper.setEnabled(false);
    control.eraseAlt.setEnabled(false);
	control.linkerAlt.setEnabled(false);
    control.mapperAlt.setEnabled(false);

    updateSliderVisibillity( );
	
    inputList.lockRedraw();
    
	for (auto& input : InputManager::getManager( emulator )->customHotkeys ) {
		
		appendListEntry( input.name, (InputMapping*)input.guid, nullptr );
	}
    
    inputList.unlockRedraw();
	
	mapControl.keyLayout.setEnabled( false );    
    mapControl.automap.setEnabled( false );
	
	for(auto& connectorButton : selector.connectorButtons)        
        connectorButton.checkButton->setEnabled( false );
}

auto InputLayout::updateAnalogSensitivity() -> void {
    
    auto& device = emulator->devices[ deviceId() ];
    
    auto position = _settings->get<unsigned>("analog_sensitivity_" + _underscore(device.name), 50u, {0u, 100u});
        
    mapControl.analogSensitivity.value.setText( std::to_string(position) + " " + mapControl.analogSensitivity.unit );
    
    mapControl.analogSensitivity.slider.setPosition( position );
}

auto InputLayout::updateKeyLayout() -> void {
    
    auto layout = _settings->get<int>( "keyboard_layout", InputManager::assumeLayoutType() );
    
    for( unsigned row = 0; row < mapControl.keyLayout.rows(); row++ ) {
        
        if ( layout == mapControl.keyLayout.userData( row ) ) {
            mapControl.keyLayout.setSelection( row );
            break;
        }
    }
}

auto InputLayout::updateAutofireFrequency() -> void {
    unsigned position = _settings->get<unsigned>( "autofire_frequency", 1, {1, 100} );
    control.autofireSlider.value.setText( std::to_string(position) );
    control.autofireSlider.slider.setPosition( position - 1 );
}

auto InputLayout::appendListEntry(std::string& name, InputMapping* mapping, GUIKIT::Image* image) -> void {
    
    if (image && mapping->shadowMap.size() > 0)
        image = &virtualKeyImage;
    
    inputList.append({ "", trans->get( name ), mapping->getDescription(), 
        mapping->alternate ? mapping->alternate->getDescription() : "" });
		
	if (image)
		inputList.setImage( inputList.rowCount() - 1, 0, *image );

    if (mapping->emuDevice && mapping->emuDevice->isJoypad() && GUIKIT::String::foundSubStr(name, "Toggle Autofire"))
        inputList.setRowTooltip( inputList.rowCount() - 1, trans->get("autofire on focus") );
}

auto InputLayout::updateListEntry(unsigned selection, InputMapping* mapping) -> void {
    unsigned column = !mapping->parent ? 2 : 3;
    inputList.setText(selection, column,  mapping->getDescription() );
    inputList.setSelection(selection);
}

auto InputLayout::translate() -> void {
    stopCapture();
	selector.hotkeys.setText( trans->get("hotkeys") );
    selector.plugin.setText( trans->get("plugin", {}, true) );
    inputList.setHeaderText({ "", trans->get("input"), trans->get("map"), trans->get("alternate_map")});
    mapControl.reset.setText( trans->get( "reset" ) );
    mapControl.reset.setTooltip( trans->get( "reset_device_info" ) );
    control.erase.setText( trans->get( "erase" ) );
    control.erase.setTooltip( trans->get( "erase_device_info" ) );
    control.eraseAlt.setText( trans->get( "erase" ) );
    control.eraseAlt.setTooltip( trans->get( "erase_device_info" ) );
	control.linker.setText( trans->get( "and_or_connection" ) );   
    control.linkerAlt.setText( trans->get( "and_or_connection" ) );       
    control.mapper.setText( trans->get( "assign" ) );
    control.mapperAlt.setText( trans->get( "assign" ) );
    control.alternate.setText( trans->get( "alternate", {}, true ) );
    
    mapControl.keyLayoutLabel.setText( trans->get("layout", {}, true) );
    mapControl.keyLayout.setTooltip( trans->get("keyboard_layout_tip") );
    mapControl.automap.setText( trans->get("assign") );
    mapControl.analogSensitivity.name.setText( trans->get("analog_sensitivity", {}, true) );
    
    assigner.overwriteRadio.setText( trans->get("overwrite") );
    assigner.appendRadio.setText( trans->get("append") );
    assigner.assignLabel.setText( trans->get("assignment", {}, true) );
    
    unsigned i = 0;
    mapControl.keyLayout.setText( i++, trans->get("generic"));
    for ( auto& keyboardLayout : InputManager::keyboardLayouts ) {
        mapControl.keyLayout.setText( i++, trans->get( keyboardLayout.language ) + " ( " + keyboardLayout.code + " )" );
    }
    
    for(auto& connectorButton : selector.connectorButtons) 
        connectorButton.checkButton->setText( trans->get( connectorButton.connector->name ) );

    control.autofireSlider.name.setText( trans->get("Autofire Rate", {}, true) );
    
    SliderLayout::scale({&mapControl.analogSensitivity}, "100 %");
}

auto InputLayout::displayInputCall() -> void {
    std::string text = trans->get("register_input");
    
    if (inputList.selected()) {
        auto selection = inputList.selection();    
        
        if ( inputList.getImage( selection, 0 ) == &virtualKeyImage )
            text = trans->get("register_vkey");
    }    
    
    assigner.infoLabel.setFont( GUIKIT::Font::system() );
    assigner.infoLabel.setText( text );
}

auto InputLayout::deviceId() -> unsigned {
    return selector.device.userData();
}

auto InputLayout::inputId() -> unsigned {
    return inputList.selection();
}

auto InputLayout::getMappingOfSelected(std::string& inputIdent) -> InputMapping* {
	
	if (!hotkeyMode()) {
		auto input = emulator->devices[ deviceId() ].inputs[ inputId() ];	
		
		inputIdent = input.name;	
	
		return (InputMapping*)input.guid;
	} 
	
	auto hotkey = InputManager::getManager( emulator )->customHotkeys[ inputId() ];
	
	inputIdent = hotkey.name;
	
	return (InputMapping*)hotkey.guid;
}

auto InputLayout::update() -> void {
	if (hotkeyMode())
		loadHotkeyList();
	else
		loadInputList( deviceId() );
}

auto InputLayout::updateConnectorButtons() -> void {
    
    for(auto& connectorButton : selector.connectorButtons) {
        
        auto device = emulator->getConnectedDevice( connectorButton.connector );
        
        bool willCheck = deviceId() == device->id;
        
        if (willCheck != connectorButton.checkButton->checked() )        
            connectorButton.checkButton->setChecked( willCheck );
    }
}

auto InputLayout::enableConnectorButtons() -> void {
    
    auto device = emulator->getDevice( deviceId() );
    
    for(auto& connectorButton : selector.connectorButtons)        
        connectorButton.checkButton->setEnabled( !device->isKeyboard() );   
}

auto InputLayout::eraseSelected( bool alternate ) -> void {
    if (!inputList.selected())
        return;

    stopCapture();

    std::string inputIdent;
    auto mapping = getMappingOfSelected(inputIdent);
    
    if (alternate && !mapping->alternate)
        return;
    
    if (alternate)
        mapping = mapping->alternate;

    mapping->init();
    
	InputManager::updateAllMappingsInUse(true);
        
    updateListEntry(inputId(), mapping);
}

auto InputLayout::linkSelected( bool alternate ) -> void {

    if (!inputList.selected())
        return;

    stopCapture();

    std::string inputIdent;
    auto mapping = getMappingOfSelected(inputIdent);
    
    if (alternate && !mapping->alternate)
        return;

    if (alternate)
        mapping = mapping->alternate;

    mapping->swapLinker();
    updateListEntry(inputId(), mapping);
	
	InputManager::updateAllMappingsInUse(true);
}

auto InputLayout::mapSelected( bool alternate ) -> void {
    if (InputManager::captureObject)
        return;
        
    std::string inputIdent;
    auto mapping = getMappingOfSelected(inputIdent);
    
    if (alternate && !mapping->alternate)
        return;
    
    if (alternate)
        mapping = mapping->alternate;

    assigner.infoLabel.setFont(GUIKIT::Font::system("Bold"));
    assigner.infoLabel.setText(trans->get("press_key",{
        {"%trigger%", trans->get(inputIdent)}}));

    unsigned selection = inputId();
    InputManager::capture(mapping);

    captureTimer.setEnabled();
    pollTimer.setEnabled();
    bool overwriteMode = _settings->get<unsigned>("input_assigner", 1) == 1;

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

auto InputLayout::hotkeyMode() -> bool {
	
	return selector.hotkeys.checked();
}

auto InputLayout::triggerHotkeyMode() -> void {
	if (hotkeyMode())
		return;
	
	selector.hotkeys.setChecked();
	selector.hotkeys.onToggle();
}

auto InputLayout::loadSettings() -> void {
    
    updateAssigner();

    updateKeyLayout();

    updateAutofireFrequency();
    
    update();
}

auto InputLayout::updateSliderVisibillity(Emulator::Interface::Device* emuDevice) -> void {

    if (hotkeyMode()) {
        if (mapControl.analogSensitivity.enabled())
            mapControl.analogSensitivity.setEnabled(false);
        if (control.autofireSlider.enabled())
            control.autofireSlider.setEnabled(false);
    } else {
        if (!mapControl.analogSensitivity.enabled())
            mapControl.analogSensitivity.setEnabled(true);

        bool _enabled = emuDevice && emuDevice->isJoypad();

        if (_enabled != control.autofireSlider.enabled())
            control.autofireSlider.setEnabled( _enabled );
    }
}