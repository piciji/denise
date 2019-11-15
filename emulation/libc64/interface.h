
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
    
    enum DriveGroupId {
        DriveGroupIdDisk = 0, DriveGroupIdTape = 1,
        DriveGroupIdModuleSlot = 2, DriveGroupIdMemory = 3,
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

    auto setDrivesConnected(unsigned groupId, unsigned count) -> void;
    auto getDrivesConnected(unsigned groupId) -> unsigned;
	//disk drive handling	
	auto insertDisk(unsigned driveId, uint8_t* data, unsigned size) -> void;
	auto writeProtectDisk(unsigned driveId, bool state) -> void;
	auto ejectDisk(unsigned driveId) -> void;
	auto getDiskImageSize(unsigned typeId, bool hd) -> unsigned;
	auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> uint8_t*;        
    auto getDiskListing(unsigned driveId) -> std::vector<Emulator::Interface::Listing>;
    auto selectDiskListing(unsigned driveId, unsigned pos) -> void;

	//tape drive handling
	auto insertTape(unsigned driveId, uint8_t* data, unsigned size) -> void;
	auto writeProtectTape(unsigned driveId, bool state) -> void;
	auto ejectTape(unsigned driveId) -> void;
	auto createTapeImage(unsigned& imageSize) -> uint8_t*;
    auto controlTape(unsigned driveId, TapeMode mode) -> void;
    auto getTapeControl(unsigned driveId) -> TapeMode;
    auto selectTapeListing(unsigned driveId, unsigned pos) -> void;

	//module slot handling
	auto insertModule(unsigned driveId, uint8_t* data, unsigned size) -> void;
	auto ejectModule(unsigned driveId) -> void;
	
	//memory
	auto insertMemory(unsigned driveId, uint8_t* data, unsigned size) -> void;
	auto ejectMemory(unsigned driveId) -> void;
	auto getLoadedMemory(unsigned& size) -> uint8_t*;
	auto getMemoryListing() -> std::vector<Emulator::Interface::Listing>;
	auto selectMemoryListing(unsigned pos) -> bool;    

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
	
private:
	auto prepareDevices() -> void;
	auto prepareDrives() -> void;
	auto prepareMemory() -> void;
	auto prepareFirmware() -> void;
	auto prepareCpus() -> void;
	auto prepareChipset() -> void;
    auto prepareFeatures() -> void;
    auto prepareStats() -> void;
    auto preparePalettes() -> void;
};

}
