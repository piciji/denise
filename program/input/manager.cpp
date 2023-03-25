
#include "manager.h"
#include "../program.h"
#include "../view/view.h"
#include "../config/config.h"
#include "../emuconfig/config.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../tools/chronos.h"
#include <algorithm>
#include <cstdlib>

std::vector<InputManager*> inputManagers;
InputMapping* InputManager::captureObject = nullptr;
unsigned InputManager::retry = 0;
bool InputManager::driverChange = false;
bool InputManager::urgentUpdate = true;
std::vector<Hid::Device*> InputManager::hidDevices;
std::vector<InputManager::DeviceRemap> InputManager::remapDevices;
std::vector<Hotkey> InputManager::hotkeys;
std::vector<Hotkey> InputManager::hiddenHotkeys;
std::vector<KeyboardLayout> InputManager::keyboardLayouts = { 
    {KeyboardLayout::Type::Fr, "french", "fr"},
    {KeyboardLayout::Type::De, "german", "de"},
    {KeyboardLayout::Type::Uk, "english", "uk"},
    {KeyboardLayout::Type::Us, "english", "us"},
};
InputManager::JIT InputManager::jit;

#include "hotkeys.cpp"
#include "global.cpp"
#include "capture.cpp"
#include "mapping.cpp"
#include "layout.cpp"

InputManager::InputManager(Emulator::Interface* emulator) {
    this->emulator = emulator;
    this->sendKeyChange = emulator && emulator->needExternalKeyUpdates();
}

auto InputManager::addMapping(InputMapping* mapping) -> void {
	mappings.push_back( mapping );
    mappingsInUse.push_back( mapping );
}

auto InputManager::addMappingInUse(InputMapping* mapping) -> void {
    
    if (mapping->isShadowed || (mapping->hids.size() > 0))
        mappingsInUse.push_back( mapping );
    else
        mapping->state = 0;

    for(auto& hid : mapping->hids)
        hid.disable = false;

    if (mapping->alternate) {
        if (mapping->alternate->isShadowed || (mapping->alternate->hids.size() > 0))
            mappingsInUse.push_back( mapping->alternate );
        else
            mapping->alternate->state = 0;

        for(auto& hid : mapping->alternate->hids)
            hid.disable = false;
    }
}

auto InputManager::unmapDevice(unsigned deviceId) -> void {
    for (auto& input : emulator->devices[deviceId].inputs) {
        
        auto mapping = (InputMapping*)input.guid;
        
        mapping->init();
        
        if (mapping->alternate)
            mapping->alternate->init();
    }   
    // remove empty alternate mappings
    updateMappingsInUse();
}

