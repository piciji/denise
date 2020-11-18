
#include "manager.h"

auto InputManager::getManager( Emulator::Interface* emulator ) -> InputManager* {
	
	for (auto manager : inputManagers) {
		if (manager->emulator == emulator)
			return manager;
	}
	return nullptr;
}

auto InputManager::build() -> void {
    setHotkeys();
    setMappings();	
}

auto InputManager::init() -> void {       
	hidDevices = inputDriver->poll();
    assignChangedDeviceState();
	
    for (auto manager : inputManagers) {
        
        if (!manager->emulator)
            continue;
        
        manager->updateAnalogSensitivity();
        
        auto settings = program->getSettings( manager->emulator );
        
        auto alreadyMapped = settings->get<bool>( "automapped", false);    
        
        if (alreadyMapped)
            continue;
        
        manager->autoAssign( assumeLayoutType(), false );	                    
        
        settings->set<bool>( "automapped", true );
    }           
        
    autoAssignHotkeys();
	bindHidsGlobal();
}

auto InputManager::assumeLayoutType() -> KeyboardLayout::Type {
    
    auto lang = GUIKIT::System::getOSLang();
    
    if (lang == GUIKIT::System::Language::DE)
        return KeyboardLayout::Type::De;
    
    if (lang == GUIKIT::System::Language::US)
        return KeyboardLayout::Type::Us;
    
    if (lang == GUIKIT::System::Language::FR)
        return KeyboardLayout::Type::Fr;

    return KeyboardLayout::Type::Uk;
}

auto InputManager::setMappings() -> void {
    Emulator::Interface::Device::Input* alternateInput;
            
	for (auto manager : inputManagers) {
		if (manager->emulator) {		
            auto settings = program->getSettings( manager->emulator );
            
            for (auto& device : manager->emulator->devices) {  
                for (auto& input : device.inputs) {

                    std::string settingIdent = device.name + "_" + std::to_string(input.id);
                    GUIKIT::String::toLowerCase(GUIKIT::String::delSpaces(settingIdent));
                    InputMapping* mapper = new InputMapping;
                    mapper->setting = settings->add( settingIdent );
                    mapper->state = 0;
                    mapper->type = (InputMapping::Type)input.type;
                    mapper->anded = 0;
                    mapper->emuDevice = &device;
                    mapper->parent = nullptr;
                    mapper->alternate = nullptr;
                    mapper->inputManager = manager;
                    input.guid = (uintptr_t) mapper;
                    
                    for( auto& inputId : input.shadowMap ) {
                        auto inputPtr = &device.inputs[ inputId ];

                        if (inputPtr->isDigital())
                            mapper->shadowMap.push_back( (InputMapping*)(inputPtr->guid) );
                    }
                    
                    manager->addMapping( mapper );
                    if (!mapper->isAnalog())
                        mapper->generateAlternate( settings );
                }
            }
			
			manager->setCustomHotkeys();
			
			for(auto& item : manager->customHotkeys) {
				InputMapping* mapper = new InputMapping;
				mapper->setting = settings->add( "hotkey_" + std::to_string(item.id) );
				mapper->state = 0;
				mapper->type = InputMapping::Type::Switch;
				mapper->anded = 1;
				mapper->emuDevice = nullptr;
				mapper->hotkeyId = item.id;
				mapper->parent = nullptr;
				mapper->inputManager = manager;
				item.guid = (uintptr_t) mapper;

				if (item.share) {
					for (auto _manager : inputManagers)
						_manager->addMapping( mapper ); //hotkeys are shared between all manager instances
				} else
					manager->addMapping( mapper );

				mapper->generateAlternate( settings );
			}
		}
	}
	
	for(auto& item : hotkeys) {
		item.share = true; // always share global hotkeys
        InputMapping* mapper = new InputMapping;
		mapper->setting = globalSettings->add( "hotkey_" + std::to_string(item.id) );
        mapper->state = 0;
        mapper->type = InputMapping::Type::Switch;
		mapper->anded = 1;
        mapper->emuDevice = nullptr;
		mapper->hotkeyId = item.id;
        mapper->parent = nullptr;
        mapper->inputManager = nullptr; // shared mapper don't belong to a specific manager
        item.guid = (uintptr_t) mapper;
		
		for (auto manager : inputManagers)
			manager->addMapping( mapper ); //hotkeys are shared between all manager instances
        
        mapper->generateAlternate( globalSettings );
    }
}

