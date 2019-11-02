
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
    bool lockRegion = false;
    
    auto set(int argc, char** argv) -> void;
    
    auto parse() -> void;
    
    auto autoloadImages() -> void;
    
    auto getEmulator( std::string ident ) -> Emulator::Interface*;
    
    auto updateFeature( Emulator::Interface* emulator, unsigned ident, int value) -> void;
    
    auto updateChipset( Emulator::Interface* emulator, unsigned ident) -> void;
    
    auto updateRegion( Emulator::Interface* emulator, bool pal ) -> void;
    
    auto prepareDrives( Emulator::Interface* emulator ) -> void;
    
    auto collectAllowedSuffix() -> std::vector<std::string>;
    
    auto setCycles(std::string arg) -> void;
    
    auto setReuSize(std::string arg) -> void;
    
    auto setAneMagic(std::string arg) -> void;
};

extern Cmd* cmd;