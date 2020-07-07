
#pragma once

#include "../interface.h"

namespace LIBC64 {
	
struct Interface : Emulator::Interface {

	Interface();
    
    enum FeatureId {
        FeatureIdSid = 0, FeatureIdFilter = 1, FeatureIdDigiboost = 2, FeatureIdBias = 3,
        FeatureIdCiaRev = 4, FeatureIdCpuAneMagic = 5, FeatureIdGlueLogic = 6,
        FeatureIdLeftLineAnomaly = 7,
    };
    
    enum MediaGroupId {
        MediaGroupIdDisk = 0, MediaGroupIdTape = 1,
        MediaGroupIdProgram = 2, MediaGroupIdExpansionGame = 3, MediaGroupIdExpansionReu = 4,
        MediaGroupIdExpansionActionReplay = 5, MediaGroupIdExpansionEasyFlash = 6, MediaGroupIdExpansionRetroReplay = 7,
    };
    
    enum ExpansionId {
        ExpansionIdNone = 0, ExpansionIdGame = 1, ExpansionIdReu = 2, ExpansionIdActionReplay = 3,
        ExpansionIdEasyFlash = 4, ExpansionIdRetroReplay = 5,
    };
    
    enum CartridgeId {
        CartridgeIdNoRom = 0xffff,
        CartridgeIdDefault = 0, CartridgeIdDefault8k = 256, CartridgeIdDefault16k = 257,
        CartridgeIdUltimax = 258, CartridgeIdOcean = 5, CartridgeIdFunplay = 7,
        CartridgeIdSuperGames = 8, CartridgeIdSystem3 = 15, CartridgeIdZaxxon = 18,
        CartridgeIdActionReplayMK2 = 50, CartridgeIdActionReplayMK3 = 35,
        CartridgeIdActionReplayMK4 = 30, CartridgeIdActionReplayV41AndHigher = 1, 
        CartridgeIdEasyFlash = 32, CartridgeIdRetroReplay = 36, CartridgeIdNordicReplay = 261
    };
    
    static const std::string Version;
    
    // petscii will be converted to ascii or screencodes to be viewed in host
    bool convertToScreencode = false;
 
	//controls
	auto connect(unsigned connectorId, unsigned deviceId) -> void;
    auto connect(Connector* connector, Device* device) -> void;
    auto getConnectedDevice( Connector* connector ) -> Device*;
    auto getCursorPosition( Device* device, int16_t& x, int16_t& y ) -> bool;
    
	auto power() -> void;
	auto reset() -> void;
	auto powerOff() -> void;
	auto run() -> void; //emulate one frame
    auto runAhead(unsigned frames) -> void;
    auto runAheadPerformance(bool state) -> void;
	auto setRegion(Region region) -> void;
    auto getRegion() -> Region;	
	
    auto convertPetsciiToScreencode(bool state) -> void;

    auto setDrivesConnected(MediaGroup* group, unsigned count) -> void;
    auto getDrivesConnected(MediaGroup* group) -> unsigned;
    auto setDriveSpeed(MediaGroup* group, double rpm, double wobble) -> void;
	//disk drive handling	
	auto insertDisk(Media* media, uint8_t* data, unsigned size) -> void;
	auto writeProtectDisk(Media* media, bool state) -> void;
    auto isWriteProtectedDisk(Media* media) -> bool;
	auto ejectDisk(Media* media) -> void;
	auto getDiskImageSize(unsigned typeId, bool hd) -> unsigned;
	auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> uint8_t*;        
    auto getDiskListing(Media* media) -> std::vector<Emulator::Interface::Listing>;
    auto getDiskPreview(uint8_t* data, unsigned size, Media* media = nullptr) -> std::vector<Emulator::Interface::Listing>;
    auto selectDiskListing(Media* media, unsigned pos) -> void;
    
	//tape drive handling
	auto insertTape(Media* media, uint8_t* data, unsigned size) -> void;
	auto writeProtectTape(Media* media, bool state) -> void;
    auto isWriteProtectedTape(Media* media) -> bool;
	auto ejectTape(Media* media) -> void;
	auto createTapeImage(unsigned& imageSize) -> uint8_t*;
    auto controlTape(Media* media, TapeMode mode) -> void;
    auto getTapeControl(Media* media) -> TapeMode;
    auto selectTapeListing(Media* media, unsigned pos) -> void;

	//expansion handling
	auto insertExpansionImage(Media* media, uint8_t* data, unsigned size) -> void;
    auto writeProtectExpansion(Media* media, bool state) -> void;
    auto isWriteProtectedExpansion(Media* media) -> bool;
    auto ejectExpansionImage(Media* media) -> void;
    auto createExpansionImage(MediaGroup* group, unsigned& imageSize) -> uint8_t*;
    auto getMediaForCustomFileSuffix(std::string suffix) -> Media*;
    auto isExpansionBootable() -> bool;
	
	//program
	auto insertProgram(Media* media, uint8_t* data, unsigned size) -> void;
	auto ejectProgram(Media* media) -> void;
	auto getLoadedProgram(unsigned& size) -> uint8_t*;
	auto getProgramListing(Media* media) -> std::vector<Emulator::Interface::Listing>;
    auto getProgramPreview(uint8_t* data, unsigned size) -> std::vector<Emulator::Interface::Listing>;
	auto selectProgramListing(Media* media, unsigned pos) -> bool;    

    //expansion
    auto setExpansion(unsigned expansionId) -> void;
    auto unsetExpansion() -> void;
    auto getExpansion() -> Expansion*;
    auto analyzeExpansion(uint8_t* data, unsigned size) -> Expansion*;
    auto setExpansionJumper( Media* media, unsigned jumperId, bool state ) -> void;
    auto getExpansionJumper( Media* media, unsigned jumperId ) -> bool;
    
	//savestates
    auto checkstate(uint8_t* data, unsigned size) -> bool;
	auto savestate(unsigned& size) -> uint8_t*;
	auto loadstate(uint8_t* data, unsigned size) -> bool;       

	//firmware
	auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
    auto getCharRom() -> Firmware*;
	
	//features
    auto setFeature(unsigned featureId, int value) -> void;
    auto getFeature(unsigned featureId) -> int;
    
    //crop
	auto crop( CropType type, bool aspectCorrect, unsigned left = 0, unsigned right = 0, unsigned top = 0, unsigned bottom = 0 ) -> void;
    auto cropWidth() -> unsigned;
    auto cropHeight() -> unsigned;
    auto cropTop() -> unsigned;
    auto cropLeft() -> unsigned;
    auto cropData() -> uint16_t*;
    auto cropPitch() -> unsigned;

    // performance amd accuracy
    auto videoCycleAccuracy(bool state) -> void;
    auto videoScanlineThread(bool state) -> void;
    auto videoAddMeta(bool state) -> void;
    auto diskHighLoadThread(bool state) -> void;
    auto diskIdle(bool state) -> void;
    auto audioRealtimeThread(bool state) -> void;
    
	auto setChipset(unsigned chipsetId) -> void;
    auto getChipset() -> unsigned;
    
    auto activateDebugCart( unsigned limitCycles = 0 ) -> void;
    auto fastForward(unsigned config) -> void;
    
    auto getLuma(uint8_t index, bool newRevision) -> double;
    auto getChroma(uint8_t index) -> double; 
    
    auto setLineCallback(bool state, unsigned scanline = 0) -> void;
    auto setFinishVblankCallback(bool state) -> void;
    
    auto setMemory(MemoryType* memoryType, unsigned memoryId) -> void;
	
    auto hasFreezerButton() -> bool;
    auto freeze() -> void;
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
