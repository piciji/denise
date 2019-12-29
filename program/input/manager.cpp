
#include "manager.h"
#include "../program.h"
#include "../view/view.h"
#include "../config/config.h"
#include "../emuconfig/config.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../tools/status.h"
#include <algorithm>
#include <cstdlib>

std::vector<InputManager*> inputManagers;
InputMapping* InputManager::captureObject = nullptr;
unsigned InputManager::retry = 0;
bool InputManager::driverChange = false;
std::vector<Hid::Device*> InputManager::hidDevices;
std::vector<InputManager::DeviceRemap> InputManager::remapDevices;
std::vector<Hotkey> InputManager::hotkeys;
std::vector<KeyboardLayout> InputManager::keyboardLayouts = { 
    {KeyboardLayout::Type::Fr, "french", "fr"},
    {KeyboardLayout::Type::De, "german", "de"},
    {KeyboardLayout::Type::Uk, "english", "uk"},
    {KeyboardLayout::Type::Us, "english", "us"},
};

#include "hotkeys.cpp"
#include "global.cpp"
#include "capture.cpp"
#include "mapping.cpp"
#include "layout.cpp"

InputManager::InputManager(Emulator::Interface* emulator) {    
    this->emulator = emulator;
}

auto InputManager::addMapping(InputMapping* mapping) -> void {
	mappings.push_back( mapping );
    mappingsInUse.push_back( mapping );
}

auto InputManager::addMappingInUse(InputMapping* mapping) -> void {
    
    mappingsInUse.push_back( mapping );
    if (mapping->alternate && (mapping->alternate->hids.size() > 0) )
        mappingsInUse.push_back( mapping->alternate );
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

auto InputManager::update() -> void {
	int16_t value;
    unsigned ignoreBelow = 0;
    std::vector<InputMapping*> shadows;
    bool aSwitch;
    unsigned hidSize;
    uiMouse.updated = false;
    InputMapping* useMapping;
	
    updateAndTrigger();    
    
	for(auto mapping : mappingsInUse) {		       

        useMapping = mapping;
        if (mapping->parent)
            useMapping = mapping->parent;
                
        if (!mapping->isAnalog()) {
            if ( (mapping->sortedNext == nullptr) && (useMapping->state != 0) )
                // parent or alternate mapping has already triggered, no need to check the other one
                continue;        

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
            
			for(auto& hid : hids) {
                
			#ifdef DEBUG_INPUT_CHANGE
				mapping->informChange(hid);
			#endif

                value = mapping->adjustDigitalValue<false>( hid );
				
				if ( value != 0) {
                    
                    if (hid.disable)
                        continue;                    
                    
					if (aSwitch && (mapping->adjustDigitalValue<true>( hid ) != 0) )
                        continue;					
                    
                    if (blockedByAndTrigger( hid ))
                        continue;
                    
                    useMapping->state = value;
                    for(auto shadow : useMapping->shadowMap)
                        shadows.push_back( shadow );

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
            }

            if (!aSwitch || atLeastOneKeyHasSwitched) {
                
                addAndTrigger(mapping);
                
                useMapping->state = 1;
                for (auto shadow : useMapping->shadowMap)
                    shadows.push_back(shadow);
            }
                        
            // 2 or more keys are "and" joined successfully.
            // dont expect shorter combinations triggering too, e.g. alt + 1 should not triggering 1 and so on.
            // works only if longest "and" connected mappers have been ordered first
            ignoreBelow = hidSize;            					
		}
        
        Next:
            continue;
	}
    
    if(shadows.size())
        for(auto shadow : shadows)
            shadow->state = 1;
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

auto InputManager::updateMappingsInUse() -> void {
    
    std::vector<Emulator::Interface::Device*> connectedDevices;
    
    if (emulator) {        
        for (auto& connector : emulator->connectors)            
            connectedDevices.push_back( emulator->getConnectedDevice( &connector ) );
    }
    
    mappingsInUse.clear();        
    
    for(auto mapping : mappings) {	
        
        // hotkeys
        if (!mapping->emuDevice)            
            addMappingInUse( mapping );
        
        else if (mapping->emuDevice->isKeyboard())
            addMappingInUse( mapping );
        
        else if ( GUIKIT::Vector::find( connectedDevices, mapping->emuDevice ) )
            addMappingInUse( mapping );                
    }
    
    sort();
    priorizeConnectedDevicesOverKeyboard();
}

auto InputManager::sort() -> void {
	//always sort longest "and" connected mappers first
	std::sort(mappingsInUse.begin(), mappingsInUse.end(), [ ](const InputMapping* lhs, const InputMapping* rhs) {
		return lhs->anded == rhs->anded ? lhs->hids.size() > rhs->hids.size() : lhs->anded;
	});

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

