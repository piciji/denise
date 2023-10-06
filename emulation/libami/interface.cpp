
#include "interface.h"
#include "system/system.h"
#include "drive/diskStructure.h"
#include <cstring>
#include <cstdlib>

namespace LIBAMI {

const std::string Interface::Version = "217";

Interface::Interface() : Emulator::Interface( "Amiga" ) {

    prepareMedia();
    prepareFirmware();
    prepareDevices();
    prepareModels();
    preparePalettes();
    prepareExpansions();

    system = new System( this );
}

Interface::~Interface() {
    if (system)
        delete system;
}

auto Interface::prepareFirmware() -> void {
    firmwares.push_back({FirmwareIdKick, "Kickrom"});
    firmwares.push_back({FirmwareIdExt, "Extrom"});
}

auto Interface::prepareModels() -> void {

    models.push_back({ModelIdSystem, "Amiga", Model::Type::Radio, Model::Purpose::SubModels, 1, {0, 2}, {"A1000", "A500 (Full OCS)", "A500 (ECS Agnus, OCS Denise)"} });
    models.push_back({ModelIdSampleFetch, "PAULA Sample Interval", Model::Type::Radio, Model::Purpose::AudioResampler, 3, {0, 11}, {"8", "16", "24", "32", "40", "48", "56", "64", "80", "96", "112", "128"}});
    models.push_back({ModelIdAudioFilter, "PAULA Filter", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 6}, {"Auto", "Off (A500)", "Software (A500)", "On (A500)", "Off (A1200)", "Software (A1200)", "On (A1200)"}});
    models.push_back({ModelIdRegion, "Region", Model::Type::Radio, Model::Purpose::GraphicChip, 0, {0, 1}, { "PAL", "NTSC" }});
    models.push_back({ModelIdDiskDrivesConnected, "Disk Drives", Model::Type::Combo, Model::Purpose::DriveSettings, 1, {0, 4}, { "0", "1", "2", "3", "4" }});
    models.push_back({ModelIdDiskDriveSpeed, "Disk Speed", Model::Type::Slider, Model::Purpose::DriveSettings, 30000, {27500, 32500}, {}, 500, 100.0 });
    models.push_back({ModelIdDiskDriveWobble, "Disk Wobble", Model::Type::Slider, Model::Purpose::DriveSettings, 50, {0, 500}, {}, 50, 100.0 });
    models.push_back({ModelIdDiskDriveStepperSeekTime, "Stepper Seek Time", Model::Type::Slider, Model::Purpose::DriveSettings, 0, {0, 180}, {}, 180, 10.0 });
    models.push_back({ModelIdDiskDriveStepperAccessTime, "Stepper Access Time", Model::Type::Slider, Model::Purpose::DriveSettings, 0, {0, 30}, {}, 30, 10.0 });
    models.push_back({ModelIdDiskTurbo, "Disk Turbo", Model::Type::Radio, Model::Purpose::DriveSettings, 0, {0, 4}, { "1x", "2x", "4x", "8x", "MAX" }});

    models.push_back({ModelIdChipMem, "Chip Mem", Model::Type::Slider, Model::Purpose::Memory, 1, {0, 3}, { "256 KB", "512 KB", "1 MB", "2 MB" }});
    models.push_back({ModelIdSlowMem, "Slow Mem", Model::Type::Slider, Model::Purpose::Memory, 1, {0, 4}, { "0", "512 KB", "1 MB", "1.5 MB", "1.75 MB" }});
    models.push_back({ModelIdFastMem, "Fast Mem", Model::Type::Slider, Model::Purpose::Memory, 0, {0, 8}, { "0", "64 KB", "128 KB", "256 KB", "512 KB", "1 MB", "2 MB", "4 MB", "8 MB" }});

    models.push_back({ModelIdRTC, "RTC", Model::Type::Switch, Model::Purpose::Misc, 0});
    models.push_back({ModelIdSerialLoopback, "Serial Loopback", Model::Type::Switch, Model::Purpose::Misc, 0});
}

auto Interface::prepareMedia() -> void {
    mediaGroups.push_back({MediaGroupIdDisk, "disk", MediaGroup::Type::Disk, {"adf", "dms", "ipf", "adz", "exe"}, {"adf", "ext.adf"} });
    // mediaGroups.push_back({MediaGroupIdHardDisk, "hd", MediaGroup::Type::HardDisk, {"hdf"}, {"hdf"} });

    {   auto& group = mediaGroups[MediaGroupIdDisk];
        group.media.push_back({0, "DF0", 0, &group});
        group.media.push_back({1, "DF1", 0, &group});
        group.media.push_back({2, "DF2", 0, &group});
        group.media.push_back({3, "DF3", 0, &group});
        group.selected = nullptr;
    }

//    {   auto& group = mediaGroups[MediaGroupIdHardDisk];
//        group.media.push_back({0, "DH0", 0, &group});
//        group.media.push_back({1, "DH1", 0, &group});
//        group.selected = nullptr;
//    }

    for(auto& group : mediaGroups) {
        group.expansion = nullptr;

        for(auto& media : group.media) {
            media.pcbLayout = nullptr;
            media.secondary = false;
        }
    }
}

