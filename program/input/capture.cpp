
#include "manager.h"

DRIVER::Input::KeyCallback InputManager::keyCallback = []() {
    if (Program::focused && activeEmulator) {
        activeEmulator->informAboutKeyUpdate();
        //statusHandler->setMessage("key change");
    }
};

auto InputManager::setupKeycodeTransfer() -> void {
    if (sendKeyChange)
        inputDriver->setKeyboardCallback( &keyCallback );
    else
        inputDriver->setKeyboardCallback( nullptr );
}

auto InputManager::resetJit() -> void {
    jit.enable = false;
    jit.lastTimestamp = 0;
}

auto InputManager::poll() -> void {
    if (captureObject)
        return;

    if (!jit.enable) {
        fetch();
        if (activeInputManager->sendKeyChange)
            activeInputManager->update<true>();
        else
            activeInputManager->update<false>();
        //logger->log("normal", true);
    } else {
        jit.enable = false;
        //logger->log("jit", true);
    }

    if (emuThread->enabled)
        emuThread->pollHotkeys = true;
    else
        pollHotkeys();
}

auto InputManager::jitPoll(int delay) -> bool {
    if (captureObject)
        return false;
    
    auto ts = Chronos::getTimestampInMilliseconds();

    if(autofireDirection) {
        delay = 1;
        autofireDirection--;
    }

    if ((ts - jit.lastTimestamp) >= ( (delay >= 0) ? delay : jit.rescanDelay)) {
        jit.lastTimestamp = ts;
        //statusHandler->setMessage("jit", 1);
        fetch();
        if (activeInputManager->sendKeyChange)
            activeInputManager->update<true>();
        else
            activeInputManager->update<false>();
        jit.enable = true;
        return true;
    }       
    
    return false;
}

auto InputManager::fetch() -> void {
    auto curDevices = inputDriver->poll();
	
	bool changed = curDevices.size() != hidDevices.size();
	if (!changed) {
		for(unsigned i=0; i < hidDevices.size(); i++) {
			changed = hidDevices[i] != curDevices[i];
			if (changed) break;
		}
	}
	if(changed) {
		hidDevices = curDevices;
		bindHidsGlobal();

		for( auto emuView : emuConfigViews ) {
            if (emuView->inputLayout)
                emuView->inputLayout->update();
        }
	}
}

auto InputManager::capture(InputMapping* _captureObject) -> void {
    if (captureObject) return;
    captureObject = _captureObject;
    for (auto manager : inputManagers)
        manager->andTriggers.clear();
    retry = 0;
    fetch();
}

auto InputManager::capture( bool overwriteExisting ) -> bool {
    static unsigned countAssignments = 0;

    if (captureObject == nullptr)
        return false;
    
    fetch();
	
	for (auto hidDevice : hidDevices) {
		for(auto& group : hidDevice->groups) {
			for(auto& input : group.inputs) {
                if (std::abs(input.value) <= std::abs(input.oldValue) ) continue;
				unsigned qualifier = InputMapping::Qualifier::None;
				unsigned result = captureObject->checkSanity(hidDevice, group.id, input.id);
				if (result == 0) continue;				
				
				if (result == 1) { //analog
					if (hidDevice->isJoypad() && input.value >= -16384 && input.value <= 16384) continue;
                    if (hidDevice->isMouse() && std::abs(input.value) < 16 ) continue;                    
				} else if (result == 3) { //digital axis, hats needs qualifier
					if		(input.value < -16384) qualifier = InputMapping::Qualifier::Lo;
					else if (input.value > +16384) qualifier = InputMapping::Qualifier::Hi;
					else	continue;
				}				

                if ( overwriteExisting && (retry == 0) )
                    captureObject->hids.clear();
                
				bool alreadyMapped = false;
				for(auto& hid : captureObject->hids) {
					if (hid.device == hidDevice && hid.group->id == group.id && hid.input->id == input.id && hid.qualifier == qualifier) {
						alreadyMapped = true;
						break;
					}						
				}
				
                if (!alreadyMapped) {                   
                    if (captureObject->hids.size() == InputManager::MaxMappings) {                        
                        if (!overwriteExisting)
                            GUIKIT::Vector::eraseVectorPos( captureObject->hids, 0 );
                        else
                            goto CaptureEnd;
                    }
                    
                    captureObject->hids.push_back({ hidDevice, &group, &input, qualifier, 0 });
                    countAssignments++;
                }
                
                if (result == 1)
                    goto CaptureEnd;
                
                if (retry == 0)
                   retry = 3; 
			}
		}
	}    
    
    if (retry && (--retry == 0) ) {                	

        CaptureEnd:
        if (overwriteExisting && (countAssignments > 1) ) {
            captureObject->anded = true;
        }
        preventSharingOfAutoFireMappings(captureObject, captureObject->hids.back());
		captureObject->updateSetting();
		captureObject = nullptr;
        retry = 0;
        countAssignments = 0;
		updateAllMappingsInUse();
		return true;
	}
    
	return 0;
}

auto InputManager::preventSharingOfAutoFireMappings(InputMapping* captureObject, InputMapping::Assign& captureHid) -> void {
    unsigned pos;
    bool found = false;
    std::vector<Emulator::Interface::Device::Input*> _inputs;

    if (captureObject->anded && (captureObject->hids.size() > 1) )
        return;
    auto manager = captureObject->inputManager;
    if (!manager)
        return;
    auto emulator = manager->emulator;
    if (!emulator)
        return;

    if (!captureObject->emuDevice || !captureObject->emuDevice->isJoypad())
        return;

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    for (auto& input : captureObject->emuDevice->inputs) {
        InputMapping* mapper = (InputMapping*)input.guid;

        if (!mapper)
            continue;

        if (input.key == Emulator::Interface::Key::Button || input.key == Emulator::Interface::Key::Autofire || input.key == Emulator::Interface::Key::ToggleAutofire ) {
            if (mapper == captureObject || mapper->alternate == captureObject)
                found = true;
            else
                _inputs.push_back( &input );
        }
    }

    if (!found || (_inputs.size() == 0) )
        return;

    for (auto input : _inputs) {
        InputMapping* mapper = (InputMapping*)input->guid;

        while(1) {
            pos = 0;
            if (mapper->anded && (mapper->hids.size() > 1) );
            else {
                for (auto& hid : mapper->hids) {
                    if (hid.device == captureHid.device && hid.group->id == captureHid.group->id &&
                        hid.input->id == captureHid.input->id && hid.qualifier == captureHid.qualifier) {
                        GUIKIT::Vector::eraseVectorPos(mapper->hids, pos);
                        mapper->updateSetting();
                        if (emuView && emuView->inputLayout)
                            emuView->inputLayout->updateListEntry(input->id, mapper, false);
                        break;
                    }
                    pos++;
                }
            }

            if (mapper->alternate)
                mapper = mapper->alternate;
            else
                break;
        }
    }
}
