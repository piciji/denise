
#pragma once

#include "../interface.h"

namespace LIBC64 {

struct System;

struct Interface : Emulator::Interface {

	Interface();
    ~Interface();
    System* system = nullptr;
    
    enum ModelId {
        ModelIdSid, ModelIdFilter, ModelIdDigiboost, ModelIdBias6581,
        ModelIdCiaRev, ModelIdCpuAneMagic, ModelIdGlueLogic,
        ModelIdLeftLineAnomaly, ModelIdVicIIModel, ModelIdCpuLaxMagic,
		ModelIdDisableGreyDotBug, ModelIdSidFilterType, ModelIdSidSampleFetch, ModelIdBias8580,
        ModelIdSidMulti, ModelIdSidExternal, ModelIdSidFilterVolumeEqualizer,
                     ModelIdSid1Left, ModelIdSid1Right, ModelIdSid1Adr, ModelIdSid2, ModelIdSid2Left, ModelIdSid2Right, ModelIdSid2Adr,
        ModelIdSid3, ModelIdSid3Left, ModelIdSid3Right, ModelIdSid3Adr, ModelIdSid4, ModelIdSid4Left, ModelIdSid4Right, ModelIdSid4Adr,
        ModelIdSid5, ModelIdSid5Left, ModelIdSid5Right, ModelIdSid5Adr, ModelIdSid6, ModelIdSid6Left, ModelIdSid6Right, ModelIdSid6Adr,
        ModelIdSid7, ModelIdSid7Left, ModelIdSid7Right, ModelIdSid7Adr, ModelIdSid8, ModelIdSid8Left, ModelIdSid8Right, ModelIdSid8Adr,
        ModelIdCiaBurstMode, ModelIdDriveParallelCable, ModelIdDiskDriveModel, ModelIdDiskDrivesConnected, ModelIdTapeDrivesConnected,
        ModelIdDiskDriveSpeed, ModelIdDiskDriveWobble, ModelIdTapeDriveWobble, ModelIdDriveFastLoader, ModelIdDiskDriveStepperSeekTime,
        ModelIdDriveRam20To3F, ModelIdDriveRam40To5F, ModelIdDriveRam60To7F, ModelIdDriveRam80To9F, ModelIdDriveRamA0ToBF,
        ModelIdCycleAccurateVideo, ModelIdDiskThread, ModelIdDiskOnDemand, ModelIdD64Accuracy, ModelIdDisalignTrack,

        ModelIdReuRam, ModelIdGeoRam, ModelIdIntensifyPseudoStereo,
    };
    
    enum MediaGroupId {
        MediaGroupIdDisk = 0, MediaGroupIdTape = 1, MediaGroupIdProgram = 2,
        MediaGroupIdExpansionGame = 3, MediaGroupIdExpansionEasyFlash = 4, MediaGroupIdExpansionEasyFlash3 = 5,
        MediaGroupIdExpansionFreezer = 6, MediaGroupIdExpansionRetroReplay = 7,        
        MediaGroupIdExpansionGeoRam = 8, MediaGroupIdExpansionReu = 9, MediaGroupIdExpansionRS232 = 10, MediaGroupIdExpansionFastloader = 11,
    };
    
    enum ExpansionId {
        ExpansionIdNone = 0, ExpansionIdGame = 1, ExpansionIdEasyFlash = 2, ExpansionIdEasyFlash3 = 3,
        ExpansionIdFreezer = 4, ExpansionIdRetroReplay = 5, ExpansionIdGeoRam = 6, ExpansionIdReu = 7,
        ExpansionIdReuRetroReplay = 8, ExpansionIdRS232 = 9, ExpansionIdFastloader = 10,
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
        CartridgeIdSwiftlink = 270, CartridgeIdTurbo232 = 271,
    };

    enum FirmwareId {
        FirmwareIdKernal, FirmwareIdBasic, FirmwareIdChar,
        FirmwareIdVC1541II, FirmwareIdVC1541, FirmwareIdVC1541C, FirmwareIdVC1571, FirmwareIdVC1570,
        FirmwareIdExpanded,
    };
    
    static const std::string Version;

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
    auto runAheadPreventJit(bool state) -> void;
    auto getRegionEncoding() -> Region;	
	auto getRegionGeometry() -> Region;
	auto getSubRegion() -> SubRegion;
    auto setMonitorFpsRatio(double ratio) -> void;
	
    auto convertPetsciiToScreencode(bool state) -> void;
    auto loadWithColumn(bool state) -> void;