template<bool changeTrigger> auto InputManager::update() -> void {

    if (InputManager::urgentUpdate) {
        alternateSort();
        andTriggers.clear();
        InputManager::urgentUpdate = false;
    }
    
	int16_t value;
    unsigned ignoreBelow = 0;
    std::vector<InputMapping*> shadows;
    std::vector<InputMapping*> illegals;
    std::vector<InputMapping*> changed;
    bool hasShadow = false;
    bool hasIllegal = false;
    bool aSwitch;
    unsigned hidSize;
    uiMouse.updated = false;
    InputMapping* useMapping;
    Emulator::Interface::Device* emuDevice;
	
    updateAndTrigger();    
    
	for(auto mapping : mappingsInUse) {
        useMapping = mapping;
        if (mapping->parent)
            useMapping = mapping->parent;
                
        if (!mapping->isAnalog()) {
            if ( (mapping->sortedNext == nullptr) && (useMapping->state != 0) )
                // parent or alternate mapping has already triggered, no need to check the other one
                continue;

            if constexpr (changeTrigger) {
                if (useMapping->state)
                    changed.insert(changed.begin(), useMapping);
            }

            useMapping->state = 0;
        }
        
		auto& hids = mapping->hids;	
        hidSize = hids.size();
        aSwitch = mapping->isSwitch();
        
		if (hidSize == 0 )
            continue;
        
        if (mapping->isAnalog()) {
            
            if (mapping->emuDevice && mapping->emuDevice->isLightDevice()) {                
                
                if (hids.size() > 1) {    
                    
                    if ( (mapping->analogTimer == 0) || (--mapping->analogTimer == 0) ) {
                        // sort the hids by change delta.
                        // we use only the hid with the biggest delta                        
                        mapping->sortHidsByValue();    
                        
                        mapping->analogTimer = 5;
                    }                                                                   
                }
                
                mapping->state = mapping->adjustAnalogValue<true>( hids[0] ); 
                
            } else {
                
                value = 0;
                // 'and' logic is not applied, if emulation requests analog input.
                // to apply 'or' logic for multiple assignments we can not use the first
                // element which have a value different zero, because analog joypads show
                // sometimes minimal values in resting state.
                // multiple assignments will be simply added together.
                for(auto& hid : hids)  {
                    value += mapping->adjustAnalogValue<false>( hid );
                }
                
                mapping->state = value;
            }  
            
        } else if (!mapping->anded || (hidSize == 1) ) { //ored
            
            if (ignoreBelow)
                continue;
            
			for(auto& hid : hids) {
                
			#ifdef DEBUG_INPUT_CHANGE
				mapping->informChange(hid);
			#endif

                value = mapping->adjustDigitalValue<false>( hid );
				
				if ( value != 0) {
                    emuDevice = mapping->emuDevice;
                    if (emuDevice) {
                        if (!Program::focused && !hid.device->isJoypad() )
                            continue;
                        if (emuDevice->isMouse() && !inputDriver->mIsAcquired())
                            continue;
                    }

                  //  if (!Program::focused && mapping->emuDevice && !hid.device->isJoypad() )
                    //    continue;

                    if (hid.disable)
                        continue;
                    
					if (aSwitch && (mapping->adjustDigitalValue<true>( hid ) != 0) )
                        continue;					
                    
                    if (blockedByAndTrigger( hid ))
                        continue;
					
					if (aSwitch) {
                        emuThread->lockHotkeys();
                        hotkeyTriggers.push_back(useMapping);
                        emuThread->unlockHotkeys();
                        if constexpr (changeTrigger)
                            activeEmulator->sendKeyChange(0, nullptr);
                        break;
                    } else if (useMapping->autoFire && Program::focused) {
                        handleAutofire(mapping, useMapping, mapping->adjustDigitalValue<true>(hid) == 0);
                    } else {
                        useMapping->state = value;
                        if constexpr (changeTrigger) {
                            if (!GUIKIT::Vector::eraseVectorElement(changed, useMapping)) {
                                changed.push_back(useMapping);
                            }
                        }
                    }

                    if (useMapping->illegalMapping && !oppositeDirections) {
                        illegals.push_back(useMapping);
                        hasIllegal = true;
                    }

                    for(auto shadow : useMapping->shadowMap) {
                        shadow->virtualLinked = useMapping;
                        shadows.push_back(shadow);
                        hasShadow = true;
                    }

					break;
				}
			}
		} else { //anded	

            if (ignoreBelow && (hidSize < ignoreBelow))
                continue;
            
            bool atLeastOneKeyHasSwitched = false; //some keys are hold and final key is switched

            for (auto& hid : hids) {

                if (mapping->adjustDigitalValue<false>(hid) == 0) {
                    goto Next; //all mapped keys should be pressed

                } else if (mapping->adjustDigitalValue<true>(hid) == 0)
                    atLeastOneKeyHasSwitched = true;

                //if (!Program::focused && mapping->emuDevice && !hid.device->isJoypad() )
                  //  goto Next;

                emuDevice = mapping->emuDevice;
                if (emuDevice) {
                    if (!Program::focused && !hid.device->isJoypad() )
                        goto Next;
                    if (emuDevice->isMouse() && !inputDriver->mIsAcquired())
                        goto Next;
                }
            }

            if (!aSwitch || atLeastOneKeyHasSwitched) {
                
                if (aSwitch)
                    addAndTrigger(mapping);
                else {
                    for (auto& hid : hids) {
                        if (blockedByAndTrigger( hid ))
                            goto Next;
                    }                    
                }

                if (aSwitch) {
                    emuThread->lockHotkeys();
                    hotkeyTriggers.push_back(useMapping);
                    emuThread->unlockHotkeys();
                    if constexpr (changeTrigger)
                        activeEmulator->sendKeyChange(0, nullptr);
                } else if (useMapping->autoFire && Program::focused) {
                    handleAutofire(mapping, useMapping, atLeastOneKeyHasSwitched);
                } else {
                    useMapping->state = 1;
                    if constexpr (changeTrigger) {
                        if (!GUIKIT::Vector::eraseVectorElement(changed, useMapping))
                            changed.push_back(useMapping);
                    }
                }

                for (auto shadow : useMapping->shadowMap) {
                    shadow->virtualLinked = useMapping;
                    shadows.push_back(shadow);
                    hasShadow = true;
                }
            }
                        
            // 2 or more keys are "and" joined successfully.
            // dont expect shorter combinations triggering too, e.g. alt + 1 should not triggering 1 and so on.
            // works only if longest "and" connected mappers have been ordered first
            ignoreBelow = hidSize;            					
		}
        
        Next:
        continue;
	}

    if constexpr (!changeTrigger) {
        if (hasShadow) {
            for (auto shadow: shadows)
                shadow->state = shadow->virtualLinked->state;
        }
    } else {
        for(auto cMapping : changed) {
            if (cMapping->shadowMap.size()) {
                for (auto shadow: cMapping->shadowMap)
                    activeEmulator->sendKeyChange(cMapping->state, shadow->emuInput);
            } else
                activeEmulator->sendKeyChange(cMapping->state, cMapping->emuInput);
        }
    }

    if(hasIllegal) {
        for(auto illegal : illegals) {
            if (illegal->state == illegal->illegalMapping->state) {
                illegal->state = 0;
                illegal->illegalMapping->state = 0;
            }
        }
    }

    if (allowTouchlessAutofire && Program::focused)
        handleTouchlessAutofire();
}

