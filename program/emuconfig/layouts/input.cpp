
AutofireControl::AutofireControl(Emulator::Interface* emulator) : autofireSlider("") {
    append(label, {0u, 0u}, 5);

    auto manager = InputManager::getManager(emulator);

    for (auto& device : emulator->devices) {
        if (!device.isJoypad())
            continue;

        for (auto& input : device.inputs) {
            if (input.key == Emulator::Interface::Key::ToggleAutofire) {
                auto toggleButton = new GUIKIT::Button;
                auto mapping = (InputMapping*)input.guid;

                buttons.push_back( {toggleButton, mapping->shadowMap[0], manager->isAutofireActive(mapping->shadowMap[0])} );
                append(*toggleButton, {0u, 0u}, 10);

                break; // only first fire button
            }
        }
    }

    append(autofireSlider, {~0u, 0u}, 10);
    append(autofireHold, {0u, 0u});
    autofireSlider.slider.setLength( 99 );
    autofireSlider.updateValueWidth( "99" );

    setAlignment( 0.5 );
}

InputSelector::InputSelector() {
    append(device, {0u, 0u}, 10);
	append(hotkeys, {0u, 0u}, 10);
    append(globalHotkeys, {0u, 0u}, 10);
    append(grabMouseLeft, {0u, 0u}, 10);
	append(spacer, {~0u, 0u});
    append(plugin, {0u, 0u}, 10);
    setAlignment(0.5);
}

InputControl::OptionControl::PrioritiseLayout::PrioritiseLayout() {
    append(label, {0u, 0u}, 7);
    append(none, {0u, 0u}, 5);
    append(controlPort, {0u, 0u}, 5);
    append(keyboard, {0u, 0u});

    GUIKIT::RadioBox::setGroup( none, controlPort, keyboard );
    setAlignment(0.5);
}

InputControl::OptionControl::OptionControl() {
    append(prioritiseLayout, {0u, 0u}, GUIKIT::Font::scale(8));
    append(oppositeDirections, {0u, 0u});
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

    append(optionControl, {0u, 0u});

    append(spacing, {~0u, 0u});
    append(alternate, {0u, 0u}, 5);  
    append(mapperAlt, {0u, 0u}, 10);     
	append(linkerAlt, {0u, 0u}, 10);     
    append(eraseAlt, {0u, 0u});

    setAlignment( 0.5 );
}

