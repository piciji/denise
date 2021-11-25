
#include <SDL2/SDL.h>

#include "../tools/hid.h"
#include "../tools/crc32.h"
#include "../tools/tools.h"
#include "../tools/chronos.h"

namespace DRIVER {
    
struct SdlInput {
	
	struct Joypad {
		SDL_Joystick* handle = nullptr;
		Hid::Joypad* hid = nullptr;		
	};
	
	std::vector<Joypad> joypads;

    auto init() -> bool {
		
		term();
        
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0) {
			return false;
		}

		SDL_JoystickEventState(SDL_IGNORE);
		
		for(unsigned id = 0; id < (unsigned)SDL_NumJoysticks(); id++) {			
			Joypad jp;
			jp.handle = SDL_JoystickOpen(id);
			if(!jp.handle) continue;
			
			jp.hid = new Hid::Joypad;			

			unsigned axes = SDL_JoystickNumAxes(jp.handle);
			unsigned hats = SDL_JoystickNumHats(jp.handle);
			unsigned buttons = SDL_JoystickNumButtons(jp.handle);

			SDL_JoystickGUID guid = SDL_JoystickGetGUID(jp.handle);
            char guidStr[30] = {0};
			SDL_JoystickGetGUIDString(guid, guidStr, sizeof (guidStr));
			CRC32 crc32((uint8_t*)guidStr, sizeof (guidStr));
			
			jp.hid->id = uniqueDeviceId( joypads, crc32.value() );
			auto joyname = SDL_JoystickName( jp.handle );
			std::string displayname = joyname ? (std::string)joyname : "";
			
			if(displayname.empty()) displayname = "Joypad";
			
			jp.hid->name = uniqueDeviceName(joypads, displayname);
			
			for (unsigned i=0; i < axes; i++) {
				std::string axis = std::to_string(i);
				
				switch(i) {
					case 0: axis = "X"; break;
					case 1: axis = "Y"; break;
					case 2: axis = "Z"; break;
					case 3: axis = "Z|Rot"; break;
					case 4: axis = "X|Rot"; break;
					case 5: axis = "Y|Rot"; break;
				}				
				jp.hid->axes().append(axis);
			}
			
			for (unsigned hat=0; hat < hats; hat++) {
				jp.hid->hats().append( std::to_string(hat) + ".X" );
				jp.hid->hats().append( std::to_string(hat) + ".Y" );
			}
            
			for (unsigned button=0; button < buttons; button++) 
				jp.hid->buttons().append( std::to_string(button) );
			
			joypads.push_back(jp);
		}
		
		return true;
	}
		
    auto term() -> void {
        
		for(auto& jp : joypads) {
			if (jp.handle) SDL_JoystickClose(jp.handle), jp.handle = nullptr;
			if (jp.hid) delete jp.hid, jp.hid = nullptr;
		}
		joypads.clear();
        joypads.shrink_to_fit();
		
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
	
    auto pollJoypad(std::vector<Hid::Device*>& devices) -> void {
		SDL_JoystickUpdate();
		unsigned countJoy = (unsigned)SDL_NumJoysticks();
		
		if(joypads.size() != countJoy) {
			init();
		}
		
		for(auto& jp : joypads) {

			auto& hats = jp.hid->hats();
			
			for (uint hat = 0; hat < hats.inputs.size() / 2; hat++ ) {
				unsigned char state = SDL_JoystickGetHat(jp.handle, hat);
				unsigned x = hat * 2;
							
				hats.inputs[x + 0].setValue( state & SDL_HAT_LEFT ? -32768 : (state & SDL_HAT_RIGHT ? +32767 : 0) );
				hats.inputs[x + 1].setValue( state & SDL_HAT_UP ? -32768 : (state & SDL_HAT_DOWN ? +32767 : 0) );
			}
            
			for(auto& input : jp.hid->axes().inputs)
				input.setValue( (signed short)SDL_JoystickGetAxis(jp.handle, input.id) );
			
			
			for(auto& input : jp.hid->buttons().inputs)
				input.setValue( (bool)SDL_JoystickGetButton(jp.handle, input.id) );			
			
			devices.push_back(jp.hid);
		}
	}
			
    ~SdlInput() { term(); }
};
    
}
