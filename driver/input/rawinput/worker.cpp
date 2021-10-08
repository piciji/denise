
struct RawWorker;

#include "keyboard.cpp"
#include "mouse.cpp"
#include "joypad.cpp"

struct RawWorker {
	
	CRITICAL_SECTION mcsSc;
	HWND hwnd = nullptr;
	bool ready = false;
	bool deviceChanged = false;
    bool libError = false;
	
	RawMouse mouse;
	RawKeyboard keyboard;
	RawJoypad joypad;
    
    RawWorker() {
        // used for joypad handling
        libError = !lookupHidLibrary();
    }
	
    auto initDevices() -> void {	
		deviceChanged = false;
		joypad.init();
        mouse.init();
        
		unsigned deviceCount = 0;
		GetRawInputDeviceList(NULL, &deviceCount, sizeof (RAWINPUTDEVICELIST));
		RAWINPUTDEVICELIST* list = new RAWINPUTDEVICELIST[deviceCount];
		GetRawInputDeviceList(list, &deviceCount, sizeof (RAWINPUTDEVICELIST));

		for (unsigned n = 0; n < deviceCount; n++) {
            
            unsigned pos = deviceCount - n - 1; // add devices in reverse 
			RID_DEVICE_INFO info;
			unsigned size;
			info.cbSize = size = sizeof (RID_DEVICE_INFO);
			GetRawInputDeviceInfo(list[pos].hDevice, RIDI_DEVICEINFO, &info, &size);

			if (info.dwType == RIM_TYPEHID) {
				if (info.hid.usUsagePage != 1 || (info.hid.usUsage != 4 && info.hid.usUsage != 5))
					continue;
					
                if (!libError)
                    joypad.add( list[pos].hDevice );
                
			} else if (info.dwType == RIM_TYPEMOUSE) {
					
				mouse.add( list[pos].hDevice );                
            }
		}

		delete[] list;
	}
	
	auto wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		if (msg == WM_DEVICECHANGE) {
			EnterCriticalSection( &mcsSc );
			deviceChanged = true;
			LeaveCriticalSection( &mcsSc );
		}			
		
		if (msg != WM_INPUT)
            return DefWindowProc(hwnd, msg, wparam, lparam);

		unsigned size = 0;
		GetRawInputData((HRAWINPUT) lparam, RID_INPUT, NULL, &size, sizeof (RAWINPUTHEADER));
        
        if (size) {
            RAWINPUT* input = new RAWINPUT[size];
            GetRawInputData((HRAWINPUT) lparam, RID_INPUT, input, &size, sizeof (RAWINPUTHEADER));

            EnterCriticalSection( &mcsSc );

            if (input->header.dwType == RIM_TYPEKEYBOARD) {
                keyboard.update(input);
            }

            if (input->header.dwType == RIM_TYPEMOUSE) {
                mouse.update(input);
            }

            if (input->header.dwType == RIM_TYPEHID) {
                if (!libError)
                    joypad.update(input);
            }

            LeaveCriticalSection( &mcsSc );
            
            LRESULT result = DefRawInputProc(&input, size, sizeof (RAWINPUTHEADER));
            delete[] input;

            return result;        
        }
        
        return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	
	auto run() -> void {		
		WNDCLASS wc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hInstance = GetModuleHandle(0);
		wc.lpfnWndProc = RawInputWndProc;
		wc.lpszClassName = L"RawInputClass";
		wc.lpszMenuName = 0;
		wc.style = CS_VREDRAW | CS_HREDRAW;
		RegisterClass(&wc);

		hwnd = CreateWindow(L"RawInputClass", L"RawInputClass", WS_POPUP, 0, 0, 64, 64, 0, 0, GetModuleHandle(0), 0);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
		
        initDevices();
		keyboard.init(); // no multi keyboard support (at the moment)
		
		RAWINPUTDEVICE riDevice[ 4 ];
		
		riDevice[0].usUsagePage = 1;
		riDevice[0].usUsage = 6; //Keyboard
		riDevice[0].dwFlags = RIDEV_INPUTSINK;
		riDevice[0].hwndTarget = hwnd;

		riDevice[1].usUsagePage = 1;
		riDevice[1].usUsage = 2; //Mouse
		riDevice[1].dwFlags = RIDEV_INPUTSINK;
		riDevice[1].hwndTarget = hwnd;

		riDevice[2].usUsagePage = 1;
		riDevice[2].usUsage = 4; //Joypads
		riDevice[2].dwFlags = RIDEV_INPUTSINK;
		riDevice[2].hwndTarget = hwnd;	
        
        riDevice[3].usUsagePage = 1;
		riDevice[3].usUsage = 5; //Joysticks
		riDevice[3].dwFlags = RIDEV_INPUTSINK;
		riDevice[3].hwndTarget = hwnd;	        
				
		RegisterRawInputDevices(riDevice, 4, sizeof(RAWINPUTDEVICE));
				
		EnterCriticalSection( &mcsSc );
		ready = true;
		LeaveCriticalSection( &mcsSc );

		while (true) {
			MSG msg;
			GetMessage(&msg, hwnd, 0, 0);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	static auto CALLBACK RawInputWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		RawWorker* worker = (RawWorker*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		return worker->wndProc(hwnd, msg, wparam, lparam);
	}
	
	static auto WINAPI EntryPoint(void* param) -> DWORD {
		RawWorker* worker = (RawWorker*)param;
		worker->run();
		return 0;
	}

	auto term() -> void {
		if (hwnd) DestroyWindow(hwnd);
		ready = false;
		deviceChanged = false;
	}
};


