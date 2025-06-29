
#include "manager.h"

auto InputMapping::checkSanity(Hid::Device* device, unsigned groupId, unsigned inputId) -> unsigned {
	//check boundaries, could loaded from corrupted settings file
	if (device->groups.size() <= groupId) return 0;
	if (device->group(groupId).inputs.size() <= inputId) return 0;		
    
    if (isSwitch() && device->isMouse()) {
        // don't use left mouse button for hotkeys (i.e. fullscreen -> big problem)
        if (device->group(groupId).inputs[inputId].name.find("Left") != std::string::npos)            
            return 0;
    }
    
    //allowed host input type for requested emulation input type
	if (isAnalog()) {
		if ((device->isMouse() && groupId == Hid::Mouse::GroupID::Axis)
		|| (device->isJoypad() && groupId == Hid::Joypad::GroupID::Axis)
		|| (device->isJoypad() && groupId == Hid::Joypad::GroupID::Hat))
			return 1;
	} else { //digital button or switch
        if (device->isMouse() && emuDevice && emuDevice->isJoypadOrMultiAdapter())
            return 0;

		if ((device->isMouse() && groupId == Hid::Mouse::GroupID::Button)
		|| (device->isJoypad() && groupId == Hid::Joypad::GroupID::Button)
		|| (device->isKeyboard() && groupId == Hid::Keyboard::GroupID::Button))
			return 2;

		if ((device->isJoypad() && groupId == Hid::Joypad::GroupID::Axis)
		|| (device->isJoypad() && groupId == Hid::Joypad::GroupID::Hat)
		|| (device->isJoypad() && groupId == Hid::Joypad::GroupID::Trigger)) {
			return 3;
		}			
	}
	return 0;
}

auto InputMapping::updateSetting() -> void {
    hasUnknownAssignment = false;
    
    if (hids.size() == 0) {
        setting->value = "";
        return;
    }

    setting->value = anded ? "1" : "0";
	
	for(auto& hid : hids) {
        
		setting->value += "|" + std::to_string(hid.device->id) + "|" 
				+ std::to_string(hid.group->id) + "|"
				+ std::to_string(hid.input->id) + "|"
				+ std::to_string(hid.qualifier);
	}
}

auto InputMapping::init() -> void {
	hids.clear();
    state = 0;
	updateSetting();
}

inline auto InputMapping::applyMouseSensitivity( int16_t value ) -> int16_t {
	
	return value * analogSensitivity / 50;
}

inline auto InputMapping::applyAxisSensitivity( int16_t value ) -> int16_t {

    if (emuDevice->isPaddles())
        value = (value * analogSensitivity / 50) >> 8;
    else
	    value = (value * analogSensitivity / 50) >> 10;
    
    // some analog joypads show minimal movement in resting state.
    if ( value == 1 || value == -1 )
        value = 0;
    
    return value;
}

template<bool lightMode> auto InputMapping::adjustAnalogValue( Assign& hid ) -> int16_t {

    // for light gun or pen driven software there will be no cursor displayed. in emulation
    // we need some cursor to track movement. if mouse is captured we need to draw a cursor.
    // otherwise we use mouse cursor directly and convert scaled host position to emulated
    // raw screen position
    if (lightMode && !inputManager->emulator)
        return 0;

    Hid::Device* device = hid.device;
    unsigned groupId = hid.group->id;
    int16_t value = hid.input->value;    
    
    if (device->isMouse()) {                       
        
        if (!inputDriver->mIsAcquired()) {
            
            if (lightMode) {
                // if mouse is not acquired we use the mouse position given by OS 
                // within viewport
                if ( !inputManager->uiMouse.updated ) {
                    inputManager->uiMouse.updated = true;
                    inputManager->uiMouse.pos = program->absoluteMouseToEmu( inputManager->emulator );
                }

                // expect first input element as x-axis
                if (hid.input->id == 0) 
                    value = inputManager->uiMouse.pos.x;
                else
                    value = inputManager->uiMouse.pos.y;            

                emuDevice->userData = 1; // inform emu to not render a cursor
            } else    
                value = 0;
            
        } else {
            
            value = applyMouseSensitivity( value );    
        }
        
    } else if (device->isJoypad()) {        
        
        if (groupId == Hid::Joypad::GroupID::Axis)            
            value = applyAxisSensitivity( value );        

        // no, don't use hats for analog movement.
        // we simply add the smallest possible delta.
        // that is slow but you can target each position.
        else if (groupId == Hid::Joypad::GroupID::Hat)
            value = value < 0 ? -1 : (value > 0 ? +1 : 0);
    }
        
    return value; 
}

