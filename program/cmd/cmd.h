
#pragma once

#include <vector>
#include "../program.h"

struct Cmd {
    
    Cmd(int argc, char** argv) {        
        set(argc, argv);
    }
    
    struct Options {
        std::string ident;
        std::string description;
        std::string param;
    };
    
    std::vector<Options> options;
    
    std::vector<std::string> arguments;

    bool autoload = false;
    bool noDriver = false;
    bool noGui = false;
    bool debug = false;
	bool aggressiveFastforward = false;
    bool helpRequested = false;
    bool versionRequested = false;
	uint8_t autostartPrg = 1;
	bool autostartPrgOverride = false;
    std::string screenshotPath = "";
    
    auto set(int argc, char** argv) -> void;
    
    auto parse() -> void;
    
    auto autoloadImages() -> void;
    
    auto updateModel( Emulator::Interface* emulator, unsigned ident, int value) -> void;
    
    auto prepareDrives( Emulator::Interface* emulator ) -> void;
    
    auto collectAllowedSuffix() -> std::vector<std::string>;
    
    auto setCycles(std::string arg) -> void;
    
    auto setReuSize(std::string arg) -> void;
    
    auto setAneMagic(std::string arg) -> void;
	
	auto setLaxMagic(std::string arg) -> void;
	
	auto setAutoStartPrg(std::string arg) -> void;
    
    auto printHelp() -> void;
    
    auto saveExitScreenshot() -> void;
};

extern Cmd* cmd;