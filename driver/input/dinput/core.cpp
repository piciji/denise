
#include <initguid.h>
#include <dinput.h>
#include <dbt.h>
#include <cstring>
#include <cstdint>

#include "../../tools/win.h"
#include "../../tools/hid.h"
#include "../../tools/tools.h"
#include "../../tools/chronos.h"

#ifndef DI8DEVCLASS_GAMECTRL
#define DI8DEVCLASS_GAMECTRL DIDEVTYPE_JOYSTICK
#endif

namespace DRIVER {

struct DI_IDENT_CORE : DI_IDENT {
    HWND hwndMain = nullptr;
	HWND hwndHotPlug = nullptr;
    HANDLE changeHandler = nullptr;
    HANDLE workerThread = nullptr;
    CRITICAL_SECTION criticalSection;

    KeyCallback* keyCallback = nullptr;
    
#if DIRECTINPUT_VERSION == 0x0500
    LPDIRECTINPUT din;
    LPDIRECTINPUTDEVICE dinKey, dinMouse;
#elif DIRECTINPUT_VERSION == 0x0700     
    LPDIRECTINPUT7 din;
    LPDIRECTINPUTDEVICE7 dinKey, dinMouse;    
#else
    LPDIRECTINPUT8 din;
    LPDIRECTINPUTDEVICE8 dinKey, dinMouse;
#endif
    
    bool mAcquired = false;
	bool deviceChanged = false;
    unsigned char keyStates[256] = {0};

	struct Joypad {
        
#if DIRECTINPUT_VERSION == 0x0500
        LPDIRECTINPUTDEVICE handle = nullptr;
        LPDIRECTINPUTDEVICE2 pollHandle = nullptr;
#elif DIRECTINPUT_VERSION == 0x0700     
        LPDIRECTINPUTDEVICE7 handle = nullptr;   
#else
        LPDIRECTINPUTDEVICE8 handle = nullptr;
#endif        
    
		Hid::Joypad* hid = nullptr;
	};
	std::vector<Joypad> joypads;

#if DIRECTINPUT_VERSION == 0x0500
        LPDIRECTINPUTDEVICE curJoyHandle = nullptr;
#elif DIRECTINPUT_VERSION == 0x0700     
        LPDIRECTINPUTDEVICE7 curJoyHandle = nullptr;   
#else
        LPDIRECTINPUTDEVICE8 curJoyHandle = nullptr;
#endif     
    
	Hid::Mouse* hidMouse = nullptr;
	Hid::Keyboard* hidKeyboard = nullptr;
	
    auto enumJoypads(const DIDEVICEINSTANCE* instance) -> bool {
		Joypad jp;
		
#if DIRECTINPUT_VERSION == 0x0500  
        HRESULT hr = din->CreateDevice(instance->guidInstance, &jp.handle, 0);
        jp.handle->QueryInterface( IID_IDirectInputDevice2, (LPVOID*)&jp.pollHandle );
#elif DIRECTINPUT_VERSION == 0x0700        
		HRESULT hr = din->CreateDeviceEx(instance->guidInstance, IID_IDirectInputDevice7, (void**)&jp.handle, 0);
#else
        HRESULT hr = din->CreateDevice(instance->guidInstance, &jp.handle, 0);
#endif        
		if (FAILED(hr)) return DIENUM_CONTINUE;
		
		jp.hid = new Hid::Joypad;
		curJoyHandle = jp.handle;
		
		auto joyname = Win::utf8_t(instance->tszProductName);
		auto guid = instance->guidInstance;
		jp.hid->id = uniqueDeviceId( joypads, guid.Data1 );
		if (joyname.empty()) joyname = "Joypad";
		jp.hid->name = uniqueDeviceName(joypads, joyname);
		
		DIDEVCAPS mCaps;
		mCaps.dwSize = sizeof(DIDEVCAPS);
		jp.handle->GetCapabilities(&mCaps);

		auto mNumAxes		= mCaps.dwAxes;        
        // resulting position values are stored in fixed variables: lX, lY, lZ ...
        // hmm ... so don't know which axes are active.
        // thats why we check for all axes
        mNumAxes = 6;
        
		auto mNumButtons	= mCaps.dwButtons;
		auto mNumHats		= mCaps.dwPOVs;

		for (unsigned i = 0; i < mNumAxes; i++) {
			std::string axis = std::to_string(i);

			switch (i) {
				case 0: axis = "X"; break;
				case 1: axis = "Y"; break;
				case 2: axis = "Z"; break;
				case 3: axis = "Z|Rot"; break;
				case 4: axis = "X|Rot"; break;
				case 5: axis = "Y|Rot"; break;
			}
			jp.hid->axes().append( axis );
		}
		
		for (unsigned hat = 0; hat < mNumHats; hat++) {
			jp.hid->hats().append( std::to_string(hat) + ".X" );
			jp.hid->hats().append( std::to_string(hat) + ".Y" );
		}
		
		for (unsigned button = 0; button < mNumButtons; button++) {
			jp.hid->buttons().append( std::to_string(button) );
		}

		jp.handle->SetDataFormat(&c_dfDIJoystick2);

		jp.handle->SetCooperativeLevel(hwndMain, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
		jp.handle->EnumObjects(EnumJoypadAxesCallback, (void*) this, DIDFT_ABSAXIS);
		
		joypads.push_back(jp);

		return DIENUM_CONTINUE;
	}
	
    auto initAxis(const DIDEVICEOBJECTINSTANCE* instance) -> bool {
		DIPROPRANGE range;
		memset(&range, 0, sizeof (DIPROPRANGE));
		range.diph.dwSize = sizeof (DIPROPRANGE);
		range.diph.dwHeaderSize = sizeof (DIPROPHEADER);
		range.diph.dwHow = DIPH_BYID;
		range.diph.dwObj = instance->dwType;
		range.lMin = -32768;
		range.lMax = +32767;
		curJoyHandle->SetProperty(DIPROP_RANGE, &range.diph);

		return DIENUM_CONTINUE;
	}
	
    static auto CALLBACK EnumJoypadsCallback(const DIDEVICEINSTANCE* instance, void* pRef) -> BOOL {
		DI_IDENT_CORE* ptr = (DI_IDENT_CORE*) pRef;
		return ptr->enumJoypads(instance);
	}
	
    static auto CALLBACK EnumJoypadAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, void* pRef) -> BOOL {
		DI_IDENT_CORE* ptr = (DI_IDENT_CORE*) pRef;
		return ptr->initAxis(instance);
	}

