
#include "manager.h"
#include "../tools/filesetting.h"
#include "../program.h"
#include "../states/states.h"
#include "../cmd/cmd.h"
#include <cstring>

std::vector<FirmwareManager*> firmwareManagers;

FirmwareManager::FirmwareManager(Emulator::Interface* emulator) {
    this->emulator = emulator;
}

FirmwareManager::~FirmwareManager() {
    clear();
}

auto FirmwareManager::reload() -> void {

    for (unsigned i = 0; i <= maxSets; i++) {
        for (auto& firmware : emulator->firmwares) {
            auto fSetting = getSetting( &firmware, i );
            if (i != 0) {
                fSetting->update();
            }
        }
    }

    clear();
}

auto FirmwareManager::clear() -> void {
    for (auto& image : imagesStore) {
        auto activeImage = findActiveImage( image.firmware );

        if (activeImage && (image.data == activeImage->data) )
            activeImage->data = nullptr;

        if (image.data)
            delete[] image.data;
    }

    for (auto& image : imagesActive)
        if (image.data)
            delete[] image.data;

    imagesStore.clear();
    imagesActive.clear();
}

auto FirmwareManager::useImage(Emulator::Interface::Firmware* firmware, unsigned storeLevel) -> bool {
    
    auto image = findImage( firmware, storeLevel );
    
	if (!image)
        return false;
	
	if (!cmd->debug && !image->data)
		return false;
    
    auto activeImage = findActiveImage( firmware );
    
    if (!activeImage) {
        imagesActive.push_back({ firmware, image->data, image->size, storeLevel });
        
    } else {
        
        if (activeImage->data) {
            if ( !dataInStore( activeImage ) )
                delete[] activeImage->data;
        }
        
        activeImage->data = image->data;
        activeImage->size = image->size;
        activeImage->storeLevel = storeLevel;
    }     
    
    emulator->setFirmware(firmware->id, image->data, image->size, storeLevel == 0);

    States::getInstance( emulator )->updateFirmware( !storeLevel ? nullptr : getSetting( firmware, storeLevel ), firmware );
    
    return true;
}

auto FirmwareManager::addImage(Emulator::Interface::Firmware* firmware, unsigned storeLevel, uint8_t* data, unsigned size) -> void {
    
    auto image = findImage( firmware, storeLevel );
    
    if (!image) {        
        imagesStore.push_back({ firmware, data, size, storeLevel });        
        
    } else {
        
        if ( image->data ) {            
            auto activeImage = findActiveImage( firmware );
            
            if ( !activeImage || (image->data != activeImage->data ) )           
                delete[] image->data;
        }
                
        image->data = data;
        image->size = size;
    }    
}

auto FirmwareManager::swapIn(Emulator::Interface::Firmware* firmware, unsigned storeLevel) -> std::vector<std::string> {
	
	missingFirmware.clear();
	    
    insertFirmware( firmware, storeLevel );
	
	return missingFirmware;
}

auto FirmwareManager::insertFirmware(Emulator::Interface::Firmware* firmware, unsigned storeLevel) -> void {
    
    while(1) {                               

        if (useImage( firmware, storeLevel))
            break;

        if (loadImage( firmware, storeLevel)) {
            if (useImage( firmware, storeLevel))
                break;
        }

        if (storeLevel == 0)
            // already default firmware
            break;

        // switch back to default firmware and try again
        storeLevel = 0;
    }     
}

auto FirmwareManager::dataInStore( Image* forImage ) -> bool {
    
    for (auto& image : imagesStore) {          
        if (image.firmware == forImage->firmware && image.data == forImage->data)
            return true;
    }
    
    return false;
}

auto FirmwareManager::findImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> Image* {

    for (auto& image : imagesStore) {

        if (image.firmware == firmware && image.storeLevel == storeLevel)            
            return &image;        
    }
    
    return nullptr;
}

auto FirmwareManager::findActiveImage( Emulator::Interface::Firmware* firmware ) -> Image* {

    for (auto& image : imagesActive) {

        if (image.firmware == firmware)            
            return &image;        
    }
    
    return nullptr;
}

auto FirmwareManager::getInstance( Emulator::Interface* emulator ) -> FirmwareManager* {
	
	for (auto firmwareManager : firmwareManagers) {
		if (firmwareManager->emulator == emulator)
			return firmwareManager;
	}
    
	return nullptr;
}

auto FirmwareManager::insertDefault(bool onlyKernal) -> void {

    for (auto& firmware : emulator->firmwares ) {

        if (!onlyKernal || (firmware.id == 0))
            insertFirmware(&firmware, 0);
    }
}

auto FirmwareManager::insert(bool onlyKernal) -> std::vector<std::string> {
    
    missingFirmware.clear();
    
    auto storeLevel = program->getSettings(emulator)->get<unsigned>( "use_firmware", 0 ); 
    
    for (auto& firmware : emulator->firmwares ) {

        if (!onlyKernal || (firmware.id == 0))
            insertFirmware(&firmware, storeLevel);
    }   
    
    return missingFirmware;
}

auto FirmwareManager::getStoreLevelInConfig() -> unsigned {
    static GUIKIT::Setting* setting = program->getSettings(emulator)->getOrInit("use_firmware", 0);
    return (unsigned)*setting;
}

auto FirmwareManager::getStoreLevelInUse() -> unsigned {
    for (auto& firmware : emulator->firmwares ) {
        auto image = findActiveImage(&firmware);
        if (image)
            return image->storeLevel;
    }
    return 0;
}

auto FirmwareManager::loadImage( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> bool {
	if (storeLevel == 0 && cmd->debug) {
		addImage(firmware, storeLevel, nullptr, 0);
		return true;
	}
    
    FileSetting* fSetting = getSetting( firmware, storeLevel );    
    
    if (fSetting->path.empty())
        return false;
    
    GUIKIT::File file( fSetting->path );
    uint8_t* data = nullptr;

    if (file.exists() && file.isSizeValid(MAX_MEDIUM_SIZE) &&
        file.isSizeValid(fSetting->id, MAX_FIRMWARE_SIZE) &&
        ((data = file.archiveData(fSetting->id)) != nullptr)
    ) {    
        unsigned size = file.archiveDataSize(fSetting->id);
        uint8_t* useData = new uint8_t[size];
        std::memcpy(useData, data, size);
        addImage(firmware, storeLevel, useData, size);
        
        return true;
    }
    
    if (!GUIKIT::Vector::find( missingFirmware, fSetting->path) )
        missingFirmware.push_back( fSetting->path );
    
    return false;
}

auto FirmwareManager::getSetting( Emulator::Interface::Firmware* firmware, unsigned storeLevel ) -> FileSetting* {
    FileSetting* fSetting = 
        FileSetting::getInstance( emulator, firmware->name + "_" + std::to_string( storeLevel ));

    if (storeLevel == 0) {
        fSetting->id = 0;
        if ( dynamic_cast<LIBAMI::Interface*>(emulator) || (firmware->id == LIBC64::Interface::FirmwareId::FirmwareIdExpanded))
            fSetting->path = "";
        else
            fSetting->path = program->dataFolder() + firmware->name;

        fSetting->setSaveable(false);
    }
    
    return fSetting;
}
