
#include <xkbcommon/xkbcommon.h>
#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xcb/xfixes.h>

#include <gdk/gdkx.h>
#include <cstring>
#include <thread>

#include "../tools/hid.h"

namespace DRIVER {

struct XCBInput : public Input {
	Hid::Mouse* hidMouse = nullptr;
	Hid::Keyboard* hidKeyboard = nullptr;
    KeyCallback* keyCallback = nullptr;

	std::string joypadDriver = "";
	unsigned char _keycode[256];
	std::atomic<bool> kill;

#ifdef DRV_UHID
    Uhid* uhid;
#endif
#ifdef DRV_UDEV
	Udev* udev;
#endif

	xcb_connection_t* conn = nullptr;
	xcb_screen_t* screen;

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
        bool back;
        bool forward;
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
    	kill = false;

	#ifdef DRV_UHID
		if (joypadDriver == "uhid")
			if (!uhid->init()) {}
	#endif
	#ifdef DRV_UDEV
		if (joypadDriver == "udev")
			if (!udev->init()) {}
	#endif
    	conn = xcb_connect(nullptr, nullptr);
    	if (!conn)
    		return false;
    	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    	struct {
    		xcb_input_event_mask_t info;
    		xcb_input_xi_event_mask_t mask;
    	} inp;

    	inp.info.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
    	inp.info.mask_len = 1;
    	inp.mask = (xcb_input_xi_event_mask_t) (
    		(int)XCB_INPUT_XI_EVENT_MASK_RAW_KEY_PRESS | (int)XCB_INPUT_XI_EVENT_MASK_RAW_KEY_RELEASE
    		| (int)XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_PRESS | (int)XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_RELEASE
    		| (int)XCB_INPUT_XI_EVENT_MASK_RAW_MOTION);
    	xcb_input_xi_select_events(conn, screen->root, 1, &inp.info);

    	memset(&_keycode, 0, sizeof _keycode);
    	memset(&keyState, 0, sizeof keyState);

    	const xcb_setup_t* setup = xcb_get_setup(conn);

    	xcb_get_keyboard_mapping_cookie_t cookie = xcb_get_keyboard_mapping( conn, setup->min_keycode + 1, setup->max_keycode - setup->min_keycode);

    	xcb_get_keyboard_mapping_reply_t* mapping = xcb_get_keyboard_mapping_reply( conn, cookie, nullptr );

    	int nkeycodes = mapping->length / mapping->keysyms_per_keycode;
    	xcb_keysym_t* keysyms  = (xcb_keysym_t*)(mapping + 1);

    	char keysymName[64];
    	for(int i = 0; i < nkeycodes; i++) {
    		uint8_t keycode = i + setup->min_keycode + 1;
    		xkb_keysym_get_name(keysyms[i * mapping->keysyms_per_keycode], keysymName, sizeof(keysymName));

    	//	printf("keycode %s %3u %d", keysymName, keycode, keysyms[i * mapping->keysyms_per_keycode]);
    	//	putchar('\n');

    		hidKeyboard->buttons().append( (std::string)keysymName, getKeyCode( (std::string)keysymName, keycode) );

    		_keycode[i] = keycode;
    	}

    	free(mapping);

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
		hidMouse->buttons().append("Back");
		hidMouse->buttons().append("Forward");

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
    	if(conn) xcb_disconnect(conn), conn = nullptr;

		if(hidMouse) delete hidMouse, hidMouse = nullptr;
		if(hidKeyboard) delete hidKeyboard, hidKeyboard = nullptr;
	}