auto Interface::prepareExpansions() -> void {

    expansions.push_back( { ExpansionIdNone, "Empty", Expansion::Type::Empty, nullptr, nullptr } );
    //expansions.push_back( { ExpansionIdFast, "Fast", Expansion::Type::Ram, &memoryTypes[2], nullptr } );

}

auto Interface::prepareDevices() -> void {
    connectors.push_back( {0, "Port 1", Connector::Type::Port1} );
    connectors.push_back( {1, "Port 2", Connector::Type::Port2} );
    unsigned id = 0;

    {   Device device{ id++, "Unassigned", Device::Type::None };
        devices.push_back(device);
    }

    {   Device device{ id++, "Joypad #1", Device::Type::Joypad };
        device.inputs.push_back( {0, "Up", Key::Direction} );
        device.inputs.push_back( {1, "Down", Key::Direction} );
        device.inputs.push_back( {2, "Left", Key::Direction} );
        device.inputs.push_back( {3, "Right", Key::Direction} );
        device.inputs.push_back( {4, "Button 1", Key::Button} );
        device.inputs.push_back( {5, "Button 2", Key::Button} );

        device.addVirtual( "Button 1 Turbo", { 4 }, Key::Autofire );
        device.addVirtual( "Button 1 Autofire", { 4 }, Key::ToggleAutofire );
        device.addVirtual( "Button 2 Turbo", { 5 }, Key::Autofire );
        device.addVirtual( "Button 2 Autofire", { 5 }, Key::ToggleAutofire );
        device.addVirtual( "Left Turbo", { 2 }, Key::AutofireDirection );
        device.addVirtual( "Right Turbo", { 3 }, Key::AutofireDirection );

        device.addVirtual( "Diagonal Up-Right", { 0, 3 }, Key::JoyUpRight );
        device.addVirtual( "Diagonal Down-Right", { 1, 3 }, Key::JoyDownRight );
        device.addVirtual( "Diagonal Up-Left", { 0, 2 }, Key::JoyUpLeft );
        device.addVirtual( "Diagonal Down-Left", { 1, 2 }, Key::JoyDownLeft );

        devices.push_back(device);
        device.id = id++;
        device.name = "Joypad #2";
        devices.push_back(device);
    }

    {   Device device{ id++, "Mouse #1", Device::Type::Mouse };
        device.inputs.push_back( {0, "X-Axis"} );
        device.inputs.push_back( {1, "Y-Axis"} );
        device.inputs.push_back( {2, "Button Left"} );
        device.inputs.push_back( {3, "Button Right"} );

        devices.push_back(device);
        device.id = id++;
        device.name = "Mouse #2";
        devices.push_back(device);
    }

    {   Device device{ id++, "Keyboard", Device::Type::Keyboard };
        device.inputs.push_back( {0, "A (Q)", Key::A, {{Device::Layout::Uk, "A"}, {Device::Layout::Us, "A"}, {Device::Layout::De, "A"}, {Device::Layout::Fr, "Q"}} } );
        device.inputs.push_back( {1, "B", Key::B } );
        device.inputs.push_back( {2, "C", Key::C } );
        device.inputs.push_back( {3, "D", Key::D } );
        device.inputs.push_back( {4, "E", Key::E } );
        device.inputs.push_back( {5, "F", Key::F } );
        device.inputs.push_back( {6, "G", Key::G } );
        device.inputs.push_back( {7, "H", Key::H } );
        device.inputs.push_back( {8, "I", Key::I } );
        device.inputs.push_back( {9, "J", Key::J } );
        device.inputs.push_back( {10, "K", Key::K } );
        device.inputs.push_back( {11, "L", Key::L } );
        device.inputs.push_back( {12, "M (,)", Key::M, { {Device::Layout::Uk, "M"}, {Device::Layout::Us, "M"}, {Device::Layout::De, "M"}, {Device::Layout::Fr, ","}} });
        device.inputs.push_back( {13, "N", Key::N } );
        device.inputs.push_back( {14, "O", Key::O } );
        device.inputs.push_back( {15, "P", Key::P } );
        device.inputs.push_back( {16, "Q (A)", Key::Q, { {Device::Layout::Uk, "Q"}, {Device::Layout::Us, "Q"}, {Device::Layout::De, "Q"}, {Device::Layout::Fr, "A"}}} );
        device.inputs.push_back( {17, "R", Key::R } );
        device.inputs.push_back( {18, "S", Key::S } );
        device.inputs.push_back( {19, "T", Key::T } );
        device.inputs.push_back( {20, "U", Key::U } );
        device.inputs.push_back( {21, "V", Key::V } );
        device.inputs.push_back( {22, "W (Z)", Key::W, { {Device::Layout::Uk, "W"}, {Device::Layout::Us, "W"}, {Device::Layout::De, "W"}, {Device::Layout::Fr, "Z"}}} );
        device.inputs.push_back( {23, "X", Key::X } );
        device.inputs.push_back( {24, "Y (Z)", Key::Y, { {Device::Layout::Uk, "Y"}, {Device::Layout::Us, "Y"}, {Device::Layout::De, "Z"},  {Device::Layout::Fr, "Y"} }} );
        device.inputs.push_back( {25, "Z (Y W)", Key::Z, { {Device::Layout::Uk, "Z"}, {Device::Layout::Us, "Z"}, {Device::Layout::De, "Y"}, {Device::Layout::Fr, "W"} }} );

        device.inputs.push_back( {26, "0", Key::D0, { {Device::Layout::Uk, "0"}, {Device::Layout::Us, "0"}, {Device::Layout::De, "0"}, {Device::Layout::Fr, "à"}} } );
        device.inputs.push_back( {27, "1", Key::D1, { {Device::Layout::Uk, "1"}, {Device::Layout::Us, "1"}, {Device::Layout::De, "1"}, {Device::Layout::Fr, "&"}}} );
        device.inputs.push_back( {28, "2", Key::D2, { {Device::Layout::Uk, "2"}, {Device::Layout::Us, "2"}, {Device::Layout::De, "2"}, {Device::Layout::Fr, "é"}}} );
        device.inputs.push_back( {29, "3", Key::D3, { {Device::Layout::Uk, "3"}, {Device::Layout::Us, "3"}, {Device::Layout::De, "3"}, {Device::Layout::Fr, "\""}}} );
        device.inputs.push_back( {30, "4", Key::D4, { {Device::Layout::Uk, "4"}, {Device::Layout::Us, "4"}, {Device::Layout::De, "4"}, {Device::Layout::Fr, "´"}}} );
        device.inputs.push_back( {31, "5", Key::D5, { {Device::Layout::Uk, "5"}, {Device::Layout::Us, "5"}, {Device::Layout::De, "5"}, {Device::Layout::Fr, "("}}} );
        device.inputs.push_back( {32, "6", Key::D6, { {Device::Layout::Uk, "6"}, {Device::Layout::Us, "6"}, {Device::Layout::De, "6"}, {Device::Layout::Fr, "§"}}} );
        device.inputs.push_back( {33, "7", Key::D7, { {Device::Layout::Uk, "7"}, {Device::Layout::Us, "7"}, {Device::Layout::De, "7"}, {Device::Layout::Fr, "è"}}} );
        device.inputs.push_back( {34, "8", Key::D8, { {Device::Layout::Uk, "8"}, {Device::Layout::Us, "8"}, {Device::Layout::De, "8"}, {Device::Layout::Fr, "!"}}} );
        device.inputs.push_back( {35, "9", Key::D9, { {Device::Layout::Uk, "9"}, {Device::Layout::Us, "9"}, {Device::Layout::De, "9"}, {Device::Layout::Fr, "ç"}}} );

        device.inputs.push_back( {36, "Keypad 0", Key::NumPad0} );
        device.inputs.push_back( {37, "Keypad 1", Key::NumPad1} );
        device.inputs.push_back( {38, "Keypad 2", Key::NumPad2} );
        device.inputs.push_back( {39, "Keypad 3", Key::NumPad3} );
        device.inputs.push_back( {40, "Keypad 4", Key::NumPad4} );
        device.inputs.push_back( {41, "Keypad 5", Key::NumPad5} );
        device.inputs.push_back( {42, "Keypad 6", Key::NumPad6} );
        device.inputs.push_back( {43, "Keypad 7", Key::NumPad7} );
        device.inputs.push_back( {44, "Keypad 8", Key::NumPad8} );
        device.inputs.push_back( {45, "Keypad 9", Key::NumPad9} );

        device.inputs.push_back( {46, "Keypad NumLock", Key::NumLock, { {Device::Layout::Uk, "Keypad ("}, {Device::Layout::Us, "Keypad ("}, {Device::Layout::De, "Keypad ["}, {Device::Layout::Fr, "Keypad ["} } } );
        device.inputs.push_back( {47, "Keypad ScrollLock", Key::ScrollLock, { {Device::Layout::Uk, "Keypad )"}, {Device::Layout::Us, "Keypad )"}, {Device::Layout::De, "Keypad ]"}, {Device::Layout::Fr, "Keypad ]"} }} );
        device.inputs.push_back( {48, "Keypad /", Key::NumDivide} );
        device.inputs.push_back( {49, "Keypad *", Key::NumMultiply} );
        device.inputs.push_back( {50, "Keypad -", Key::NumSubtract} );
        device.inputs.push_back( {51, "Keypad +", Key::NumAdd} );
        device.inputs.push_back( {52, "Keypad enter", Key::NumEnter} );
        device.inputs.push_back( {53, "Keypad .", Key::NumComma} );

        device.inputs.push_back( {54, "F1", Key::F1} );
        device.inputs.push_back( {55, "F2", Key::F2} );
        device.inputs.push_back( {56, "F3", Key::F3} );
        device.inputs.push_back( {57, "F4", Key::F4} );
        device.inputs.push_back( {58, "F5", Key::F5} );
        device.inputs.push_back( {59, "F6", Key::F6} );
        device.inputs.push_back( {60, "F7", Key::F7} );
        device.inputs.push_back( {61, "F8", Key::F8} );
        device.inputs.push_back( {62, "F9", Key::F9} );
        device.inputs.push_back( {63, "F10", Key::F10} );

        device.inputs.push_back( {64, "Cursor Up", Key::CursorUp} );
        device.inputs.push_back( {65, "Cursor Down", Key::CursorDown} );
        device.inputs.push_back( {66, "Cursor Left", Key::CursorLeft} );
        device.inputs.push_back( {67, "Cursor Right", Key::CursorRight} );

        device.inputs.push_back( {68, "Space", Key::Space} );
        device.inputs.push_back( {69, "Backspace", Key::Backspace} );
        device.inputs.push_back( {70, "Tab", Key::Tab} );
        device.inputs.push_back( {71, "Return", Key::Return} );
        device.inputs.push_back( {72, "Esc", Key::Esc} );
        device.inputs.push_back( {73, "Del", Key::Del} );
        device.inputs.push_back( {74, "Left Shift", Key::ShiftLeft} );
        device.inputs.push_back( {75, "Right Shift", Key::ShiftRight} );
        device.inputs.push_back( {76, "Capslock", Key::ShiftLock} );
        device.inputs.push_back( {77, "Ctrl", Key::ControlLeft} );
        device.inputs.push_back( {78, "Left Alt", Key::AltLeft} );
        device.inputs.push_back( {79, "Right Alt", Key::AltRight} );
        device.inputs.push_back( {80, "Left Ami", Key::SystemLeft} );
        device.inputs.push_back( {81, "Right Ami", Key::SystemRight} );
        device.inputs.push_back( {82, "Help", Key::Help} );
        // these keys differs between keyboard layouts
        device.inputs.push_back( {83, "shared 1a", Key::Shared1, { {Device::Layout::Uk, "["}, {Device::Layout::Us, "["}, {Device::Layout::De, "ü"}, {Device::Layout::Fr, "^"} } } );
        device.inputs.push_back( {84, "shared 1b", Key::Shared2, { {Device::Layout::Uk, "]"}, {Device::Layout::Us, "]"}, {Device::Layout::De, "+"}, {Device::Layout::Fr, "$"} } } );
        device.inputs.push_back( {85, "shared 29", Key::Shared3, { {Device::Layout::Uk, ";"}, {Device::Layout::Us, ";"}, {Device::Layout::De, "ö"}, {Device::Layout::Fr, "M"} } } );
        device.inputs.push_back( {86, "shared 38", Key::Shared4, { {Device::Layout::Uk, ","}, {Device::Layout::Us, ","}, {Device::Layout::De, ","}, {Device::Layout::Fr, ";"} } } );
        device.inputs.push_back( {87, "shared 39", Key::Shared5, { {Device::Layout::Uk, "."}, {Device::Layout::Us, "."}, {Device::Layout::De, "."}, {Device::Layout::Fr, ":"} } } );
        device.inputs.push_back( {88, "shared 3a", Key::Shared6, { {Device::Layout::Uk, "/"}, {Device::Layout::Us, "/"}, {Device::Layout::De, "-"}, {Device::Layout::Fr, "="} } } );
        device.inputs.push_back( {89, "shared 0d", Key::Shared7, { {Device::Layout::Uk, "\\"}, {Device::Layout::Us, "\\"}, {Device::Layout::De, "\\"}, {Device::Layout::Fr, "\\"} } } );
        device.inputs.push_back( {90, "shared 2a", Key::Shared8, { {Device::Layout::Uk, "#"}, {Device::Layout::Us, "´"}, {Device::Layout::De, "ä"}, {Device::Layout::Fr, "ù"} }} );

        // 2 more keys on some layouts, i.e. french, german layout
        device.inputs.push_back( {91, "shared 2b", Key::Shared9, { {Device::Layout::Uk, "unused"}, {Device::Layout::Us, "unused"}, {Device::Layout::De, "#"}, {Device::Layout::Fr, "µ"} } } );
        device.inputs.push_back( {92, "shared 30", Key::Shared10, { {Device::Layout::Uk, "unused"}, {Device::Layout::Us, "unused"}, {Device::Layout::De, "<"}, {Device::Layout::Fr, "<"} } } );

        device.inputs.push_back( {93, "shared 00", Key::Shared11, { {Device::Layout::Uk, "`"}, {Device::Layout::Us, "`"}, {Device::Layout::De, "`"}, {Device::Layout::Fr, "`"} } } );
        device.inputs.push_back( {94, "shared 0b", Key::Shared12, { {Device::Layout::Uk, "-"}, {Device::Layout::Us, "-"}, {Device::Layout::De, "ß"}, {Device::Layout::Fr, ")"} } } );
        device.inputs.push_back( {95, "shared 0c", Key::Shared13, { {Device::Layout::Uk, "="}, {Device::Layout::Us, "="}, {Device::Layout::De, "dead ´"}, {Device::Layout::Fr, "-"} } } );

        // virtual inputs (no physical keys)
        device.addVirtual( "shift + 1", { 75, 27 }, Key::ShiftAnd1, { {Device::Layout::Uk, "!"}, {Device::Layout::Us, "!"}, {Device::Layout::De, "!"}, {Device::Layout::Fr, "1"} } );
        device.addVirtual( "shift + 2", { 75, 28 }, Key::ShiftAnd2, { {Device::Layout::Uk, "\""}, {Device::Layout::Us, "@"}, {Device::Layout::De, "\""}, {Device::Layout::Fr, "2"} } );
        device.addVirtual( "shift + 3", { 75, 29 }, Key::ShiftAnd3, { {Device::Layout::Uk, "£"}, {Device::Layout::Us, "#"}, {Device::Layout::De, "§"}, {Device::Layout::Fr, "3"} } );
        device.addVirtual( "shift + 4", { 75, 30 }, Key::ShiftAnd4, { {Device::Layout::Uk, "$"}, {Device::Layout::Us, "$"}, {Device::Layout::De, "$"}, {Device::Layout::Fr, "4"} } );
        device.addVirtual( "shift + 5", { 75, 31 }, Key::ShiftAnd5, { {Device::Layout::Uk, "%"}, {Device::Layout::Us, "%"}, {Device::Layout::De, "%"}, {Device::Layout::Fr, "5"} } );
        device.addVirtual( "shift + 6", { 75, 32 }, Key::ShiftAnd6, { {Device::Layout::Uk, "^"}, {Device::Layout::Us, "^"}, {Device::Layout::De, "&"}, {Device::Layout::Fr, "6"} } );
        device.addVirtual( "shift + 7", { 75, 33 }, Key::ShiftAnd7, { {Device::Layout::Uk, "&"}, {Device::Layout::Us, "&"}, {Device::Layout::De, "/"}, {Device::Layout::Fr, "7"} } );
        device.addVirtual( "shift + 8", { 75, 34 }, Key::ShiftAnd8, { {Device::Layout::Uk, "*"}, {Device::Layout::Us, "*"}, {Device::Layout::De, "("}, {Device::Layout::Fr, "8"} } );
        device.addVirtual( "shift + 9", { 75, 35 }, Key::ShiftAnd9, { {Device::Layout::Uk, "("}, {Device::Layout::Us, "("}, {Device::Layout::De, ")"}, {Device::Layout::Fr, "9"} } );
        device.addVirtual( "shift + 0", { 75, 26 }, Key::ShiftAnd0, { {Device::Layout::Uk, ")"}, {Device::Layout::Us, ")"}, {Device::Layout::De, "="}, {Device::Layout::Fr, "0"} } );

        device.addVirtual( "alt + shift + 2", { 79, 75, 28 }, Key::AltAndShift2, { {Device::Layout::Fr, "@"}  } );
        device.addVirtual( "alt + shift + 3", { 79, 75, 29 }, Key::AltAndShift3, { {Device::Layout::Fr, "#"}  } );

        device.addVirtual( "alt + 2", { 79, 28 }, Key::AltAnd2, { {Device::Layout::De, "@"}  } );

        device.addVirtual( "shift + shared 1a", { 75, 83 }, Key::ShiftShared1, { {Device::Layout::Uk, "{"}, {Device::Layout::Us, "{"}, {Device::Layout::De, "Ü"}, {Device::Layout::Fr, "¨"} } );
        device.addVirtual( "shift + shared 1b", { 75, 84 }, Key::ShiftShared2, { {Device::Layout::Uk, "}"}, {Device::Layout::Us, "}"}, {Device::Layout::De, "*"}, {Device::Layout::Fr, "*"} } );
        device.addVirtual( "shift + shared 29", { 75, 85 }, Key::ShiftShared3, { {Device::Layout::Uk, ":"}, {Device::Layout::Us, ":"}, {Device::Layout::De, "Ö"}, {Device::Layout::Fr, "M"} } );
        device.addVirtual( "shift + shared 38", { 75, 86 }, Key::ShiftShared4, { {Device::Layout::Uk, "<"}, {Device::Layout::Us, "<"}, {Device::Layout::De, ";"}, {Device::Layout::Fr, "."} } );
        device.addVirtual( "shift + shared 39", { 75, 87 }, Key::ShiftShared5, { {Device::Layout::Uk, ">"}, {Device::Layout::Us, ">"}, {Device::Layout::De, ":"}, {Device::Layout::Fr, "/"} } );
        device.addVirtual( "shift + shared 3a", { 75, 88 }, Key::ShiftShared6, { {Device::Layout::Uk, "?"}, {Device::Layout::Us, "?"}, {Device::Layout::De, "_"}, {Device::Layout::Fr, "+"} } );
        device.addVirtual( "shift + shared 0d", { 75, 89 }, Key::ShiftShared7, { {Device::Layout::Uk, "|"}, {Device::Layout::Us, "|"}, {Device::Layout::De, "|"}, {Device::Layout::Fr, "|"} } );
        device.addVirtual( "shift + shared 2a", { 75, 90 }, Key::ShiftShared8, { {Device::Layout::Uk, "@"}, {Device::Layout::Us, "\""}, {Device::Layout::De, "Ä"}, {Device::Layout::Fr, "%"} } );
        device.addVirtual( "shift + shared 2b", { 75, 91 }, Key::ShiftShared9, { {Device::Layout::Uk, "unused"}, {Device::Layout::Us, "unused"}, {Device::Layout::De, "^"}, {Device::Layout::Fr, "£"} } );
        device.addVirtual( "shift + shared 30", { 75, 92 }, Key::ShiftShared10, { {Device::Layout::Uk, "unused"}, {Device::Layout::Us, "unused"}, {Device::Layout::De, ">"}, {Device::Layout::Fr, ">"} } );
        device.addVirtual( "shift + shared 00", { 75, 93 }, Key::ShiftShared11, { {Device::Layout::Uk, "~"}, {Device::Layout::Us, "~"}, {Device::Layout::De, "~"}, {Device::Layout::Fr, "~"} } );
        device.addVirtual( "shift + shared 0b", { 75, 94 }, Key::ShiftShared12, { {Device::Layout::Uk, "_"}, {Device::Layout::Us, "_"}, {Device::Layout::De, "?"}, {Device::Layout::Fr, "°"} } );
        device.addVirtual( "shift + shared 0c", { 75, 95 }, Key::ShiftShared13, { {Device::Layout::Uk, "+"}, {Device::Layout::Us, "+"}, {Device::Layout::De, "`"}, {Device::Layout::Fr, "_"} } );

        device.addVirtual( "shift + num lock", { 75, 46 }, Key::ShiftNumLock, { {Device::Layout::De, "Keypad {"}, {Device::Layout::Fr, "Keypad {"} } );
        device.addVirtual( "shift + scroll lock", { 75, 47 }, Key::ShiftScrollLock, {  {Device::Layout::De, "Keypad }"}, {Device::Layout::Fr, "Keypad }"} } );

        devices.push_back(device);
    }

    for (auto& device : devices) {
        device.userData = 0;

        for (auto& input : device.inputs) {
            input.type = input.name.find("Axis") != std::string::npos ? 1 : 0;
            input.guid = 0;
        }
    }
}

