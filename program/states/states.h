
#pragma once

#include <vector>
#include <string>
#include "../../guikit/api.h"
#include "../tools/filesetting.h"
#include "../view/view.h"
#include "../emuconfig/config.h"
#include "../tools/filepool.h"
#include "../tools/status.h"
#include "../input/manager.h"

struct States {        
    
    States(Emulator::Interface* emulator);
        
    GUIKIT::Settings* saveSettings;
    Emulator::Interface* emulator;
    std::vector<std::string> errorPaths;
    bool forcePowerNextLoad = false;
	    
    struct InsertImage {
        FileSetting* setting;        
        Emulator::Interface::Media* media;
    };

    std::vector<InsertImage> inserted;
    
    struct InsertFirmware {
        FileSetting* setting;        
        Emulator::Interface::Firmware* firmware;
    };
        
    std::vector<InsertFirmware> insertedFirmware;
    
    auto statesFolder() -> std::string;  
    
    auto generateAutoPath() -> std::string;
    
    auto load( std::string path = "", bool prependFolder = false ) -> void;
    
    auto updateConnectedDevices() -> void;
    
    auto updateTapeMenu() -> void;
    
    auto updateModels() -> void;
    
    auto statusMessage( std::string langKey, std::string replacer ) -> void;
    
    auto save( std::string path = "", bool prependFolder = false ) -> void;  
    
    auto loadImagePaths( GUIKIT::Settings* loadSettings ) -> std::vector<Emulator::Interface::Media*>;
    
    auto loadFirmwarePaths( GUIKIT::Settings* loadSettings ) -> void;
    
    auto saveImagePaths( std::string path ) -> bool;
    
    auto updateImage( FileSetting* setting, Emulator::Interface::Media* media ) -> void;
    
    auto findImage( Emulator::Interface::Media* media ) -> InsertImage*;
    
    auto updateFirmware( FileSetting* setting, Emulator::Interface::Firmware* firmware ) -> void;
    
    auto findFirmware( Emulator::Interface::Firmware* firmware ) -> InsertFirmware*;
    
    auto copySetting( FileSetting* target, FileSetting* src ) -> void;
    
    auto changeSlot( bool down ) -> void;    
    
    auto updateSaveable() -> void;
    
    auto updateExpansionJumper() -> void;
    
    auto updateWriteProtection(std::vector<Emulator::Interface::Media*> loadedMedia) -> void;
    
    auto oneMediumOnly(Emulator::Interface::MediaGroup* group, Emulator::Interface::Media* mediaInUse) -> void;
    
    static auto getInstance( Emulator::Interface* emulator ) -> States*;
    static auto getInstanceAuto() -> States*;    
};

extern std::vector<States*> states;