	auto sendCloseEvent() -> void {
    	xcb_client_message_event_t event;
    	const xcb_window_t window = xcb_generate_id(conn);

    	memset(&event, 0, sizeof(event));
    	event.response_type = XCB_CLIENT_MESSAGE;
    	event.format = 32;
    	event.sequence = 0;
    	event.data.data32[0] = 0;
    	event.window = window;
    	event.type = XCB_SET_CLOSE_DOWN_MODE;

    	xcb_create_window(conn, XCB_COPY_FROM_PARENT,
				  window, screen->root,
				  0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
				  screen->root_visual, 0, nullptr);

    	xcb_send_event(conn, false, window, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char*>(&event));
    	xcb_destroy_window(conn, window);
    	xcb_flush(conn);
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
        hidMouse->buttons().inputs[3].setValue(mouseState.back);
        hidMouse->buttons().inputs[4].setValue(mouseState.forward);

        keyMutex.unlock();

        devices.push_back(hidKeyboard);
        devices.push_back(hidMouse);

	#ifdef DRV_UHID
		if (joypadDriver == "uhid") uhid->pollJoypad(devices);
	#endif
	#ifdef DRV_UDEV
		if (joypadDriver == "udev") udev->pollJoypad(devices);
	#endif

		return devices;
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
            xcb_warp_pointer(conn, None, screen->root, 0, 0, 0, 0, relativex, absY);

        } else if( absX < (x + warpMargin)) {
            // left to right
            relativex = x + w - warpMargin;
            xcb_warp_pointer(conn, None, screen->root, 0, 0, 0, 0, relativex, absY);
        }

