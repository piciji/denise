#include <X11/extensions/Xfixes.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <gdk/gdkx.h>
#include <cstring>
#include <thread>
#include <atomic>

#include "../tools/hid.h"
#include "../tools/chronos.h"

namespace DRIVER {

struct XInput : public Input {
	Hid::Mouse* hidMouse = nullptr;
	Hid::Keyboard* hidKeyboard = nullptr;
	
	std::string joypadDriver = "";
    unsigned char _keycode[256];

    std::atomic<bool> kill;
    
#ifdef DRV_SDLINPUT
    SdlInput* sdl;
#endif
#ifdef DRV_UDEV
	Udev* udev;
#endif	
	
    Display* display = nullptr;
    Window rootwindow;
    unsigned relativex, relativey;
    int rememberX, rememberY;
    const unsigned warpMargin = 50;
    std::mutex keyMutex;

    char keyState[32];

    struct {
        int16_t x = 0;
        int16_t y = 0;
        bool left = false;
        bool middle = false;
        bool right = false;
        bool up;
        bool down;
    } mouseState;
    
    bool mouseAcquired;
    uintptr_t handle;
        
    auto init(uintptr_t _handle) -> bool {
        
		term();
		
		handle = _handle;
		hidKeyboard = new Hid::Keyboard;
		hidMouse = new Hid::Mouse;
		hidKeyboard->id = 0;
		hidMouse->id = 1;

	#ifdef DRV_SDLINPUT
		if (joypadDriver == "sdl")
			if (!sdl->init()) {}
	#endif
	#ifdef DRV_UDEV
		if (joypadDriver == "udev")
			if (!udev->init()) {}
	#endif
		display = XOpenDisplay(0);
		rootwindow = DefaultRootWindow(display);

		memset(&_keycode, 0, sizeof _keycode);
		unsigned _count = 0;
        char* ident = nullptr;
        
        for(unsigned keycode = 0; keycode <= 0xff; keycode++) {
            
            KeySym keySym = XkbKeycodeToKeysym( display, keycode, 0, 0 );
                        
			ident = XKeysymToString(keySym);
            
			if (!ident)
                continue;
					
			hidKeyboard->buttons().append( (std::string)ident, getKeyCode( (std::string)ident, (uint8_t)keycode ) );
            //printf("%s %d \n", ident, keycode);
            
			_keycode[_count++] = keycode;
        }
        		
		mouseAcquired = false;
		relativex = 0;
		relativey = 0;
        rememberX = 0;
        rememberY = 0;

		hidMouse->axes().append("X");
		hidMouse->axes().append("Y");

		hidMouse->buttons().append("Left");		
		hidMouse->buttons().append("Middle");		
		hidMouse->buttons().append("Right");
		hidMouse->buttons().append("Up");
		hidMouse->buttons().append("Down");

        initWorker();

		return true;
	}
    
