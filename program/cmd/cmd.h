
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
	bool aggressiveFastforward = false;
    bool helpRequested = false;
    bool versionRequested = false;
	uint8_t autostartPrg = 1;
    std::string screenshotPath = "";
    std::string diskListing = "";
    
    auto set(int argc, char** argv) -> void;
    
    auto parse() -> void;
    
    auto autoloadImages() -> void;
    
    auto updateModel( Emulator::Interface* emulator, unsigned ident, int value) -> void;

    auto collectAllowedSuffix() -> std::vector<std::string>;
    
    auto getCycles(std::string arg) -> unsigned;
    
    auto setReuSize(std::string arg) -> void;
	
	auto setGeoRamSize(std::string arg) -> void;
    
    auto setAneMagic(std::string arg) -> void;
	
	auto setLaxMagic(std::string arg) -> void;
	
	auto setAutoStartPrg(std::string arg) -> void;
    
    auto printHelp() -> void;
    
    auto saveExitScreenshot() -> void;
};

extern Cmd* cmd;