auto InputManager::handleTouchlessAutofire() -> void {
    for (auto mapping : autoFireMappings) {
        if (!mapping->state) {
            bool initAutofire = false;
            for(auto& hid : mapping->hids) {
                if (mapping->adjustDigitalValue<true>(hid) == 1) {
                    initAutofire = true;
                    break;
                }
            }
            if (!initAutofire && mapping->alternate) {
                for(auto& hid : mapping->alternate->hids) {
                    if (mapping->alternate->adjustDigitalValue<true>(hid) == 1) {
                        initAutofire = true;
                        break;
                    }
                }
            }

            handleAutofire(mapping, mapping, initAutofire);
        }
    }
}

auto InputManager::handleAutofire(InputMapping* mapping, InputMapping* useMapping, bool toggleOn) -> void {
    if (toggleOn) { // start autofire
        mapping->autoFirePos = 0;
        useMapping->state = 1;
    } else {
        if (!jit.enable ||
            ((Chronos::getTimestampInMilliseconds() - jit.lastTimestamp) > jit.rescanDelay)) {
            if (mapping->autoFirePos == 0) {
                if (autoFireHold) {
                    mapping->autoFirePos = autoFireFrequency + 1;
                    useMapping->state = 1;
                } else {
                    mapping->autoFirePos = autoFireFrequency;
                    useMapping->state = 0x80;
                }
            } else {
                mapping->autoFirePos--;
                if (mapping->autoFirePos == 0)
                    useMapping->state = 1;
            }
        } else if (mapping->autoFirePos == 0)
            useMapping->state = 1;
        else
            useMapping->state = 0x80;
    }
}

