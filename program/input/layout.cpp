
#include "manager.h"

typedef Emulator::Interface::Key EmuKey;

auto InputManager::automap( KeyboardLayout::Type type, Emulator::Interface::Key key, Emulator::Interface* emulator ) -> std::vector<std::vector<Hid::Key>> {
    
    // Note: the Hid key identifier correspond to the uk layout (reference layout) .
    // for other layouts you have to translate the visible key to the uk key at a given position
    
    if(GUIKIT::Application::isCocoa()) {
        // mac keyboard differs slightly from a pc keyboard.
        
        switch(type) {
            case KeyboardLayout::Type::Fr:
                if ( key == EmuKey::NumberSign ) return {{Hid::Key::Grave}};
                if ( key == EmuKey::At ) return {{Hid::Key::Grave, Hid::Key::ShiftRight}, {Hid::Key::Grave, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::Minus ) return {{Hid::Key::Equal}};
                if ( key == EmuKey::ExclamationMark ) return {{Hid::Key::D8}};
                if ( key == EmuKey::Plus ) return {{Hid::Key::Slash, Hid::Key::ShiftRight}, {Hid::Key::Slash, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::Equal ) return {{Hid::Key::Slash}};
                if ( key == EmuKey::Asterisk ) return {{Hid::Key::ClosedSquareBracket, Hid::Key::ShiftRight}, {Hid::Key::ClosedSquareBracket, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::Pound ) return {{Hid::Key::NumberSign, Hid::Key::ShiftRight}, {Hid::Key::NumberSign, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::Acute ) return {{Hid::Key::NumberSign}};

                if ( key == EmuKey::AltAndShift2 ) return {{Hid::Key::Grave}};
                if ( key == EmuKey::AltAndShift3 ) return {{Hid::Key::ShiftRight, Hid::Key::Grave}};
                if ( key == EmuKey::Shared13 ) return {{Hid::Key::Equal}};
                if ( key == EmuKey::D8 ) return {{Hid::Key::D8}};
                if ( key == EmuKey::ShiftShared6 ) return {{Hid::Key::Slash, Hid::Key::ShiftRight}};
                if ( key == EmuKey::Shared6 ) return {{Hid::Key::Slash}};
                if ( key == EmuKey::ShiftShared2 ) return {{Hid::Key::ClosedSquareBracket, Hid::Key::ShiftRight}};
                if ( key == EmuKey::ShiftShared9 ) return {{Hid::Key::NumberSign, Hid::Key::ShiftRight}};
                if ( key == EmuKey::Shared11 ) return {{Hid::Key::NumberSign}};
                if ( key == EmuKey::ShiftShared13 ) return {{Hid::Key::Equal, Hid::Key::ShiftRight }};
                if ( key == EmuKey::Shared1 ) return {{Hid::Key::OpenSquareBracket}};
                if ( key == EmuKey::ShiftShared7 ) return {{}}; // no pipe
                if ( key == EmuKey::D6 ) return {{Hid::Key::D6}};       
                break;
            case KeyboardLayout::Type::De:                
                if ( key == EmuKey::At ) return {{Hid::Key::L, Hid::Key::AltRight}}; // @
                if ( key == EmuKey::AltAnd2 ) return {{Hid::Key::L, Hid::Key::AltRight}}; // @                
                if ( key == EmuKey::ShiftShared7 ) return {{Hid::Key::D7, Hid::Key::AltRight}}; // |
                if ( key == EmuKey::Pipe ) return {{Hid::Key::D7, Hid::Key::AltRight}}; // |

                if ( key == EmuKey::ShiftNumLock ) return {{Hid::Key::D8, Hid::Key::AltRight}};
                if ( key == EmuKey::ShiftScrollLock ) return {{Hid::Key::D9, Hid::Key::AltRight}};
                break;
            case KeyboardLayout::Type::Us:
                // i would say there are no differences
                break;
            case KeyboardLayout::Type::Uk:
                if ( key == EmuKey::Acute ) return {{Hid::Key::Backslash}};
                if ( key == EmuKey::NumberSign ) return {{Hid::Key::D3, Hid::Key::AltRight}};
                if ( key == EmuKey::At ) return {{Hid::Key::D2, Hid::Key::ShiftRight}, {Hid::Key::D2, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::DoubleQuotes ) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}, {Hid::Key::Apostrophe, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::Pipe ) return {{Hid::Key::NumberSign, Hid::Key::ShiftRight}, {Hid::Key::NumberSign, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::ShiftShared8 ) return {{Hid::Key::D2, Hid::Key::ShiftRight}};
                if ( key == EmuKey::Shared8 ) return {{Hid::Key::D3, Hid::Key::AltRight}};
                if ( key == EmuKey::Shared7 ) return {{Hid::Key::NumberSign}};
                if ( key == EmuKey::ShiftAnd2 ) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}};
                if ( key == EmuKey::ShiftShared7 ) return {{Hid::Key::NumberSign, Hid::Key::ShiftRight}};
                if ( key == EmuKey::ShiftShared11 ) return {{Hid::Key::Backslash, Hid::Key::ShiftRight}};
                break;
        }
    }

    if ((type == KeyboardLayout::Type::De) && dynamic_cast<LIBAMI::Interface*>(emulator)) {
        if (key == EmuKey::Y) return {{Hid::Key::Z}};
        if (key == EmuKey::Z) return {{Hid::Key::Y}};
    } else if ((type == KeyboardLayout::Type::Fr) && dynamic_cast<LIBAMI::Interface*>(emulator)) {
        if (key == EmuKey::A) return {{Hid::Key::Q}};
        if (key == EmuKey::Q) return {{Hid::Key::A}};
        if (key == EmuKey::W) return {{Hid::Key::Z}};
        if (key == EmuKey::Z) return {{Hid::Key::W}};
        if (key == EmuKey::M) return {{Hid::Key::Semicolon}};
        if (key == EmuKey::Shared3) return {{Hid::Key::M}};
    }

    if ( key == EmuKey::A ) return {{Hid::Key::A}};
    if ( key == EmuKey::B ) return {{Hid::Key::B}};
    if ( key == EmuKey::C ) return {{Hid::Key::C}};
    if ( key == EmuKey::D ) return {{Hid::Key::D}};
    if ( key == EmuKey::E ) return {{Hid::Key::E}};
    if ( key == EmuKey::F ) return {{Hid::Key::F}};
    if ( key == EmuKey::G ) return {{Hid::Key::G}};
    if ( key == EmuKey::H ) return {{Hid::Key::H}};
    if ( key == EmuKey::I ) return {{Hid::Key::I}};
    if ( key == EmuKey::J ) return {{Hid::Key::J}};
    if ( key == EmuKey::K ) return {{Hid::Key::K}};
    if ( key == EmuKey::L ) return {{Hid::Key::L}};
    if ( key == EmuKey::M ) return {{Hid::Key::M}};
    if ( key == EmuKey::N ) return {{Hid::Key::N}};
    if ( key == EmuKey::O ) return {{Hid::Key::O}};
    if ( key == EmuKey::P ) return {{Hid::Key::P}};
    if ( key == EmuKey::Q ) return {{Hid::Key::Q}};
    if ( key == EmuKey::R ) return {{Hid::Key::R}};
    if ( key == EmuKey::S ) return {{Hid::Key::S}};
    if ( key == EmuKey::T ) return {{Hid::Key::T}};
    if ( key == EmuKey::U ) return {{Hid::Key::U}};
    if ( key == EmuKey::V ) return {{Hid::Key::V}};
    if ( key == EmuKey::W ) return {{Hid::Key::W}};
    if ( key == EmuKey::X ) return {{Hid::Key::X}};
    if ( key == EmuKey::Y ) return {{Hid::Key::Y}};
    if ( key == EmuKey::Z ) return {{Hid::Key::Z}};

    if ( key == EmuKey::F1 ) return {{Hid::Key::F1}};
    if ( key == EmuKey::F2 ) return {{Hid::Key::F2}};
    if ( key == EmuKey::F3 ) return {{Hid::Key::F3}};
    if ( key == EmuKey::F4 ) return {{Hid::Key::F4}};
    if ( key == EmuKey::F5 ) return {{Hid::Key::F5}};
    if ( key == EmuKey::F6 ) return {{Hid::Key::F6}};
    if ( key == EmuKey::F7 ) return {{Hid::Key::F7}};
    if ( key == EmuKey::F8 ) return {{Hid::Key::F8}};
    if ( key == EmuKey::F9 ) return {{Hid::Key::F9}};
    if ( key == EmuKey::F10 ) return {{Hid::Key::F10}};
    
    if ( key == EmuKey::CursorRight ) return {{Hid::Key::CursorRight}};
    if ( key == EmuKey::CursorDown ) return {{Hid::Key::CursorDown}};
    if ( key == EmuKey::CursorLeft ) return {{Hid::Key::CursorLeft}};
    if ( key == EmuKey::CursorUp ) return {{Hid::Key::CursorUp}};

    if ( key == EmuKey::Backspace ) return {{Hid::Key::Backspace}};    
    if ( key == EmuKey::Return ) return {{Hid::Key::Return}};
    if ( key == EmuKey::ShiftLeft ) return {{Hid::Key::ShiftLeft}};
    if ( key == EmuKey::ShiftRight ) return {{Hid::Key::ShiftRight}};
           
    if ( key == EmuKey::Space ) return {{Hid::Key::Space}};
    if ( key == EmuKey::ArrowLeft ) return {{Hid::Key::End}};
    if ( key == EmuKey::Home ) return {{Hid::Key::Home}};
    if ( key == EmuKey::RunStop ) return {{Hid::Key::Esc}};
    if ( key == EmuKey::ArrowUp ) return {{Hid::Key::Grave}};
    if ( key == EmuKey::ControlLeft ) return {{Hid::Key::ControlLeft}};
    if ( key == EmuKey::Commodore ) return {{Hid::Key::ControlLeft}};
    if ( key == EmuKey::Ctrl ) return {{Hid::Key::Tab}};
    if ( key == EmuKey::ShiftLock ) return {{Hid::Key::CapsLock}};
    if ( key == EmuKey::Restore ) return {{Hid::Key::Prior}};
    if ( key == EmuKey::Del ) return {{Hid::Key::Delete}};
    if ( key == EmuKey::Help ) return {{Hid::Key::Home}};
    if ( key == EmuKey::Esc ) return {{Hid::Key::Esc}};
    if ( key == EmuKey::Tab ) return {{Hid::Key::Tab}};
    if ( key == EmuKey::AltLeft ) return {{Hid::Key::AltLeft}};
    if ( key == EmuKey::AltRight ) return {{Hid::Key::AltRight}};
    if ( key == EmuKey::SystemLeft ) return {{Hid::Key::SuperLeft}};
    if ( key == EmuKey::SystemRight ) return {{ !GUIKIT::Application::isCocoa() ? Hid::Key::Menu : Hid::Key::SuperRight}};
    
    if ( key == EmuKey::NumPad0 ) return {{Hid::Key::NumPad0}};
    if ( key == EmuKey::NumPad1 ) return {{Hid::Key::NumPad1}};
    if ( key == EmuKey::NumPad2 ) return {{Hid::Key::NumPad2}};
    if ( key == EmuKey::NumPad3 ) return {{Hid::Key::NumPad3}};
    if ( key == EmuKey::NumPad4 ) return {{Hid::Key::NumPad4}};
    if ( key == EmuKey::NumPad5 ) return {{Hid::Key::NumPad5}};
    if ( key == EmuKey::NumPad6 ) return {{Hid::Key::NumPad6}};
    if ( key == EmuKey::NumPad7 ) return {{Hid::Key::NumPad7}};
    if ( key == EmuKey::NumPad8 ) return {{Hid::Key::NumPad8}};
    if ( key == EmuKey::NumPad9 ) return {{Hid::Key::NumPad9}};
    if ( key == EmuKey::NumDivide ) return {{Hid::Key::NumDivide}};
    if ( key == EmuKey::NumMultiply ) return {{Hid::Key::NumMultiply}};
    if ( key == EmuKey::NumSubtract ) return {{Hid::Key::NumSubtract}};
    if ( key == EmuKey::NumAdd ) return {{Hid::Key::NumAdd}};
    if ( key == EmuKey::NumEnter ) return {{Hid::Key::NumEnter}};
    if ( key == EmuKey::NumComma ) return {{Hid::Key::NumComma}};                                                            
    
    switch(type) {
        case KeyboardLayout::Type::Fr:
            if (dynamic_cast<LIBC64::Interface*>(emulator)) {
                if ( key == EmuKey::D0 ) return {{Hid::Key::D0, Hid::Key::ShiftRight}, {Hid::Key::D0, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D1 ) return {{Hid::Key::D1, Hid::Key::ShiftRight}, {Hid::Key::D1, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D2 ) return {{Hid::Key::D2, Hid::Key::ShiftRight}, {Hid::Key::D2, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D3 ) return {{Hid::Key::D3, Hid::Key::ShiftRight}, {Hid::Key::D3, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D4 ) return {{Hid::Key::D4, Hid::Key::ShiftRight}, {Hid::Key::D4, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D5 ) return {{Hid::Key::D5, Hid::Key::ShiftRight}, {Hid::Key::D5, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D6 ) return {{Hid::Key::D6, Hid::Key::ShiftRight}, {Hid::Key::D6, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D7 ) return {{Hid::Key::D7, Hid::Key::ShiftRight}, {Hid::Key::D7, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D8 ) return {{Hid::Key::D8, Hid::Key::ShiftRight}, {Hid::Key::D8, Hid::Key::ShiftLeft}};
                if ( key == EmuKey::D9 ) return {{Hid::Key::D9, Hid::Key::ShiftRight}, {Hid::Key::D9, Hid::Key::ShiftLeft}};
            } else {
                if (key == EmuKey::D0) return {{Hid::Key::D0}};
                if (key == EmuKey::D1) return {{Hid::Key::D1}};
                if (key == EmuKey::D2) return {{Hid::Key::D2}};
                if (key == EmuKey::D3) return {{Hid::Key::D3}};
                if (key == EmuKey::D4) return {{Hid::Key::D4}};
                if (key == EmuKey::D5) return {{Hid::Key::D5}};
                if (key == EmuKey::D6) return {{Hid::Key::Slash, Hid::Key::ShiftRight}};
                if (key == EmuKey::D7) return {{Hid::Key::D7}};
                if (key == EmuKey::D8) return {{Hid::Key::Slash}};
                if (key == EmuKey::D9) return {{Hid::Key::D9}};
            }

            if ( key == EmuKey::Plus ) return {{Hid::Key::Equal, Hid::Key::ShiftRight}, {Hid::Key::Equal, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Minus ) return {{Hid::Key::D6}};
            if ( key == EmuKey::Colon ) return {{Hid::Key::Period}};
            if ( key == EmuKey::Semicolon ) return {{Hid::Key::Comma}};
            if ( key == EmuKey::Pound ) return {{Hid::Key::ClosedSquareBracket, Hid::Key::ShiftRight}, {Hid::Key::ClosedSquareBracket, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Asterisk ) return {{Hid::Key::NumberSign}};
            if ( key == EmuKey::Equal ) return {{Hid::Key::Equal}};
            if ( key == EmuKey::Slash ) return {{Hid::Key::Period, Hid::Key::ShiftRight}, {Hid::Key::Period, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::At ) return {{Hid::Key::D0, Hid::Key::AltRight}};
            if ( key == EmuKey::NumberSign ) return {{Hid::Key::D3, Hid::Key::AltRight}};
            if ( key == EmuKey::Ampersand ) return {{Hid::Key::D1}};
            if ( key == EmuKey::Acute ) return {{Hid::Key::D7, Hid::Key::AltRight}};
            if ( key == EmuKey::ParenthesesLeft ) return {{Hid::Key::D5}};
            if ( key == EmuKey::ParenthesesRight ) return {{Hid::Key::Minus}};
            if ( key == EmuKey::Greater ) return {{Hid::Key::Backslash, Hid::Key::ShiftRight}, {Hid::Key::Backslash, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Less ) return {{Hid::Key::Backslash}};
            if ( key == EmuKey::OpenSquareBracket) return {{Hid::Key::D5, Hid::Key::AltRight}};
            if ( key == EmuKey::ClosedSquareBracket) return {{Hid::Key::Minus, Hid::Key::AltRight}};
            if ( key == EmuKey::QuestionMark ) return {{Hid::Key::Semicolon, Hid::Key::ShiftRight}, {Hid::Key::Semicolon, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Pipe ) return {{Hid::Key::D6, Hid::Key::AltRight}};
            if ( key == EmuKey::NumLock ) return {{Hid::Key::D5, Hid::Key::AltRight}};
            if ( key == EmuKey::ScrollLock ) return {{Hid::Key::Minus, Hid::Key::AltRight}};

            if ( key == EmuKey::Shared1 ) return {{Hid::Key::OpenSquareBracket}};
            if ( key == EmuKey::Shared2 ) return {{Hid::Key::ClosedSquareBracket}};

            if ( key == EmuKey::Shared4 ) return {{Hid::Key::Comma}};
            if ( key == EmuKey::Shared5 ) return {{Hid::Key::Period}};
            if ( key == EmuKey::Shared6 ) return {{Hid::Key::Equal}};
            if ( key == EmuKey::Shared7 ) return {{Hid::Key::D8, Hid::Key::AltRight}};
            if ( key == EmuKey::Shared8 ) return {{Hid::Key::Apostrophe}};
            if ( key == EmuKey::Shared9 ) return {{Hid::Key::ShiftRight, Hid::Key::NumberSign}};
            if ( key == EmuKey::Shared10 ) return {{Hid::Key::Backslash}};
            if ( key == EmuKey::Shared11 ) return {{Hid::Key::D7, Hid::Key::AltRight}};
            if ( key == EmuKey::Shared12 ) return {{Hid::Key::Minus}};
            if ( key == EmuKey::Shared13 ) return {{Hid::Key::D6}};

            if ( key == EmuKey::ShiftShared2 ) return {{Hid::Key::NumberSign}};
            if ( key == EmuKey::ShiftShared4 ) return {{Hid::Key::Comma, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared5 ) return {{Hid::Key::Period, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared6 ) return {{Hid::Key::Equal, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared7 ) return {{Hid::Key::D6, Hid::Key::AltRight}};
            if ( key == EmuKey::ShiftShared8 ) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared9 ) return {{Hid::Key::ClosedSquareBracket, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared10 ) return {{Hid::Key::Backslash, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared11 ) return {{Hid::Key::D2, Hid::Key::AltRight}};
            if ( key == EmuKey::ShiftShared12 ) return {{Hid::Key::Minus, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared13 ) return {{Hid::Key::D8}};
            if ( key == EmuKey::ShiftAnd6 ) return {{Hid::Key::ShiftRight, Hid::Key::D6}};
            if ( key == EmuKey::ShiftAnd8 ) return {{Hid::Key::ShiftRight, Hid::Key::D8}};
            if ( key == EmuKey::AltAndShift2 ) return {{Hid::Key::AltRight, Hid::Key::D0}};
            if ( key == EmuKey::AltAndShift3 ) return {{Hid::Key::AltRight, Hid::Key::D3}};

            if ( key == EmuKey::ShiftNumLock ) return {{Hid::Key::D4, Hid::Key::AltRight}};
            if ( key == EmuKey::ShiftScrollLock ) return {{Hid::Key::Equal, Hid::Key::AltRight}};
            if ( key == EmuKey::Comma ) return {{Hid::Key::Semicolon}};
            if ( key == EmuKey::Period ) return {{Hid::Key::Comma, Hid::Key::ShiftRight}, {Hid::Key::Comma, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Percent ) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}, {Hid::Key::Apostrophe, Hid::Key::ShiftLeft}};

            if ( key == EmuKey::ExclamationMark ) return {{Hid::Key::Slash}};
            if ( key == EmuKey::DoubleQuotes ) return {{Hid::Key::D3}};
            if ( key == EmuKey::Dollar ) return {{Hid::Key::ClosedSquareBracket}};
            break;
        case KeyboardLayout::Type::De:
            if (key == EmuKey::D0) return {{Hid::Key::D0}};
            if (key == EmuKey::D1) return {{Hid::Key::D1}};
            if (key == EmuKey::D2) return {{Hid::Key::D2}};
            if (key == EmuKey::D3) return {{Hid::Key::D3}};
            if (key == EmuKey::D4) return {{Hid::Key::D4}};
            if (key == EmuKey::D5) return {{Hid::Key::D5}};
            if (key == EmuKey::D6) return {{Hid::Key::D6}};
            if (key == EmuKey::D7) return {{Hid::Key::D7}};
            if (key == EmuKey::D8) return {{Hid::Key::D8}};
            if (key == EmuKey::D9) return {{Hid::Key::D9}};

            if ( key == EmuKey::Plus ) return {{Hid::Key::ClosedSquareBracket}};
            if ( key == EmuKey::Minus ) return {{Hid::Key::Slash}};
            if ( key == EmuKey::Colon ) return {{Hid::Key::Period, Hid::Key::ShiftRight}, {Hid::Key::Period, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Semicolon ) return {{Hid::Key::Comma, Hid::Key::ShiftRight}, {Hid::Key::Comma, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Pound ) return {{Hid::Key::Apostrophe}};
            if ( key == EmuKey::Asterisk ) return {{Hid::Key::ClosedSquareBracket, Hid::Key::ShiftRight}, {Hid::Key::ClosedSquareBracket, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Equal ) return {{Hid::Key::D0, Hid::Key::ShiftRight}, {Hid::Key::D0, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Slash ) return {{Hid::Key::D7, Hid::Key::ShiftRight}, {Hid::Key::D7, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::At ) return {{Hid::Key::Q, Hid::Key::AltRight}};
            if ( key == EmuKey::NumberSign ) return {{Hid::Key::NumberSign}};
            if ( key == EmuKey::Ampersand ) return {{Hid::Key::D6, Hid::Key::ShiftRight}};
            if ( key == EmuKey::Acute ) return {{Hid::Key::Equal}};
            if ( key == EmuKey::ParenthesesLeft ) return {{Hid::Key::D8, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ParenthesesRight ) return {{Hid::Key::D9, Hid::Key::ShiftRight}};
            if ( key == EmuKey::Greater ) return {{Hid::Key::Backslash, Hid::Key::ShiftRight}, {Hid::Key::Backslash, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Less ) return {{Hid::Key::Backslash}};
            if ( key == EmuKey::OpenSquareBracket ) return {{Hid::Key::D8, Hid::Key::AltRight}};
            if ( key == EmuKey::ClosedSquareBracket ) return {{Hid::Key::D9, Hid::Key::AltRight}};
            if ( key == EmuKey::QuestionMark ) return {{Hid::Key::Minus, Hid::Key::ShiftRight}, {Hid::Key::Minus, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Pipe ) return {{Hid::Key::Backslash, Hid::Key::AltRight}};
            if ( key == EmuKey::NumLock ) return {{Hid::Key::D8, Hid::Key::AltRight}};
            if ( key == EmuKey::ScrollLock ) return {{Hid::Key::D9, Hid::Key::AltRight}};  
            if ( key == EmuKey::Shared1 ) return {{Hid::Key::OpenSquareBracket}};
            if ( key == EmuKey::Shared2 ) return {{Hid::Key::ClosedSquareBracket}};
            if ( key == EmuKey::Shared3 ) return {{Hid::Key::Semicolon}};
            if ( key == EmuKey::Shared4 ) return {{Hid::Key::Comma}};
            if ( key == EmuKey::Shared5 ) return {{Hid::Key::Period}};
            if ( key == EmuKey::Shared6 ) return {{Hid::Key::Slash}};
            if ( key == EmuKey::Shared7 ) return {{Hid::Key::Minus, Hid::Key::AltRight}};     
            if ( key == EmuKey::Shared8 ) return {{Hid::Key::Apostrophe}};
            if ( key == EmuKey::Shared9 ) return {{Hid::Key::NumberSign}};
            if ( key == EmuKey::Shared10 ) return {{Hid::Key::Backslash}};
            if ( key == EmuKey::Shared11 ) return {{Hid::Key::Equal, Hid::Key::ShiftRight}};
            if ( key == EmuKey::Shared12 ) return {{Hid::Key::Minus}};
            if ( key == EmuKey::Shared13 ) return {{Hid::Key::Equal}};

            if ( key == EmuKey::ShiftShared7 ) return {{Hid::Key::Backslash, Hid::Key::AltRight}};
            if ( key == EmuKey::ShiftShared9 ) return {{Hid::Key::Grave}};
            if ( key == EmuKey::ShiftShared10 ) return {{Hid::Key::Backslash, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared11 ) return {{Hid::Key::ClosedSquareBracket, Hid::Key::AltRight}};
            if ( key == EmuKey::ShiftShared13 ) return {{Hid::Key::Backslash, Hid::Key::ShiftRight}};    
            if ( key == EmuKey::ShiftNumLock ) return {{Hid::Key::D7, Hid::Key::AltRight}};
            if ( key == EmuKey::ShiftScrollLock ) return {{Hid::Key::D0, Hid::Key::AltRight}};
            if ( key == EmuKey::AltAnd2 ) return {{Hid::Key::Q, Hid::Key::AltRight}};
            if ( key == EmuKey::Period ) return {{Hid::Key::Period}};
            if ( key == EmuKey::Comma ) return {{Hid::Key::Comma}};    
            
            break;
        case KeyboardLayout::Type::Us:
            if ( key == EmuKey::Pound ) return {{Hid::Key::Backslash}};
            if ( key == EmuKey::At ) return {{Hid::Key::D2, Hid::Key::ShiftRight}, {Hid::Key::D2, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::DoubleQuotes ) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}, {Hid::Key::Apostrophe, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::NumberSign ) return {{Hid::Key::D3, Hid::Key::ShiftRight}, {Hid::Key::D3, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Shared7 ) return {{Hid::Key::NumberSign}};
            if ( key == EmuKey::Shared8 ) return {{Hid::Key::Grave}};
            if ( key == EmuKey::ShiftShared8 ) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared11) return {{Hid::Key::Grave, Hid::Key::ShiftRight}};
            
        case KeyboardLayout::Type::Uk:
            if (key == EmuKey::D0) return {{Hid::Key::D0}};
            if (key == EmuKey::D1) return {{Hid::Key::D1}};
            if (key == EmuKey::D2) return {{Hid::Key::D2}};
            if (key == EmuKey::D3) return {{Hid::Key::D3}};
            if (key == EmuKey::D4) return {{Hid::Key::D4}};
            if (key == EmuKey::D5) return {{Hid::Key::D5}};
            if (key == EmuKey::D6) return {{Hid::Key::D6}};
            if (key == EmuKey::D7) return {{Hid::Key::D7}};
            if (key == EmuKey::D8) return {{Hid::Key::D8}};
            if (key == EmuKey::D9) return {{Hid::Key::D9}};

            if ( key == EmuKey::Plus ) return {{Hid::Key::Equal, Hid::Key::ShiftRight}, {Hid::Key::Equal, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Minus ) return {{Hid::Key::Minus}};
            if ( key == EmuKey::Colon ) return {{Hid::Key::Semicolon, Hid::Key::ShiftRight}, {Hid::Key::Semicolon, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Semicolon ) return {{Hid::Key::Semicolon}};
            if ( key == EmuKey::Pound ) return {{Hid::Key::D3, Hid::Key::ShiftRight}, {Hid::Key::D3, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Asterisk ) return {{Hid::Key::D8, Hid::Key::ShiftRight}, {Hid::Key::D8, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Equal ) return {{Hid::Key::Equal}};
            if ( key == EmuKey::Slash ) return {{Hid::Key::Slash}};
            if ( key == EmuKey::At ) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}, {Hid::Key::Apostrophe, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::NumberSign ) return {{Hid::Key::NumberSign}};
            if ( key == EmuKey::Ampersand ) return {{Hid::Key::D7, Hid::Key::ShiftRight}, {Hid::Key::D7, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::Acute ) return {{Hid::Key::Grave}};
            if ( key == EmuKey::ParenthesesLeft ) return {{Hid::Key::D9, Hid::Key::ShiftRight}, {Hid::Key::D9, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::ParenthesesRight ) return {{Hid::Key::D0, Hid::Key::ShiftRight}, {Hid::Key::D0, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::OpenSquareBracket ) return {{Hid::Key::OpenSquareBracket}};
            if ( key == EmuKey::ClosedSquareBracket ) return {{Hid::Key::ClosedSquareBracket}};
            if ( key == EmuKey::Pipe ) return {{Hid::Key::Backslash, Hid::Key::ShiftRight}, {Hid::Key::Backslash, Hid::Key::ShiftLeft}};
            if ( key == EmuKey::NumLock ) return {{Hid::Key::NumLock}};
            if ( key == EmuKey::ScrollLock ) return {{Hid::Key::ScrollLock}};
            if ( key == EmuKey::Shared1 ) return {{Hid::Key::OpenSquareBracket}};
            if ( key == EmuKey::Shared2 ) return {{Hid::Key::ClosedSquareBracket}};
            if ( key == EmuKey::Shared3 ) return {{Hid::Key::Semicolon}};
            if ( key == EmuKey::Shared4 ) return {{Hid::Key::Comma}};
            if ( key == EmuKey::Shared5 ) return {{Hid::Key::Period}};
            if ( key == EmuKey::Shared6 ) return {{Hid::Key::Slash}};
            if ( key == EmuKey::Shared7 ) return {{Hid::Key::Backslash}};   
            if ( key == EmuKey::Shared8 ) return {{Hid::Key::NumberSign}};
            if ( key == EmuKey::Shared11 ) return {{Hid::Key::Apostrophe}};
            if ( key == EmuKey::Shared12 ) return {{Hid::Key::Minus}};
            if ( key == EmuKey::Shared13 ) return {{Hid::Key::Equal}};

            if ( key == EmuKey::ShiftShared8) return {{Hid::Key::Apostrophe, Hid::Key::ShiftRight}};
            if ( key == EmuKey::ShiftShared11) return {{Hid::Key::NumberSign, Hid::Key::ShiftRight}};
            if ( key == EmuKey::Period ) return {{Hid::Key::Period}};
            if ( key == EmuKey::Comma ) return {{Hid::Key::Comma}};     
            break;
    }
    
    return {{}};
}