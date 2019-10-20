
#include "interface.h"
#include "system/system.h"
#include "prg/prg.h"
#include "tape/tape.h"
#include "sid/sid.h"
#include "vic/vicII.h"
#include "input/input.h"
#include "disk/iec.h"
#include "disk/structure/structure.h"
#include "system/gluelogic.h"
#include "../tools/crop.h"

namespace LIBC64 {

const std::string Interface::Version = "106";
    
Interface::Interface() : Emulator::Interface( "C64" ) {        
    system = new System( this );

	prepareMedia();
	prepareFirmware();	
	prepareDevices();
	prepareChipset();
    prepareFeatures(); 
    prepareStats();
    preparePalettes();    
    prepareMemory();
    prepareExpansions();
}

auto Interface::prepareMemory() -> void {
    memoryTypes.push_back( {0, "REU", 0} );
    
    {   auto& memory = memoryTypes[0].memory;
        memory.push_back( {0, 128} );
        memory.push_back( {1, 256} );
        memory.push_back( {2, 512} );
        memory.push_back( {3, 1024} );
        memory.push_back( {4, 2048} );
        memory.push_back( {5, 4096} );
        memory.push_back( {6, 8192} );
        memory.push_back( {7, 16384} );
    }
}

auto Interface::prepareMedia() -> void {
	mediaGroups.push_back({MediaGroupIdDisk, "disk", MediaGroup::Type::Disk, {"d64", "g64"}, {"d64", "g64"} });
	mediaGroups.push_back({MediaGroupIdTape, "tape", MediaGroup::Type::Tape, {"tap"}, {"tap"} });	
	mediaGroups.push_back({MediaGroupIdMemory, "memory", MediaGroup::Type::Memory, {"prg", "p00", "t64"}, {"prg"}});
    mediaGroups.push_back({MediaGroupIdExpansionGame, "game module", MediaGroup::Type::Expansion, {"bin, crt"}, {} });
    mediaGroups.push_back({MediaGroupIdExpansionReu, "reu", MediaGroup::Type::Expansion, {"bin, crt"}, {} });

	{   auto& group = mediaGroups[MediaGroupIdDisk];
    
		group.media.push_back({0, "Device 8", 0, &group, nullptr});
		group.media.push_back({1, "Device 9", 0, &group, nullptr});
		group.media.push_back({2, "Device 10", 0, &group, nullptr});
		group.media.push_back({3, "Device 11", 0, &group, nullptr});
        
        group.selected = nullptr;
        group.expansion = nullptr;
	}

	{   auto& group = mediaGroups[MediaGroupIdTape];
		group.media.push_back({0, "Datasette", 0, &group, nullptr});
        
        group.selected = nullptr;
        group.expansion = nullptr;
	}
	
	{   auto& group = mediaGroups[MediaGroupIdMemory];
		group.media.push_back({0, "Memory", 0, &group, nullptr});
        
        group.selected = nullptr;
        group.expansion = nullptr;
	}
    
    {   auto& group = mediaGroups[MediaGroupIdExpansionGame];
		group.media.push_back({0, "Game Module 0", 0, &group, nullptr});
        group.media.push_back({1, "Game Module 1", 0, &group, nullptr});
        group.media.push_back({2, "Game Module 2", 0, &group, nullptr});
        group.media.push_back({3, "Game Module 3", 0, &group, nullptr});
        
        group.selected = &group.media[0];   
        group.expansion = nullptr;
	}
    
    {   auto& group = mediaGroups[MediaGroupIdExpansionReu];
		group.media.push_back({0, "REU 0", 0, &group, nullptr});
        group.media.push_back({1, "REU 1", 0, &group, nullptr});
        group.media.push_back({2, "REU 2", 0, &group, nullptr});
        group.media.push_back({3, "REU 3", 0, &group, nullptr});
        
        group.selected = &group.media[0];
        group.expansion = nullptr;
	}
}

auto Interface::prepareExpansions() -> void {
    
    expansions.push_back( { ExpansionIdNone, "Empty", Expansion::Type::Empty, nullptr, nullptr } );
    expansions.push_back( { ExpansionIdGame, "Game", Expansion::Type::Game, nullptr, &mediaGroups[MediaGroupIdExpansionGame] } );
    
    {   auto& expansion = expansions.back();        
        expansion.pcbs.push_back( {CartridgeIdDefault, "Default"} );
        expansion.pcbs.push_back( {CartridgeIdDefault8k, "Default8k"} );
        expansion.pcbs.push_back( {CartridgeIdDefault16k, "Default16k"} );
        expansion.pcbs.push_back( {CartridgeIdUltimax, "Ultimax"} );
        expansion.pcbs.push_back( {CartridgeIdOcean, "Ocean"} );
        expansion.pcbs.push_back( {CartridgeIdFunplay, "Funplay"} );
        expansion.pcbs.push_back( {CartridgeIdSuperGames, "SuperGames"} );
        expansion.pcbs.push_back( {CartridgeIdSystem3, "System3"} );
        expansion.pcbs.push_back( {CartridgeIdZaxxon, "Zaxxon"} );
        
        mediaGroups[MediaGroupIdExpansionGame].expansion = &expansion;
    }
    
    expansions.push_back( { ExpansionIdReu, "REU", Expansion::Type::Ram, &memoryTypes[0], &mediaGroups[MediaGroupIdExpansionReu] } );     
    
    {   auto& expansion = expansions.back();  
        mediaGroups[MediaGroupIdExpansionReu].expansion = &expansion;
    }
}

auto Interface::prepareStats() -> void {
    
    stats.push_back( { Region::Pal, (double)C64_FREQUENCY_PAL / (double)SID_SAMPLE_COUNTER, 50.1245, false } );
    stats.push_back( { Region::Ntsc, (double)C64_FREQUENCY_NTSC / (double)SID_SAMPLE_COUNTER, 59.826, false } );
}

auto Interface::preparePalettes() -> void {
    
    palettes.push_back({ 0, "Colodore PAL", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x813338}, {"Cyan", 0x75cec8},
        {"Purple", 0x8e3c97}, {"Green", 0x56ac4d}, {"Blue", 0x2e2c9b}, {"Yellow", 0xedf071},
        {"Orange", 0x8e5029}, {"Brown", 0x553800}, {"Light Red", 0xc46c71}, {"Dark Gray", 0x4a4a4a},
        {"Medium Gray", 0x7b7b7b}, {"Light Green", 0xa9ff9f}, {"Light Blue", 0x706deb}, {"Light Gray", 0xb2b2b2}
    } });
    
    palettes.push_back({ 1, "Community Colors", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0xaf2a29}, {"Cyan", 0x6ed8cc},
        {"Purple", 0xb03fb6}, {"Green", 0x4ac64a}, {"Blue", 0x3739c4}, {"Yellow", 0xe4ed4e},
        {"Orange", 0xb6591c}, {"Brown", 0x683808}, {"Light Red", 0xea746c}, {"Dark Gray", 0x4d4d4d},
        {"Medium Gray", 0x848484}, {"Light Green", 0xa6fa9e}, {"Light Blue", 0x707ce6}, {"Light Gray", 0xb6b6b5}
    } });
    
    palettes.push_back({ 2, "Pepto PAL", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x68372b}, {"Cyan", 0x70a4b2},
        {"Purple", 0x6f3d86}, {"Green", 0x588d43}, {"Blue", 0x352879}, {"Yellow", 0xb8c76f},
        {"Orange", 0x6f4f25}, {"Brown", 0x433900}, {"Light Red", 0x9a6759}, {"Dark Gray", 0x444444},
        {"Medium Gray", 0x6c6c6c}, {"Light Green", 0x9ad284}, {"Light Blue", 0x6c5eb5}, {"Light Gray", 0x959595}
    } });
    
    palettes.push_back({ 3, "Pepto PAL old VIC's", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x58291d}, {"Cyan", 0x91c6d5},
        {"Purple", 0x915ca8}, {"Green", 0x588d43}, {"Blue", 0x352879}, {"Yellow", 0xb8c76f},
        {"Orange", 0x916f43}, {"Brown", 0x433900}, {"Light Red", 0x9a6759}, {"Dark Gray", 0x353535},
        {"Medium Gray", 0x747474}, {"Light Green", 0x9ad284}, {"Light Blue", 0x7466be}, {"Light Gray", 0xb8b8b8}
    } });
    
    palettes.push_back({ 4, "Pepto NTSC", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x67372B}, {"Cyan", 0x70A3B1},
        {"Purple", 0x6F3D86}, {"Green", 0x588C42}, {"Blue", 0x342879}, {"Yellow", 0xB7C66E},
        {"Orange", 0x6F4E25}, {"Brown", 0x423800}, {"Light Red", 0x996659}, {"Dark Gray", 0x434343},
        {"Medium Gray", 0x6B6B6B}, {"Light Green", 0x9AD183}, {"Light Blue", 0x6B5EB5}, {"Light Gray", 0x959595}
    } });
    
    palettes.push_back({ 5, "Pepto NTSC Sony", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x7C352B}, {"Cyan", 0x5AA6B1},
        {"Purple", 0x694185}, {"Green", 0x5D8643}, {"Blue", 0x212E78}, {"Yellow", 0xCFBE6F},
        {"Orange", 0x894A26}, {"Brown", 0x5B3300}, {"Light Red", 0xAF6459}, {"Dark Gray", 0x434343},
        {"Medium Gray", 0x6B6B6B}, {"Light Green", 0xA0CB84}, {"Light Blue", 0x5665B3}, {"Light Gray", 0x959595}
    } });
    
    palettes.push_back({ 6, "Go Dot", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x880000}, {"Cyan", 0xAAFFEE},
        {"Purple", 0xCC44CC}, {"Green", 0x00CC55}, {"Blue", 0x0000AA}, {"Yellow", 0xEEEE77},
        {"Orange", 0xDD8855}, {"Brown", 0x664400}, {"Light Red", 0xFE7777}, {"Dark Gray", 0x333333},
        {"Medium Gray", 0x777777}, {"Light Green", 0xAAFF66}, {"Light Blue", 0x0088FF}, {"Light Gray", 0xBBBBBB}
    } });
    
    palettes.push_back({ 7, "Christopher Jam", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x7d202c}, {"Cyan", 0x4fb3a5},
        {"Purple", 0x84258c}, {"Green", 0x339840}, {"Blue", 0x2a1b9d}, {"Yellow", 0xbfd04a},
        {"Orange", 0x7f410d}, {"Brown", 0x4c2e00}, {"Light Red", 0xb44f5c}, {"Dark Gray", 0x3c3c3c},
        {"Medium Gray", 0x646464}, {"Light Green", 0x7ce587}, {"Light Blue", 0x6351db}, {"Light Gray", 0x939393}
    } });

    palettes.push_back({ 8, "RGB", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0xFF0000}, {"Cyan", 0x00FFFF},
        {"Purple", 0xFF00FF}, {"Green", 0x00FF00}, {"Blue", 0x0000FF}, {"Yellow", 0xFFFF00},
        {"Orange", 0xFF8000}, {"Brown", 0x804000}, {"Light Red", 0xFF8080}, {"Dark Gray", 0x404040},
        {"Medium Gray", 0x808080}, {"Light Green", 0x80FF80}, {"Light Blue", 0x8080FF}, {"Light Gray", 0xC0C0C0}
    } });
          
    palettes.push_back({ 9, "C64 HQ", false, {
        {"Black", 0x0A0A0A}, {"White", 0xFFF8FF}, {"Red", 0x851F02}, {"Cyan", 0x65CDA8},
        {"Purple", 0xA73B9F}, {"Green", 0x4DAB19}, {"Blue", 0x1A0C92}, {"Yellow", 0xEBE353},
        {"Orange", 0xA94B02}, {"Brown", 0x441E00}, {"Light Red", 0xD28074}, {"Dark Gray", 0x464646},
        {"Medium Gray", 0x8B8B8B}, {"Light Green", 0x8EF68E}, {"Light Blue", 0x4D91D1}, {"Light Gray", 0xBABABA}
    } });
    
    palettes.push_back({ 10, "C64s", false, {
        {"Black", 0x000000}, {"White", 0xFCFCFC}, {"Red", 0xA80000}, {"Cyan", 0x54FCFC},
        {"Purple", 0xA800A8}, {"Green", 0x00A800}, {"Blue", 0x0000A8}, {"Yellow", 0xFCFC00},
        {"Orange", 0xA85400}, {"Brown", 0x802C00}, {"Light Red", 0xFC5454}, {"Dark Gray", 0x545454},
        {"Medium Gray", 0x808080}, {"Light Green", 0x54FC54}, {"Light Blue", 0x5454FC}, {"Light Gray", 0xA8A8A8}
    } });
    
    palettes.push_back({ 11, "Ccs64", false, {
        {"Black", 0x101010}, {"White", 0xFFFFFF}, {"Red", 0xE04040}, {"Cyan", 0x60FFFF},
        {"Purple", 0xE060E0}, {"Green", 0x40E040}, {"Blue", 0x4040E0}, {"Yellow", 0xFFFF40},
        {"Orange", 0xE0A040}, {"Brown", 0x9C7448}, {"Light Red", 0xFFA0A0}, {"Dark Gray", 0x545454},
        {"Medium Gray", 0x888888}, {"Light Green", 0xA0FFA0}, {"Light Blue", 0xA0A0FF}, {"Light Gray", 0xC0C0C0}
    } });
    
    palettes.push_back({ 12, "Frodo", false, {
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Red", 0xCC0000}, {"Cyan", 0x00FFCC},
        {"Purple", 0xFF00FF}, {"Green", 0x00CC00}, {"Blue", 0x0000CC}, {"Yellow", 0xFFFF00},
        {"Orange", 0xFF8800}, {"Brown", 0x884400}, {"Light Red", 0xFF8888}, {"Dark Gray", 0x444444},
        {"Medium Gray", 0x888888}, {"Light Green", 0x88FF88}, {"Light Blue", 0x8888FF}, {"Light Gray", 0xCCCCCC}
    } });
    
    palettes.push_back({ 13, "PC64", false, {
        {"Black", 0x212121}, {"White", 0xFFFFFF}, {"Red", 0xB52121}, {"Cyan", 0x73FFFF},
        {"Purple", 0xB521B5}, {"Green", 0x21B521}, {"Blue", 0x2121B5}, {"Yellow", 0xFFFF21},
        {"Orange", 0xB57321}, {"Brown", 0x944221}, {"Light Red", 0xFF7373}, {"Dark Gray", 0x737373},
        {"Medium Gray", 0x949494}, {"Light Green", 0x73FF73}, {"Light Blue", 0x7373FF}, {"Light Gray", 0xB5B5B5}
    } });
    
    palettes.push_back({ 14, "Deekay", false, {
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Red", 0x882000}, {"Cyan", 0x68D0A8},
        {"Purple", 0xA838A0}, {"Green", 0x50B818}, {"Blue", 0x181090}, {"Yellow", 0xF0E858},
        {"Orange", 0xA04800}, {"Brown", 0x472B1B}, {"Light Red", 0xC87870}, {"Dark Gray", 0x484848},
        {"Medium Gray", 0x808080}, {"Light Green", 0x98FF98}, {"Light Blue", 0x5090D0}, {"Light Gray", 0xB8B8B8}
    } });
    
    palettes.push_back({ 15, "Ptoing", false, {
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Red", 0x8C3E34}, {"Cyan", 0x7ABFC7},
        {"Purple", 0x8D47B3}, {"Green", 0x68A941}, {"Blue", 0x3E31A2}, {"Yellow", 0xD0DC71},
        {"Orange", 0x905F25}, {"Brown", 0x574200}, {"Light Red", 0xBB776D}, {"Dark Gray", 0x545454},
        {"Medium Gray", 0x808080}, {"Light Green", 0xACEA88}, {"Light Blue", 0x7C70DA}, {"Light Gray", 0xABABAB}
    } });   

    for( auto& palette : palettes )
        for( auto& paletteColor : palette.paletteColors )
            paletteColor.updateChannels();
}

