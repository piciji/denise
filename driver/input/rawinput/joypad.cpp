
struct RawJoypad {
	#define RJ_STEP(exp) { if( !(exp) ) goto End; }
	#define RJ_FREE(p)  if( p ) HeapFree(data.heap, 0, p), p = nullptr;

	struct Joypad {
		HANDLE handle = nullptr;
		HANDLE ntHandle = nullptr;
		Hid::Joypad* hid = nullptr;
		
		bool isXInputDevice = false;
		
		std::vector<uint8_t> buttons;
		
		struct Hats {
			int16_t x;
			int16_t y;
		};
		std::vector<Hats> hats;
        
        int16_t axis[6] = {0};
        uint8_t axisMap[6];
	};
	std::vector<Joypad> joypads;

	struct {
		PHIDP_PREPARSED_DATA pPreparsedData = nullptr;
		HIDP_CAPS Caps;
		PHIDP_BUTTON_CAPS pButtonCaps = nullptr;
		PHIDP_VALUE_CAPS pValueCaps = nullptr;
		HANDLE heap;
	} data;
	
	auto add( HANDLE handle ) -> void {
		
		if (!updatePreparsedData(handle)) return;
		
		Joypad jp;		
		jp.handle = handle;
		
		wchar_t path[PATH_MAX];
		unsigned size = sizeof (path) - 1;
		GetRawInputDeviceInfo(handle, RIDI_DEVICENAME, &path, &size);

		std::string _path = Win::utf8_t(path);
		if (_path.find("IG_") != std::string::npos)
			jp.isXInputDevice = true;
		
		jp.ntHandle = CreateFileW(
			path, 0u, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, 0u, nullptr
		);
		if (!jp.ntHandle) return;

		wchar_t nameBuffer[PATH_MAX];

		if (!HidD_GetProductString(jp.ntHandle, nameBuffer, 100)) {
			wcscpy(nameBuffer, L"Joypad");
		}				
				
		jp.hid = new Hid::Joypad;
		CRC32 crc32((uint8_t*)_path.c_str(), _path.size());		
				
		jp.hid->id = uniqueDeviceId(joypads, crc32.value() );
		auto joyname = Win::utf8_t( nameBuffer );
		if (joyname.empty()) joyname = "Joypad";
		jp.hid->name = uniqueDeviceName(joypads, joyname);
		
		unsigned buttonCount = data.pButtonCaps->Range.UsageMax - data.pButtonCaps->Range.UsageMin + 1;
		
		for(unsigned button = 0; button < buttonCount; button++) {
			jp.hid->buttons().append( std::to_string(button) );
			jp.buttons.push_back(0);
		}

		unsigned hat = 0;
		unsigned axes = 0;
		std::vector<uint8_t> usages;
        
		for (unsigned i = 0; i < data.Caps.NumberInputValueCaps; i++)            
            usages.push_back( data.pValueCaps[i].Range.UsageMin );		
        
        std::sort(usages.begin(), usages.end()); 
        
        for (auto& usage : usages) {
            switch (usage) {
                case 0x30:
                    jp.hid->axes().append( "X" );                    
                    jp.axisMap[axes++] = 0;
                    break;
                case 0x31:
                    jp.hid->axes().append( "Y" );
                    jp.axisMap[axes++] = 1;
                    break;
                case 0x32:
                    jp.hid->axes().append( "Z" );
                    jp.axisMap[axes++] = 2;
                    break;
                case 0x33:
                    jp.hid->axes().append( "X|Rot" );
                    jp.axisMap[axes++] = 3;
                    break;
                case 0x34:
                    jp.hid->axes().append( "Y|Rot" );
                    jp.axisMap[axes++] = 4;
                    break;
                case 0x35: 
                    jp.hid->axes().append( "Z|Rot" );
                    jp.axisMap[axes++] = 5;
                    break;
                case 0x39: // Hat Switch
                    jp.hid->hats().append( std::to_string(hat) + ".X" );
                    jp.hid->hats().append( std::to_string(hat) + ".Y" );
                    jp.hats.push_back({0,0});
                    hat++;
                    break;
            }            
        }
        				
		joypads.push_back(jp);		
	}
	
	auto init() -> void {
		term();
	}
	
	auto term() -> void {		
		for(auto& joypad : joypads ) {			
			if(joypad.hid) delete joypad.hid;
			if(joypad.ntHandle) CloseHandle( joypad.ntHandle );
		}
		joypads.clear();
		
		RJ_FREE(data.pPreparsedData);	
		RJ_FREE(data.pButtonCaps);
		RJ_FREE(data.pValueCaps);
	}
	
