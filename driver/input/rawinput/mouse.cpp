
struct RawMouse {
	
	HWND handle;
	bool mAcquired = false;
	
    struct Mouse {
        HANDLE handle = nullptr;
        HANDLE ntHandle = nullptr;

        long lastX = 0;
        long lastY = 0;

        long relativeX = 0;
        long relativeY = 0;
        long relativeZ = 0;
        bool buttons[5] = {false};
        Hid::Mouse* hid = nullptr;

        unsigned sortIdent;
    };
    
    std::vector<Mouse> mice;
    
	auto mAcquire() -> void {
		if (!mAcquired && (GetForegroundWindow() == handle)) {
			mAcquired = true;
			ShowCursor(false);
			SetFocus(handle);
			SetCapture(handle);
			RECT rc;
			GetWindowRect(handle, &rc);
			ClipCursor(&rc);
		}
	}

	auto mUnacquire() -> void {
		if (mAcquired) {
			mAcquired = false;
			ReleaseCapture();
			ClipCursor(nullptr);
			ShowCursor(true);
		}
	}

	auto mIsAcquired() -> bool {
		//return GetCapture() == handle;
		return mAcquired;
	}
    
    auto add( HANDLE handle ) -> void {
        // multi mice support
        Mouse mouse;
        mouse.handle = handle;
        
        wchar_t path[PATH_MAX];
		unsigned size = sizeof (path) - 1;
		GetRawInputDeviceInfo(handle, RIDI_DEVICENAME, &path, &size);
        
        std::string _path = Win::utf8_t(path);

        mouse.ntHandle = CreateFileW(
            path, 0u, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, 0u, nullptr
        );
        
        if (!mouse.ntHandle)
            return;      
        
        mouse.hid = new Hid::Mouse;
		CRC32 crc32((uint8_t*)_path.c_str(), _path.size());		
        
        mouse.sortIdent = crc32.value();
        mouse.hid->id = uniqueDeviceId(mice, 1);
		mouse.hid->name = uniqueDeviceName(mice, "Mouse");

        mouse.hid->axes().append("X");
        mouse.hid->axes().append("Y");
        mouse.hid->axes().append("Z");

        mouse.hid->buttons().append("Left");
        mouse.hid->buttons().append("Right");
        mouse.hid->buttons().append("Middle");
        mouse.hid->buttons().append("Back");
        mouse.hid->buttons().append("Forward");
		
		mice.insert(mice.begin(), mouse);
    }

    auto sortMice() -> void {
        if (mice.size() < 2)
            return;

        std::vector<unsigned> sortIdents;

        for (auto& m : mice)
            sortIdents.push_back(m.sortIdent);

        std::sort(sortIdents.begin(), sortIdents.end());

        int id = 1;
        for (auto& sortIdent : sortIdents) {
            for (int i = 0; i < mice.size(); i++) {
                Mouse& mouse = mice[i];

                if (mouse.sortIdent == sortIdent) {
                    mouse.hid->id = id++;
                    break;
                }
            }
        }
    }
	
	auto init() -> void {
		term();
	}
	
	auto term() -> void {
        
        for( auto& mouse : mice ) {
            if (mouse.hid) delete mouse.hid;
            if (mouse.ntHandle) CloseHandle(mouse.ntHandle);
        }
        
        mice.clear();
	}

	auto update(RAWINPUT* input) -> void {

        Mouse* pMouse = nullptr;
        auto& iMouse = input->data.mouse;

        for (auto& mouse : mice) {
            if (mouse.handle == input->header.hDevice) {
                pMouse = &mouse;
                break;
            }
        }
        if (!pMouse)
            return;                
        
		if ((iMouse.usFlags & 1) == MOUSE_MOVE_RELATIVE) {
			InterlockedExchangeAdd(&pMouse->relativeX, iMouse.lLastX);
			InterlockedExchangeAdd(&pMouse->relativeY, iMouse.lLastY);

		} else if (iMouse.lLastX || iMouse.lLastY) {
            bool vDesktop = (iMouse.usFlags & MOUSE_VIRTUAL_DESKTOP) ? true : false;
            int w = GetSystemMetrics(vDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            int h = GetSystemMetrics(vDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            int x = (int)(((float)iMouse.lLastX / 65535.0f) * w);
            int y = (int)(((float)iMouse.lLastY / 65535.0f) * h);

			InterlockedExchangeAdd(&pMouse->relativeX, x - pMouse->lastX);
			InterlockedExchangeAdd(&pMouse->relativeY, y - pMouse->lastY);

            pMouse->lastX = x;
            pMouse->lastY = y;
        }

        auto bFlags = iMouse.usButtonFlags;

		if (bFlags & RI_MOUSE_WHEEL) {
			InterlockedExchangeAdd(&pMouse->relativeZ, iMouse.usButtonData);
		}

		if (bFlags & RI_MOUSE_BUTTON_1_DOWN) pMouse->buttons[0] = true;
		if (bFlags & RI_MOUSE_BUTTON_1_UP) pMouse->buttons[0] = false;
		if (bFlags & RI_MOUSE_BUTTON_2_DOWN) pMouse->buttons[1] = true;
		if (bFlags & RI_MOUSE_BUTTON_2_UP) pMouse->buttons[1] = false;
		if (bFlags & RI_MOUSE_BUTTON_3_DOWN) pMouse->buttons[2] = true;
		if (bFlags & RI_MOUSE_BUTTON_3_UP) pMouse->buttons[2] = false;
		if (bFlags & RI_MOUSE_BUTTON_4_DOWN) pMouse->buttons[3] = true;
		if (bFlags & RI_MOUSE_BUTTON_4_UP) pMouse->buttons[3] = false;
		if (bFlags & RI_MOUSE_BUTTON_5_DOWN) pMouse->buttons[4] = true;
		if (bFlags & RI_MOUSE_BUTTON_5_UP) pMouse->buttons[4] = false;
	}

	auto poll(std::vector<Hid::Device*>& devices) -> void {
		
        for( auto& mouse : mice ) {

			long _x = InterlockedExchange(&mouse.relativeX, 0);
        	long _y = InterlockedExchange(&mouse.relativeY, 0);
        	long _z = InterlockedExchange(&mouse.relativeZ, 0);

            mouse.hid->axes().inputs[0].setValue( _x );
            mouse.hid->axes().inputs[1].setValue( _y );
            mouse.hid->axes().inputs[2].setValue( _z );

            for (auto& input : mouse.hid->buttons().inputs)
                input.setValue( mouse.buttons[input.id] );

            devices.push_back( mouse.hid );
        }        
	}
	
	~RawMouse() {
		term();
	}
};



