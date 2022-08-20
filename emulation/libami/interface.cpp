
#include "interface.h"
#include "system/system.h"
#include <cstring>
#include <cstdlib>

namespace LIBAMI {

const std::string Interface::Version = "114";

Interface::Interface() : Emulator::Interface( "Amiga" ) {

    prepareFirmware();
    prepareModels();
    prepareMedia();
    prepareMemory();
    prepareDevices();
    preparePalettes();
    prepareExpansions();

    system = new System( this );
}

auto Interface::prepareFirmware() -> void {
    firmwares.push_back({FirmwareIdKick, "Kickrom"});
    firmwares.push_back({FirmwareIdExt, "Extrom"});
}

auto Interface::prepareModels() -> void {
    models.push_back({ModelIdLowPassFilter, "Low Pass Filter", Model::Type::Switch, Model::Purpose::AudioSettings, 1}); //0 - off, 1 - on, means software decides
}

auto Interface::prepareMedia() -> void {
    mediaGroups.push_back({MediaGroupIdDisk, "disk", MediaGroup::Type::Disk, {"adf", "adz"}, {"adf"} });
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

auto Interface::prepareMemory() -> void {
    memoryTypes.push_back( {0, "Chip", 1} ); // built in memory
    memoryTypes.push_back( {1, "Slow", 0} ); // extra memory slot in trapdoor of amiga, not expansion port
    // memoryTypes.push_back( {2, "Fast", 0} ); // fast mem defined by inserted expansion

    {   auto& memory = memoryTypes[0].memory;
        memory.push_back( {0, 256} );
        memory.push_back( {1, 512} );
        memory.push_back( {2, 1024} );
        memory.push_back( {3, 2048} );
    }

    {   auto& memory = memoryTypes[1].memory;
        memory.push_back( {0, 0} );
        memory.push_back( {1, 512} );
        memory.push_back( {2, 1024} );
        memory.push_back( {3, 1536} );
        memory.push_back( {4, 1792} );
    }

//    {   auto& memory = memoryTypes[2].memory;
//        memory.push_back( {0, 1024} );
//        memory.push_back( {1, 2048} );
//        memory.push_back( {2, 4096} );
//        memory.push_back( {3, 8192} );
//    }
}

auto Interface::prepareExpansions() -> void {

    //expansions.push_back( { ExpansionIdNone, "Empty", Expansion::Type::Empty, nullptr, nullptr } );
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
        device.inputs.push_back( {0, "A", Key::A} ); device.inputs.push_back( {1, "B", Key::B} );
        device.inputs.push_back( {2, "C", Key::C} ); device.inputs.push_back( {3, "D", Key::D} );
        device.inputs.push_back( {4, "E", Key::E} ); device.inputs.push_back( {5, "F", Key::F} );
        device.inputs.push_back( {6, "G", Key::G} ); device.inputs.push_back( {7, "H", Key::H} );
        device.inputs.push_back( {8, "I", Key::I} ); device.inputs.push_back( {9, "J", Key::J} );
        device.inputs.push_back( {10, "K", Key::K} ); device.inputs.push_back( {11, "L", Key::L} );
        device.inputs.push_back( {12, "M", Key::M} ); device.inputs.push_back( {13, "N", Key::N} );
        device.inputs.push_back( {14, "O", Key::O} ); device.inputs.push_back( {15, "P", Key::P} );
        device.inputs.push_back( {16, "Q", Key::Q} ); device.inputs.push_back( {17, "R", Key::R} );
        device.inputs.push_back( {18, "S", Key::S} ); device.inputs.push_back( {19, "T", Key::T} );
        device.inputs.push_back( {20, "U", Key::U} ); device.inputs.push_back( {21, "V", Key::V} );
        device.inputs.push_back( {22, "W", Key::W} ); device.inputs.push_back( {23, "X", Key::X} );
        device.inputs.push_back( {24, "Y", Key::Y} ); device.inputs.push_back( {25, "Z", Key::Z} );

        device.inputs.push_back( {26, "0", Key::D0} ); device.inputs.push_back( {27, "1", Key::D1} );
        device.inputs.push_back( {28, "2", Key::D2} ); device.inputs.push_back( {29, "3", Key::D3} );
        device.inputs.push_back( {30, "4", Key::D4} ); device.inputs.push_back( {31, "5", Key::D5} );
        device.inputs.push_back( {32, "6", Key::D6} ); device.inputs.push_back( {33, "7", Key::D7} );
        device.inputs.push_back( {34, "8", Key::D8} ); device.inputs.push_back( {35, "9", Key::D9} );

        device.inputs.push_back( {36, "Keypad 0", Key::NumPad0} ); device.inputs.push_back( {37, "Keypad 1", Key::NumPad1} );
        device.inputs.push_back( {38, "Keypad 2", Key::NumPad2} ); device.inputs.push_back( {39, "Keypad 3", Key::NumPad3} );
        device.inputs.push_back( {40, "Keypad 4", Key::NumPad4} ); device.inputs.push_back( {41, "Keypad 5", Key::NumPad5} );
        device.inputs.push_back( {42, "Keypad 6", Key::NumPad6} ); device.inputs.push_back( {43, "Keypad 7", Key::NumPad7} );
        device.inputs.push_back( {44, "Keypad 8", Key::NumPad8} ); device.inputs.push_back( {45, "Keypad 9", Key::NumPad9} );
        device.inputs.push_back( {46, "Keypad NumLock", Key::NumLock, { {Device::Layout::Uk, "Keypad ("}, {Device::Layout::Us, "Keypad ("}, {Device::Layout::De, "Keypad ["} } } );
        device.inputs.push_back( {47, "Keypad ScrollLock", Key::ScrollLock, { {Device::Layout::Uk, "Keypad )"}, {Device::Layout::Us, "Keypad )"}, {Device::Layout::De, "Keypad ]"} }} );
        device.inputs.push_back( {48, "Keypad /", Key::NumDivide} );
        device.inputs.push_back( {49, "Keypad *", Key::NumMultiply} );
        device.inputs.push_back( {50, "Keypad -", Key::NumSubtract} );
        device.inputs.push_back( {51, "Keypad +", Key::NumAdd} );
        device.inputs.push_back( {52, "Keypad enter", Key::NumEnter} );
        device.inputs.push_back( {53, "Keypad del", Key::NumComma} );

        device.inputs.push_back( {54, "F1", Key::F1} ); device.inputs.push_back( {55, "F2", Key::F2} );
        device.inputs.push_back( {56, "F3", Key::F3} ); device.inputs.push_back( {57, "F4", Key::F4} );
        device.inputs.push_back( {58, "F5", Key::F5} ); device.inputs.push_back( {59, "F6", Key::F6} );
        device.inputs.push_back( {60, "F7", Key::F7} ); device.inputs.push_back( {61, "F8", Key::F8} );
        device.inputs.push_back( {62, "F9", Key::F9} ); device.inputs.push_back( {63, "F10", Key::F10} );

        device.inputs.push_back( {64, "Cursor Up", Key::CursorUp} );
        device.inputs.push_back( {65, "Cursor Down", Key::CursorDown} );
        device.inputs.push_back( {66, "Cursor Left", Key::CursorLeft} );
        device.inputs.push_back( {67, "Cursor Right", Key::CursorRight} );

        device.inputs.push_back( {68, "Space", Key::Space} ); device.inputs.push_back( {69, "Backspace", Key::Backspace} );
        device.inputs.push_back( {70, "Tab", Key::Tab} ); device.inputs.push_back( {71, "Return", Key::Return} );
        device.inputs.push_back( {72, "Esc", Key::Esc} ); device.inputs.push_back( {73, "Del", Key::Del} );
        device.inputs.push_back( {74, "Left Shift", Key::ShiftLeft} ); device.inputs.push_back( {75, "Right Shift", Key::ShiftRight} );
        device.inputs.push_back( {76, "Capslock", Key::ShiftLock} ); device.inputs.push_back( {77, "Ctrl", Key::ControlLeft} );
        device.inputs.push_back( {78, "Left Alt", Key::AltLeft} ); device.inputs.push_back( {79, "Right Alt", Key::AltRight} );
        device.inputs.push_back( {80, "Left Ami", Key::SystemLeft} ); device.inputs.push_back( {81, "Right Ami", Key::SystemRight} );
        device.inputs.push_back( {82, "Help", Key::Help} );
        // these keys differs between keyboard layouts, we use the uk keyboard layout for identifying the keys
        device.inputs.push_back( {83, "shared 1", Key::Shared1, { {Device::Layout::Uk, "["}, {Device::Layout::Us, "["}, {Device::Layout::De, "ü"}, {Device::Layout::Fr, "è"} } } );
        device.inputs.push_back( {84, "shared 2", Key::Shared2, { {Device::Layout::Uk, "]"}, {Device::Layout::Us, "]"}, {Device::Layout::De, "+"}, {Device::Layout::Fr, "+"} } } );
        device.inputs.push_back( {85, "shared 3", Key::Shared3, { {Device::Layout::Uk, ";"}, {Device::Layout::Us, ";"}, {Device::Layout::De, "ö"}, {Device::Layout::Fr, "ò"} } } );
        device.inputs.push_back( {86, "shared 4", Key::Shared4, { {Device::Layout::Uk, ","}, {Device::Layout::Us, ","}, {Device::Layout::De, ","}, {Device::Layout::Fr, ","} } } );
        device.inputs.push_back( {87, "shared 5", Key::Shared5, { {Device::Layout::Uk, "."}, {Device::Layout::Us, "."}, {Device::Layout::De, "."}, {Device::Layout::Fr, "."} } } );
        device.inputs.push_back( {88, "shared 6", Key::Shared6, { {Device::Layout::Uk, "/"}, {Device::Layout::Us, "/"}, {Device::Layout::De, "-"}, {Device::Layout::Fr, "-"} } } );
        device.inputs.push_back( {89, "shared 7", Key::Shared7, { {Device::Layout::Uk, "\\"}, {Device::Layout::Us, "\\"}, {Device::Layout::De, "\\"}, {Device::Layout::Fr, "\\"} } } );
        device.inputs.push_back( {90, "shared 8", Key::Shared8, { {Device::Layout::Uk, "`"}, {Device::Layout::Us, "`"}, {Device::Layout::De, "`"}, {Device::Layout::Fr, "`"} }} );
        device.inputs.push_back( {91, "shared 9", Key::Shared9, { {Device::Layout::Uk, "#"}, {Device::Layout::Us, "´"}, {Device::Layout::De, "ä"}, {Device::Layout::Fr, "à"} } } );
        device.inputs.push_back( {92, "shared 10", Key::Shared10, { {Device::Layout::Uk, "-"}, {Device::Layout::Us, "-"}, {Device::Layout::De, "ß"}, {Device::Layout::Fr, "´"} } } );
        device.inputs.push_back( {93, "shared 11", Key::Shared11, { {Device::Layout::Uk, "="}, {Device::Layout::Us, "="}, {Device::Layout::De, "´"}, {Device::Layout::Fr, "ì"} } } );
        // 2 more keys on some layouts, i.e. french, german layout
        device.inputs.push_back( {94, "extra 1", Key::Shared12, { {Device::Layout::De, "#"}, {Device::Layout::Fr, "ù"} } } );
        device.inputs.push_back( {95, "extra 2", Key::Shared13, { {Device::Layout::De, "<"}, {Device::Layout::Fr, "<"} } } );

        // virtual inputs (no physical keys)
        // for amiga there are different layouts
        device.addVirtual( "shift + 1", { 27, 75 }, Key::ShiftAnd1, { {Device::Layout::Uk, "!"}, {Device::Layout::Us, "!"}, {Device::Layout::De, "!"}, {Device::Layout::Fr, "!"} } );
        device.addVirtual( "shift + 2", { 28, 75 }, Key::ShiftAnd2, { {Device::Layout::Uk, "\""}, {Device::Layout::Us, "@"}, {Device::Layout::De, "\""}, {Device::Layout::Fr, "\""} } );
        device.addVirtual( "shift + 3", { 29, 75 }, Key::ShiftAnd3, { {Device::Layout::Uk, "£"}, {Device::Layout::Us, "#"}, {Device::Layout::De, "§"}, {Device::Layout::Fr, "£"} } );
        device.addVirtual( "shift + 4", { 30, 75 }, Key::ShiftAnd4, { {Device::Layout::Uk, "$"}, {Device::Layout::Us, "$"}, {Device::Layout::De, "$"}, {Device::Layout::Fr, "$"} } );
        device.addVirtual( "shift + 5", { 31, 75 }, Key::ShiftAnd5, { {Device::Layout::Uk, "%"}, {Device::Layout::Us, "%"}, {Device::Layout::De, "%"}, {Device::Layout::Fr, "%"} } );
        device.addVirtual( "shift + 6", { 32, 75 }, Key::ShiftAnd6, { {Device::Layout::Uk, "^"}, {Device::Layout::Us, "^"}, {Device::Layout::De, "&"}, {Device::Layout::Fr, "&"} } );
        device.addVirtual( "shift + 7", { 33, 75 }, Key::ShiftAnd7, { {Device::Layout::Uk, "&"}, {Device::Layout::Us, "&"}, {Device::Layout::De, "/"}, {Device::Layout::Fr, "/"} } );
        device.addVirtual( "shift + 8", { 34, 75 }, Key::ShiftAnd8, { {Device::Layout::Uk, "*"}, {Device::Layout::Us, "*"}, {Device::Layout::De, "("}, {Device::Layout::Fr, "("} } );
        device.addVirtual( "shift + 9", { 35, 75 }, Key::ShiftAnd9, { {Device::Layout::Uk, "("}, {Device::Layout::Us, "("}, {Device::Layout::De, ")"}, {Device::Layout::Fr, ")"} } );
        device.addVirtual( "shift + 0", { 26, 75 }, Key::ShiftAnd0, { {Device::Layout::Uk, ")"}, {Device::Layout::Us, ")"}, {Device::Layout::De, "="}, {Device::Layout::Fr, "="} } );

        device.addVirtual( "alt + 2", { 28, 79 }, Key::AltAnd2, { {Device::Layout::De, "@"} } );

        device.addVirtual( "shift + shared 1", { 83, 75 }, Key::ShiftShared1, { {Device::Layout::Uk, "{"}, {Device::Layout::Us, "{"}, {Device::Layout::De, "shift + ü"}, {Device::Layout::Fr, "é"} } );
        device.addVirtual( "shift + shared 2", { 84, 75 }, Key::ShiftShared2, { {Device::Layout::Uk, "}"}, {Device::Layout::Us, "}"}, {Device::Layout::De, "*"}, {Device::Layout::Fr, "*"} } );
        device.addVirtual( "shift + shared 3", { 85, 75 }, Key::ShiftShared3, { {Device::Layout::Uk, ":"}, {Device::Layout::Us, ":"}, {Device::Layout::De, "shift + ö"}, {Device::Layout::Fr, "@"} } );
        device.addVirtual( "shift + shared 4", { 86, 75 }, Key::ShiftShared4, { {Device::Layout::Uk, "<"}, {Device::Layout::Us, "<"}, {Device::Layout::De, ";"}, {Device::Layout::Fr, ";"} } );
        device.addVirtual( "shift + shared 5", { 87, 75 }, Key::ShiftShared5, { {Device::Layout::Uk, ">"}, {Device::Layout::Us, ">"}, {Device::Layout::De, ":"}, {Device::Layout::Fr, ":"} } );
        device.addVirtual( "shift + shared 6", { 88, 75 }, Key::ShiftShared6, { {Device::Layout::Uk, "?"}, {Device::Layout::Us, "?"}, {Device::Layout::De, "_"}, {Device::Layout::Fr, "_"} } );
        device.addVirtual( "shift + shared 7", { 89, 75 }, Key::ShiftShared7, { {Device::Layout::Uk, "|"}, {Device::Layout::Us, "|"}, {Device::Layout::De, "|"}, {Device::Layout::Fr, "|"} } );
        device.addVirtual( "shift + shared 8", { 90, 75 }, Key::ShiftShared8, { {Device::Layout::Uk, "~"}, {Device::Layout::Us, "~"}, {Device::Layout::De, "~"}, {Device::Layout::Fr, "~"} } );
        device.addVirtual( "shift + shared 9", { 91, 75 }, Key::ShiftShared9, { {Device::Layout::Uk, "@"}, {Device::Layout::Us, "\""}, {Device::Layout::De, "shift + ä"}, {Device::Layout::Fr, "#"} } );
        device.addVirtual( "shift + shared 10", { 92, 75 }, Key::ShiftShared10, { {Device::Layout::Uk, "_"}, {Device::Layout::Us, "_"}, {Device::Layout::De, "?"}, {Device::Layout::Fr, "?"} } );
        device.addVirtual( "shift + shared 11", { 93, 75 }, Key::ShiftShared11, { {Device::Layout::Uk, "+"}, {Device::Layout::Us, "+"}, {Device::Layout::De, "`"}, {Device::Layout::Fr, "^"} } );
        device.addVirtual( "shift + extra 1", { 94, 75 }, Key::ShiftShared12, { {Device::Layout::De, "^"}, {Device::Layout::Fr, "§"} } );
        device.addVirtual( "shift + extra 2", { 95, 75 }, Key::ShiftShared13, { {Device::Layout::De, ">"}, {Device::Layout::Fr, ">"} } );

        device.addVirtual( "shift + num lock", { 46, 75 }, Key::ShiftNumLock, { {Device::Layout::De, "Keypad {"}, {Device::Layout::Fr, "Keypad {"} } );
        device.addVirtual( "shift + scroll lock", { 47, 75 }, Key::ShiftScrollLock, {  {Device::Layout::De, "Keypad }"}, {Device::Layout::Fr, "Keypad }"} } );

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

    palettes.push_back({ 0, "Default", false });

    for (unsigned i = 0; i < 4096; i++) {
        uint32_t color = ((i & 0xf) << 0) | ((i & 0xf) << 4)
                         | ((i & 0xf0) << 4) | ((i & 0xf0) << 8)
                         | ((i & 0xf00) << 8) | ((i & 0xf00) << 12);

        palettes[0].paletteColors.push_back( {"", color} );

        palettes[0].paletteColors.back().updateChannels();
    }
}

auto Interface::connect(unsigned connectorId, unsigned deviceId) -> void {

}

auto Interface::connect(Connector* connector, Device* device) -> void {

}

auto Interface::getConnectedDevice( Connector* connector ) -> Device* {

    return getUnplugDevice();
}

auto Interface::setMemory(MemoryType* memoryType, unsigned memoryId) -> void {
    if (!memoryType)
        return;

    if (memoryId >= memoryType->memory.size())
        memoryId = memoryType->defaultMemoryId;

    auto memory = getMemoryById(*memoryType, memoryId);

    system->agnus.setMemory( memoryType->id, memory->size );
}

auto Interface::setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void {
    if (typeId >= firmwares.size()) return;

    system->setFirmware( typeId, data, size );
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

auto Interface::setModelValue(unsigned modelId, int value) -> void {
    return;
}

auto Interface::insertDisk(Media* media, uint8_t* data, unsigned size, bool loadGracefully) -> void {
    if (!media || !media->group->isDisk())
        return;
}

auto Interface::writeProtectDisk(Media* media, bool state) -> void {
    if (!media || !media->group->isDisk())
        return;
}

auto Interface::ejectDisk(Media* media) -> void {
    if (!media || !media->group->isDisk())
        return;
}

auto Interface::insertHardDisk(Media* media, unsigned size) -> void {
    if (!media || !media->group->isHardDisk())
        return;
}

auto Interface::ejectHardDisk(Media* media) -> void {
    if (!media || !media->group->isHardDisk())
        return;
}

auto Interface::createDiskImage(unsigned typeId, bool hd, std::string name, bool ffs) -> Data {

    return {nullptr, 0};
}

auto Interface::createHardDisk(std::function<void (uint8_t* buffer, unsigned length, unsigned offset)> onCreate, unsigned size, std::string name) -> void {
    unsigned bufferLength = 10u * 1024u * 1024u;
    if (size > (512u * 1024u * 1024u) ) {
        bufferLength = 50u * 1024u * 1024u;
    }
    uint8_t* data = new uint8_t[bufferLength];

    for ( long long offset = 0; offset < size; offset += bufferLength ) {
        memset(data, 0, bufferLength);

        unsigned length = bufferLength;
        if ((offset + bufferLength) > size) {
            length = size - offset;
        }

        onCreate(data, length, offset);
    }

    delete[] data;
}

auto Interface::savestate() -> uint8_t* {
    return nullptr;
}
auto Interface::checkstate(uint8_t* data, unsigned size) -> bool {
    return false;
}
auto Interface::loadstate(uint8_t* data, unsigned size) -> bool {
    return false;
}

auto Interface::sendKeyChange(bool pressed, Device::Input* input) -> void {
    system->input.keyboard.sendKeyChange(pressed, input);
}

auto Interface::informAboutKeyUpdate() -> void {
    system->informAboutKeyUpdate();
}

}