auto InputManager::autoAssign( KeyboardLayout::Type type, bool keyboardOnly ) -> void {
    
    if(!emulator)
        return;

    for( auto& device : emulator->devices ) {
        if (device.name.find("#2") != std::string::npos)
            continue;
        
        for (auto& input : device.inputs) {
            auto mapper = (InputMapping*) input.guid;
            auto setting = mapper->setting->value;
            if (!setting.empty() && (setting != "0"))
                continue;
            
            std::vector<std::vector<Hid::Key>> keys;
            if (device.isKeyboard())
                keys = automap( type, input.key );

            for (auto hidDevice : hidDevices) {
                if ( (hidDevice->isKeyboard() && device.isKeyboard()) || 
                    ( !keyboardOnly && (
                        (hidDevice->isMouse() && (device.isMouse() || device.isLightDevice() || device.isPaddles() )) ||
                        (hidDevice->isKeyboard() && device.isJoypad() )
                    ) )                    
                    ) {
                    for(auto& group : hidDevice->groups) {
                        for(auto& hidInput : group.inputs) {

                            if (device.isKeyboard()) {
                                mapper->anded = 1;
                                mapper->alternate->anded = 1;

                                if (keys[0].size() == 0 && keys[1].size() == 0) // all elements were mapped
                                    break;

                                for( auto& key : keys[0] ) {
                                    if (key == hidInput.key) {
                                        mapper->hids.push_back( {hidDevice, &group, &hidInput, 0, 0} );
                                        // better we remove the element, as a result multiple
                                        // mappings of the same key can not accidently found twice
                                        GUIKIT::Vector::eraseVectorElement( keys[0], key );
                                        break;
                                    }
                                }
                                if (keys.size() > 1)
                                    for (auto& key : keys[1]) {
                                        if (key == hidInput.key) {
                                            mapper->alternate->hids.push_back({hidDevice, &group, &hidInput, 0, 0});
                                            GUIKIT::Vector::eraseVectorElement(keys[1], key);
                                            break;
                                        }
                                    }
                            } else if ( device.isJoypad() ) {
                                if (matchButtons(&input, &hidInput)) {
                                    mapper->hids.push_back( {hidDevice, &group, &hidInput, 0, 0} );
                                    mapper->anded = 0;                                    
                                    break;
                                }                                
                            } else if (matchButtons(&input, &hidInput) || (input.name.find(hidInput.name) != std::string::npos) ) {
                                mapper->hids.push_back( {hidDevice, &group, &hidInput, 0, 0} );
                                mapper->anded = 0;
                                break;
                            }                                        
                        }
                    }						
                }					
            }
                          
            mapper->updateSetting();
            if (mapper->alternate)
                mapper->alternate->updateSetting();
        }			
    }
}

auto InputManager::matchButtons( Emulator::Interface::Device::Input* emuInput, Hid::Input* hidInput ) -> bool {
    
    if (emuInput->name == "Up" && hidInput->key == Hid::Key::CursorUp) return true;
    if (emuInput->name == "Down" && hidInput->key == Hid::Key::CursorDown) return true;
    if (emuInput->name == "Left" && hidInput->key == Hid::Key::CursorLeft) return true;
    if (emuInput->name == "Right" && hidInput->key == Hid::Key::CursorRight) return true;
    if (emuInput->name == "Button 1" && hidInput->key == Hid::Key::Space) return true;
    
    if (emuInput->name == "Button X" && hidInput->name == "Left") return true;
    if (emuInput->name == "Button Y" && hidInput->name == "Right") return true;
    
    if (emuInput->name == "Trigger" && hidInput->name == "Left") return true;
    
    if (emuInput->name == "Touch" && hidInput->name == "Left") return true;
    if (emuInput->name == "Button" && hidInput->name == "Right") return true;
    
    return false;
}

