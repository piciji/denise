
#include "../../tools/win.h"
#include "../../tools/hid.h"
#include "../../tools/tools.h"
#include "../../tools/crc32.h"
#include "../../tools/chronos.h"

#include <hidsdi.h>
#include <dbt.h>

namespace DRIVER {
	
#include "hid.cpp"
#include "keyboard.cpp"
#include "mouse.cpp"
#include "joypad.cpp"
	
struct RawInput : Input {
	HWND hwnd = nullptr;
	bool deviceChanged = false;
	bool libError = false;

	RawMouse mouse;
	RawKeyboard keyboard;
	RawJoypad joypad;

	RawInput() {
		libError = !lookupHidLibrary();
	}

	auto poll() -> std::vector<Hid::Device*> {		
		std::vector<Hid::Device*> devices;				

		if(deviceChanged) {
			initDevices();
			deviceChanged = false;
		}
		
		keyboard.poll( devices );

		mouse.poll( devices );
		
		joypad.poll( devices );

		return devices;
	}
	
	auto init( uintptr_t handle ) -> bool {
		term();

		WNDCLASS wc = {0};
		wc.lpfnWndProc   = RawInputWndProc;
		wc.lpszClassName = L"RawInputClass";
		wc.hInstance = GetModuleHandle(0);

		if (!RegisterClass(&wc) && (GetLastError() != ERROR_CLASS_ALREADY_EXISTS))
			return false;

		if (!(hwnd = CreateWindowEx(0, wc.lpszClassName,
			NULL, 0, 0, 0, 0, 0,
			HWND_MESSAGE, NULL, NULL, NULL))) {
			UnregisterClass(wc.lpszClassName, NULL);
			return false;
		}

		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

		DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
		ZeroMemory(&notificationFilter, sizeof (notificationFilter));
		notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		notificationFilter.dbcc_size = sizeof (notificationFilter);

		RegisterDeviceNotification(hwnd, &notificationFilter,
				DEVICE_NOTIFY_WINDOW_HANDLE |
				DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

		mouse.handle = (HWND)handle;

		initDevices();
		keyboard.init(); // no multi keyboard support (at the moment)

		RAWINPUTDEVICE riDevice[ 4 ];

		riDevice[0].usUsagePage = 1;
		riDevice[0].usUsage = 6; //Keyboard
		riDevice[0].dwFlags = RIDEV_INPUTSINK | RIDEV_NOHOTKEYS;
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

		return true;
	}

	auto initDevices() -> void {
		joypad.init();
		mouse.init();
		keyboard.initKeys();

		unsigned deviceCount = 0;
		GetRawInputDeviceList(NULL, &deviceCount, sizeof (RAWINPUTDEVICELIST));
		RAWINPUTDEVICELIST* list = new RAWINPUTDEVICELIST[deviceCount];
		GetRawInputDeviceList(list, &deviceCount, sizeof (RAWINPUTDEVICELIST));
		logger->log("add");
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
		mouse.sortMice();
		delete[] list;
	}
	
	auto mAcquire() -> void {
		mouse.mAcquire();
	}
	
	auto mUnacquire() -> void {
		mouse.mUnacquire();
	}
	
	auto mIsAcquired() -> bool {
		return mouse.mIsAcquired();
	}

    auto setKeyboardCallback( KeyCallback* callback ) -> void {
        keyboard.keyCallback = callback;
    }
		
	auto term() -> void {
		if (hwnd) DestroyWindow(hwnd);
		deviceChanged = false;
	}

	auto wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		if (msg == WM_DEVICECHANGE)
			deviceChanged = true;

		if (deviceChanged) {
			DefWindowProc(hwnd, msg, wparam, lparam);
			return 0;
		}

		if (msg != WM_INPUT)
			return DefWindowProc(hwnd, msg, wparam, lparam);

		unsigned size = 0;
		GetRawInputData((HRAWINPUT) lparam, RID_INPUT, NULL, &size, sizeof (RAWINPUTHEADER));

		if (size) {
			RAWINPUT* input = new RAWINPUT[size];
			GetRawInputData((HRAWINPUT) lparam, RID_INPUT, input, &size, sizeof (RAWINPUTHEADER));

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

			delete[] input;
		}

		DefWindowProc(hwnd, msg, wparam, lparam);
		return 0;
	}

	static auto CALLBACK RawInputWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		RawInput* worker = (RawInput*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (!worker)
			return DefWindowProc(hwnd, msg, wparam, lparam);

		return worker->wndProc(hwnd, msg, wparam, lparam);
	}

	
	~RawInput() { term(); } 
};

}