template<bool useOldValue> auto InputMapping::adjustDigitalValue( Assign& hid ) -> int16_t {

    Hid::Device* device = hid.device;
    unsigned groupId = hid.group->id;
    unsigned qualifier = hid.qualifier;
    int16_t value = useOldValue ? hid.input->oldValue : hid.input->value;
        
    if (!device->isJoypad())
        return value; // means keyboard or mouse buttons
    
    // mouse axis is not assignable for digital input. we don't need to look for it here
    
    if (groupId == Hid::Joypad::GroupID::Button)
        return value; // joypad button        
    
    // joypad hat, trigger or axis
    if (qualifier == Qualifier::Lo) 
        return value < -analogSensitivity;

    else if (qualifier == Qualifier::Hi)
        return value > analogSensitivity;
    
    return value; // should not happen     
}

auto InputMapping::informChange(Assign& hid) -> void {
	int16_t value = adjustDigitalValue<false>( hid );
	int16_t oldValue = adjustDigitalValue<true>( hid );
	
	if (value != oldValue) {
        statusHandler->setMessage(std::to_string(value), false, true);
	}	
}

inline auto InputMapping::translateElement(std::string str) -> std::string {
    return trans->get( GUIKIT::String::capitalize( str ) );
}

auto InputMapping::getDescription() -> std::string {
	std::string out = "";

	unsigned pos = 0;    
    
	for(auto& hid : hids) {
        
        out += translateElement(hid.device->name);

        if (!hid.device->isKeyboard())
            out += "." + translateElement(hid.group->name) + ".";
        else
            out += ".";

        if (hid.device->isJoypad())
            out += hid.input->name;
        else
            out += translateElement(hid.input->name);

        if (hid.qualifier == Qualifier::Hi)
            out += ".Hi";

        if (hid.qualifier == Qualifier::Lo)
            out += ".Lo";
                    
		if ( (++pos < hids.size()) || hasUnknownAssignment)
			out += " " + (anded ? trans->get("and") : trans->get("or")) + " ";
	}
    
    if (hasUnknownAssignment)
        out += trans->get("unknown_device");

	return out;	
}

auto InputMapping::swapLinker() -> void {
        
    if (this->isAnalog() || setting->value.empty() || (hids.size() < 2))
        anded = 0;
    else
        anded ^= 1;
            
    if (!setting->value.empty())
       setting->value.replace(0, 1, anded ? "1" : "0" );
}

auto InputMapping::sortHidsByValue() -> void {
    
    std::sort(hids.begin(), hids.end(), [ ](const Assign& lhs, const Assign& rhs) {
        
        return std::abs(lhs.input->value - lhs.input->oldValue) > std::abs(rhs.input->value - rhs.input->oldValue);
    }); 
}

auto InputMapping::generateAlternate( GUIKIT::Settings* settingContainer ) -> void {
    
    this->alternate = new InputMapping;
    this->alternate->parent = this;
    this->alternate->setting = settingContainer->add( this->setting->getIdent() + "_alt" );
    this->alternate->state = 0;
    this->alternate->type = this->type;
    this->alternate->anded = this->anded;
    this->alternate->emuDevice = this->emuDevice;
	this->alternate->hotkeyId = this->hotkeyId;
    this->alternate->alternate = nullptr;
    this->alternate->isShadowed = this->isShadowed;
    this->alternate->inputManager = this->inputManager;
    
}