	auto updatePreparsedData( HANDLE handle ) -> bool {
		RJ_FREE(data.pPreparsedData);	
		RJ_FREE(data.pButtonCaps);
		RJ_FREE(data.pValueCaps);
		
		UINT bufferSize;
		USHORT length;
		data.heap = GetProcessHeap();
		
		RJ_STEP( GetRawInputDeviceInfo(handle, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0)
		RJ_STEP( data.pPreparsedData = (PHIDP_PREPARSED_DATA) HeapAlloc(data.heap, 0, bufferSize) )
		RJ_STEP( (int)GetRawInputDeviceInfo(handle, RIDI_PREPARSEDDATA, data.pPreparsedData, &bufferSize) >= 0)
			
		RJ_STEP( HidP_GetCaps(data.pPreparsedData, &data.Caps) == HIDP_STATUS_SUCCESS )
		RJ_STEP( data.pButtonCaps = (PHIDP_BUTTON_CAPS) HeapAlloc(data.heap, 0, sizeof (HIDP_BUTTON_CAPS) * data.Caps.NumberInputButtonCaps) )

		length = data.Caps.NumberInputButtonCaps;
		RJ_STEP( HidP_GetButtonCaps(HidP_Input, data.pButtonCaps, &length, data.pPreparsedData) == HIDP_STATUS_SUCCESS )
				
		RJ_STEP( data.pValueCaps = (PHIDP_VALUE_CAPS) HeapAlloc(data.heap, 0, sizeof (HIDP_VALUE_CAPS) * data.Caps.NumberInputValueCaps) )
		length = data.Caps.NumberInputValueCaps;
		RJ_STEP( HidP_GetValueCaps(HidP_Input, data.pValueCaps, &length, data.pPreparsedData) == HIDP_STATUS_SUCCESS )
				
		return true;
		
		End:			
		return false;
	}
	
	auto update(RAWINPUT* input) -> void {
		
		Joypad* pJoypad = nullptr;
		
		for( auto& joypad : joypads ) {
			if (joypad.handle == input->header.hDevice) {
				pJoypad = &joypad;
				break;
			}
		}
		if (!pJoypad) return;
		if (!updatePreparsedData(pJoypad->handle)) return;
		unsigned long value;
		unsigned hat = 0;
		
		unsigned long usageLength = data.pButtonCaps->Range.UsageMax - data.pButtonCaps->Range.UsageMin + 1;
		USAGE usage[usageLength];
		
		RJ_STEP( HidP_GetUsages( HidP_Input, data.pButtonCaps->UsagePage, 0, usage, &usageLength, data.pPreparsedData,
			(PCHAR) input->data.hid.bRawData, input->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS )

		for(auto& button : pJoypad->buttons)	
			button = 0;
		
		unsigned pos;
		for(unsigned i = 0; i < usageLength; i++) {
			pos = usage[i] - data.pButtonCaps->Range.UsageMin;
			if (pos >= pJoypad->buttons.size()) continue;
			pJoypad->buttons[pos] = 1;
		}
		
		for (unsigned i = 0; i < data.Caps.NumberInputValueCaps; i++) {
			RJ_STEP( HidP_GetUsageValue( HidP_Input, data.pValueCaps[i].UsagePage, 0, data.pValueCaps[i].Range.UsageMin, &value, data.pPreparsedData,
				(PCHAR) input->data.hid.bRawData, input->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS )					
                    
			switch (data.pValueCaps[i].Range.UsageMin) {
                case 0x30:
                case 0x31:
                case 0x32:
                case 0x33:
                case 0x34:
                case 0x35: {
                    signed range = data.pValueCaps[i].LogicalMax - data.pValueCaps[i].LogicalMin;
                    if (range == 0)
                        range = 1;
                    
                    int32_t _value = ((int32_t)value - data.pValueCaps[i].LogicalMin) * 65535ll / range - 32767;
                    
                    pJoypad->axis[data.pValueCaps[i].Range.UsageMin & 7] = sclamp<16>( _value);
                } break;
                
				case 0x39: // Hat Switch
					if (hat == pJoypad->hats.size())
						break;
					pJoypad->hats[hat].x = (value == 5 || value == 6 || value == 7) ? -32768
							: ( (value == 1 || value == 2 || value == 3) ? +32767 : 0 );
					pJoypad->hats[hat].y = (value == 7 || value == 0 || value == 1) ? -32768
							: ( (value == 3 || value == 4 || value == 5) ? +32767 : 0 );
					hat++;
					break;
			}
		}

		End:
		return;					
	}
	
	auto poll(std::vector<Hid::Device*>& devices) -> void {								
        
        unsigned ts = 0;
        
		for(auto& jp : joypads) {
			auto& hats = jp.hid->hats();
			
			for (unsigned hat = 0; hat < hats.inputs.size() / 2; hat++ ) {							
				hats.inputs[hat * 2 + 0].setValue( jp.hats[hat].x );
				hats.inputs[hat * 2 + 1].setValue( jp.hats[hat].y );
			}
            
			for(auto& input : jp.hid->axes().inputs) {
                
                input.setValue( jp.axis[ jp.axisMap[ input.id ] ] );                
			}
            
            if(ts == 0)
                ts = Chronos::getTimestampInMicroseconds();           
            
            jp.hid->axes().timeStamp = ts;

			for(auto& input : jp.hid->buttons().inputs)
				input.setValue( jp.buttons[input.id] );
			
			devices.push_back(jp.hid);
		}
	}
	#undef RJ_STEP
	#undef RJ_FREE

	~RawJoypad() {
		term();
	}
};
