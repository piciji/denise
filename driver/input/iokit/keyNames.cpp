
struct OsxKeyNames {
    /**
     thanks goes to Jens Ayton for the const lists
     */
    enum {
        VK_F1				= 122,
        VK_F2				= 120,
        VK_F3				= 99,
        VK_F4				= 118,
        VK_F5				= 96,
        VK_F6				= 97,
        VK_F7				= 98,
        VK_F8				= 100,
        VK_F9				= 101,
        VK_F10			= 109,
        VK_F11			= 103,
        VK_F12			= 111,
        VK_F13			= 105,
        VK_F14			= 107,
        VK_F15			= 113,
        VK_F16			= 106,
        VK_F17			= 64,
        VK_F18			= 79,
        VK_F19			= 80,
        VK_F20			= 90,

        VK_Menu			= 110,
        
        VK_Esc			= 53,
        VK_Clear			= 71,
        
        VK_Tab = 48,
        VK_Space			= 49,
        
        VK_CapsLock		= 57,
        VK_Shift			= 56,
        VK_Option			= 58,
        VK_Control		= 59,
        VK_rShift			= 60,
        VK_rOption		= 61,
        VK_rControl		= 62,
        VK_Command		= 55,
        VK_Return			= 36,
        VK_Backspace		= 51,
        VK_Delete			= 117,
        VK_Help			= 114,
        VK_Home			= 115,
        VK_PgUp			= 116,
        VK_PgDn			= 121,
        VK_End			= 119,
        VK_LArrow			= 123,
        VK_RArrow			= 124,
        VK_UArrow			= 126,
        VK_DArrow			= 125,
        VK_KpdEnter		= 76,
        VK_KbdEnter		= 52,		/*	Powerbooks (and some early Macs) */
        VK_Fn				= 63,

        
        VK_Kpd_0			= 81,
        VK_Kpd_1			= 75,
        VK_Kpd_2			= 67,
        VK_Kpd_3			= 89,
        VK_Kpd_4			= 91,
        VK_Kpd_5			= 92,
        VK_Kpd_6			= 78,
        VK_Kpd_7			= 86,
        VK_Kpd_8			= 87,
        VK_Kpd_9			= 88,
        VK_Kpd_A			= 69,
        VK_Kpd_B			= 83,
        VK_Kpd_C			= 84,
        VK_Kpd_D			= 85,
        VK_Kpd_E			= 82,
        VK_Kpd_F			= 65,
        
        /* 2.1b5: values from new list in Event.h in OS X 10.5 */
        VK_VolumeUp		= 72,
        VK_VolumeDown		= 73,
        VK_Mute			= 74,
        VK_rCommand = 0x6001,
        VK_Unknown = 0xffff,
    };

    
    struct Map {
        uint16_t virtualKey;
        CFStringRef translated;
    };
    
    std::vector<Map> map;
    std::vector<uint16_t> hids;
    UCKeyboardLayout* keyboardLayout = nullptr;
    