auto Interface::preparePalettes() -> void {
    uint32_t color;
    uint8_t r;
    uint8_t g;
    uint8_t b;

    palettes.push_back({ 0, "Default", false });

    for (unsigned i = 0; i < 4096; i++) {
        color = ((i & 0xf) << 0) | ((i & 0xf) << 4)
                         | ((i & 0xf0) << 4) | ((i & 0xf0) << 8)
                         | ((i & 0xf00) << 8) | ((i & 0xf00) << 12);

        r = (color >> 16) & 0xff;
        g = (color >> 8) & 0xff;
        b = (color >> 0) & 0xff;

        palettes[0].paletteColors.push_back( {"", color, r, g, b} );
    }
}

auto Interface::connect(unsigned connectorId, unsigned deviceId) -> void {
    system->input.connectControlport( getConnector( connectorId ), getDevice( deviceId ) );
}

auto Interface::connect(Connector* connector, Device* device) -> void {
    system->input.connectControlport( connector, device );
}

auto Interface::getConnectedDevice( Connector* connector ) -> Device* {
    auto device = system->input.getConnectedDevice( connector );

    if (!device)
        return getUnplugDevice();

    return device;
}

auto Interface::getCursorPosition( Device* device, int16_t& x, int16_t& y ) -> bool {
    return system->input.getCursorPosition( device, x, y );
}

