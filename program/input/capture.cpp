
#include "manager.h"

auto InputManager::resetJit() -> void {
    jit.enable = false;
    jit.lastTimestamp = 0;
}

auto InputManager::poll() -> void {
    if (captureObject)
        return;

    if (!jit.enable) {
        fetch();
        getManager(activeEmulator)->update();
      //  logger->log("normal", true);
    } else {
        jit.enable = false;
      //  logger->log("jit", true);
    }
            
    pollHotkeys();
}

auto InputManager::jitPoll() -> bool {
    if (captureObject)
        return false;
    
    auto ts = Chronos::getTimestampInMilliseconds();
    
    if ((ts - jit.lastTimestamp) >= jit.rescanDelay) {
        
        jit.lastTimestamp = ts;
        
        fetch();
        getManager( activeEmulator )->update();
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
		if (configView)
		    configView->inputLayout->loadInputList();
		for( auto emuView : emuConfigViews )
            emuView->inputLayout->update();
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
		captureObject->updateSetting();
		captureObject = nullptr;
        retry = 0;
		updateAllMappingsInUse();        
		return true;
	}
    
	return false;
}
