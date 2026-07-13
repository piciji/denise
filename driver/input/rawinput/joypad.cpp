
struct RawJoypad {
	#define RJ_STEP(exp) { if( !(exp) ) goto End; }
	#define RJ_FREE(p, h)  if( p ) HeapFree(h, 0, p), p = nullptr;

	struct Joypad {
		HANDLE handle = nullptr;
		HANDLE ntHandle = nullptr;
		Hid::Joypad* hid = nullptr;
		
		bool isXInputDevice = false;
		
		std::vector<uint8_t> buttons;
		
		struct Hats {
			volatile int16_t x;
			volatile int16_t y;
		};
		std::vector<Hats> hats;
		int dPadHatPos = -1;

        long axis[6] = {0};
        uint8_t axisMap[6];

		HIDP_CAPS Caps;
		PHIDP_BUTTON_CAPS pButtonCaps = nullptr;
		PHIDP_VALUE_CAPS pValueCaps = nullptr;
		PHIDP_PREPARSED_DATA pPreparsedData = nullptr;
		HANDLE heap;
	};
	std::vector<Joypad> joypads;

	auto add( HANDLE handle ) -> void {

		//logger->log("preparsed");
		Joypad jp;		
		jp.handle = handle;
		jp.dPadHatPos = -1;
		jp.isXInputDevice = false;

		if (!parseCaps(jp, handle))
			return;
		
		wchar_t path[PATH_MAX];
		unsigned size = sizeof (path) - 1;
		GetRawInputDeviceInfo(handle, RIDI_DEVICENAME, &path, &size);

		std::string _path = Win::utf8_t(path);
		if (_path.find("IG_") != std::string::npos)
			jp.isXInputDevice = true;

		//logger->log(jp.isXInputDevice ? "x mode" : "d mode");
		
		jp.ntHandle = CreateFileW(
			path, 0u, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, 0u, nullptr
		);
		if (!jp.ntHandle) return;
		// logger->log("opened");
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
		
		unsigned buttonCount = jp.pButtonCaps->Range.UsageMax - jp.pButtonCaps->Range.UsageMin + 1;
		//logger->log("buttons " + std::to_string(buttonCount));
		for(unsigned button = 0; button < buttonCount; button++) {
			jp.hid->buttons().append( std::to_string(button) );
			jp.buttons.push_back(0);
		}

		unsigned hat = 0;
		unsigned axes = 0;
		std::vector<uint8_t> usages;
        
		for (unsigned i = 0; i < jp.Caps.NumberInputValueCaps; i++)
            usages.push_back( jp.pValueCaps[i].Range.UsageMin );
        
        std::sort(usages.begin(), usages.end()); 
        
        for (auto& usage : usages) {
            switch (usage) {
                case 0x30:
                	//logger->log("add axis X");
                    jp.hid->axes().append( "X" );                    
                    jp.axisMap[axes++] = 0;
                    break;
                case 0x31:
                	//logger->log("add axis Y");
                    jp.hid->axes().append( "Y" );
                    jp.axisMap[axes++] = 1;
                    break;
                case 0x32:
                	//logger->log("add axis Z");
                    jp.hid->axes().append( "Z" );
                    jp.axisMap[axes++] = 2;
                    break;
                case 0x33:
                	//logger->log("add axis X|Rot");
                    jp.hid->axes().append( "X|Rot" );
                    jp.axisMap[axes++] = 3;
                    break;
                case 0x34:
                	//logger->log("add axis Y|Rot");
                    jp.hid->axes().append( "Y|Rot" );
                    jp.axisMap[axes++] = 4;
                    break;
                case 0x35:
                	//logger->log("add axis Z|Rot");
                    jp.hid->axes().append( "Z|Rot" );
                    jp.axisMap[axes++] = 5;
                    break;
                case 0x39: // Hat Switch
                	//logger->log("add Hat Switch");
                    jp.hid->hats().append( std::to_string(hat) + ".X" );
                    jp.hid->hats().append( std::to_string(hat) + ".Y" );
                    jp.hats.push_back({0,0});
                    hat++;
                    break;
            	case 0x90:
            		//logger->log("add Dpad Up");
            		jp.hid->hats().append( std::to_string(hat) + ".D-pad X" );
            		jp.hid->hats().append( std::to_string(hat) + ".D-pad Y" );
            		jp.hats.push_back({0,0});
            		jp.dPadHatPos = hat;
            		hat++;
            		break;
            	case 0x91:
            		//logger->log("add Dpad Down");
            		break;
            	case 0x92:
            		//logger->log("add Dpad Right");
            		break;
            	case 0x93:
            		//logger->log("add Dpad Left");
            		break;

            	case 0x36:
            		//logger->log("add Slider");
            		break;
            	case 0x37:
            		//logger->log("add Dial");
            		break;
            	case 0x38:
            		//logger->log("add Wheel");
            		break;
            	case 0xbb:
            		//logger->log("add Throttle");
            		break;
            	case 0xba:
            		//logger->log("add Rudder");
            		break;
            }            
        }
        				
		joypads.push_back(jp);		
	}
	