auto Interface::setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void {
    if (typeId >= firmwares.size()) return;

    system->setFirmware( typeId, data, size, allowPatching );
}

auto Interface::power() -> void {
    system->power();
}

auto Interface::reset() -> void {
    system->power( true );
}

auto Interface::powerOff() -> void {
    system->powerOff();
}

auto Interface::run() -> void {
    system->run();
}

auto Interface::runAhead(unsigned frames) -> void {
    system->setRunAhead(frames);
}

auto Interface::runAheadPerformance(bool state) -> void {
    system->runAhead.performance = state;
}

auto Interface::runAheadPreventJit(bool state) -> void {
    system->runAhead.preventJit = state;
    system->input.updateSampling();
}

auto Interface::getRegionEncoding() -> Region {
    return system->ntsc ? Region::Ntsc : Region::Pal;
}

auto Interface::getRegionGeometry() -> Region {
    return system->agnus.ntsc ? Region::Ntsc : Region::Pal;
}

auto Interface::getSubRegion() -> SubRegion {
    return system->ntsc ? SubRegion::Ntsc_M : SubRegion::Pal_B;
}

auto Interface::setMonitorFpsRatio(double ratio) -> void {
    system->hintSlowSpeed( ratio < 0.5 );
}

auto Interface::setModelValue(unsigned modelId, int value) -> void {
    switch (modelId) {
        case ModelIdSystem:
            system->setModel( value );
            break;
        case ModelIdRegion:
            system->setRegion( value );
            break;
        case ModelIdAudioFilter:
            system->paula.setFilterMode(value);
            break;
        case ModelIdSampleFetch:
            system->setResampleQuality( value );
            break;
        case ModelIdDiskDrivesConnected:
            system->setDrivesEnabled(value);
            break;
        case ModelIdDiskDriveWobble:
            DiskDrive::setWobble( value );
            break;
        case ModelIdDiskDriveSpeed:
            DiskDrive::setSpeed( value );
            break;
        case ModelIdDiskDriveStepperSeekTime:
            DiskDrive::setStepperSeekTime( value );
            break;
        case ModelIdDiskDriveStepperAccessTime:
            DiskDrive::setStepperMinTime( value );
            break;
        case ModelIdDiskTurbo:
            system->paula.setTurbo(value);
            break;
        case ModelIdChipMem:
            system->setChipmem(value);
            break;
        case ModelIdSlowMem:
            system->setSlowmem(value);
            break;
        case ModelIdFastMem:
            system->setFastmem(value);
            break;
        case ModelIdRTC:
            system->setRTC(value);
            break;
        case ModelIdSerialLoopback:
            system->paula.loopBack = !!value;
            break;
    }
}