auto InputManager::priorizeConnectedDevicesOverKeyboard() -> void {

    for(auto mapping : mappingsInUse) {        
        for(auto& hid : mapping->hids)
            hid.disable = false;
    }
    
    if (!emulator)
        return;
    
    Emulator::Interface::Device* keyboard = nullptr;
    
    for( auto& device : emulator->devices ) {
        if (device.isKeyboard()) {
            keyboard = &device;
            break;
        }
    }
    
    if (!keyboard)
        return;
    
    std::vector<Emulator::Interface::Device*> connectedDevices;    
    for (auto& connector : emulator->connectors)  {
        
        auto device = emulator->getConnectedDevice( &connector );
        
        if (!device->isUnplugged())
            connectedDevices.push_back( device );
    }         
        
    if (connectedDevices.size() == 0)
        return;
    
    for( auto& device : emulator->devices ) {
        if (device.isKeyboard())
            continue;
        
        if (!GUIKIT::Vector::find( connectedDevices, &device ))
            continue;
        
        for (auto& input : device.inputs) {            
            if (!input.isDigital())
                continue;
            
            auto mapper = (InputMapping*) input.guid;
            
            while(1) {
                
                if (mapper->hids.size() == 0)
                    goto NextMapper;

                if (mapper->hids.size() > 1)
                    goto NextMapper;

                for (auto& hid : mapper->hids) {

                    for(auto& keyboardInput : keyboard->inputs) {

                        auto keyboardMapper = (InputMapping*) keyboardInput.guid;
                        
                        while(1) {

                            if (keyboardMapper->hids.size() == 0)
                                goto NextKeyboardMapper;

                            if (keyboardMapper->hids.size() > 1)
                                goto NextKeyboardMapper;

                            for (auto& keyboardHid : keyboardMapper->hids) {

                                if (keyboardHid.disable)
                                    continue;
                                
                                if (keyboardHid.device == hid.device && keyboardHid.group == hid.group
                                && keyboardHid.input == hid.input && keyboardHid.qualifier == hid.qualifier ) {
                                    keyboardHid.disable = true;                                                        
                                }
                            }
                            
                            NextKeyboardMapper:
                            if (keyboardMapper->alternate)
                                keyboardMapper = keyboardMapper->alternate;
                            else
                                break;
                        }
                    }
                }
                NextMapper:
                if (mapper->alternate)
                    mapper = mapper->alternate;
                else
                    break;
            }            
        }
    }
}

auto InputManager::autoAssignHotkeys() -> void {
    for (auto& hotkey : hotkeys) {
        if (hotkey.id == Hotkey::Id::CaptureMouse) {
            auto mapper = (InputMapping*)hotkey.guid;
            auto setting = mapper->setting->value;
            if (!setting.empty())
                continue;
			
			for (auto hidDevice : hidDevices) {
				if(hidDevice->isMouse()) {
					for (auto& hidInput : hidDevice->group(Hid::Mouse::GroupID::Button).inputs) {
						if ( hidInput.name.find("Middle") != std::string::npos ) {
							mapper->hids.push_back( {hidDevice, &hidDevice->group(Hid::Mouse::GroupID::Button), &hidInput, 0, 0} );
							mapper->anded = 1;
							mapper->updateSetting();
							break;
						}
					}
				}  
			}	              
            break;
        }
    }
}

// load settings data at application start, reload only when host devices changing
// missing devices will be removed
auto InputManager::bindHidsGlobal( ) -> void {	
    
	for (auto manager : inputManagers)
        manager->bindHids();        
	
    clearLastDeviceState();
}

auto InputManager::bindHids( ) -> void {
    
    for (auto mapping : mappings) {
        while (1) {

            mapping->hids.clear();
            auto parts = GUIKIT::String::split(mapping->setting->value, '|');

            if (parts.size() == 0)
                goto Done;

            mapping->anded = parts[0] != "0";
            mapping->hasUnknownAssignment = false;
            GUIKIT::Vector::eraseVectorPos(parts, 0);

            for (;;) {
                if (mapping->hids.size() == InputManager::MaxMappings) break;
                if (parts.size() < 4) break;
                try {
                    unsigned deviceId = std::stoul(parts[0]);
                    unsigned groupId = std::stoul(parts[1]);
                    unsigned inputId = std::stoul(parts[2]);
                    unsigned qualifier = std::stoul(parts[3]);

                    Hid::Device* hidDevice = getDeviceFromIdent(deviceId);

                    if (hidDevice) {

                        unsigned result = mapping->checkSanity(hidDevice, groupId, inputId);
                        if (result == 0);
                        else if (result == 3 && qualifier == 0);

                        else mapping->hids.push_back({hidDevice, &hidDevice->group(groupId),
                                &hidDevice->group(groupId).inputs[inputId], qualifier, 0});
                    } else if (!driverChange)
                        mapping->hasUnknownAssignment = true;

                } catch (...) {
                    /* conversion to uint failed */                }

                GUIKIT::Vector::eraseVectorPos(parts, 0, 4);
            }

Done:
            if (driverChange)
                // remove unknown devices when driver is changing
                mapping->updateSetting(); //restructure setting in case of corruption or removed devices

            if (!mapping->alternate)
                break;

            mapping = mapping->alternate;
        }
    }
    
    updateMappingsInUse();
}

