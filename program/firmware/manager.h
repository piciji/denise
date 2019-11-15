
#pragma once

#include "../../emulation/interface.h"

struct FileSetting;

struct FirmwareManager {
    
    const unsigned maxSets = 2;
    
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
    
    auto useImage(Emulator::Interface::Firmware* firmware, unsigned storeLevel) -> bool;
    auto addImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel, uint8_t* data, unsigned size) -> void;
    auto findImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> Image*;
    auto findActiveImage( Emulator::Interface::Firmware* firmware ) -> Image*;
    auto dataInStore( Image* forImage ) -> bool;
    auto loadImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> bool;
    auto insert() -> std::vector<std::string>;
    auto getSetting( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> FileSetting*;
    
    static auto getInstance( Emulator::Interface* emulator ) -> FirmwareManager*;
    FirmwareManager(Emulator::Interface* emulator);
    ~FirmwareManager();
};

extern std::vector<FirmwareManager*> firmwareManagers;