auto Interface::getModelValue(unsigned modelId) -> int {
    switch (modelId) {
        case ModelIdSystem:                         return (int)system->getModel();
        case ModelIdRegion:                         return (int)system->ntsc;
        case ModelIdAudioFilter:                    return system->paula.filterMode;
        case ModelIdSampleFetch:                    return system->paula.getResampleQuality();
        case ModelIdDiskDrivesConnected:            return system->getDrivesEnabled();
        case ModelIdDiskDriveWobble:                return (int)DiskDrive::wobble;
        case ModelIdDiskDriveSpeed:                 return (int)DiskDrive::rpm;
        case ModelIdDiskDriveStepperSeekTime:       return (int)(DiskDrive::stepperSeekTimeBase);
        case ModelIdDiskDriveStepperAccessTime:     return (int)(DiskDrive::stepperMinTimeBase);
        case ModelIdDiskTurbo:                      return (int)system->paula.turboRequested;
        case ModelIdChipMem:                        return system->getChipmem();
        case ModelIdSlowMem:                        return system->getSlowmem();
        case ModelIdFastMem:                        return system->getFastmem();
        case ModelIdRTC:                            return (int)system->useRTC();
        case ModelIdSerialLoopback:                 return (int)system->paula.loopBack;
    }

    return 0;
}

