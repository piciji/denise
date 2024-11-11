
#pragma once

#include "../../emulation/interface.h"

struct FileSetting;

struct FirmwareManager {
    
    const unsigned maxSets = 6;
    
    Emulator::Interface* emulator;
    
    struct Image {
        Emulator::Interface::Firmware* firmware;
        uint8_t* data;
        unsigned size;
        unsigned storeLevel;
    };
    
    std::vector<Image> imagesStore;
    std::vector<Image> imagesActive;
    
    std::vector<std::string> missingFirmware;
    bool fallbackToDefaultFirmware = true;
    
    auto useImage(Emulator::Interface::Firmware* firmware, unsigned storeLevel) -> bool;
    auto addImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel, uint8_t* data, unsigned size) -> void;
    auto findImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> Image*;
    auto findActiveImage( Emulator::Interface::Firmware* firmware ) -> Image*;
    auto dataInStore( Image* forImage ) -> bool;
    auto loadImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> bool;
    auto insert(bool onlyKernal = false) -> std::vector<std::string>;
    auto getStoreLevelInUse() -> unsigned;
    auto getStoreLevelInConfig() -> unsigned;
    auto insertDefault(bool onlyKernal = false) -> void;
    auto insertFirmware(Emulator::Interface::Firmware* firmware, unsigned storeLevel) -> void;
    auto getSetting( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> FileSetting*;
	auto swapIn(unsigned storeLevel, int firmwareId = -1) -> std::vector<std::string>;
    auto clear() -> void;
    auto reload() -> void;
    
    static auto getInstance( Emulator::Interface* emulator ) -> FirmwareManager*;
    FirmwareManager(Emulator::Interface* emulator, bool fallbackToDefaultFirmware);
    ~FirmwareManager();
};

extern std::vector<FirmwareManager*> firmwareManagers;