InputMapControl::InputMapControl() : analogSensitivity("%") {
    append(automap, {0u, 0u}, 5);
    append(keyLayoutLabel, {0u, 0u}, 5);
    append(keyLayout, {0u, 0u}, 10);
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



InputLayout::InputLayout(TabWindow* tabWindow) : autofireControl(tabWindow->emulator) {
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
    append(autofireControl, {~0u, 0u});

    captureTimer.setInterval(3500);
    pollTimer.setInterval(200);


    for (auto& button : autofireControl.buttons) {
        auto mapping = button.mapping;
        auto toggleButton = button.toggleButton;

        toggleButton->onActivate = [this, mapping]() {
            if (!activeEmulator)
                return;

            auto manager = mapping->inputManager;

            emuThread->lock();
            manager->toggleAutofire(mapping);

            statusHandler->setMessage( trans->get( manager->isAutofireActive(mapping) ? "Autofire active" : "Autofire inactive" ), 5 );

            this->updatedAutofireButtonHints();

            emuThread->unlock();
        };
    }

    autofireControl.autofireSlider.slider.onChange = [this](unsigned position) {
        auto manager = InputManager::getManager( this->emulator );
        position += 1;
        _settings->set<unsigned>( "autofire_frequency", position );
        autofireControl.autofireSlider.setValue( std::to_string(position) );

        emuThread->lock();
        manager->updateAutofireFrequency();
        emuThread->unlock();
    };

    autofireControl.autofireHold.onToggle = [this](bool checked) {
        auto manager = InputManager::getManager( this->emulator );
        _settings->set<bool>( "autofire_hold", checked );
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
        else if (globalHotkeyMode())
            InputManager::unmapHotkeys();
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
        mapSelected( inputList.column() == 3 );
        emuThread->unlock();
    };

    mapControl.automap.onActivate = [&]() {
        if (hotkeyMode() || globalHotkeyMode())
            return;

        auto selection = mapControl.keyLayout.selection();

        auto& device = emulator->devices[ deviceId() ];
        
        if (!isAutomapEnabled(device))
            return;
        
        if ( mes->question( trans->get("layout_map_question") ) ) {

            emuThread->lock();
            InputManager::getManager( this->emulator )->unmapDevice( deviceId() );                
            
            auto type = mapControl.keyLayout.userData( selection );

            if (device.isKeyboard())
                InputManager::getManager( this->emulator )->autoAssign( (KeyboardLayout::Type)type, true );
            else
                InputManager::getManager( this->emulator )->autoAssign( device );

            InputManager::getManager( this->emulator )->updateMappingsInUse();
            
            update();
            emuThread->unlock();
        }
    };
    
    mapControl.keyLayout.onChange = [&]() {
        
		if (hotkeyMode() || globalHotkeyMode())
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
    };

    mapControl.analogSensitivity.slider.onChange = [this](unsigned position) {
		if(hotkeyMode() || globalHotkeyMode())
			return;

        emuThread->lock();
        auto& device = emulator->devices[ deviceId() ];

        mapControl.analogSensitivity.value.setText( std::to_string(position) + " " + mapControl.analogSensitivity.unit );    
    
        _settings->set<unsigned>("analog_sensitivity_" + _underscore(device.name), position);
        
        InputManager::getManager( this->emulator )->updateAnalogSensitivity( &device );
        emuThread->unlock();
    };    
	
	selector.hotkeys.onToggle = [this]() {
	    selector.globalHotkeys.setChecked(false);
        emuThread->lock();
		stopCapture();
		update();
        emuThread->unlock();
	};

    selector.globalHotkeys.onToggle = [this]() {
        selector.hotkeys.setChecked(false);
        emuThread->lock();
        stopCapture();
        update();
        emuThread->unlock();
    };

    selector.grabMouseLeft.onToggle = [this](bool checked) {
        emuThread->lock();
        stopCapture();
        _settings->set<bool>("grab_mouse_left", checked);
        if (emulator == activeEmulator)
            view->updateMouseGrab();
        emuThread->unlock();
    };

    control.optionControl.oppositeDirections.onToggle = [this](bool checked) {
        emuThread->lock();
        _settings->set<bool>("allow_opposite_directions", checked);
        InputManager::getManager( this->emulator )->updateMiscSettings();
        emuThread->unlock();
    };

    control.optionControl.prioritiseLayout.none.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned >("prioritise_mappings", 0);
        InputManager::getManager( this->emulator )->updateMiscSettings();
        InputManager::getManager( this->emulator )->updateMappingsInUse();
        emuThread->unlock();
    };

    control.optionControl.prioritiseLayout.controlPort.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned >("prioritise_mappings", 1);
        InputManager::getManager( this->emulator )->updateMiscSettings();
        InputManager::getManager( this->emulator )->updateMappingsInUse();
        emuThread->unlock();
    };

    control.optionControl.prioritiseLayout.keyboard.onActivate = [this]() {
        emuThread->lock();
        _settings->set<unsigned >("prioritise_mappings", 2);
        InputManager::getManager( this->emulator )->updateMiscSettings();
        InputManager::getManager( this->emulator )->updateMappingsInUse();
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

    updateMiscSettings();

    loadDeviceList();
}

auto InputLayout::updateAssigner() -> void {
    if (_settings->get<unsigned>("input_assigner", 1) == 1)
        assigner.overwriteRadio.setChecked();
    else
        assigner.appendRadio.setChecked();
}

auto InputLayout::updateMiscSettings() -> void {
    control.optionControl.oppositeDirections.setChecked( _settings->get<bool>("allow_opposite_directions", false) );

    auto prio = _settings->get<unsigned>("prioritise_mappings", 1, {0,2});
    switch(prio) {
        case 0: control.optionControl.prioritiseLayout.none.setChecked(); break;
        case 1: control.optionControl.prioritiseLayout.controlPort.setChecked(); break;
        case 2: control.optionControl.prioritiseLayout.keyboard.setChecked(); break;
    }

    selector.grabMouseLeft.setChecked( _settings->get<bool>("grab_mouse_left", false) );
}

auto InputLayout::stopCapture() -> void {
    pollTimer.setEnabled(false);
    pollTimer.setInterval(200);
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
        selector.globalHotkeys.setChecked(false);
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
    
    if (device.isJoypadOrMultiAdapter())
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
                    
        appendListEntry( useName, input, image );
        
        if (counter) {
            if (--counter == 0) {
                inputList.unlockRedraw();
            }                
        }
    }
    
    inputList.unlockRedraw();
    
    mapControl.keyLayout.setEnabled( device.isKeyboard() );    
    mapControl.automap.setEnabled( isAutomapEnabled(device) );
    
    updateConnectorButtons();
    enableConnectorButtons();
    updateAnalogSensitivity();
}

auto InputLayout::isAutomapEnabled(Emulator::Interface::Device& device) -> bool {
    if (device.isKeyboard())
        return mapControl.keyLayout.selection() != 0;

    return device.isMouse() || device.isLightDevice() || device.isPaddles();
}

