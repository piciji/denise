
struct RawKeyboard {
	Hid::Keyboard* hidKeyboard = nullptr;
	bool keys[256] = {0};
	
	auto init() -> void {
		term();
		
		hidKeyboard = new Hid::Keyboard;
		hidKeyboard->id = 0;
		
		for(unsigned id = 0; id <= 0xff; id++) {
            std::string ident = Win::translateKeyName( id, false );
			hidKeyboard->buttons().append( ident, Win::getKeyCode( ident, id ) );
		}
	}
	
	auto term() -> void {
		if(hidKeyboard) delete hidKeyboard;
		hidKeyboard = nullptr;
	}

	auto update(RAWINPUT* input) -> void {
		unsigned scanCode = input->data.keyboard.MakeCode;
		unsigned flags = input->data.keyboard.Flags;
		unsigned virtualKey = input->data.keyboard.VKey;
		bool wasUp = ((flags & RI_KEY_BREAK) != 0);

		if (virtualKey == 255)
			return;

		if (virtualKey == VK_NUMLOCK) {
			// correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
			scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) | 0x100;
		}

		const bool isE0 = ((flags & RI_KEY_E0) != 0);
		const bool isE1 = ((flags & RI_KEY_E1) != 0);

		if (isE1) {
			// for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
			// however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
			if (virtualKey == VK_PAUSE)
				scanCode = 0x45;
			else
				scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
		}

		if (scanCode & 0x100) scanCode -= 0x80;

		if (isE0) scanCode |= 0x80;

		keys[scanCode & 0xff] = !wasUp;
	}
	
	auto poll(std::vector<Hid::Device*>& devices) -> void {
		
		for (auto& input : hidKeyboard->buttons().inputs)
			input.setValue( keys[input.id] );
		
		devices.push_back(hidKeyboard);
	}	
	
	~RawKeyboard() {
		term();
	}
};