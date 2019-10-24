
#include "../interface.h"

namespace LIBAMI {
	
struct Interface : Emulator::Interface  {

	Interface();

	~Interface() {}		
    
    enum FeatureId {
        FeatureIdLowPassFilter = 0,
    };
    
    enum MediaGroupId {
        MediaGroupIdDisk = 0, MediaGroupIdHardDisk = 1
    };
        
    enum ExpansionId {
        ExpansionIdNone = 0, ExpansionIdFast = 1,
    };   

	//controls
	auto connect(unsigned connectorId, unsigned deviceId) -> void;
    auto connect(Connector* connector, Device* device) -> void;
    auto getConnectedDevice( Connector* connector ) -> Device*;
    
	auto setCpu(unsigned cpuId) -> void;
	auto setMemory(MemoryType* memoryType, unsigned memoryId) -> void;
	auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
	auto power() -> void;
	auto run() -> void; //emulate one frame
	auto setRegion(unsigned id) -> void; //0 - pal, 1 - ntsc

	//drive handling
	auto setDrivesConnected(MediaGroup* group, unsigned count) -> void;
	auto insertDisk(Media* media, uint8_t* data, unsigned size) -> void;
	auto writeProtectDisk(Media* media, bool state) -> void;
	auto ejectDisk(Media* media) -> void;
	auto insertHardDisk(Media* media, unsigned size) -> void; //uses read and write callbacks
	auto ejectHardDisk(Media* media) -> void;

	//create blank images
	auto getDiskImageSize(unsigned typeId, bool hd) -> unsigned; //get size needed for a new disk image
	auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> uint8_t*;        
	auto createHardDisk(std::function<void (uint8_t* buffer, unsigned length, unsigned offset)> onCreate, unsigned size, std::string name = "") -> void;

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
	auto prepareMedia() -> void;
	auto preparePalettes() -> void;
	auto prepareChipset() -> void;
	auto prepareFeatures() -> void;
	auto prepareFirmware() -> void;
    auto prepareStats() -> void;
    auto prepareExpansions() -> void;
};
	
}