    OsxKeyNames() {
        
        hids.insert(hids.end(), {
            VK_Unknown,		/* Reserved (no event indicated) */
            VK_Unknown,		/* ErrorRollOver */
            VK_Unknown,		/* POSTFail */
            VK_Fn,		/* ErrorUndefined */
            0x00,				/* a and A */
            0x0B,				/* b and B */
            0x08,				/* ... */
            0x02,
            0x0E,
            0x03,
            0x05,
            0x04,
            0x22,
            0x26,
            0x28,
            0x25,
            0x2E,
            0x2D,
            0x1F,
            0x23,
            0x0C,
            0x0F,
            0x01,
            0x11,
            0x20,
            0x09,
            0x0D,
            0x07,
            0x10,
            0x06,				/* z and Z */
            0x12,				/* 1 */
            0x13,				/* 2 */
            0x14,				/* ... */
            0x15,
            0x17,
            0x16,
            0x1A,
            0x1C,
            0x19,				/* 9 */
            0x1D,				/* 0 */
            VK_Return,		/* Keyboard Return (ENTER) */
            VK_Esc,			/* Escape */
            VK_Backspace,		/* Delete (Backspace) */
            0x30,				/* Tab */
            VK_Space,			/* Space bar */
            0x1B,				/* - and _ */
            0x18,				/* = and + */
            0x21,				/* [ and { */
            0x1E,				/* ] and } */
            0x2A,				/* \ and | */
            0x2A,		/* "Non-US # and ~" ?? hash */
            0x29,				/* ; and : */
            0x27,				/* ' and " */
            0x32,				/* ` and ~ */
            0x2B,				/* , and < */
            0x2F,				/* . and > */
            0x2C,				/* / and ? */
            VK_CapsLock,		/* Caps Lock */
            VK_F1,			/* F1 */
            VK_F2,			/* ... */
            VK_F3,
            VK_F4,
            VK_F5,
            VK_F6,
            VK_F7,
            VK_F8,
            VK_F9,
            VK_F10,
            VK_F11,
            VK_F12,			/* F12 */
            VK_Unknown,		/* Print Screen */
            VK_Unknown,		/* Scroll Lock */
            VK_Unknown,		/* Pause */
            VK_Help,		/* Insert */
            VK_Home,			/* Home */
            VK_PgUp,			/* Page Up */
            VK_Delete,		/* Delete Forward */
            VK_End,			/* End */
            VK_PgDn,			/* Page Down */
            VK_RArrow,		/* Right Arrow */
            VK_LArrow,		/* Left Arrow */
            VK_DArrow,		/* Down Arrow */
            VK_UArrow,		/* Up Arrow */
            VK_Clear,			/* Keypad Num Lock and Clear */
            0x4B,				/* Keypad / */
            0x43,				/* Keypad * */
            0x4E,				/* Keypad - */
            0x45,				/* Keypad + */
            VK_KpdEnter,		/* Keypad ENTER */
            0x53,				/* Keypad 1 */
            0x54,				/* Keypad 2 */
            0x55,				/* Keypad 3 */
            0x56,				/* Keypad 4 */
            0x57,				/* Keypad 5 */
            0x58,				/* Keypad 6 */
            0x59,				/* Keypad 7 */
            0x5B,				/* Keypad 8 */
            0x5C,				/* Keypad 9 */
            0x52,				/* Keypad 0 */
            0x41,				/* Keypad . */
            0x0a,				/* "Keyboard Non-US \ and |" */
            VK_Unknown,		/* "Keyboard Application" (Windows key for Windows 95, and "Compose".) */
            VK_Unknown,
            0x51,				/* Keypad = */
            VK_F13,			/* F13 */
            VK_F14,			/* ... */
            VK_F15,
            VK_F16,
            VK_F17,
            VK_F18,
            VK_F19,
            VK_F20,
            VK_Unknown,
            VK_Unknown,
            VK_Unknown,
            VK_Unknown,			/* F24 */
            VK_Unknown,		/* Keyboard Execute */
            VK_Help,			/* Keyboard Help */
            VK_Unknown,		/* Keyboard Menu */
            VK_Unknown,		/* Keyboard Select */
            VK_Unknown,		/* Keyboard Stop */
            VK_Unknown,		/* Keyboard Again */
            VK_Unknown,		/* Keyboard Undo */
            VK_Unknown,		/* Keyboard Cut */
            VK_Unknown,		/* Keyboard Copy */
            VK_Unknown,		/* Keyboard Paste */
            VK_Unknown,		/* Keyboard Find */
            VK_Mute,			/* Keyboard Mute */
            VK_VolumeUp,		/* Keyboard Volume Up */
            VK_VolumeDown,	/* Keyboard Volume Down */
            VK_CapsLock,		/* Keyboard Locking Caps Lock */
            VK_Unknown,		/* Keyboard Locking Num Lock */
            VK_Unknown,		/* Keyboard Locking Scroll Lock */
            0x41,				/*	Keypad Comma ("Keypad Comma is the appropriate usage for the Brazilian
                                 keypad period (.) key. This represents the closest possible  match, and
                                 system software should do the correct mapping based on the current locale
                                 setting." If strange stuff happens on a (physical) Brazilian keyboard,
                                 I'd like to know about it. */
            0x51,				/* Keypad Equal Sign ("Used on AS/400 Keyboards.") */
            VK_Unknown,		/* Keyboard International1 (Brazilian / and ? key? Kanji?) */
            VK_Unknown,		/* Keyboard International2 (Kanji?) */
            VK_Unknown,		/* Keyboard International3 (Kanji?) */
            VK_Unknown,		/* Keyboard International4 (Kanji?) */
            VK_Unknown,		/* Keyboard International5 (Kanji?) */
            VK_Unknown,		/* Keyboard International6 (Kanji?) */
            VK_Unknown,		/* Keyboard International7 (Kanji?) */
            VK_Unknown,		/* Keyboard International8 (Kanji?) */
            VK_Unknown,		/* Keyboard International9 (Kanji?) */
            VK_Unknown,		/* Keyboard LANG1 (Hangul/English toggle) */
            VK_Unknown,		/* Keyboard LANG2 (Hanja conversion key) */
            VK_Unknown,		/* Keyboard LANG3 (Katakana key) */		// kVKC_Kana?
            VK_Unknown,		/* Keyboard LANG4 (Hirigana key) */
            VK_Unknown,		/* Keyboard LANG5 (Zenkaku/Hankaku key) */
            VK_Unknown,		/* Keyboard LANG6 */
            VK_Unknown,		/* Keyboard LANG7 */
            VK_Unknown,		/* Keyboard LANG8 */
            VK_Unknown,		/* Keyboard LANG9 */
            VK_Unknown,		/* Keyboard Alternate Erase ("Example, Erase-Eaze™ key.") */
            VK_Unknown,		/* Keyboard SysReq/Attention */
            VK_Unknown,		/* Keyboard Cancel */
            VK_Unknown,		/* Keyboard Clear */
            VK_Unknown,		/* Keyboard Prior */
            VK_Unknown,		/* Keyboard Return */
            VK_Unknown,		/* Keyboard Separator */
            VK_Unknown,		/* Keyboard Out */
            VK_Unknown,		/* Keyboard Oper */
            VK_Unknown,		/* Keyboard Clear/Again */
            VK_Unknown,		/* Keyboard CrSel/Props */
            VK_Unknown,		/* Keyboard ExSel */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Keypad 00 */
            VK_Unknown,		/* Keypad 000 */
            VK_Unknown,		/* Thousands Separator */
            VK_Unknown,		/* Decimal Separator */
            VK_Unknown,		/* Currency Unit */
            VK_Unknown,		/* Currency Sub-unit */
            VK_Unknown,		/* Keypad ( */
            VK_Unknown,		/* Keypad ) */
            VK_Unknown,		/* Keypad { */
            VK_Unknown,		/* Keypad } */
            VK_Unknown,		/* Keypad Tab */
            VK_Unknown,		/* Keypad Backspace */
            VK_Unknown,		/* Keypad A */
            VK_Unknown,		/* Keypad B */
            VK_Unknown,		/* Keypad C */
            VK_Unknown,		/* Keypad D */
            VK_Unknown,		/* Keypad E */
            VK_Unknown,		/* Keypad F */
            VK_Unknown,		/* Keypad XOR */
            VK_Unknown,		/* Keypad ^ */
            VK_Unknown,		/* Keypad % */
            VK_Unknown,		/* Keypad < */
            VK_Unknown,		/* Keypad > */
            VK_Unknown,		/* Keypad & */
            VK_Unknown,		/* Keypad && */
            VK_Unknown,		/* Keypad | */
            VK_Unknown,		/* Keypad || */
            VK_Unknown,		/* Keypad : */
            VK_Unknown,		/* Keypad # */
            VK_Unknown,		/* Keypad Space */
            VK_Unknown,		/* Keypad @ */
            VK_Unknown,		/* Keypad ! */
            VK_Unknown,		/* Keypad Memory Store */
            VK_Unknown,		/* Keypad Memory Recall */
            VK_Unknown,		/* Keypad Memory Clear */
            VK_Unknown,		/* Keypad Memory Add */
            VK_Unknown,		/* Keypad Memory Subtract */
            VK_Unknown,		/* Keypad Memory Multiply */
            VK_Unknown,		/* Keypad Memory Divide */
            VK_Unknown,		/* Keypad +/- */
            VK_Unknown,		/* Keypad Clear */
            VK_Unknown,		/* Keypad Clear Entry */
            VK_Unknown,		/* Keypad Binary */
            VK_Unknown,		/* Keypad Octal */
            VK_Unknown,		/* Keypad Decimal */
            VK_Unknown,		/* Keypad Hexadecimal */
            VK_Unknown,		/* Reserved */
            VK_Unknown,		/* Reserved */
            VK_Control,		/* Keyboard LeftControl */
            VK_Shift,			/* Keyboard LeftShift */
            VK_Option,		/* Keyboard LeftAlt */
            VK_Command,		/* Keyboard LeftGUI */
            VK_rControl,		/* Keyboard RightControl */
            VK_rShift,		/* Keyboard RightShift */
            VK_rOption,		/* Keyboard RightAlt */
            VK_rCommand,    /* Keyboard RightGUI */
        });
        
        map.push_back({VK_CapsLock, CFSTR("Caps Lock")});
        map.push_back({VK_Shift, CFSTR("Left Shift")});
        map.push_back({VK_Control, CFSTR("Left Control")});
        map.push_back({VK_Option, CFSTR("Left Option")});
        map.push_back({VK_rShift, CFSTR("Right Shift")});
        map.push_back({VK_rControl, CFSTR("Right Control")});
        map.push_back({VK_rOption, CFSTR("Right Option")});
        map.push_back({VK_Command, CFSTR("Left Command")});
        map.push_back({VK_rCommand, CFSTR("Right Command")});
        map.push_back({VK_Return, CFSTR("Return")});
        map.push_back({VK_Backspace, CFSTR("Backspace")});
        map.push_back({VK_Delete, CFSTR("Delete")});
        map.push_back({VK_Help, CFSTR("Help")});
        map.push_back({VK_Home, CFSTR("Home")});
        map.push_back({VK_PgUp, CFSTR("Page Up")});
        map.push_back({VK_PgDn, CFSTR("Page Down")});
        map.push_back({VK_End, CFSTR("End")});
        map.push_back({VK_LArrow, CFSTR("Left Arrow")});
        map.push_back({VK_RArrow, CFSTR("Right Arrow")});
        map.push_back({VK_UArrow, CFSTR("Up Arrow")});
        map.push_back({VK_DArrow, CFSTR("Down Arrow")});
        map.push_back({VK_Fn, CFSTR("Fn")});
        map.push_back({VK_KpdEnter, CFSTR("Enter")});
        map.push_back({VK_Space, CFSTR("Space")});
        map.push_back({VK_Mute, CFSTR("Mute")});
        map.push_back({VK_VolumeDown, CFSTR("Volume Down")});
        map.push_back({VK_VolumeUp, CFSTR("Volume Up")});
        map.push_back({VK_F1, CFSTR("F1")});
        map.push_back({VK_F2, CFSTR("F2")});
        map.push_back({VK_F3, CFSTR("F3")});
        map.push_back({VK_F4, CFSTR("F4")});
        map.push_back({VK_F5, CFSTR("F5")});
        map.push_back({VK_F6, CFSTR("F6")});
        map.push_back({VK_F7, CFSTR("F7")});
        map.push_back({VK_F8, CFSTR("F8")});
        map.push_back({VK_F9, CFSTR("F9")});
        map.push_back({VK_F10, CFSTR("F10")});
        map.push_back({VK_F11, CFSTR("F11")});
        map.push_back({VK_F12, CFSTR("F12")});
        map.push_back({VK_F13, CFSTR("F13")});
        map.push_back({VK_F14, CFSTR("F14")});
        map.push_back({VK_F15, CFSTR("F15")});
        map.push_back({VK_F16, CFSTR("F16")});
        map.push_back({VK_F17, CFSTR("F17")});
        map.push_back({VK_F18, CFSTR("F18")});
        map.push_back({VK_F19, CFSTR("F19")});
        map.push_back({VK_F20, CFSTR("F20")});
        map.push_back({VK_Tab, CFSTR("Tab")});
        map.push_back({VK_Esc, CFSTR("Esc")});
        map.push_back({VK_Clear, CFSTR("Clear")});

        setLayout();
    }
    
