
#pragma once

#include "../interface.h"

namespace LIBAMI {

struct Interface : Emulator::Interface  {

    Interface();

    ~Interface() {}

    enum ModelId {
        ModelIdSystem,
        ModelIdLowPassFilter,
    };

    enum MediaGroupId {
        MediaGroupIdDisk = 0, MediaGroupIdHardDisk = 1
    };

    enum ExpansionId {
        ExpansionIdNone = 0, ExpansionIdFast = 1,
    };

    enum FirmwareId {
        FirmwareIdKick, FirmwareIdExt,
    };

    static const std::string Version;

    //controls
    auto connect(unsigned connectorId, unsigned deviceId) -> void;
    auto connect(Connector* connector, Device* device) -> void;
    auto getConnectedDevice( Connector* connector ) -> Device*;

    auto setMemory(MemoryType* memoryType, unsigned memoryId) -> void;
    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void;
    auto power() -> void;
    auto reset() -> void;
    auto powerOff() -> void;
    auto run() -> void; //emulate one frame

    //drive handling
    auto insertDisk(Media* media, uint8_t* data, unsigned size, bool loadGracefully = false) -> void;
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

    auto setModelValue(unsigned modelId, int value) -> void;

    auto sendKeyChange(bool pressed, Device::Input* input) -> void;
    auto informAboutKeyUpdate() -> void;
    auto setLineCallback(bool state, unsigned scanline) -> void;

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
