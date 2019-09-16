
#pragma once

#include <vector>
#include "../program.h"

struct Cmd {
    
    Cmd(int argc, char** argv) {        
        set(argc, argv);
    }
    
    std::vector<std::string> arguments;

    bool autoload = false;
    bool noDriver = false;
    bool noGui = false;
    bool debug = false;
    
    auto set(int argc, char** argv) -> void;
    
    auto parse() -> void;
    
    auto autoloadImages() -> void;
    
    auto getEmulator( std::string ident ) -> Emulator::Interface*;
    
    auto updateFeature( Emulator::Interface* emulator, unsigned ident, int value) -> void;
    
    auto updateChipset( Emulator::Interface* emulator, unsigned ident, bool pal) -> void;
    
    auto prepareDrives( Emulator::Interface* emulator ) -> void;
    
    auto collectAllowedSuffix() -> std::vector<std::string>;
};

extern Cmd* cmd;