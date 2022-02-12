
struct RawMouse {
	
	HWND handle;
	bool mAcquired = false;
	
    struct Mouse {
        HANDLE handle = nullptr;
        HANDLE ntHandle = nullptr;
        
        signed relativeX = 0;
        signed relativeY = 0;
        signed relativeZ = 0;
        bool buttons[5] = {0};
        Hid::Mouse* hid = nullptr;        
    };
    
    std::vector<Mouse> mice;
    
	auto mAcquire() -> void {
		if (!mAcquired) {
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
        
        mouse.hid->id = uniqueDeviceId(mice, crc32.value() );
		mouse.hid->name = uniqueDeviceName(mice, "Mouse");

        mouse.hid->axes().append("X");
        mouse.hid->axes().append("Y");
        mouse.hid->axes().append("Z");

        mouse.hid->buttons().append("Left");
        mouse.hid->buttons().append("Right");
        mouse.hid->buttons().append("Middle");
        mouse.hid->buttons().append("Up");
        mouse.hid->buttons().append("Down");
		
		mice.insert(mice.begin(), mouse);   
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

        for (auto& mouse : mice) {
            if (mouse.handle == input->header.hDevice) {
                pMouse = &mouse;
                break;
            }
        }
        if (!pMouse)
            return;                
        
		if ((input->data.mouse.usFlags & 1) == MOUSE_MOVE_RELATIVE) {
			pMouse->relativeX += input->data.mouse.lLastX;
			pMouse->relativeY += input->data.mouse.lLastY;
		}

		if (input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
			pMouse->relativeZ += (int16_t) input->data.mouse.usButtonData;
		}

		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) pMouse->buttons[0] = 1;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) pMouse->buttons[0] = 0;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) pMouse->buttons[1] = 1;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) pMouse->buttons[1] = 0;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) pMouse->buttons[2] = 1;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP) pMouse->buttons[2] = 0;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) pMouse->buttons[3] = 1;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) pMouse->buttons[3] = 0;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) pMouse->buttons[4] = 1;
		if (input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) pMouse->buttons[4] = 0;
	}

	auto poll(std::vector<Hid::Device*>& devices) -> void {
		
        for( auto& mouse : mice ) {
            
            mouse.hid->axes().inputs[0].setValue( mouse.relativeX );
            mouse.hid->axes().inputs[1].setValue( mouse.relativeY );
            mouse.hid->axes().inputs[2].setValue( mouse.relativeZ );

            mouse.relativeX = mouse.relativeY = mouse.relativeZ = 0;

            for (auto& input : mouse.hid->buttons().inputs)
                input.setValue( mouse.buttons[input.id] );

            devices.push_back( mouse.hid );
        }        
	}
	
	~RawMouse() {
		term();
	}
};



