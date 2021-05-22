
#include "../interface.h"

namespace LIBAMI {
	
struct Interface : Emulator::Interface  {

	Interface();

	~Interface() {}		
    
    enum ModelId {
        ModelIdLowPassFilter = 0,
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
    
	auto setMemory(MemoryType* memoryType, unsigned memoryId) -> void;
	auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
	auto power() -> void;
	auto run() -> void; //emulate one frame

	//drive handling
	auto setDrivesConnected(MediaGroup* group, unsigned count) -> void;
	auto insertDisk(Media* media, uint8_t* data, unsigned size) -> void;
	auto writeProtectDisk(Media* media, bool state) -> void;
	auto ejectDisk(Media* media) -> void;
	auto insertHardDisk(Media* media, unsigned size) -> void; //uses read and write callbacks
	auto ejectHardDisk(Media* media) -> void;

	//create blank images
	auto createDiskImage(unsigned typeId, bool hd = false, std::string name = "", bool ffs = false) -> Data;
	auto createHardDisk(std::function<void (uint8_t* buffer, unsigned length, unsigned offset)> onCreate, unsigned size, std::string name = "") -> void;

	//savestates
	auto savestate() -> uint8_t*;
    auto checkstate(uint8_t* data, unsigned size) -> bool;
	auto loadstate(uint8_t* data, unsigned size) -> bool;
    
	auto setModel(unsigned modelId, int value) -> void;
    
private:
	auto prepareDevices() -> void;
	auto prepareMemory() -> void;
	auto prepareMedia() -> void;
	auto preparePalettes() -> void;
	auto prepareModels() -> void;
	auto prepareFirmware() -> void;
    auto prepareExpansions() -> void;
};
	
}