auto Interface::getModelIdOfEnabledDrives(MediaGroup* group) -> unsigned {
    if (group && group->isDisk())
        return ModelIdDiskDrivesConnected;

    return ~0;
}

auto Interface::insertDisk(Media* media, uint8_t* data, unsigned size, bool loadGracefully) -> void {
    if (!media || !media->group->isDisk())
        return;

    system->diskDrives[ media->id ].attach(data, size);
}

auto Interface::writeProtectDisk(Media* media, bool state) -> void {
    if (!media || !media->group->isDisk())
        return;

    system->diskDrives[ media->id ].structure.writeProtected = state;
}

auto Interface::isWriteProtectedDisk(Media* media) -> bool {
    if (!media || !media->group->isDisk())
        return false;

    return system->diskDrives[ media->id ].structure.writeProtected;
}

auto Interface::ejectDisk(Media* media) -> void {
    if (!media || !media->group->isDisk())
        return;

    system->diskDrives[ media->id ].detach();
}

auto Interface::createDiskImage(unsigned typeId, std::string name, bool hd, bool ffs, bool bootable) -> Data {
    return DiskStructure::create( system, (DiskStructure::Type) typeId, name, hd, ffs, bootable );
}

auto Interface::getDiskListing(Media* media) -> std::vector<Emulator::Interface::Listing> {
    if (!media || !media->group->isDisk())
        return {};

    return system->diskDrives[ media->id ].structure.getListing();
}