	auto init() -> void {
		term();
	}
	
	auto term() -> void {		
		for(auto& jp : joypads ) {
			if(jp.hid) delete jp.hid;
			if(jp.ntHandle) CloseHandle( jp.ntHandle );

			RJ_FREE(jp.pPreparsedData, jp.heap);
			RJ_FREE(jp.pButtonCaps, jp.heap);
			RJ_FREE(jp.pValueCaps, jp.heap);
		}
		joypads.clear();
	}

	auto getValByRange(unsigned long value, USHORT bits, bool neg) -> int32_t {
		if (neg)
			return (value & (bits >= 32 ? 0x80000000 : (1 << (bits - 1)))) ? value | (bits >= 32 ? 0x80000000 : (-1 << bits)) : value;

		return value & (bits >= 32 ? 0xffffffff : ((1 << bits) - 1));
	}
	
	auto parseCaps( Joypad& jp, HANDLE handle ) -> bool {
		USHORT length;
		UINT bufferSize;
		jp.heap = GetProcessHeap();
		
		RJ_STEP( GetRawInputDeviceInfo(handle, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0)
		RJ_STEP( jp.pPreparsedData = (PHIDP_PREPARSED_DATA) HeapAlloc(jp.heap, 0, bufferSize) )
		RJ_STEP( (int)GetRawInputDeviceInfo(handle, RIDI_PREPARSEDDATA, jp.pPreparsedData, &bufferSize) >= 0)
			
		RJ_STEP( HidP_GetCaps(jp.pPreparsedData, &jp.Caps) == HIDP_STATUS_SUCCESS )
		RJ_STEP( jp.pButtonCaps = (PHIDP_BUTTON_CAPS) HeapAlloc(jp.heap, 0, sizeof (HIDP_BUTTON_CAPS) * jp.Caps.NumberInputButtonCaps) )

		length = jp.Caps.NumberInputButtonCaps;
		RJ_STEP( HidP_GetButtonCaps(HidP_Input, jp.pButtonCaps, &length, jp.pPreparsedData) == HIDP_STATUS_SUCCESS )
				
		RJ_STEP( jp.pValueCaps = (PHIDP_VALUE_CAPS) HeapAlloc(jp.heap, 0, sizeof (HIDP_VALUE_CAPS) * jp.Caps.NumberInputValueCaps) )
		length = jp.Caps.NumberInputValueCaps;
		RJ_STEP( HidP_GetValueCaps(HidP_Input, jp.pValueCaps, &length, jp.pPreparsedData) == HIDP_STATUS_SUCCESS )

		for (unsigned i = 0; i < jp.Caps.NumberInputValueCaps; i++) {
			jp.pValueCaps[i].LogicalMin = getValByRange(jp.pValueCaps[i].LogicalMin, jp.pValueCaps->BitSize, jp.pValueCaps[i].LogicalMin < 0);
			jp.pValueCaps[i].LogicalMax = getValByRange(jp.pValueCaps[i].LogicalMax, jp.pValueCaps->BitSize, jp.pValueCaps[i].LogicalMin < 0);
		}

		return true;
		
		End:
		RJ_FREE(jp.pPreparsedData, jp.heap);
		RJ_FREE(jp.pButtonCaps, jp.heap);
		RJ_FREE(jp.pValueCaps, jp.heap);
		return false;
	}
	
	auto update(RAWINPUT* input) -> void {
		
		Joypad* pJoypad = nullptr;
		int dPadHatPos = -1;
		
		for( auto& joypad : joypads ) {
			if (joypad.handle == input->header.hDevice) {
				pJoypad = &joypad;
				dPadHatPos = pJoypad->dPadHatPos;
				if (dPadHatPos >= 0) {
					pJoypad->hats[dPadHatPos].x = 0;
					pJoypad->hats[dPadHatPos].y = 0;
				}
				break;
			}
		}
		if (!pJoypad)
			return;

		unsigned long value;
		unsigned hat = 0;
		
		unsigned long usageLength = pJoypad->pButtonCaps->Range.UsageMax - pJoypad->pButtonCaps->Range.UsageMin + 1;
		USAGE* usage = new USAGE[usageLength];
		
		RJ_STEP( HidP_GetUsages( HidP_Input, pJoypad->pButtonCaps->UsagePage, 0, usage, &usageLength, pJoypad->pPreparsedData,
			(PCHAR) input->data.hid.bRawData, input->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS )

		std::fill(pJoypad->buttons.begin(), pJoypad->buttons.end(), 0);
		
		unsigned pos;
		for(unsigned i = 0; i < usageLength; i++) {
			pos = usage[i] - pJoypad->pButtonCaps->Range.UsageMin;
			if (pos >= pJoypad->buttons.size())
				continue;
			pJoypad->buttons[pos] = 1;
		}
		
		for (unsigned i = 0; i < pJoypad->Caps.NumberInputValueCaps; i++) {
			RJ_STEP( HidP_GetUsageValue( HidP_Input, pJoypad->pValueCaps[i].UsagePage, 0, pJoypad->pValueCaps[i].Range.UsageMin, &value, pJoypad->pPreparsedData,
				(PCHAR) input->data.hid.bRawData, input->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS )					

			unsigned short usageMin = pJoypad->pValueCaps[i].Range.UsageMin;

			switch (usageMin) {
                case 0x30:
                case 0x31:
                case 0x32:
                case 0x33:
                case 0x34:
                case 0x35: {
                    auto& pCaps = pJoypad->pValueCaps[i];

                    signed range = pCaps.LogicalMax - pCaps.LogicalMin;
                    if (range == 0) { // todo fixme
                        InterlockedExchange(&(pJoypad->axis[usageMin & 7]), sclamp<16>( ((value & 0xff) - 128) << 8 ));
                        //pJoypad->axis[usageMin & 7] = sclamp<16>( ((value & 0xff) - 128) << 8 );
                            
                    } else {
                        int32_t _value = getValByRange(value, pCaps.BitSize, pCaps.LogicalMin < 0);

                        _value = (((long long)(_value - pCaps.LogicalMin) * 65535ll) / (long long)range) - 32767ll;

                        InterlockedExchange(&(pJoypad->axis[usageMin & 7]), sclamp<16>( _value) );
                        //pJoypad->axis[usageMin & 7] = sclamp<16>( _value);
                    }                                            
                } break;

				case 0x37:
					//logger->log("dial");
					//logger->log(std::to_string(value), 0);
					//logger->log(std::to_string(pJoypad->pValueCaps[i].LogicalMin), 0);
					//logger->log(std::to_string(pJoypad->pValueCaps[i].LogicalMax), 0);
					break;
                
				case 0x39: // Hat Switch
					if (hat == pJoypad->dPadHatPos)
						hat++;
					if (hat == pJoypad->hats.size())
						break;

					//logger->log("upd Hat");
                    if (pJoypad->isXInputDevice) {
                    	int16_t _x = (value == 6 || value == 7 || value == 8) ? -32768
								: ( (value == 2 || value == 3 || value == 4) ? +32767 : 0 );

                    	int16_t _y = (value == 4 || value == 5 || value == 6) ? +32767
								: ( (value == 8 || value == 1 || value == 2) ? -32768 : 0 );

                        pJoypad->hats[hat].x = _x;
                        pJoypad->hats[hat].y = _y;

                    } else {
                    	int16_t _x = (value == 5 || value == 6 || value == 7) ? -32768
								: ( (value == 1 || value == 2 || value == 3) ? +32767 : 0 );

                    	int16_t _y = (value == 7 || value == 0 || value == 1) ? -32768
								: ( (value == 3 || value == 4 || value == 5) ? +32767 : 0 );

                    	pJoypad->hats[hat].x = _x;
                    	pJoypad->hats[hat].y = _y;
                    }
                    
                    hat++;
					break;
				case 0x90:
					//logger->log("upd D-pad up");
					if (dPadHatPos >= 0)
						pJoypad->hats[dPadHatPos].y = 32767;
					break;
				case 0x91:
					//logger->log("upd D-pad down");
					if (dPadHatPos >= 0)
						pJoypad->hats[dPadHatPos].y = -32768;
					break;
				case 0x92:
					//logger->log("upd D-pad right");
					if (dPadHatPos >= 0)
						pJoypad->hats[dPadHatPos].x = 32767;
					break;
				case 0x93:
					//logger->log("upd D-pad left");
					if (dPadHatPos >= 0)
						pJoypad->hats[dPadHatPos].x = -32768;
					break;
				default:
					//logger->log("upd misc " + std::to_string(data.pValueCaps[i].Range.UsageMin));
					break;
			}
		}

	End:
		delete[] usage;
	}
	
	auto poll(std::vector<Hid::Device*>& devices) -> void {								

		for(auto& jp : joypads) {
			auto& hats = jp.hid->hats();
			
			for (unsigned hat = 0; hat < hats.inputs.size() / 2; hat++ ) {
				hats.inputs[hat * 2 + 0].setValue( jp.hats[hat].x );
				hats.inputs[hat * 2 + 1].setValue( jp.hats[hat].y );
			}
            
			for(auto& input : jp.hid->axes().inputs) {
                input.setValue( jp.axis[ jp.axisMap[ input.id ] ] );
			}

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