inline auto InputManager::updateAndTrigger() -> void {
    
    if (andTriggers.size() == 0)
        return;
    
    std::vector<InputMapping*> toDelete;
    
    for(auto andTrigger : andTriggers ) {
        bool next = false;
        
        for (auto& hid : andTrigger->hids) {
            if (andTrigger->adjustDigitalValue<false>(hid) != 0) {
                next = true;
                break;
            }                
        }
        
        if (next)
            continue;
        
        toDelete.push_back( andTrigger );
    }
    
    for (auto delMapping : toDelete)        
        GUIKIT::Vector::eraseVectorElement( andTriggers, delMapping );    
}

inline auto InputManager::addAndTrigger(InputMapping* newTrigger) -> void {
    
    for(auto andTrigger : andTriggers ) {
        if (andTrigger == newTrigger)
            return;
    }
    
    andTriggers.push_back(newTrigger);
}

inline auto InputManager::blockedByAndTrigger(InputMapping::Assign& hid) -> bool {
    
    for(auto andTrigger : andTriggers ) {
        for (auto& andHid : andTrigger->hids) {
            if (hid.device == andHid.device && hid.group == andHid.group && hid.input == andHid.input)
                return true;            
        }
    }
    return false;
}

auto InputManager::updateAllMappingsInUse( bool emuOnly ) -> void {
	
	for (auto manager : inputManagers) {
		
		if (emuOnly && !manager->emulator)
			continue;
			
		manager->updateMappingsInUse();		
	}
}

auto InputManager::updateMappingsInUse() -> void {
    
    std::vector<Emulator::Interface::Device*> connectedDevices;
    
    if (emulator) {        
        for (auto& connector : emulator->connectors)            
            connectedDevices.push_back( emulator->getConnectedDevice( &connector ) );
    }

    auto copyAutofireMappings = autoFireMappings;
    autoFireMappings.clear();
    mappingsInUse.clear();
    
    for(auto mapping : mappings) {	
        
        // hotkeys
        if (!mapping->emuDevice)            
            addMappingInUse( mapping );
        
        else if (mapping->emuDevice->isKeyboard())
            addMappingInUse( mapping );
        
        else if ( GUIKIT::Vector::find( connectedDevices, mapping->emuDevice ) ) {
            addMappingInUse(mapping);

            if (GUIKIT::Vector::find(copyAutofireMappings, mapping))
                autoFireMappings.push_back(mapping);
        }
    }

    allowTouchlessAutofire = autoFireMappings.size() > 0;
    sort();

    if(prioritise)
        prioritiseConnectedDevicesOverKeyboard();

    if (emulator && (autoFireMappings.size() != copyAutofireMappings.size())) {
        auto emuView = EmuConfigView::TabWindow::getView( emulator );
        if (emuView && emuView->inputLayout)
            emuView->inputLayout->updatedAutofireButtonHints();
    }
}

auto InputManager::sort() -> void {
	//always sort longest "and" connected mappers first
	std::sort(mappingsInUse.begin(), mappingsInUse.end(), [ ](const InputMapping* lhs, const InputMapping* rhs) {
		//return lhs->anded == rhs->anded ? lhs->hids.size() > rhs->hids.size() : lhs->anded;
        return (lhs->anded == rhs->anded) ? ((lhs->hids.size() == rhs->hids.size()) ? (lhs->isSwitch() > rhs->isSwitch()) : (lhs->hids.size() > rhs->hids.size())) : lhs->anded;
	});
        
    // this is needed because hotplug mappings are shared.
    InputManager::urgentUpdate = true;
}

auto InputManager::alternateSort() -> void {
    std::vector<InputMapping*> temp;
    
    for(auto mapping : mappingsInUse) {
        
        if ( GUIKIT::Vector::eraseVectorElement( temp, mapping ) )
            continue;
        
        if (mapping->parent) {
            // alternate mapping
            mapping->sortedNext = mapping->parent;
            mapping->parent->sortedNext = nullptr;
            temp.push_back( mapping->parent );
            
        } else if (mapping->alternate) {
            mapping->sortedNext = mapping->alternate;
            mapping->alternate->sortedNext = nullptr;
            temp.push_back( mapping->alternate );
        }
    }
}