    auto getKeyCode(std::string ident, uint8_t keycode ) -> Hid::Key {
        // keycodes are device indepandant and describe the position on keyboard.        
        // for easier use we assign the keycodes to human readable enums of the uk layout.        
        auto key = Hid::Input::getKeyCode( ident );
        
        if (key != Hid::Key::Unknown)
            return key;
        
        if (keycode == 36) return Hid::Key::Return;
        if (keycode == 65) return Hid::Key::Space;
        if (keycode == 22) return Hid::Key::Backspace;
        if (keycode == 23) return Hid::Key::Tab;
        if (keycode == 9) return Hid::Key::Esc;
        if (keycode == 66) return Hid::Key::CapsLock;
       
        if (keycode == 49) return Hid::Key::Grave;
        if (keycode == 20) return Hid::Key::Minus;
        if (keycode == 21) return Hid::Key::Equal;
        if (keycode == 35) return Hid::Key::ClosedSquareBracket;        
        if (keycode == 51) return Hid::Key::NumberSign;
        if (keycode == 94) return Hid::Key::Backslash;
        if (keycode == 59) return Hid::Key::Comma;
        if (keycode == 60) return Hid::Key::Period;
        if (keycode == 61) return Hid::Key::Slash;
        if (keycode == 135) return Hid::Key::Menu;        
        if (keycode == 48) return Hid::Key::Apostrophe;
        if (keycode == 47) return Hid::Key::Semicolon;
        if (keycode == 34) return Hid::Key::OpenSquareBracket;
        
        if (keycode == 118) return Hid::Key::Insert;
        if (keycode == 110) return Hid::Key::Home;
        if (keycode == 112) return Hid::Key::Prior;
        if (keycode == 119) return Hid::Key::Delete;
        if (keycode == 115) return Hid::Key::End;
        if (keycode == 117) return Hid::Key::Next;
        if (keycode == 107) return Hid::Key::Print;
        if (keycode == 78) return Hid::Key::ScrollLock;
        if (keycode == 127) return Hid::Key::Pause;
        
        if (keycode == 116) return Hid::Key::CursorDown;
        if (keycode == 113) return Hid::Key::CursorLeft;
        if (keycode == 114) return Hid::Key::CursorRight;
        if (keycode == 111) return Hid::Key::CursorUp;
        
        if (keycode == 50) return Hid::Key::ShiftLeft;
        if (keycode == 62) return Hid::Key::ShiftRight;
        if (keycode == 64) return Hid::Key::AltLeft;
        if (keycode == 108) return Hid::Key::AltRight;
        if (keycode == 37) return Hid::Key::ControlLeft;
        if (keycode == 105) return Hid::Key::ControlRight;
        if (keycode == 133) return Hid::Key::SuperLeft;
        if (keycode == 134) return Hid::Key::SuperRight;
        
        if (keycode == 90) return Hid::Key::NumPad0;
        if (keycode == 87) return Hid::Key::NumPad1;
        if (keycode == 88) return Hid::Key::NumPad2;
        if (keycode == 89) return Hid::Key::NumPad3;
        if (keycode == 83) return Hid::Key::NumPad4;
        if (keycode == 84) return Hid::Key::NumPad5;
        if (keycode == 85) return Hid::Key::NumPad6;
        if (keycode == 79) return Hid::Key::NumPad7;
        if (keycode == 80) return Hid::Key::NumPad8;
        if (keycode == 81) return Hid::Key::NumPad9;
        if (keycode == 91) return Hid::Key::NumComma;
        if (keycode == 106) return Hid::Key::NumDivide;
        if (keycode == 63) return Hid::Key::NumMultiply;
        if (keycode == 82) return Hid::Key::NumSubtract;
        if (keycode == 86) return Hid::Key::NumAdd;
        if (keycode == 104) return Hid::Key::NumEnter;
        if (keycode == 77) return Hid::Key::NumLock;
        
        // on french keyboard M <> ;
        if (keycode == 58) return Hid::Key::Semicolon;
        // digits on french keyboards are secondary functions        
        if (keycode == 10) return Hid::Key::D1;
        if (keycode == 11) return Hid::Key::D2;
        if (keycode == 12) return Hid::Key::D3;
        if (keycode == 13) return Hid::Key::D4;
        if (keycode == 14) return Hid::Key::D5;
        if (keycode == 15) return Hid::Key::D6;
        if (keycode == 16) return Hid::Key::D7;
        if (keycode == 17) return Hid::Key::D8;
        if (keycode == 18) return Hid::Key::D9;
        if (keycode == 19) return Hid::Key::D0;

        
        return Hid::Key::Unknown;
    }
	
    auto term() -> void {
		if(display) XCloseDisplay(display), display = nullptr;

		if(hidMouse) delete hidMouse, hidMouse = nullptr;
		if(hidKeyboard) delete hidKeyboard, hidKeyboard = nullptr;
	}
	