auto Interface::prepareFeatures() -> void {
	// use old Sid 6581 instead of the newer 8580
	features.push_back({FeatureIdSid, "Sid 6581/8580", Feature::Type::Switch, 0, true, false});	
	// 0 - off, 1 - on, means software decides
    features.push_back({FeatureIdFilter, "Sid Filter", Feature::Type::Switch, 1, true, false});
	// amplifies Sid 8580 digi sounds
	features.push_back({FeatureIdDigiboost, "Sid 8580 Digi Boost", Feature::Type::Switch, 0, true, false});
	// adjust center frequency for Sid 6581
	features.push_back({FeatureIdBias, "Sid Filter Bias", Feature::Type::Range, 500, true, false, {-5000, 5000} });
	// experimental accuracy: runs in concurrent thread beacause of on the fly calculation
	// each voltage input for mixer / filter is modelled as individual transistor
	features.push_back({FeatureIdSidAccuracy, "Sid Hazard", Feature::Type::Switch, 0, true, true});
    // distinguish between old and new ( 6526a ) cia chips
    features.push_back({FeatureIdCiaRev, "Cia 6526a/6526", Feature::Type::Switch, 1, false, false});
    // ANE magic byte value depends on cpu manufacturer and unemulatable behaviour like heat
    features.push_back({FeatureIdCpuAneMagic, "ANE Magic Byte", Feature::Type::Hex, 0xee, false, false, { 0, 0xff }});
    // c64c use custom ic instead of discrete glue logic
    features.push_back({FeatureIdGlueLogic, "Custom IC Glue Logic", Feature::Type::Switch, 0, false, false});
    // disk drive thread consumes a single core 100%, usefull when emulating more than two drives
    features.push_back({FeatureIdPowerThread, "Disk Core 100%", Feature::Type::Switch, 0, true, true});
}

