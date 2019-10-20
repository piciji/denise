
#pragma once

#include "../interface.h"

namespace LIBC64 {
	
struct Interface : Emulator::Interface {

	Interface();
    
    enum FeatureId {
        FeatureIdSid = 0, FeatureIdFilter = 1, FeatureIdDigiboost = 2, FeatureIdBias = 3,
        FeatureIdSidAccuracy = 4, FeatureIdCiaRev = 5, FeatureIdCpuAneMagic = 6, FeatureIdGlueLogic = 7,
        FeatureIdPowerThread = 8,
    };
    
    enum MediaGroupId {
        MediaGroupIdDisk = 0, MediaGroupIdTape = 1,
        MediaGroupIdMemory = 2, MediaGroupIdExpansionGame = 3, MediaGroupIdExpansionReu = 4
    };
    
    enum ExpansionId {
        ExpansionIdNone = 0, ExpansionIdGame = 1, ExpansionIdReu = 2,
    };
    
    enum CartridgeId {
        CartridgeIdDefault = 0, CartridgeIdDefault8k = 256, CartridgeIdDefault16k = 257,
        CartridgeIdUltimax = 258, CartridgeIdOcean = 5, CartridgeIdFunplay = 7,
        CartridgeIdSuperGames = 8, CartridgeIdSystem3 = 15, CartridgeIdZaxxon = 18
    };
    
    static const std::string Version;
    
    // petscii will be converted to ascii or screencodes to be viewed in host
    bool convertToScreencode;

	//controls
	auto connect(unsigned connectorId, unsigned deviceId) -> void;
    auto connect(Connector* connector, Device* device) -> void;
    auto getConnectedDevice( Connector* connector ) -> Device*;
    auto getCursorPosition( Device* device, int16_t& x, int16_t& y ) -> bool;
    
	auto power() -> void;
	auto reset() -> void;
	auto powerOff() -> void;
	auto run() -> void; //emulate one frame
	auto setRegion(Region region) -> void;
    auto getRegion() -> Region;
	auto getMemory(unsigned typeId) -> Memory*;
	
    auto convertPetsciiToScreencode(bool state) -> void;

    auto setDrivesConnected(MediaGroup* group, unsigned count) -> void;
    auto getDrivesConnected(MediaGroup* group) -> unsigned;
	//disk drive handling	
	auto insertDisk(Media* media, uint8_t* data, unsigned size) -> void;
	auto writeProtectDisk(Media* media, bool state) -> void;
	auto ejectDisk(Media* media) -> void;
	auto getDiskImageSize(unsigned typeId, bool hd) -> unsigned;
	auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> uint8_t*;        
    auto getDiskListing(Media* media) -> std::vector<Emulator::Interface::Listing>;
    auto selectDiskListing(Media* media, unsigned pos) -> void;

	//tape drive handling
	auto insertTape(Media* media, uint8_t* data, unsigned size) -> void;
	auto writeProtectTape(Media* media, bool state) -> void;
	auto ejectTape(Media* media) -> void;
	auto createTapeImage(unsigned& imageSize) -> uint8_t*;
    auto controlTape(Media* media, TapeMode mode) -> void;
    auto getTapeControl(Media* media) -> TapeMode;
    auto selectTapeListing(Media* media, unsigned pos) -> void;

	//module slot handling
	auto insertExpansionImage(Media* media, uint8_t* data, unsigned size) -> void;
	auto ejectExpansionImage(Media* media) -> void;
	
	//memory
	auto insertMemory(Media* media, uint8_t* data, unsigned size) -> void;
	auto ejectMemory(Media* media) -> void;
	auto getLoadedMemory(unsigned& size) -> uint8_t*;
	auto getMemoryListing(Media* media) -> std::vector<Emulator::Interface::Listing>;
	auto selectMemoryListing(Media* media, unsigned pos) -> bool;    

    //expansion
    auto setExpansion(unsigned expansionId) -> void;
    auto getExpansion() -> Expansion*;
    
	//savestates
    auto checkstate(uint8_t* data, unsigned size) -> bool;
	auto savestate(unsigned& size) -> uint8_t*;
	auto loadstate(uint8_t* data, unsigned size) -> bool;       

	//firmware
	auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
	
	//features
    auto setFeature(unsigned featureId, int value) -> void;
    auto getFeature(unsigned featureId) -> int;
    
    //crop
	auto crop( CropType type, bool aspectCorrect, unsigned left = 0, unsigned right = 0, unsigned top = 0, unsigned bottom = 0 ) -> void;
    auto cropWidth() -> unsigned;
    auto cropHeight() -> unsigned;
    auto cropTop() -> unsigned;
    auto cropLeft() -> unsigned;
	
	auto setChipset(unsigned chipsetId) -> void;
    auto getChipset() -> unsigned;
    
    auto activateDebugCart( unsigned limitCycles = 0 ) -> void;
    auto disableFilterCircuit() -> void;
    
    auto getLuma(uint8_t index, bool newRevision) -> double;
    auto getChroma(uint8_t index) -> double; 
    
    auto setLineCallback(bool state, unsigned scanline = 0) -> void;
    auto setFinishVblankCallback(bool state) -> void;
    
    auto setMemory(unsigned typeId, unsigned memoryId) -> void;
	
private:
	auto prepareDevices() -> void;
	auto prepareMedia() -> void;
	auto prepareFirmware() -> void;
	auto prepareChipset() -> void;
    auto prepareFeatures() -> void;
    auto prepareStats() -> void;
    auto preparePalettes() -> void;
    auto prepareExpansions() -> void;
    auto prepareMemory() -> void;
};

}