        if (absY > ((y + h) - warpMargin)) {
            // bottom to top
            relativey = y + warpMargin;
            xcb_warp_pointer(conn, None, screen->root, 0, 0, 0, 0, absX, relativey);

        } else if( absY < (y + warpMargin)) {
            // top to bottom
            relativey = y + h - warpMargin;
            xcb_warp_pointer(conn, None, screen->root, 0, 0, 0, 0, absX, relativey);
        }
    }

    auto initWorker() -> void {

        std::thread worker([this] {
        	kill = false;
        	//setvbuf (stdout, NULL, _IONBF, 0);

			xcb_generic_event_t* event;
			while ((event = xcb_wait_for_event(conn))) {
				if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {

					auto clientMessage = reinterpret_cast<const xcb_client_message_event_t *>(event);
					if (clientMessage->type == XCB_SET_CLOSE_DOWN_MODE) {
						// printf("kill");
						free(event);
						break;
					}
				}

				xcb_ge_event_t* e = (xcb_ge_event_t*)event;

				switch (e->event_type) {
					case XCB_INPUT_RAW_KEY_PRESS: {
						auto ev = (xcb_input_raw_key_press_event_t*)event;
						keyMutex.lock();
						keyState[ev->detail >> 3] |= 1 << (ev->detail & 7);
						keyMutex.unlock();
						if (keyCallback)
							(*keyCallback)();
					}	break;
					case XCB_INPUT_RAW_KEY_RELEASE: {
						auto ev = (xcb_input_raw_key_release_event_t*)event;
						keyMutex.lock();
						keyState[ev->detail >> 3] &= ~(1 << (ev->detail & 7));
						keyMutex.unlock();
						if (keyCallback)
							(*keyCallback)();
					} break;
					case XCB_INPUT_RAW_BUTTON_PRESS: {
						auto ev = (xcb_input_raw_button_press_event_t*)event;
						keyMutex.lock();
						if (ev->detail == XCB_BUTTON_INDEX_1) mouseState.left = true;
						else if (ev->detail == XCB_BUTTON_INDEX_2) mouseState.middle = true;
						else if (ev->detail == XCB_BUTTON_INDEX_3) mouseState.right = true;
						else if (ev->detail == 8) mouseState.back = true;
						else if (ev->detail == 9) mouseState.forward = true;
						keyMutex.unlock();
					} break;
					case XCB_INPUT_RAW_BUTTON_RELEASE: {
						auto ev = (xcb_input_raw_button_release_event_t*)event;
						keyMutex.lock();
						if (ev->detail == XCB_BUTTON_INDEX_1) mouseState.left = false;
						else if (ev->detail == XCB_BUTTON_INDEX_2) mouseState.middle = false;
						else if (ev->detail == XCB_BUTTON_INDEX_3) mouseState.right = false;
						else if (ev->detail == 8) mouseState.back = false;
						else if (ev->detail == 9) mouseState.forward = false;
						keyMutex.unlock();
					} break;
					case XCB_INPUT_RAW_MOTION: {
						auto ev = (xcb_input_raw_motion_event_t*)event;
						int axis_len = xcb_input_raw_button_press_axisvalues_length(ev);
						int16_t rx = 0;
						int16_t ry = 0;

						if (axis_len) {
							xcb_input_fp3232_t* axisvalues = xcb_input_raw_button_press_axisvalues_raw(ev);
							for (int i = 0; i < axis_len; ++i) {
								xcb_input_fp3232_t value = axisvalues[i];

								if (i == 0) {
									rx = (int16_t)value.integral;
								} else if (i == 1) {
									ry = (int16_t)value.integral;
								}
							}
						}

						keyMutex.lock();
						mouseState.x += rx;
						mouseState.y += ry;
						keyMutex.unlock();
                        if (mIsAcquired()) {
                        	xcb_query_pointer_reply_t* reply = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, screen->root), 0);
	                        warpMouse( reply->root_x, reply->root_y );
                        }
					} break;

				}

			 	free(event);
			}

        	kill = true;
        });

        worker.detach();
    }

    auto setKeyboardCallback( KeyCallback* callback ) -> void {
        this->keyCallback = callback;
    }

    auto mAcquire() -> void {
		if (mIsAcquired() /*|| ((gdk_window_get_state((GdkWindow*)handle) & GDK_WINDOW_STATE_FOCUSED) == 0)*/ )
			return;

    	auto cookie = xcb_grab_pointer(conn, 1, GDK_WINDOW_XID( (GdkWindow*)handle ),
						  XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_LEAVE_WINDOW,
						  XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
						  XCB_WINDOW_NONE, XCB_CURSOR_NONE, XCB_CURRENT_TIME);

    	auto reply = xcb_grab_pointer_reply(conn, cookie, nullptr);

    	if (!reply || reply->status != XCB_GRAB_STATUS_SUCCESS) {
    		mouseAcquired = false;
    	} else {
    		xcb_query_pointer_cookie_t c = xcb_query_pointer(conn, screen->root);
    		xcb_query_pointer_reply_t* r = xcb_query_pointer_reply(conn, c, nullptr);
    		rememberX = r->root_x;
    		rememberY = r->root_y;

    		xcb_xfixes_query_version(conn, 4, 0);
    		xcb_xfixes_hide_cursor(conn, screen->root);

    		mouseAcquired = true;
    		xcb_flush(conn);
    	}
	}
    auto mUnacquire() -> void {

		if (mIsAcquired()) {
			if (rememberX && rememberY)
				xcb_warp_pointer(conn, XCB_NONE, screen->root, 0, 0, 0, 0, rememberX, rememberY);

			xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
			xcb_xfixes_show_cursor(conn, screen->root);
			xcb_flush(conn);

            mouseAcquired = false;
		}
	}

    auto mIsAcquired() -> bool {
		return mouseAcquired;
    }

	XCBInput(std::string joypadDriver = "") {
		this->joypadDriver = joypadDriver;

		#ifdef DRV_UHID
			if (this->joypadDriver == "uhid") uhid = new Uhid();
		#endif
		#ifdef DRV_UDEV
			if (this->joypadDriver == "udev") udev = new Udev();
		#endif

		handle = 0;
	}
	~XCBInput() {
    	sendCloseEvent();
    	while (!kill)
    		std::this_thread::yield();

    	term();

		#ifdef DRV_UHID
			if (joypadDriver == "uhid") delete uhid;
		#endif
		#ifdef DRV_UDEV
			if (joypadDriver == "udev") delete udev;
		#endif
	}
};

}