auto Interface::getDiskPreview(uint8_t* data, unsigned size, Media* media) -> std::vector<Emulator::Interface::Listing> {
    return DiskStructure::getPreview( system, data, size);
}


//auto Interface::createHardDisk(std::function<void (uint8_t* buffer, unsigned length, unsigned offset)> onCreate, unsigned size, std::string name) -> void {
//    unsigned bufferLength = 10u * 1024u * 1024u;
//    if (size > (512u * 1024u * 1024u) ) {
//        bufferLength = 50u * 1024u * 1024u;
//    }
//    uint8_t* data = new uint8_t[bufferLength];
//
//    for ( long long offset = 0; offset < size; offset += bufferLength ) {
//        memset(data, 0, bufferLength);
//
//        unsigned length = bufferLength;
//        if ((offset + bufferLength) > size) {
//            length = size - offset;
//        }
//
//        onCreate(data, length, offset);
//    }
//
//    delete[] data;
//}

auto Interface::savestate(unsigned& size) -> uint8_t* {
    return system->serialize( size );
}
auto Interface::checkstate(uint8_t* data, unsigned size) -> bool {
    return system->checkSerialization( data, size );
}
auto Interface::loadstate(uint8_t* data, unsigned size) -> bool {
    return system->unserialize( data, size );
}