auto Interface::prepareChipset() -> void {    
    chipsets.push_back({0, "VIC-II 65xx"});
	chipsets.push_back({1, "VIC-II 85xx"});
}

auto Interface::prepareFirmware() -> void {
	firmwares.push_back({0, "Kernal"});
	firmwares.push_back({1, "Basic"});
	firmwares.push_back({2, "Char"});
    firmwares.push_back({3, "VC1541-II"});
}

auto Interface::prepareDevices() -> void {
	connectors.push_back( {0, "Port 1", Connector::Type::Port1} );
	connectors.push_back( {1, "Port 2", Connector::Type::Port2} );

    unsigned id = 0;
    
	{   Device device{ id++, "Unassigned", Device::Type::None };
        devices.push_back(device);
	}

	{   Device device{ id++, "Joypad #1", Device::Type::Joypad };
		device.inputs.push_back( {0, "Up"} );
		device.inputs.push_back( {1, "Down"} );
		device.inputs.push_back( {2, "Left"} );
		device.inputs.push_back( {3, "Right"} );
		device.inputs.push_back( {4, "Button 1"} );
        
        devices.push_back(device);
        
        device.id = id++;
        device.name = "Joypad #2";
        devices.push_back(device);
	}

	{   Device device{ id++, "Mouse 1351 #1", Device::Type::Mouse };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Button Left"} );
		device.inputs.push_back( {3, "Button Right"} );

        devices.push_back(device);
        device.id = id++;
        device.name = "Mouse 1351 #2";
        devices.push_back(device);
	}

    {   Device device{ id++, "Mouse Neos #1", Device::Type::Mouse };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Button Left"} );
		device.inputs.push_back( {3, "Button Right"} );

        devices.push_back(device);
        device.id = id++;
        device.name = "Mouse Neos #2";
        devices.push_back(device);
	}
    
    {   Device device{ id++, "Paddles #1", Device::Type::Paddles };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Button X"} );
		device.inputs.push_back( {3, "Button Y"} );

        devices.push_back(device);
        device.id = id++;
        device.name = "Paddles #2";
        devices.push_back(device);
	}
    
    {   Device device{ id++, "Magnum Light Phaser", Device::Type::LightGun };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Trigger"} );

        devices.push_back(device);
	}    
    
    {   Device device{ id++, "Stack Light Rifle", Device::Type::LightGun };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Trigger"} );

        devices.push_back(device);
	}
    
    {   Device device{ id++, "Gun Stick #1", Device::Type::LightGun };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Trigger"} );

        devices.push_back(device);
        
        device.id = id++;
        device.name = "Gun Stick #2";
        devices.push_back(device);
	}
        
    {   Device device{ id++, "Inkwell Light Pen", Device::Type::LightPen };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Touch"} );
        device.inputs.push_back( {3, "Button"} );

        devices.push_back(device);
	}
        
    {   Device device{ id++, "Stack Light Pen", Device::Type::LightPen };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Touch"} );
        device.inputs.push_back( {3, "Button"} );

        devices.push_back(device);
	}
    
	{   Device device{ id++, "Keyboard", Device::Type::Keyboard };
	
		device.inputs.push_back( {0, "0", Key::D0 } ); device.inputs.push_back( {1, "1", Key::D1 } );
		device.inputs.push_back( {2, "2", Key::D2 } ); device.inputs.push_back( {3, "3", Key::D3 } );
		device.inputs.push_back( {4, "4", Key::D4 } ); device.inputs.push_back( {5, "5", Key::D5 } );
		device.inputs.push_back( {6, "6", Key::D6 } ); device.inputs.push_back( {7, "7", Key::D7 } );
		device.inputs.push_back( {8, "8", Key::D8 } ); device.inputs.push_back( {9, "9", Key::D9 } );
        
		device.inputs.push_back( {10, "A", Key::A } ); device.inputs.push_back( {11, "B", Key::B } );
		device.inputs.push_back( {12, "C", Key::C } ); device.inputs.push_back( {13, "D", Key::D } );
		device.inputs.push_back( {14, "E", Key::E } ); device.inputs.push_back( {15, "F", Key::F } );
		device.inputs.push_back( {16, "G", Key::G } ); device.inputs.push_back( {17, "H", Key::H } );
		device.inputs.push_back( {18, "I", Key::I } ); device.inputs.push_back( {19, "J", Key::J } );
		device.inputs.push_back( {20, "K", Key::K } ); device.inputs.push_back( {21, "L", Key::L } );
		device.inputs.push_back( {22, "M", Key::M } ); device.inputs.push_back( {23, "N", Key::N } );
		device.inputs.push_back( {24, "O", Key::O } ); device.inputs.push_back( {25, "P", Key::P } );
		device.inputs.push_back( {26, "Q", Key::Q } ); device.inputs.push_back( {27, "R", Key::R } );
		device.inputs.push_back( {28, "S", Key::S } ); device.inputs.push_back( {29, "T", Key::T } );
		device.inputs.push_back( {30, "U", Key::U } ); device.inputs.push_back( {31, "V", Key::V } );
		device.inputs.push_back( {32, "W", Key::W } ); device.inputs.push_back( {33, "X", Key::X } );
		device.inputs.push_back( {34, "Y", Key::Y } ); device.inputs.push_back( {35, "Z", Key::Z } );
        
		device.inputs.push_back( {36, "F1", Key::F1 } ); device.inputs.push_back( {37, "F3", Key::F3 } );
		device.inputs.push_back( {38, "F5", Key::F5 } ); device.inputs.push_back( {39, "F7", Key::F7 } );
        
		device.inputs.push_back( {40, "Cursor Right", Key::CursorRight } );
		device.inputs.push_back( {41, "Cursor Down", Key::CursorDown } );
		device.inputs.push_back( {42, "Backspace", Key::Backspace } );
        device.inputs.push_back( {43, "Return", Key::Return } ); 
		device.inputs.push_back( {44, "Plus", Key::Plus } );
        device.inputs.push_back( {45, "Minus", Key::Minus } ); 
		device.inputs.push_back( {46, "Left Shift", Key::ShiftLeft } );
		device.inputs.push_back( {47, "Right Shift", Key::ShiftRight } ); 
		device.inputs.push_back( {48, ".", Key::Period } );
        device.inputs.push_back( {49, ",", Key::Comma } );
		device.inputs.push_back( {50, ":", Key::Colon } );
        device.inputs.push_back( {51, ";", Key::Semicolon } ); 
		device.inputs.push_back( {52, "Pound", Key::Pound } ); 
        device.inputs.push_back( {53, "*", Key::Asterisk } ); 
		device.inputs.push_back( {54, "Space", Key::Space } );
        device.inputs.push_back( {55, "Arrow Left", Key::ArrowLeft } ); 
		device.inputs.push_back( {56, "Home", Key::Home } );
        device.inputs.push_back( {57, "Equal", Key::Equal } ); 
		device.inputs.push_back( {58, "C=", Key::Commodore } );
        device.inputs.push_back( {59, "RunStop", Key::RunStop } ); 
		device.inputs.push_back( {60, "Arrow Up", Key::ArrowUp } );
        device.inputs.push_back( {61, "/", Key::Slash } ); 
		device.inputs.push_back( {62, "Ctrl", Key::Ctrl} );
        device.inputs.push_back( {63, "@", Key::At} ); 				
		device.inputs.push_back( {64, "ShiftLock", Key::ShiftLock } ); 
		device.inputs.push_back( {65, "Restore", Key::Restore } );
        
        // virtual inputs (no physical keys)   
        // host could overmap default behaviour to match host keyboard layout better
        // otherwise you have to know the c64 keyboard layout to find the keys
        device.addVirtual( "!", { 1, 47 }, Key::ExclamationMark );
        device.addVirtual( "\"", { 2, 47 }, Key::DoubleQuotes );
        device.addVirtual( "#", { 3, 47 }, Key::NumberSign );
        device.addVirtual( "$", { 4, 47 }, Key::Dollar );
        device.addVirtual( "%", { 5, 47 }, Key::Percent );
        device.addVirtual( "&", { 6, 47 }, Key::Ampersand  );
        device.addVirtual( "´", { 7, 47 }, Key::Acute );
        device.addVirtual( "(", { 8, 47 }, Key::ParenthesesLeft );
        device.addVirtual( ")", { 9, 47 }, Key::ParenthesesRight );        
        device.addVirtual( "F2", { 36, 47 }, Key::F2 );
        device.addVirtual( "F4", { 37, 47 }, Key::F4 );
        device.addVirtual( "F6", { 38, 47 }, Key::F6 );
        device.addVirtual( "F8", { 39, 47 }, Key::F8 );        
        device.addVirtual( "Cursor Left", { 40, 47 }, Key::CursorLeft );
        device.addVirtual( "Cursor Up", { 41, 47 }, Key::CursorUp );        
        device.addVirtual( ">", { 48, 47 }, Key::Greater );
        device.addVirtual( "<", { 49, 47 }, Key::Less );        
        device.addVirtual( "[", { 50, 47 }, Key::OpenSquareBracket );
        device.addVirtual( "]", { 51, 47 }, Key::ClosedSquareBracket );        
        device.addVirtual( "?", { 61, 47 }, Key::QuestionMark );
        device.addVirtual( "|", { 45, 47 }, Key::Pipe );
		
        devices.push_back(device); 
        
        system->input->keyboard.setDevice( &devices.back() );
	}
        
    for (auto& device : devices) {
        device.userData = 0;
        
        for (auto& input : device.inputs)
            input.type = input.name.find( "Axis" ) != std::string::npos ? 1 : 0;    
    }
}