    auto init(uintptr_t handle) -> bool {
		mAcquired = false;
		deviceChanged = false;
		hwndMain = (HWND) handle;
		term();
		
		hidKeyboard = new Hid::Keyboard;
		hidMouse = new Hid::Mouse;
		hidKeyboard->id = 0;
		hidMouse->id = 1;

        //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        HRESULT res;
        
#if DIRECTINPUT_VERSION == 0x500   
        res = CoCreateInstance(CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, IID_IDirectInput2W, (PVOID*)&din);
                
#elif DIRECTINPUT_VERSION == 0x700          
        res = CoCreateInstance(CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, IID_IDirectInput7W, (PVOID*)&din);
#else
        res = CoCreateInstance(CLSID_DirectInput8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectInput8W, (PVOID*)&din);
#endif        

        if (res != S_OK)
            return false;
        
        if (IDirectInput_Initialize(din, GetModuleHandle(0), DIRECTINPUT_VERSION) != S_OK) {
            IDirectInput_Release(din);
            return false;
        }

#if DIRECTINPUT_VERSION == 0x700          
		if (DI_OK == din->CreateDeviceEx(GUID_SysKeyboard, IID_IDirectInputDevice7, (void**)&dinKey, NULL) ) {
#else
        if (DI_OK == din->CreateDevice(GUID_SysKeyboard, &dinKey, 0)) {
#endif        			
            dinKey->SetDataFormat(&c_dfDIKeyboard);
            dinKey->SetCooperativeLevel(hwndMain, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);

            changeHandler = CreateEvent(NULL, false, false, NULL);
            if (dinKey->SetEventNotification(changeHandler) == S_OK) {
                InitializeCriticalSection(&criticalSection);
                workerThread = CreateThread(NULL, 0, DI_IDENT_CORE::EntryPoint, (void*) this, 0, NULL);
            }
            dinKey->Acquire();

            for(unsigned id = 0; id <= 0xff; id++) {
                std::string ident = Win::translateKeyName( id );
                hidKeyboard->buttons().append( ident, Win::getKeyCode( ident, id ) );
                //printf("%s %d \n", ident.c_str(), id);
            }
        }
        
#if DIRECTINPUT_VERSION == 0x700          
		if (DI_OK == din->CreateDeviceEx(GUID_SysMouse, IID_IDirectInputDevice7, (void**)&dinMouse, NULL) ) {
#else
        if (DI_OK == din->CreateDevice(GUID_SysMouse, &dinMouse, 0)) {
#endif                  		
			        
#if DIRECTINPUT_VERSION == 0x500 
            dinMouse->SetDataFormat(&c_dfDIMouse);
#else
            dinMouse->SetDataFormat(&c_dfDIMouse2);
#endif            
		
            dinMouse->SetCooperativeLevel(hwndMain, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
            dinMouse->Acquire();

            hidMouse->axes().append("X");
            hidMouse->axes().append("Y");
            hidMouse->axes().append("Z");

            hidMouse->buttons().append("Left");		
            hidMouse->buttons().append("Right");		
            hidMouse->buttons().append("Middle");
            hidMouse->buttons().append("Up");
#if DIRECTINPUT_VERSION != 0x500         
            hidMouse->buttons().append("Down");
#endif        
        }
        
		din->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoypadsCallback, (void*) this, DIEDFL_ATTACHEDONLY);

		hotPlugListener();
        
		return true;
	}
	
    auto term() -> void {
        if(workerThread) {
            DeleteCriticalSection(&criticalSection);
            TerminateThread(workerThread, 0);
        }
		dxRelease(din);
		dxRelease(dinKey);
		dxRelease(dinMouse);

		for(auto& joypad : joypads ) {
			dxRelease( joypad.handle );
#if DIRECTINPUT_VERSION == 0x500    
            dxRelease( joypad.pollHandle );
#endif            
			if(joypad.hid) delete joypad.hid;
		}
		joypads.clear();

		if(hidMouse) delete hidMouse, hidMouse = nullptr;
		if(hidKeyboard) delete hidKeyboard, hidKeyboard = nullptr;	
		if(hwndHotPlug) DestroyWindow(hwndHotPlug), hwndHotPlug = nullptr;
        if(changeHandler) CloseHandle(changeHandler), changeHandler = nullptr;
	}

    auto run() -> void {

        while(true) {
            WaitForSingleObject(changeHandler, INFINITE);

            unsigned char newStates[256] = {0};
            if (FAILED(dinKey->GetDeviceState(256, (LPVOID) &newStates))) {
                dinKey->Acquire();
                if (FAILED(dinKey->GetDeviceState(256, (LPVOID) &newStates)))
                    memset(newStates, 0, sizeof(newStates));
            }

            if (keyCallback) {
                uint8_t oldState;
                uint8_t newState;
                for (unsigned key = 0; key < 256; key++) {
                    oldState = keyStates[key];
                    newState = newStates[key];

                    if (oldState != newState) {
                        (*keyCallback)();
                    }
                }
            }

            EnterCriticalSection(&criticalSection);
            std::memcpy(&keyStates, &newStates, 256);
            LeaveCriticalSection(&criticalSection);
        }
        ExitThread(0);
    }
	
    auto poll() -> std::vector<Hid::Device*> {
		std::vector<Hid::Device*> devices;
		
		if(deviceChanged) {
			if (!init((uintptr_t)hwndMain)) 
				return devices;
		}		
		
		if (dinKey) {
         //   if (GetFocus()) {
            if (!workerThread) {
                if (FAILED(dinKey->GetDeviceState(sizeof(keyStates), (LPVOID) keyStates))) {
                    dinKey->Acquire();
                    if (FAILED(dinKey->GetDeviceState(sizeof(keyStates), (LPVOID) keyStates)))
                        memset(keyStates, 0, sizeof(keyStates));
                }
            }

            EnterCriticalSection( &criticalSection );
            for (auto& input : hidKeyboard->buttons().inputs)
                input.setValue( !!(keyStates[input.id] & 0x80) );
            LeaveCriticalSection( &criticalSection );

           // } else
             //   memset(keystate, 0, sizeof(keystate));

			devices.push_back(hidKeyboard);
		}

		if (dinMouse) {
#if DIRECTINPUT_VERSION == 0x500            
			DIMOUSESTATE m_state;
            
            if (FAILED(dinMouse -> GetDeviceState(sizeof (DIMOUSESTATE), (void*) &m_state))) {
				dinMouse -> Acquire();
				if (FAILED(dinMouse -> GetDeviceState(sizeof (DIMOUSESTATE), (void*) &m_state))) {
					memset(&m_state, 0, sizeof (DIMOUSESTATE));
				}
			}
#else
            DIMOUSESTATE2 m_state;
            
            if (FAILED(dinMouse -> GetDeviceState(sizeof (DIMOUSESTATE2), (void*) &m_state))) {
				dinMouse -> Acquire();
				if (FAILED(dinMouse -> GetDeviceState(sizeof (DIMOUSESTATE2), (void*) &m_state))) {
					memset(&m_state, 0, sizeof (DIMOUSESTATE2));
				}
			}
#endif            

#if DIRECTINPUT_VERSION == 0x500              
			hidMouse->axes().inputs[0].setValue( m_state.lX << 1 );
			hidMouse->axes().inputs[1].setValue( m_state.lY << 1 );
#else
			hidMouse->axes().inputs[0].setValue( m_state.lX );
			hidMouse->axes().inputs[1].setValue( m_state.lY );
#endif            
			hidMouse->axes().inputs[2].setValue( m_state.lZ );

			for (auto& input : hidMouse->buttons().inputs)
				input.setValue( (bool)m_state.rgbButtons[input.id] );
			
			devices.push_back(hidMouse);
		}

		DIJOYSTATE2 js;
		
		for(auto& jp : joypads) {
			
#if DIRECTINPUT_VERSION == 0x500              
			if (FAILED(jp.pollHandle->Poll())) {
				jp.handle->Acquire();
				
				if (FAILED(jp.pollHandle->Poll()))
					continue;
			}
#else
            if (FAILED(jp.handle->Poll())) {
				jp.handle->Acquire();
				
				if (FAILED(jp.handle->Poll()))
					continue;
			}

#endif			
			memset(js.rgbButtons, 0, jp.hid->buttons().inputs.size());
			jp.handle->GetDeviceState(sizeof (DIJOYSTATE2), &js);
			auto& hats = jp.hid->hats();
						
			for (unsigned hat = 0; hat < hats.inputs.size() / 2; hat++ ) {
				
				unsigned pov = js.rgdwPOV[hat];
				bool up, right, down, left;
				
				if (pov < 36000) {
					up = pov >= 31500 || pov <= 4500;
					right = pov >= 4500 && pov <= 13500;
					down = pov >= 13500 && pov <= 22500;
					left = pov >= 22500 && pov <= 31500;
				} else {
					up = right = down = left = 0;
				}

				unsigned x = hat * 2;

				hats.inputs[x + 0].setValue(left ? -32768 : (right ? +32767 : 0));
				hats.inputs[x + 1].setValue(up ? -32768 : (down ? +32767 : 0));	
			}
					
			for(auto& input : jp.hid->axes().inputs) {
				switch(input.id) {
					case 0: input.setValue( js.lX ); break;
					case 1: input.setValue( js.lY ); break;
					case 2: input.setValue( js.lZ ); break;
					case 3: input.setValue( js.lRz ); break;
					case 4: input.setValue( js.lRx ); break;
					case 5: input.setValue( js.lRy ); break;
				}
			}

			for(auto& input : jp.hid->buttons().inputs)
				input.setValue( (bool)js.rgbButtons[input.id] );
			
			devices.push_back(jp.hid);
		}					
		
		return devices;
	}
	
    auto mAcquire() -> void {
		if (!dinMouse) return;
		if (!mAcquired) {
			dinMouse->Unacquire();
			dinMouse->SetCooperativeLevel(hwndMain, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
			dinMouse->Acquire();
			mAcquired = true;
		}
	}
	
    auto mUnacquire() -> void {
		if (!dinMouse) return;
		if (mAcquired) {
			dinMouse->Unacquire();
			dinMouse->SetCooperativeLevel(hwndMain, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
			dinMouse->Acquire();
			mAcquired = false;
		}
	}
	
    auto mIsAcquired() -> bool { return mAcquired; }
	
	//hotplug support
	auto wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		switch (msg) {
			case WM_DEVICECHANGE:
				deviceChanged = true;				
				break;
			default: break;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	
	auto hotPlugListener() -> void {
		WNDCLASS wc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hInstance = GetModuleHandle(0);
		wc.lpfnWndProc = HotPlugProc;
		wc.lpszClassName = L"DInputHotPlug";
		wc.lpszMenuName = 0;
		wc.style = CS_VREDRAW;
		RegisterClass(&wc);		

		hwndHotPlug = CreateWindow(L"DInputHotPlug", L"DInputHotPlug", WS_POPUP,
		0, 0, 32, 32, NULL, NULL, GetModuleHandle(0), NULL);

		if (hwndHotPlug == NULL) return;
		SetWindowLongPtr(hwndHotPlug, GWLP_USERDATA, (LONG_PTR)this);
		
		DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
		ZeroMemory(&notificationFilter, sizeof (notificationFilter));

		notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		notificationFilter.dbcc_size = sizeof (notificationFilter);

		RegisterDeviceNotification(hwndHotPlug, &notificationFilter,
				DEVICE_NOTIFY_WINDOW_HANDLE |
				DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
	}
	
	static auto CALLBACK HotPlugProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		DI_IDENT_CORE* worker = (DI_IDENT_CORE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		return worker->wndProc(hwnd, msg, wparam, lparam);
	}

    static auto WINAPI EntryPoint(void* param) -> DWORD {
        DI_IDENT_CORE* worker = (DI_IDENT_CORE*)param;
        worker->run();
        return 0;
    }

    auto setKeyboardCallback( KeyCallback* callback ) -> void {
        this->keyCallback = callback;
    }
		
    DI_IDENT_CORE(unsigned version = 0) : DI_IDENT() {
		din = 0;
		dinKey = dinMouse = 0;
		mAcquired = deviceChanged = false;
	}
    
    ~DI_IDENT_CORE() { term(); }
};

}