auto Interface::sendKeyChange(bool pressed, Device::Input* input) -> void {
    system->input.keyboard.sendKeyChange(pressed, input);
}

auto Interface::informAboutKeyUpdate() -> void {
    system->informAboutKeyUpdate();
}

auto Interface::setLineCallback(bool state, unsigned scanline) -> void {
    system->agnus.lineCallback.use = state;
    system->agnus.lineCallback.line = scanline;
}

auto Interface::cropFrame( CropType type, Crop crop ) -> void {
    system->crop.settings.type = type;
    system->crop.settings.crop = crop;
}

auto Interface::cropWidth() -> unsigned {
    return system->crop.latest.width;
}

auto Interface::cropHeight() -> unsigned {
    return system->crop.latest.height;
}

auto Interface::cropTop() -> unsigned {
    return system->crop.latest.top;
}

auto Interface::cropLeft() -> unsigned {
    return system->crop.latest.left;
}

auto Interface::cropData16() -> uint16_t* {
    return system->crop.latest.frame;
}

auto Interface::cropPitch() -> unsigned {
    return system->crop.latest.linePitch;
}

auto Interface::requestImmediateReturn() -> void {
    system->leaveEmulation = true;
}

auto Interface::setInputSampling(uint8_t mode) -> void {
    system->input.setSampling( mode );
}

auto Interface::enableFloppySounds(bool state) -> void {
    system->setFloppySounds( state );
}

auto Interface::fastForward(unsigned config) -> void {
    system->setFastForward( config );
}

auto Interface::getForward() -> unsigned {
    return system->fastForward.config;
}

auto Interface::autoStartedByMediaGroup() -> MediaGroup* {

    return getDiskMediaGroup();
}

}
