
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

auto InputMapping::applyMouseSense( int16_t value ) -> int16_t {

    static auto mouseSensePtr = settings->getOrInit<unsigned>("mousesense", 40u, {5u, 80u});

    return value * (int16_t) (unsigned) (*mouseSensePtr) / 40;
}

auto InputMapping::applyAnalogSense( int16_t value ) -> int16_t {

    static auto analogSensePtr = settings->getOrInit<unsigned>("analogsense", 40u, {5u, 80u});

    value = (value * (int16_t) (unsigned) (*analogSensePtr) / 40) >> 10;
    
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
    
    if (!lightMode)
        emuDevice->userData = hid.group->timeStamp;
    
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
            
            value = applyMouseSense( value );    
        }
        
    } else if (device->isJoypad()) {        
        
        if (groupId == Hid::Joypad::GroupID::Axis)            
            value = applyAnalogSense( value );        

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
        return value < -16384;

    else if (qualifier == Qualifier::Hi)
        return value > +16384;
    
    return value; // should not happen     
}

auto InputMapping::informChange(Assign& hid) -> void {
	int16_t value = adjustDigitalValue<false>( hid );
	int16_t oldValue = adjustDigitalValue<true>( hid );
	
	if (value != oldValue) {
		status->addMessage( std::to_string(value) );
	}	
}

auto InputMapping::getDescription() -> std::string {
	std::string out = "";
	
	#define _transhr(part) trans->get( GUIKIT::String::capitalize( part ) )	
	unsigned pos = 0;    
    
	for(auto& hid : hids) {
        
        out += _transhr(hid.device->name);

        if (!hid.device->isKeyboard())
            out += "." + _transhr(hid.group->name) + ".";
        else
            out += ".";

        if (hid.device->isJoypad())
            out += hid.input->name;
        else
            out += _transhr(hid.input->name);

        if (hid.qualifier == Qualifier::Hi)
            out += ".Hi";

        if (hid.qualifier == Qualifier::Lo)
            out += ".Lo";
                    
		if (++pos < hids.size())
			out += " " + (anded ? trans->get("and") : trans->get("or")) + " ";
	}
	
	#undef _transhr
	return out;	
}

auto InputMapping::swapLinker() -> void {
        
    if (this->isAnalog())
        anded = 0;
    else
        anded ^= 1;
    
	updateSetting();
}

auto InputMapping::sortHidsByValue() -> void {
    
    std::sort(hids.begin(), hids.end(), [ ](const Assign& lhs, const Assign& rhs) {
        
        return std::abs(lhs.input->value - lhs.input->oldValue) > std::abs(rhs.input->value - rhs.input->oldValue);
    }); 
}

auto InputMapping::generateAlternate() -> void {
    
    this->alternate = new InputMapping;
    this->alternate->parent = this;
    this->alternate->setting = settings->add( this->setting->getIdent() + "_alt" );
    this->alternate->state = 0;
    this->alternate->type = this->type;
    this->alternate->anded = this->anded;
    this->alternate->emuDevice = this->emuDevice;
    this->alternate->alternate = nullptr;
    this->alternate->inputManager = this->inputManager;
    
}