	//disk drive handling
    //options: Bit 0 -> use traps, 1 -> send trap finish event, 7 -> temporarly disable a possible hardware speeder

	auto insertDisk(Media* media, uint8_t* data, unsigned size, bool loadGracefully = false) -> void;
	auto writeProtectDisk(Media* media, bool state) -> void;
    auto isWriteProtectedDisk(Media* media) -> bool;
	auto ejectDisk(Media* media) -> void;
	auto createDiskImage(unsigned typeId, std::string name = "", bool hd = false, bool ffs = false, bool bootable = false) -> Data;
    auto getDiskListing(Media* media) -> std::vector<Emulator::Interface::Listing>;
    auto getDiskPreview(uint8_t* data, unsigned size, Media* media = nullptr) -> std::vector<Emulator::Interface::Listing>;
    auto selectDiskListing(Media* media, unsigned pos, uint8_t options = 0) -> void;
    auto selectDiskListing(Media* media, std::string fileName, uint8_t options = 0) -> void;
    auto resetDrive(Media* media) -> void;
    auto hideDrive(Media* media) -> void;
    
	//tape drive handling
	auto insertTape(Media* media, uint8_t* data, unsigned size) -> void;
	auto writeProtectTape(Media* media, bool state) -> void;
    auto isWriteProtectedTape(Media* media) -> bool;
	auto ejectTape(Media* media) -> void;
	auto createTapeImage(unsigned& imageSize) -> uint8_t*;
    auto controlTape(Media* media, TapeMode mode) -> void;
    auto getTapeControl(Media* media) -> TapeMode;
    auto getTapeListing(Media* media) -> std::vector<Emulator::Interface::Listing>;
    auto getTapePreview(uint8_t* data, unsigned size, Media* media = nullptr) -> std::vector<Emulator::Interface::Listing>;
    auto selectTapeListing(Media* media, unsigned pos, uint8_t options = 0) -> void;

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
	auto setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void;
    auto getCharRom() -> Firmware*;
	
	//models
    auto setModelValue(unsigned modelId, int value) -> void;
    auto getModelValue(unsigned modelId) -> int;
    auto getModelIdOfEnabledDrives(MediaGroup* group) -> unsigned;
    auto getModelIdOfCycleRenderer() -> unsigned;
    
    //crop
	auto crop( CropType type, bool aspectCorrect, unsigned left = 0, unsigned right = 0, unsigned top = 0, unsigned bottom = 0 ) -> void;
    auto cropWidth() -> unsigned;
    auto cropHeight() -> unsigned;
    auto cropTop() -> unsigned;
    auto cropLeft() -> unsigned;
    auto cropData() -> uint8_t*;
    auto cropPitch() -> unsigned;

    // jit
    auto setInputSampling(uint8_t mode) -> void;

    // drive sounds
    auto enableFloppySounds(bool state) -> void;
    auto enableTapeSounds(bool state) -> void;
    auto setTapeLoadingNoise(unsigned volume) -> void;

    // sockets
    auto prepareSocket( Media* media, std::string address, std::string port ) -> void;

    auto videoAddMeta(bool state) -> void;

    auto requestImmediateReturn() -> void;
        
    auto activateDebugCart( unsigned limitCycles = 0 ) -> void;
    auto fastForward(unsigned config) -> void;
	auto getForward() -> unsigned;
    
    auto getLuma(uint8_t index, bool newRevision) -> double;
    auto getChroma(uint8_t index) -> double; 
    
    auto setLineCallback(bool state, unsigned scanline = 0) -> void;

    auto setMemoryInitParams(uint8_t value, unsigned invertEvery, unsigned randomPatternLength, unsigned repeatRandomPattern, unsigned randomChance) -> void;
	auto getMemoryInitPattern( uint8_t* pattern ) -> void;
    auto getMemorySize() -> unsigned { return 64 * 1024; }
    
    auto hasFreezeButton() -> bool;
    auto freezeButton() -> void;

    auto hasCustomCartridgeButton() -> bool;
    auto customCartridgeButton() -> void;

    auto pasteText(std::string buffer ) -> void;
    auto copyText() -> std::string;

    auto autoStartedByMediaGroup() -> MediaGroup*;
private:
	auto prepareDevices() -> void;
	auto prepareMedia() -> void;
	auto prepareFirmware() -> void;
    auto prepareModels() -> void;
    auto preparePalettes() -> void;
    auto prepareExpansions() -> void;
};

}
