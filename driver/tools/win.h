
//following is for windows OS only
#pragma once

#ifdef _MSC_VER
#pragma warning(disable:4996)
#define PATH_MAX MAX_PATH
#endif

#include <string>
#define UNICODE
#include <windows.h>
#include "hid.h"

#define dxRelease(__m) if(__m) __m->Release(), __m = 0;

struct Win {

    typedef LONG NTSTATUS, *PNTSTATUS;
    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    static auto getVersion() -> unsigned {
        static unsigned version = 0;

        if (version)
            return version;

        HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
        if (hMod) {
            RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
            if (fxPtr != nullptr) {
                RTL_OSVERSIONINFOW rovi = {0};
                rovi.dwOSVersionInfoSize = sizeof (rovi);
                if (0x00000000 == fxPtr(&rovi)) {
                    FreeLibrary(hMod);
                    version = (rovi.dwMajorVersion << 8) | rovi.dwMinorVersion;
                    if ((version >= 0xa00) && (rovi.dwBuildNumber >= 22000))
                        version += 1; // Win 11
                    return version;
                }
            }
        }
        
        if (hMod)
            FreeLibrary(hMod);
        
        OSVERSIONINFO versionInfo{0};
        versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        GetVersionEx(&versionInfo);

        version = (versionInfo.dwMajorVersion << 8) | versionInfo.dwMinorVersion;
        return version;
    }
    
    //convert wide char to utf8
    static auto utf8_t(const wchar_t* s) -> std::string {
        std::string out = "";

        if (!s) s = L"";

        char* buffer = nullptr;

        unsigned length = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);

        buffer = new char[length + 1]();

        WideCharToMultiByte(CP_UTF8, 0, s, -1, buffer, length, nullptr, nullptr);

        if (buffer) {
            out = (std::string)buffer;

            delete[] buffer;
        }

        return out;
    }

    //convert utf8 to wide char
    static auto utf16_t(const std::string& str) -> wchar_t* {

        unsigned length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

        wchar_t* buffer = new wchar_t[length + 1]();

        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, length);

        return buffer;
    }

    // get human readable name of keyboard input
    // considers keyboard layout (e.g. german ü, ä, ö, ß)
    static auto translateKeyName(unsigned element, bool directX = true) -> std::string {
        if (element & 0x80) element += 0x80;

        if (directX) {
            if (element == 0x45) element = 0x145; //Pause <> NumPad
            else if (element == 0x145) element = 0x45;
        }

        wchar_t keyText[100];
        GetKeyNameText(element << 16, keyText, 100);

        std::string result = utf8_t(keyText);	

        if (result.find("ZEHNERTASTATUR") != std::string::npos) {
            /* bug in german keyboard layouts */
            if (element == 0x135)	return "/ (zehnertastatur)";
            if (element == 0x37)	return "* (zehnertastatur)";
            //other numpad buttons working as expected
        }

        return result;
    }

    static auto getKeyCode(std::string ident, uint8_t keycode ) -> Hid::Key {
        // keycodes are device indepandant and describe the position on keyboard.        
        // for easier use we assign the keycodes to human readable enums of the uk layout.        
        auto key = Hid::Input::getKeyCode( ident );

        if (key != Hid::Key::Unknown)
            return key;

        if (keycode == 28) return Hid::Key::Return;
        if (keycode == 57) return Hid::Key::Space;
        if (keycode == 14) return Hid::Key::Backspace;
        if (keycode == 15) return Hid::Key::Tab;
        if (keycode == 1) return Hid::Key::Esc;
        if (keycode == 58) return Hid::Key::CapsLock;

        if (keycode == 41) return Hid::Key::Grave;
        if (keycode == 12) return Hid::Key::Minus;
        if (keycode == 13) return Hid::Key::Equal;
        if (keycode == 27) return Hid::Key::ClosedSquareBracket;        
        if (keycode == 43) return Hid::Key::NumberSign;
        if (keycode == 86) return Hid::Key::Backslash;
        if (keycode == 51) return Hid::Key::Comma;
        if (keycode == 52) return Hid::Key::Period;
        if (keycode == 53) return Hid::Key::Slash;
        if (keycode == 221) return Hid::Key::Menu;        
        if (keycode == 40) return Hid::Key::Apostrophe;
        if (keycode == 39) return Hid::Key::Semicolon;
        if (keycode == 26) return Hid::Key::OpenSquareBracket;

        if (keycode == 210) return Hid::Key::Insert;
        if (keycode == 199) return Hid::Key::Home;
        if (keycode == 201) return Hid::Key::Prior;
        if (keycode == 211) return Hid::Key::Delete;
        if (keycode == 207) return Hid::Key::End;
        if (keycode == 209) return Hid::Key::Next;
        if (keycode == 183) return Hid::Key::Print;
        if (keycode == 70) return Hid::Key::ScrollLock;
        if (keycode == 197) return Hid::Key::Pause;

        if (keycode == 208) return Hid::Key::CursorDown;
        if (keycode == 203) return Hid::Key::CursorLeft;
        if (keycode == 205) return Hid::Key::CursorRight;
        if (keycode == 200) return Hid::Key::CursorUp;

        if (keycode == 42) return Hid::Key::ShiftLeft;
        if (keycode == 54) return Hid::Key::ShiftRight;
        if (keycode == 56) return Hid::Key::AltLeft;
        if (keycode == 184) return Hid::Key::AltRight;
        if (keycode == 29) return Hid::Key::ControlLeft;
        if (keycode == 157) return Hid::Key::ControlRight;
        if (keycode == 219) return Hid::Key::SuperLeft;
        if (keycode == 220) return Hid::Key::SuperRight;

        if (keycode == 82) return Hid::Key::NumPad0;
        if (keycode == 79) return Hid::Key::NumPad1;
        if (keycode == 80) return Hid::Key::NumPad2;
        if (keycode == 81) return Hid::Key::NumPad3;
        if (keycode == 75) return Hid::Key::NumPad4;
        if (keycode == 76) return Hid::Key::NumPad5;
        if (keycode == 77) return Hid::Key::NumPad6;
        if (keycode == 71) return Hid::Key::NumPad7;
        if (keycode == 72) return Hid::Key::NumPad8;
        if (keycode == 73) return Hid::Key::NumPad9;
        if (keycode == 83) return Hid::Key::NumComma;
        if (keycode == 181) return Hid::Key::NumDivide;
        if (keycode == 55) return Hid::Key::NumMultiply;
        if (keycode == 74) return Hid::Key::NumSubtract;
        if (keycode == 78) return Hid::Key::NumAdd;
        if (keycode == 156) return Hid::Key::NumEnter;
        if (keycode == 69) return Hid::Key::NumLock;

        // on french keyboard M <> ;
        if (keycode == 50) return Hid::Key::Semicolon;
        // digits on french keyboards are secondary functions        
        if (keycode == 2) return Hid::Key::D1;
        if (keycode == 3) return Hid::Key::D2;
        if (keycode == 4) return Hid::Key::D3;
        if (keycode == 5) return Hid::Key::D4;
        if (keycode == 6) return Hid::Key::D5;
        if (keycode == 7) return Hid::Key::D6;
        if (keycode == 8) return Hid::Key::D7;
        if (keycode == 9) return Hid::Key::D8;
        if (keycode == 10) return Hid::Key::D9;
        if (keycode == 11) return Hid::Key::D0;
        
        return Hid::Key::Unknown;
    }

};