auto InputLayout::loadHotkeyList() -> void {
	inputList.reset();
    control.erase.setEnabled(false);
	control.linker.setEnabled(false);
    control.mapper.setEnabled(false);
    control.eraseAlt.setEnabled(false);
	control.linkerAlt.setEnabled(false);
    control.mapperAlt.setEnabled(false);

    if (mapControl.analogSensitivity.enabled())
        mapControl.analogSensitivity.setEnabled(false);
	
    inputList.lockRedraw();
    
	for (auto& input : InputManager::getManager( emulator )->customHotkeys ) {

        InputMapping* mapping = (InputMapping*)input.guid;

        inputList.append({ "", trans->get( input.name ), mapping->getDescription(),
                           mapping->alternate ? mapping->alternate->getDescription() : "" });
	}
    
    inputList.unlockRedraw();
	
	mapControl.keyLayout.setEnabled( false );    
    mapControl.automap.setEnabled( false );
	
	for(auto& connectorButton : selector.connectorButtons)        
        connectorButton.checkButton->setEnabled( false );
}

auto InputLayout::loadGlobalHotkeyList() -> void {
    inputList.reset();
    control.erase.setEnabled(false);
    control.linker.setEnabled(false);
    control.mapper.setEnabled(false);
    control.eraseAlt.setEnabled(false);
    control.linkerAlt.setEnabled(false);
    control.mapperAlt.setEnabled(false);

    if (mapControl.analogSensitivity.enabled())
        mapControl.analogSensitivity.setEnabled(false);

    inputList.lockRedraw();

    for (auto& input : InputManager::hotkeys ) {

        InputMapping* mapping = (InputMapping*)input.guid;

        inputList.append({ "", trans->get( input.name ), mapping->getDescription(),
                           mapping->alternate ? mapping->alternate->getDescription() : "" });
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

    if (!mapControl.analogSensitivity.enabled())
        mapControl.analogSensitivity.setEnabled(true);
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
    unsigned position = _settings->get<unsigned>( "autofire_frequency", 1, {1, 99} );
    autofireControl.autofireSlider.setValue( std::to_string(position) );
    autofireControl.autofireSlider.slider.setPosition( position - 1 );
    autofireControl.autofireHold.setChecked( _settings->get<bool>( "autofire_hold", false ) );
}

auto InputLayout::appendListEntry(std::string& name, Emulator::Interface::Device::Input& input, GUIKIT::Image* image) -> void {

    InputMapping* mapping = (InputMapping*)input.guid;

    if (image && mapping->shadowMap.size() > 0)
        image = &virtualKeyImage;

    std::string transName = "";
    if (mapping->emuDevice->isFourPlayerAdapter()) {
        auto parts = GUIKIT::String::explode(name, ":");
        if (parts.size() == 2) {
            transName = trans->getA( parts[0], true ) + " " + trans->get( parts[1] );
            name = parts[1];
        }
    }

    if (transName.empty())
        transName = trans->get( name );

    inputList.append({ "", transName, mapping->getDescription(),
        mapping->alternate ? mapping->alternate->getDescription() : "" });
		
	if (image)
		inputList.setImage( inputList.rowCount() - 1, 0, *image );

    if (mapping->emuDevice && mapping->emuDevice->isJoypadOrMultiAdapter()) {
        if (input.key == Emulator::Interface::Key::Direction);
        else if (input.key == Emulator::Interface::Key::Button) {
            if (name == "Button 2")
                inputList.setRowTooltip(inputList.rowCount() - 1, trans->get("Second Button tooltip"));
        } else if (input.key == Emulator::Interface::Key::ToggleAutofire)
            inputList.setRowTooltip(inputList.rowCount() - 1, trans->get("Autofire tooltip"));
        else if (input.key == Emulator::Interface::Key::Autofire)
            inputList.setRowTooltip(inputList.rowCount() - 1, trans->get("Turbo Button tooltip"));
        else if (input.key == Emulator::Interface::Key::AutofireDirection) {
            std::string example = dynamic_cast<LIBC64::Interface*>(emulator) ? "Daley Thompson's Decathlon" : "Summer Challenge";
            inputList.setRowTooltip(inputList.rowCount() - 1, trans->get("Turbo Direction tooltip", {{"%example%", example}}));
        } else // diagonal
            inputList.setRowTooltip(inputList.rowCount() - 1, trans->get("Diagonal tooltip"));
    }
}

auto InputLayout::updateListEntry(unsigned selection, InputMapping* mapping, bool setFocus) -> void {
    unsigned column = !mapping->parent ? 2 : 3;
    inputList.setText(selection, column,  mapping->getDescription() );
    if (setFocus)
        inputList.setSelection(selection);
}

auto InputLayout::translate() -> void {
    stopCapture();
	selector.hotkeys.setText( trans->get("hotkeys") );
    selector.globalHotkeys.setText( trans->get("global hotkeys") );
    selector.grabMouseLeft.setText( trans->getA("left click grabs mouse") );
    selector.plugin.setText( trans->get("plugin", {}, true) );
    control.optionControl.oppositeDirections.setText( trans->get("allow opposite directions") );
    control.optionControl.prioritiseLayout.label.setText( trans->get("prioritise double mappings", {}, true) );
    control.optionControl.prioritiseLayout.none.setText( trans->get("none") );
    control.optionControl.prioritiseLayout.none.setTooltip( trans->get("prioritise no input device") );
    control.optionControl.prioritiseLayout.controlPort.setText( trans->get("Controlport") );
    control.optionControl.prioritiseLayout.controlPort.setTooltip( trans->get("prioritise controlport") );
    control.optionControl.prioritiseLayout.keyboard.setText( trans->get("Keyboard") );
    control.optionControl.prioritiseLayout.keyboard.setTooltip( trans->get("prioritise keyboard") );
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
    mapControl.automap.setText( trans->get("automap") );
    mapControl.analogSensitivity.name.setText( trans->get("analog_sensitivity", {}, true) );
    
    assigner.overwriteRadio.setText( trans->get("overwrite") );
    assigner.appendRadio.setText( trans->get("append") );
    assigner.assignLabel.setText( trans->get("assignment", {}, true) );
    
    unsigned i = 0;
    mapControl.keyLayout.setText( i++, trans->get("positional"));
    for ( auto& keyboardLayout : InputManager::keyboardLayouts ) {
        mapControl.keyLayout.setText( i++, trans->get( keyboardLayout.language ) + " ( " + keyboardLayout.code + " )" );
    }
    
    for(auto& connectorButton : selector.connectorButtons) 
        connectorButton.checkButton->setText( trans->get( connectorButton.connector->name ) );

    autofireControl.label.setText( trans->get("toggle autofire", {}, true) );
    autofireControl.label.setTooltip( trans->get("toggle autofire hint") );

    for(auto& button : autofireControl.buttons) {
        auto mapping = button.mapping;
        button.toggleButton->setText( trans->get(mapping->emuDevice->name) );
        button.toggleButton->setTooltip( trans->get( button.enabled ? "enabled" : "disabled") );
    }

    autofireControl.autofireSlider.name.setText( trans->get("Autofire Rate", {}, true) );
    autofireControl.autofireHold.setText( trans->get("hold Autofire") );
    autofireControl.autofireHold.setTooltip( trans->get("hold Autofire tooltip") );
    
    SliderLayout::scale({&mapControl.analogSensitivity}, "100 %");
}

auto InputLayout::updatedAutofireButtonHints() -> void {
    auto manager = InputManager::getManager( emulator );

    for(auto& button : autofireControl.buttons) {
        auto mapping = button.mapping;

        bool enabled = manager->isAutofireActive( mapping );

        if (button.enabled == enabled)
            continue;

        button.enabled = enabled;

        button.toggleButton->setTooltip( trans->get( enabled ? "enabled" : "disabled") );
    }
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
    if (hotkeyMode()) {
        auto input = InputManager::getManager( emulator )->customHotkeys[ inputId() ];

        inputIdent = input.name;

        return (InputMapping*)input.guid;
    }

    if (globalHotkeyMode()) {
        auto input = InputManager::hotkeys[ inputId() ];
        inputIdent = input.name;

        return (InputMapping*)input.guid;
    }

	auto input = emulator->devices[ deviceId() ].inputs[ inputId() ];

	inputIdent = input.name;

	return (InputMapping*)input.guid;
}

auto InputLayout::update() -> void {
	if (hotkeyMode())
		loadHotkeyList();
    else if (globalHotkeyMode())
        TabWindow::updateGlobalHotkeys();
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
    TabWindow::updateGlobalHotkeys(this->tabWindow);
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
    TabWindow::updateGlobalHotkeys(this->tabWindow);
	
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

    pollTimer.onFinished = [this, mapping, selection, overwriteMode]() {
        if (InputManager::capture(overwriteMode)) {
            stopCapture();
            updateListEntry(selection, mapping);
            TabWindow::updateGlobalHotkeys(this->tabWindow);
        } else
            pollTimer.setInterval(50);
    };
    captureTimer.onFinished = [&]() {
        stopCapture();
    };
}

auto InputLayout::hotkeyMode() -> bool {
	
	return selector.hotkeys.checked();
}

auto InputLayout::globalHotkeyMode() -> bool {

    return selector.globalHotkeys.checked();
}

auto InputLayout::triggerHotkeyMode() -> void {
	if (hotkeyMode())
		return;
	
	selector.globalHotkeys.setChecked(false);
    selector.hotkeys.setChecked();
	selector.hotkeys.onToggle();
}

auto InputLayout::loadSettings() -> void {
    
    updateAssigner();

    updateKeyLayout();

    updateAutofireFrequency();

    updateMiscSettings();
    
    update();
}
