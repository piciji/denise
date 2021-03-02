
#pragma once

#include "../interface.h"

namespace LIBC64 {
	
struct Interface : Emulator::Interface {

	Interface();
    
    enum ModelId {
        ModelIdSid = 0, ModelIdFilter = 1, ModelIdDigiboost = 2, ModelIdBias6581 = 3,
        ModelIdCiaRev = 4, ModelIdCpuAneMagic = 5, ModelIdGlueLogic = 6,
        ModelIdLeftLineAnomaly = 7, ModelIdVicIIModel = 8, ModelIdCpuLaxMagic = 9,
		ModelIdDisableGreyDotBug = 10, ModelIdSidFilterType = 11, ModelIdSidSampleFetch = 12, ModelIdBias8580 = 13,
        ModelIdSidMulti = 14, ModelIdSidExternal = 15, ModelIdSidFilterVolumeEqualizer = 16,
                     ModelIdSid1Left, ModelIdSid1Right, ModelIdSid1Adr, ModelIdSid2, ModelIdSid2Left, ModelIdSid2Right, ModelIdSid2Adr,
        ModelIdSid3, ModelIdSid3Left, ModelIdSid3Right, ModelIdSid3Adr, ModelIdSid4, ModelIdSid4Left, ModelIdSid4Right, ModelIdSid4Adr,
        ModelIdSid5, ModelIdSid5Left, ModelIdSid5Right, ModelIdSid5Adr, ModelIdSid6, ModelIdSid6Left, ModelIdSid6Right, ModelIdSid6Adr,
        ModelIdSid7, ModelIdSid7Left, ModelIdSid7Right, ModelIdSid7Adr, ModelIdSid8, ModelIdSid8Left, ModelIdSid8Right, ModelIdSid8Adr,
    };
    
    enum MediaGroupId {
        MediaGroupIdDisk = 0, MediaGroupIdTape = 1,
        MediaGroupIdProgram = 2, MediaGroupIdExpansionGame = 3, MediaGroupIdExpansionReu = 4,
        MediaGroupIdExpansionFreezer = 5, MediaGroupIdExpansionEasyFlash = 6, MediaGroupIdExpansionRetroReplay = 7,
		MediaGroupIdExpansionGeoRam = 8, MediaGroupIdExpansionEasyFlash3 = 9,
    };
    
    enum ExpansionId {
        ExpansionIdNone = 0, ExpansionIdGame = 1, ExpansionIdReu = 2, ExpansionIdFreezer = 3,
        ExpansionIdEasyFlash = 4, ExpansionIdRetroReplay = 5, ExpansionIdGeoRam = 6, ExpansionIdReuRetroReplay = 7,
        ExpansionIdEasyFlash3 = 8,
    };
    
    enum CartridgeId {
        CartridgeIdNoRom = 0xffff,
        CartridgeIdDefault = 0, CartridgeIdDefault8k = 256, CartridgeIdDefault16k = 257,
        CartridgeIdUltimax = 258, CartridgeIdOcean = 5, CartridgeIdFunplay = 7,
        CartridgeIdSuperGames = 8, CartridgeIdSystem3 = 15, CartridgeIdZaxxon = 18,
        CartridgeIdActionReplayMK2 = 50, CartridgeIdActionReplayMK3 = 35,
        CartridgeIdActionReplayMK4 = 30, CartridgeIdActionReplayV41AndHigher = 1, 
        CartridgeIdEasyFlash = 32, CartridgeIdRetroReplay = 36, CartridgeIdNordicReplay = 261,
        CartridgeIdGmod2 = 60, CartridgeIdMagicDesk = 19, CartridgeIdFinalCartridge = 13,
        CartridgeIdFinalCartridge3 = 3, CartridgeIdFinalCartridgePlus = 29, CartridgeIdSimonsBasic = 4,
        CartridgeIdWarpSpeed = 16, CartridgeIdAtomicPower = 9, CartridgeIdMach5 = 51, CartridgeIdRoss = 23,
        CartridgeIdWestermann = 11, CartridgeIdPagefox = 53,
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
    auto getRegionEncoding() -> Region;	
	auto getRegionGeometry() -> Region;
	auto getSubRegion() -> SubRegion;
    auto setMonitorFpsRatio(double ratio) -> void;
	
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
    auto createExpansionImage(MediaGroup* group, unsigned& imageSize, uint8_t id = 0) -> uint8_t*;    
    auto isExpansionBootable() -> bool;
	auto hasExpansionSecondaryRom() -> bool;
	
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
    auto analyzeExpansion(uint8_t* data, unsigned size, std::string suffix = "") -> Expansion*;
    auto setExpansionJumper( Media* media, unsigned jumperId, bool state ) -> void;
    auto getExpansionJumper( Media* media, unsigned jumperId ) -> bool;
    
	//savestates
    auto checkstate(uint8_t* data, unsigned size) -> bool;
	auto savestate(unsigned& size) -> uint8_t*;
	auto loadstate(uint8_t* data, unsigned size) -> bool;       

	//firmware
	auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
    auto getCharRom() -> Firmware*;
	
	//models
    auto setModel(unsigned modelId, int value) -> void;
    auto getModel(unsigned modelId) -> int;
    
    //crop
	auto crop( CropType type, bool aspectCorrect, unsigned left = 0, unsigned right = 0, unsigned top = 0, unsigned bottom = 0 ) -> void;
    auto cropWidth() -> unsigned;
    auto cropHeight() -> unsigned;
    auto cropTop() -> unsigned;
    auto cropLeft() -> unsigned;
    auto cropData() -> uint8_t*;
    auto cropPitch() -> unsigned;

    // performance amd accuracy
    auto videoCycleAccuracy(bool state) -> void;
    auto videoScanlineThread(bool state) -> void;
    auto videoAddMeta(bool state) -> void;
    auto diskHighLoadThread(bool state) -> void;
    auto diskIdle(bool state) -> void;
    auto audioRealtimeThread(bool state) -> void;
        
    auto activateDebugCart( unsigned limitCycles = 0 ) -> void;
    auto fastForward(unsigned config) -> void;
	auto getForward() -> unsigned;
    
    auto getLuma(uint8_t index, bool newRevision) -> double;
    auto getChroma(uint8_t index) -> double; 
    
    auto setLineCallback(bool state, unsigned scanline = 0) -> void;
    auto setFinishVblankCallback(bool state) -> void;
    
    auto setMemory(MemoryType* memoryType, unsigned memoryId) -> void;
    auto setMemoryInitParams(uint8_t value, unsigned invertEvery, unsigned randomPatternLength, unsigned repeatRandomPattern, unsigned randomChance) -> void;
	auto getMemoryInitPattern( uint8_t* pattern ) -> void;
    auto getMemorySize() -> unsigned { return 64 * 1024; }
    
    auto hasFreezeButton() -> bool;
    auto freezeButton() -> void;

    auto hasCustomCartridgeButton() -> bool;
    auto customCartridgeButton() -> void;

    auto pasteText(std::string buffer ) -> void;
    auto copyText() -> std::string;
private:
	auto prepareDevices() -> void;
	auto prepareMedia() -> void;
	auto prepareFirmware() -> void;
    auto prepareModels() -> void;
    auto preparePalettes() -> void;
    auto prepareExpansions() -> void;
    auto prepareMemory() -> void;
};

}
