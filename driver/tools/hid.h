
/* structure for host input
 */

#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>

namespace Hid {		

    // for keyboards we use keycodes instead of scancodes, because keycodes are independant
    // from keyboard layouts. i.e. the scancodes for special chars like ä, ö, ü, ß will be
    // sent for some european keyboard laytouts but not for uk or us ofcourse.
    // the keycodes describe the position of a key, so you have to know how the button is labeled.
    
    // each driver can use different keycodes, as a result we map the keycodes to a driver independant
    // enum value
    enum class Key : unsigned { Unknown,
        // following keys should be the same for all layouts unsing latin chars
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
        NumPad0, NumPad1, NumPad2, NumPad3, NumPad4, NumPad5, NumPad6, NumPad7, NumPad8, NumPad9,
        NumLock/* clear (osx)*/, NumDivide, NumMultiply, NumSubtract, NumAdd, NumEnter, NumComma, NumEqual, /* osx only */
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18, F19, F20, /* osx */
        CursorRight, CursorLeft, CursorUp, CursorDown,
        Tab, Return, Space, Esc, Backspace, CapsLock, AltLeft, AltRight, ShiftLeft, ShiftRight, 
        ControlLeft, ControlRight, /* aka strg left/right */
        SuperLeft, SuperRight, /* aka windows left/right */
        Insert, Home, Prior, Delete, End, Next, Print /* system request */, ScrollLock, Pause,   
        
        // following keys can differ between keyboard layouts.
        // we use uk layout as a base for human readable names, us layout has one button less.
        // following comments describe german layout replacements.
        Comma, Period, Slash /* Minus */, Equal /* grave */, Minus /* ß */, Apostrophe /* Ä */, Grave /* circumflex */,
        Semicolon /* Ö */, Menu, OpenSquareBracket /* Ü */, ClosedSquareBracket /* Plus */, Backslash /* Less */, NumberSign, Fn, /* osx only */
           
        // of course there are a lot of secondary key functions (shift/alt) which differ between the layouts.
        // these differences will be handled in another context.
        // from a driver point of view only keys matters, not key combinations
    };
    
	struct Input {
		Input(unsigned id, std::string& name, Key key = Key::Unknown) : id(id), name(name), key(key) {
            value = oldValue = 0;
        }
        
		auto setValue(int16_t newValue) -> void {
			oldValue = value;
			value = newValue;
		}
		
        unsigned id;
		std::string name;		
        Key key; // unique key
		int16_t value = 0;
		int16_t oldValue = 0;
        
        static auto getKeyCode( std::string ident ) -> Key {
            
            std::transform(ident.begin(), ident.end(), ident.begin(), ::toupper);
            
            if (ident == "0") return Key::D0; if (ident == "1") return Key::D1;
            if (ident == "2") return Key::D2; if (ident == "3") return Key::D3;
            if (ident == "4") return Key::D4; if (ident == "5") return Key::D5;
            if (ident == "6") return Key::D6; if (ident == "7") return Key::D7;
            if (ident == "8") return Key::D8; if (ident == "9") return Key::D9;
            
            if (ident == "A") return Key::A; if (ident == "B") return Key::B;
            if (ident == "C") return Key::C; if (ident == "D") return Key::D;
            if (ident == "E") return Key::E; if (ident == "F") return Key::F;
            if (ident == "G") return Key::G; if (ident == "H") return Key::H;
            if (ident == "I") return Key::I; if (ident == "J") return Key::J;
            if (ident == "K") return Key::K; if (ident == "L") return Key::L;
            if (ident == "M") return Key::M; if (ident == "N") return Key::N;
            if (ident == "O") return Key::O; if (ident == "P") return Key::P;
            if (ident == "Q") return Key::Q; if (ident == "R") return Key::R;
            if (ident == "S") return Key::S; if (ident == "T") return Key::T;
            if (ident == "U") return Key::U; if (ident == "V") return Key::V;
            if (ident == "W") return Key::W; if (ident == "X") return Key::X;
            if (ident == "Y") return Key::Y; if (ident == "Z") return Key::Z;
            
            if (ident == "F1") return Key::F1; if (ident == "F2") return Key::F2;
            if (ident == "F3") return Key::F3; if (ident == "F4") return Key::F4;
            if (ident == "F5") return Key::F5; if (ident == "F6") return Key::F6;
            if (ident == "F7") return Key::F7; if (ident == "F8") return Key::F8;
            if (ident == "F9") return Key::F9; if (ident == "F10") return Key::F10;
            if (ident == "F11") return Key::F11; if (ident == "F12") return Key::F12;
            if (ident == "F13") return Key::F13; if (ident == "F14") return Key::F14;
            if (ident == "F15") return Key::F15; if (ident == "F16") return Key::F16;
            if (ident == "F17") return Key::F17; if (ident == "F18") return Key::F18;
            if (ident == "F19") return Key::F19; if (ident == "F20") return Key::F20;
            
            return Key::Unknown;
        }
	};	
	
	struct Group {
		Group(std::string& name, unsigned id) : name(name), id(id) { }
        
		auto append( std::string name, Key key = Key::Unknown ) -> void {
            inputs.emplace_back( Input{ (unsigned)inputs.size(), name, key } );
        }		
        unsigned id;
		std::string name;
		std::vector<Input> inputs;		
	};
	
	struct Device {
		Device(std::string name) : name(name) {}		
		auto append(std::string name, unsigned id) -> void { groups.emplace_back( Group{name, id} ); }
		auto group(unsigned id) -> Group& { return groups[id]; }
		
		virtual auto isKeyboard() const -> bool { return false; }
		virtual auto isMouse() const -> bool { return false; }
		virtual auto isJoypad() const -> bool { return false; }
		
		unsigned id;
		std::string name;
		std::vector<Group> groups;		
	};
	
	struct Keyboard : Device {
		enum GroupID : unsigned { Button };
		Keyboard() : Device("Keyboard")  { append("Button", GroupID::Button); }
		
		auto isKeyboard() const -> bool { return true; }
		auto buttons() -> Group& { return group(GroupID::Button); }
	};
	
	struct Mouse : Device {
		enum GroupID : unsigned { Axis, Button };
        Mouse() : Device("Mouse") { append("Axis", GroupID::Axis); append("Button", GroupID::Button); }
		
		auto isMouse() const -> bool { return true; }
		auto axes() -> Group& { return group(GroupID::Axis); }
		auto buttons() -> Group& { return group(GroupID::Button); }
	};
	
	struct Joypad : Device {
		enum GroupID : unsigned { Axis, Hat, Trigger, Button };
		Joypad() : Device("Joypad") { 
            append("Axis", GroupID::Axis); append("Hat", GroupID::Hat);
            append("Trigger", GroupID::Trigger); append("Button", GroupID::Button); }
		
		auto isJoypad() const -> bool { return true; }
		auto axes() -> Group& { return group(GroupID::Axis); }
		auto hats() -> Group& { return group(GroupID::Hat); }
		auto triggers() -> Group& { return group(GroupID::Trigger); }
		auto buttons() -> Group& { return group(GroupID::Button); }
	};
};
