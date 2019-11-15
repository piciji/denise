
#include "../interface.h"

namespace LIBAMI {
	
struct Interface : Emulator::Interface  {

	Interface();

	~Interface() {}		
    
    enum FeatureId {
        FeatureIdLowPassFilter = 0,
    };
    
    enum DriveGroupId {
        DriveGroupIdDisk = 0, DriveGroupIdHardDisk = 1
    };

	//controls
	auto connect(unsigned connectorId, unsigned deviceId) -> void;
    auto connect(Connector* connector, Device* device) -> void;
    auto getConnectedDevice( Connector* connector ) -> Device*;
    
	auto setCpu(unsigned cpuId) -> void;
	auto setCpuTurbo(unsigned turbo) -> void;
	auto turboSupported() -> bool { return true; }
	auto setMemory(unsigned typeId, unsigned memoryId) -> void;
	auto getMemory(unsigned typeId) -> Memory*;
	auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
	auto power() -> void;
	auto run() -> void; //emulate one frame
	auto setRegion(unsigned id) -> void; //0 - pal, 1 - ntsc

	//drive handling
	auto setDrivesConnected(unsigned groupId, unsigned count) -> void;
	auto insertDisk(unsigned driveId, uint8_t* data, unsigned size) -> void;
	auto writeProtectDisk(unsigned driveId, bool state) -> void;
	auto ejectDisk(unsigned driveId) -> void;
	auto setHardDrive(unsigned driveId, unsigned size) -> void; //uses read and write callbacks
	auto ejectHardDrive(unsigned driveId) -> void;

	//create blank images
	auto getDiskImageSize(unsigned typeId, bool hd) -> unsigned; //get size needed for a new disk image
	auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> uint8_t*;        
	auto createHardDrive(std::function<void (uint8_t* buffer, unsigned length, unsigned offset)> onCreate, unsigned size, std::string name = "") -> void;

	//savestates
	auto savestateSize(void) -> unsigned; //get size needed for a new save state
	auto savestate() -> uint8_t*;
    auto checkstate(uint8_t* data, unsigned size) -> bool;
	auto loadstate(uint8_t* data, unsigned size) -> bool;

	auto setChipset(unsigned chipsetId) -> void;
	auto setFeature(unsigned featureId, int value) -> void;
    
private:
	auto prepareDevices() -> void;
	auto prepareMemory() -> void;
	auto prepareCpus() -> void;
	auto prepareDrives() -> void;
	auto preparePalettes() -> void;
	auto prepareChipset() -> void;
	auto prepareFeatures() -> void;
	auto prepareFirmware() -> void;
    auto prepareStats() -> void;
};
	
}