auto InputManager::clearLastDeviceState() -> void {
    for (auto& remap : remapDevices)
        delete remap.remember;    
    
    remapDevices.clear();
    driverChange = false;
}

auto InputManager::rememberLastDeviceState() -> void {
    
    clearLastDeviceState();
    Hid::Device* remember;
    
    for (auto hidDevice : hidDevices) {
        
        if (hidDevice->isMouse())
            remember = new Hid::Mouse;
        else if (hidDevice->isJoypad())
            remember = new Hid::Joypad;
        else
            continue;
        // don't remember keyboard, all driver use device id = 0 for keyboards
        
        // remember id of device just before a driver change.
        // each driver has it's own unique way for generating device id's.
        // we need to connect these id's, otherwise all taken mappings will be lost when driver is changed
        remember->id = hidDevice->id;
        remember->name = hidDevice->name;
        
        remapDevices.push_back( { remember, nullptr } );
    }
    
    driverChange = true;
}

auto InputManager::assignChangedDeviceState() -> void {
    
    std::vector<Hid::Device*> todo;
    
    for (auto hidDevice : hidDevices) {
        
        bool found = false;
        
        for (auto& remap : remapDevices) {
            
            if (remap.current)
                continue;            

            if (remap.remember->isMouse() && hidDevice->isMouse()) {
                remap.current = hidDevice;
                found = true;
                break;
            }
            
            if (remap.remember->isJoypad() && hidDevice->isJoypad()) {
                if (remap.remember->name == hidDevice->name) {
                    remap.current = hidDevice;
                    found = true;
                    break;                    
                }
            }
        }
        
        if (!found)
            todo.push_back( hidDevice );       
    }
    
    for (auto hidDevice : todo) {
        for (auto& remap : remapDevices) {

            if (remap.current)
                continue;  

            if (remap.remember->isJoypad() && hidDevice->isJoypad()) {
                remap.current = hidDevice;
                break;
            }
        }
    }
}

auto InputManager::getDeviceFromIdent( unsigned id ) -> Hid::Device* {
    
    for (auto hidDevice : hidDevices) {
        
        if (hidDevice->id == id)
            return hidDevice;
    }
    
    for (auto& remap : remapDevices) {
        
        if (!remap.current)
            continue;
        
        if (remap.remember->id == id)
            return remap.current;
    }
    
    return nullptr;
}

auto InputManager::updateAnalogSensitivity(Emulator::Interface::Device* updateDevice) -> void {
    
    int tresholdLo = 200;
    int tresholdHi = 100;
    
    for (auto& device : emulator->devices) {
        
		if (updateDevice && updateDevice != &device)
			continue;		        
        
        auto sensePercent = program->getSettings(emulator)->get<unsigned>( "analog_sensitivity_" + _underscore(device.name), 50u, { 0u, 100u});

		int sense = sensePercent;
				
		if (device.isJoypad() || device.isKeyboard()) {
			sense = (sensePercent * 32768) / 100;

			sense = std::min(std::max(sense, tresholdLo), 32768 - tresholdHi);		
		} else
			sense = std::max( 5, sense );
         
        for (auto& input : device.inputs) {
            
            auto mapper = (InputMapping*)input.guid;                        
            
			mapper->analogSensitivity = sense;			
			
            if (mapper->alternate)
                mapper->alternate->analogSensitivity = mapper->analogSensitivity;
        }        
        
    }
}