    auto poll() -> std::vector<Hid::Device*> {
		std::vector<Hid::Device*> devices;

        keyMutex.lock();
        for (auto& input : hidKeyboard->buttons().inputs)
            input.setValue( (bool)(keyState[_keycode[input.id] >> 3] & (1 << (_keycode[input.id] & 7))) );

        hidMouse->axes().inputs[0].setValue(mouseState.x);
        hidMouse->axes().inputs[1].setValue(mouseState.y);
        mouseState.x = 0;
        mouseState.y = 0;

        hidMouse->buttons().inputs[0].setValue(mouseState.left);
        hidMouse->buttons().inputs[1].setValue(mouseState.middle);
        hidMouse->buttons().inputs[2].setValue(mouseState.right);
        hidMouse->buttons().inputs[3].setValue(mouseState.up);
        hidMouse->buttons().inputs[4].setValue(mouseState.down);

        keyMutex.unlock();

        devices.push_back(hidKeyboard);
        devices.push_back(hidMouse);

		//pollKeyboard( devices );

        //pollMouse( devices );
		
	#ifdef DRV_SDLINPUT
		if (joypadDriver == "sdl") sdl->pollJoypad(devices);
	#endif
	#ifdef DRV_UDEV
		if (joypadDriver == "udev") udev->pollJoypad(devices);
	#endif
			
		return devices;
	}
    
    auto pollMouse(std::vector<Hid::Device*>& devices) -> void {

        Window root_return, child_return;
        int root_x_return = 0, root_y_return = 0;
        int win_x_return = 0, win_y_return = 0;
        unsigned int mask_return = 0;

        XQueryPointer(display, rootwindow,
                &root_return, &child_return, &root_x_return, &root_y_return,
                &win_x_return, &win_y_return, &mask_return);        
        
        hidMouse->axes().inputs[0].setValue((int16_t) ((root_x_return - relativex)));
        hidMouse->axes().inputs[1].setValue((int16_t) ((root_y_return - relativey)));

        relativex = root_x_return;
        relativey = root_y_return;
        
        if (mIsAcquired())            
            warpMouse( root_x_return, root_y_return );        

        hidMouse->buttons().inputs[0].setValue((bool)(mask_return & Button1Mask));
        hidMouse->buttons().inputs[1].setValue((bool)(mask_return & Button2Mask));
        hidMouse->buttons().inputs[2].setValue((bool)(mask_return & Button3Mask));
        hidMouse->buttons().inputs[3].setValue((bool)(mask_return & Button4Mask));
        hidMouse->buttons().inputs[4].setValue((bool)(mask_return & Button5Mask));

        devices.push_back( hidMouse );
    }
    
    auto warpMouse( int absX, int absY ) -> void {
        
        // 1. a hidden cursor generates hover effects, hence we capture it in application window
        // 2. if cursor reaches screen border there will be no more deltas generated, hence we wrap 
        //    cursor around. this way the cursor is always moving.
        
        gint x, y, w, h;
        gdk_window_get_origin((GdkWindow*)handle, &x, &y);
        w = gdk_window_get_width((GdkWindow*)handle);
        h = gdk_window_get_height((GdkWindow*)handle);
        
        if (absX > ((x + w) - warpMargin)) {
            // right to left 
            relativex = x + warpMargin;
            XWarpPointer(display, None, rootwindow, 0, 0, 0, 0, relativex, absY);            
            
        } else if( absX < (x + warpMargin)) {
            // left to right
            relativex = x + w - warpMargin;
            XWarpPointer(display, None, rootwindow, 0, 0, 0, 0, relativex, absY);          
        } 
        
        if (absY > ((y + h) - warpMargin)) {
            // bottom to top
            relativey = y + warpMargin;
            XWarpPointer(display, None, rootwindow, 0, 0, 0, 0, absX, relativey);            
            
        } else if( absY < (y + warpMargin)) {
            // top to bottom
            relativey = y + h - warpMargin;
            XWarpPointer(display, None, rootwindow, 0, 0, 0, 0, absX, relativey);
        }            
    }
	
