
#pragma once

#include <vector>
#include "../program.h"

struct Cmd {
    Cmd(int argc, char** argv);

    std::vector<std::string> arguments;

    struct Attachments {
        Emulator::Interface* emulator;
        Emulator::Interface::Media* media;
        std::string path;
    };
    std::vector<Attachments> attachments;

    struct Configs {
        std::string ident;
        std::string path;
    };
    std::vector<Configs> configs;

    bool autoload = false;
    bool attach = false;
    bool noDriver = false;
    bool noGui = false;
    bool debug = false;
	bool aggressiveWarp = false;
    bool helpRequested = false;
    bool versionRequested = false;
	uint8_t autostartPrg = 1;
    std::string screenshotPath = "";
    std::string diskListing = "";
    bool hasContent = false;
    bool startInFullscreen = false;
    
    auto parse() -> void;
    
    auto autoloadImages() -> void;

    auto attachImages() -> void;
    
    auto updateModel( Emulator::Interface* emulator, unsigned ident, int value) -> void;

    auto collectAllowedSuffix() -> std::vector<std::string>;
    
    auto getCycles(std::string arg) -> unsigned;
    
    auto setReuSize(std::string arg) -> void;
	
	auto setGeoRamSize(std::string arg) -> void;

	auto setAutoStartPrg(std::string arg) -> void;

    auto setCustomConfig(std::string& ident, std::string path) -> void;

    auto hasCustomConfig(Emulator::Interface* emulator) -> bool;

    auto getCustomConfig(Emulator::Interface* emulator) -> std::string;

    auto removeCustomConfig(Emulator::Interface* emulator) -> void;
    
    auto printHelp() -> void;
    
    auto saveExitScreenshot() -> void;

    auto recommendPlaceholder() -> bool { return !debug && !noGui && !autoload; }
};

extern Cmd* cmd;