    auto setLayout() -> void {
        TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
        CFDataRef uchr = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
        
        if (uchr)
            keyboardLayout = (UCKeyboardLayout*)CFDataGetBytePtr(uchr);
        
        //auto ident = TISGetInputSourceProperty(currentKeyboard, kTISPropertyInputSourceID);
    }
    
    auto findModifier(unsigned vk, std::string& translated) -> bool {
        for(auto& item : map) {
            if(item.virtualKey == vk) {
                NSString* out = (NSString*)item.translated;
                translated = [out UTF8String];
                return true;
            }
        }
        return false;
    }
    
    auto translate(uint8_t keycode) -> std::string {
        std::string translated = "";
        
        if(keycode >= hids.size()) {
            return "unknown";
        }
        
        auto vk = hids[keycode];
        
        if(findModifier(vk, translated)) {
            return translated;
        }
        
        switch(vk) {
            case VK_Kpd_0: case VK_Kpd_1: case VK_Kpd_2:
            case VK_Kpd_3: case VK_Kpd_4: case VK_Kpd_5:
            case VK_Kpd_6: case VK_Kpd_7: case VK_Kpd_8:
            case VK_Kpd_9: case VK_Kpd_A: case VK_Kpd_B:
            case VK_Kpd_C: case VK_Kpd_D: case VK_Kpd_E:
            case VK_Kpd_F:
                translated = "Keypad ";
                break;
        }
        
        if(keyboardLayout) {
            UInt32 deadKeyState = 0;
            constexpr UniCharCount maxStringLength = 8;
            UniCharCount actualStringLength = 0;
            UniChar unicodeString[maxStringLength];
            
            OSStatus status = UCKeyTranslate(keyboardLayout,
                                             vk, kUCKeyActionDown, 0,
                                             LMGetKbdType(), 0,
                                             &deadKeyState,
                                             maxStringLength,
                                             &actualStringLength, unicodeString);
            
            
            if (actualStringLength == 0 && deadKeyState) {
                status = UCKeyTranslate(keyboardLayout,
                                        kVK_Space, kUCKeyActionDown, 0,
                                        LMGetKbdType(), 0,
                                        &deadKeyState,
                                        maxStringLength,
                                        &actualStringLength, unicodeString);
            }
            
            if(actualStringLength > 0 && status == noErr) {
                auto out = [[NSString stringWithCharacters:unicodeString length:(NSUInteger)actualStringLength] uppercaseString];
                             
                translated += [out UTF8String];
            }
        }
        
        return translated.empty() ? "unknown" : translated;
    }
};