	auto pollKeyboard(std::vector<Hid::Device*>& devices) -> void {
		if (!display) return;

		char state[32];
		XQueryKeymap(display, state);		
		
		for (auto& input : hidKeyboard->buttons().inputs)
			input.setValue( (bool)(state[_keycode[input.id] >> 3] & (1 << (_keycode[input.id] & 7))) );	
		
		devices.push_back(hidKeyboard);
	}

    auto initWorker() -> void {

        std::thread worker([this] {
            kill = false;
            char state[32];
            Window root_return, child_return;
            int root_x_return = 0, root_y_return = 0;
            int win_x_return = 0, win_y_return = 0;
            unsigned int mask_return = 0;

            while(true) {
                if (kill) {
                    kill = false;
                    return;
                }

                usleep( 10000 );

                XQueryKeymap(display, state);

                XQueryPointer(display, rootwindow,
                              &root_return, &child_return, &root_x_return, &root_y_return,
                              &win_x_return, &win_y_return, &mask_return);

                keyMutex.lock();
                mouseState.x += (int16_t) (root_x_return - relativex);
                mouseState.y += (int16_t) (root_y_return - relativey);
                mouseState.left = (bool)(mask_return & Button1Mask);
                mouseState.middle = (bool)(mask_return & Button2Mask);
                mouseState.right = (bool)(mask_return & Button3Mask);
                mouseState.up = (bool)(mask_return & Button4Mask);
                mouseState.down = (bool)(mask_return & Button5Mask);

                std::memcpy(keyState, state, 32);
                keyMutex.unlock();

                relativex = root_x_return;
                relativey = root_y_return;

                if (mIsAcquired())
                    warpMouse( root_x_return, root_y_return );
            }
        });

        worker.detach();
    }
		
    auto mAcquire() -> void {
		if (mIsAcquired()) return;

		int result = XGrabPointer(display, GDK_WINDOW_XID( (GdkWindow*)handle ), True, 0, GrabModeAsync, GrabModeAsync,
				rootwindow, 0, CurrentTime);

		if (result == GrabSuccess || result == AlreadyGrabbed) {

            XFixesHideCursor(display, rootwindow);
            
			XGrabButton(display, AnyButton, AnyModifier, rootwindow, false,
					0, GrabModeAsync, GrabModeAsync, rootwindow, 0);


            Window root_return, child_return;
            int win_x_return = 0, win_y_return = 0;
            unsigned int mask_return = 0;

            XQueryPointer(display, rootwindow,
                          &root_return, &child_return, &rememberX, &rememberY,
                          &win_x_return, &win_y_return, &mask_return);

			mouseAcquired = true;
            
			return;
		}
		mouseAcquired = false;
	}
    auto mUnacquire() -> void {
        
		if (mIsAcquired()) {
            
			XUngrabPointer(display, CurrentTime);
			XUngrabButton(display, AnyButton, AnyModifier, rootwindow);
			XFixesShowCursor(display, rootwindow);

            if (rememberX && rememberY)
                XWarpPointer(display, None, rootwindow, 0, 0, 0, 0, rememberX, rememberY);

            mouseAcquired = false;
		}  	
	}
	
    auto mIsAcquired() -> bool {
		return mouseAcquired;
    }
	
	XInput(std::string joypadDriver = "") {
		this->joypadDriver = joypadDriver;
		
		#ifdef DRV_SDLINPUT
			if (this->joypadDriver == "sdl") sdl = new SdlInput();
		#endif
		#ifdef DRV_UDEV
			if (this->joypadDriver == "udev") udev = new Udev();
		#endif
		
		handle = 0;
	}
	~XInput() {
        kill = true;
        while (kill) {
            std::this_thread::yield();
        }

		term();

		#ifdef DRV_SDLINPUT
			if (joypadDriver == "sdl") delete sdl;
		#endif
		#ifdef DRV_UDEV
			if (joypadDriver == "udev") delete udev;
		#endif
	}
};

}