auto Interface::connect(unsigned connectorId, unsigned deviceId) -> void {
    
    system->input->connectControlport( getConnector( connectorId ), getDevice( deviceId ) );
}

auto Interface::connect(Connector* connector, Device* device) -> void {
    
    system->input->connectControlport( connector, device );
}

auto Interface::getConnectedDevice( Connector* connector ) -> Device* {
    
    auto device = system->input->getConnectedDevice( connector );
    
    if (!device)
        return getUnplugDevice();
    
    return device;
}

auto Interface::getCursorPosition( Device* device, int16_t& x, int16_t& y ) -> bool {
    
    return system->input->getCursorPosition( device, x, y );
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

auto Interface::setRegion(Region region) -> void {
    system->setNtsc( region == Region::Ntsc );
}

auto Interface::getRegion() -> Region {
    return system->isNtsc() ? Region::Ntsc : Region::Pal;
}

auto Interface::setDrivesConnected(MediaGroup* group, unsigned count) -> void {
    if (!group)
        return;
    
	if (count > group->media.size())
        count = group->defaultUsage();
	
	if (group->isTape())
		tape->setEnabled( count != 0 );
    
    else if (group->isDisk())
        iecBus->setDrivesEnabled( count );
}

auto Interface::getDrivesConnected(MediaGroup* group) -> unsigned { 
    if (!group)
        return 1;
    
    if (group->isDisk())
        return iecBus->drivesConnected;
    
    if (group->isTape())
        return tape->isEnabled() ? 1 : 0;

    return 1;
}

auto Interface::insertDisk(Media* media, uint8_t* data, unsigned size) -> void {
    
    if (!media || !media->group->isDisk())
        return;
    
    iecBus->attach( media, data, size );    
}

auto Interface::writeProtectDisk(Media* media, bool state) -> void {

    if (!media || !media->group->isDisk())
        return;
    
    iecBus->writeProtect( media, state );
}

auto Interface::ejectDisk(Media* media) -> void {

    if (!media || !media->group->isDisk())
        return;
    
    iecBus->detach( media );
}

auto Interface::createDiskImage(unsigned typeId, bool hd, std::string name, bool ffs) -> uint8_t* {
	
    return Structure1541::create( (Structure1541::Type) typeId, name );
}

auto Interface::getDiskImageSize(unsigned typeId, bool hd) -> unsigned {
	
	return Structure1541::imageSize( (Structure1541::Type) typeId );
}

auto Interface::getDiskListing(Media* media) -> std::vector<Emulator::Interface::Listing> {
    
    if (!media)
        return {};
    
    return iecBus->getDiskListing( media );
}

auto Interface::selectDiskListing(Media* media, unsigned pos) -> void {
    
    iecBus->selectListing( media, pos );
}

auto Interface::insertTape(Media* media, uint8_t* data, unsigned size) -> void {
		
    if (!media || !media->group->isTape())
        return;
    
	tape->load( media, data, size );
}

auto Interface::writeProtectTape(Media* media, bool state) -> void {
	
	tape->setWriteProtect( state );
}

auto Interface::ejectTape(Media* media) -> void {
		
	tape->unload();
}

auto Interface::controlTape(Media* media, TapeMode mode) -> void {
	
    tape->setMode( (Tape::Mode)mode );
}

auto Interface::getTapeControl(Media* media) -> TapeMode {
    
    if (!media)
        return TapeMode::Stop;
    
    return (TapeMode)tape->getMode();
}

auto Interface::selectTapeListing(Media* media, unsigned pos) -> void {
	
	tape->selectListing( pos );
}

auto Interface::createTapeImage(unsigned& imageSize) -> uint8_t* {
	return tape->createTap( imageSize );
}

auto Interface::insertExpansionImage(Media* media, uint8_t* data, unsigned size) -> void {
    
    if (!media || !media->group->isExpansion())
        return;
    
    system->expansionPort->setCartridgeId( (CartridgeId)(media->pcbLayout ? media->pcbLayout->id : 0) );
    
    system->expansionPort->setRom( data, size );
}

auto Interface::ejectExpansionImage(Media* media) -> void {
    
    system->unsetExpansion();
}

auto Interface::insertMemory(Media* media, uint8_t* data, unsigned size) -> void {
		
    if (!media || !media->group->isMemory())
        return;
    
	prg->set( data, size );
}

auto Interface::ejectMemory(Media* media) -> void {
	
	prg->unset();
}

auto Interface::getLoadedMemory(unsigned& size) -> uint8_t* {
	
	return prg->getMemory( size );
}

auto Interface::getMemoryListing(Media* media) -> std::vector<Emulator::Interface::Listing> {
	
	return prg->getListing();
}

auto Interface::selectMemoryListing(Media* media, unsigned pos) -> bool {
	
	return prg->select( pos );
}

auto Interface::convertPetsciiToScreencode(bool state) -> void {
    
    convertToScreencode = state;
}

auto Interface::savestate(unsigned& size) -> uint8_t* {
	return system->serialize( size );
}

auto Interface::checkstate(uint8_t* data, unsigned size) -> bool { 
    return system->checkSerialization( data, size );
}    

auto Interface::loadstate(uint8_t* data, unsigned size) -> bool {
	return system->unserialize( data, size );
}

auto Interface::getMemory(unsigned typeId) -> Memory* {
	if (typeId >= memoryTypes.size())
        return nullptr;
    
	auto& type = memoryTypes[typeId];
    
    return &type.memory[0];
}

auto Interface::setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void {
	if (typeId >= firmwares.size()) return;
    
    system->setFirmware( typeId, data, size );
}

auto Interface::setFeature(unsigned featureId, int value) -> void {    
    switch (featureId) {
		case FeatureIdSid: {
			Sid::Type type = value & 1 ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580;
			sid->setType( type );			
		} break;
        case FeatureIdFilter:
            sid->filter.setEnable( value & 1 );
            break;
		case FeatureIdDigiboost:
            sid->setDigiBoost( value & 1 );
			break;
		case FeatureIdBias:
			sid->filter.adjustFilterBias( value );
			break;
		case FeatureIdSidAccuracy:
			sid->setMoreAccuracy( value & 1 );
			break;
        case FeatureIdCiaRev:
            system->cia1->setNewVersion( value & 1 );
            system->cia2->setNewVersion( value & 1 );
            break;
        case FeatureIdCpuAneMagic:
            //this is annoying ... look in 6502 cpu code for more informations
            system->cpu->setMagicForAne( value & 0xff );
            break;
        case FeatureIdGlueLogic:
            system->glueLogic->setType( (GlueLogic::Type)(value & 1) );
            break;
        case FeatureIdPowerThread:
            iecBus->setPowerThread( value & 1 );
            break;
    }    
}

auto Interface::getFeature(unsigned featureId) -> int {
    
    switch (featureId) {
		case FeatureIdSid:			
            return sid->type == Sid::Type::MOS_6581 ? 1 : 0;
        case FeatureIdFilter:
            return sid->filter.enabled;
		case FeatureIdDigiboost:
            return sid->digiBoost;
		case FeatureIdBias:
			return sid->filter.bias;
		case FeatureIdSidAccuracy:
			return sid->moreAccuracy;
        case FeatureIdCiaRev:
            return system->cia1->isNewVersion();
        case FeatureIdCpuAneMagic:
            return system->cpu->getMagicForAne();
        case FeatureIdGlueLogic:
            return (int)system->glueLogic->type;
        case FeatureIdPowerThread:
            return iecBus->cpuBurner;                
    }    
    return 0;
}

auto Interface::crop( CropType type, bool aspectCorrect, unsigned left, unsigned right, unsigned top, unsigned bottom ) -> void {
	
	system->crop->settings.type = type;
	system->crop->settings.aspectCorrect = aspectCorrect;
	system->crop->settings.left = left;
	system->crop->settings.right = right;
	system->crop->settings.top = top;
	system->crop->settings.bottom = bottom;
}

auto Interface::cropWidth() -> unsigned {
    
    return system->crop->latest.width;
}

auto Interface::cropHeight() -> unsigned {
    
    return system->crop->latest.height;
}

auto Interface::cropTop() -> unsigned {
    
    return system->crop->latest.top;
}

auto Interface::cropLeft() -> unsigned {
    
    return system->crop->latest.left;
}

auto Interface::setChipset(unsigned chipsetId) -> void {
	if (chipsetId >= chipsets.size()) return;
	
	vicII->setRevision65( chipsetId == 0 );
}

auto Interface::getChipset() -> unsigned {
    
    return vicII->isRevision65() ? 0 : 1;
}

auto Interface::activateDebugCart( unsigned limitCycles ) -> void {
    system->setDebugCart( true, limitCycles );    
}

auto Interface::disableFilterCircuit() -> void {
    sid->setDisableFilterMixer();
}

auto Interface::getLuma(uint8_t index, bool newRevision) -> double {
    return vicII->getLuma( index, newRevision );
}

auto Interface::getChroma(uint8_t index) -> double {
    return vicII->getChroma( index );
}

auto Interface::setLineCallback(bool state, unsigned scanline) -> void {
    
	vicII->lineCallback.use = state;
    vicII->lineCallback.line = scanline;	
}

auto Interface::setFinishVblankCallback(bool state) -> void {
    
    vicII->lineCallback.finishVblank = state;
}

auto Interface::setMemory(unsigned typeId, unsigned memoryId) -> void {
    
    for( auto& expansion : expansions ) {
        
        if (expansion.id != system->expansionPort->id)
            continue;
        
        if (expansion.memoryType->id == typeId) {
            
            auto memory = getMemoryById(*(expansion.memoryType), memoryId);
            
            if (memory)
                system->expansionPort->setRam( memory->size );            
        }
        
        break;
    }
}

auto Interface::setExpansion(unsigned expansionId) -> void {
    
    auto expansion = getExpansionById( expansionId );
    
    if (!expansion)
        return;
    
    system->setExpansion( *expansion );
}

auto Interface::getExpansion() -> Expansion* {

    return getExpansionById( system->expansionPort->id );
}

}
