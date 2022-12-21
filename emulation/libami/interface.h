
#pragma once

#include "../interface.h"

namespace LIBAMI {

struct Interface : Emulator::Interface  {

    Interface();

    ~Interface() {}

    enum ModelId {
        ModelIdSystem,
        ModelIdLowPassFilter,
        ModelIdRegion,
        ModelIdDiskDrivesConnected,
        ModelIdSampleFetch,
        ModelIdChipMem,
        ModelIdSlowMem,
    };

    enum MediaGroupId {
        MediaGroupIdDisk = 0
    };

    enum ExpansionId {
        ExpansionIdNone = 0
    };

    enum FirmwareId {
        FirmwareIdKick, FirmwareIdExt,
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

    // disk drive handling
    auto insertDisk(Media* media, uint8_t* data, unsigned size, bool loadGracefully = false) -> void;
    auto writeProtectDisk(Media* media, bool state) -> void;
    auto isWriteProtectedDisk(Media* media) -> bool;
    auto ejectDisk(Media* media) -> void;
    auto createDiskImage(unsigned typeId, std::string name = "", bool hd = false, bool ffs = false, bool bootable = false) -> Data;
    auto getDiskListing(Media* media, bool alternateLoad = false) -> std::vector<Emulator::Interface::Listing>;
    auto getDiskPreview(uint8_t* data, unsigned size, Media* media = nullptr, bool alternateLoad = false) -> std::vector<Emulator::Interface::Listing>;


    //savestates
    auto checkstate(uint8_t* data, unsigned size) -> bool;
    auto savestate(unsigned& size) -> uint8_t*;
    auto loadstate(uint8_t* data, unsigned size) -> bool;

    //firmware
    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void;

    //models
    auto setModelValue(unsigned modelId, int value) -> void;
    auto getModelValue(unsigned modelId) -> int;
    auto getModelIdOfEnabledDrives(MediaGroup* group) -> unsigned;

    //crop
    auto crop( CropType type, bool aspectCorrect, unsigned left = 0, unsigned right = 0, unsigned top = 0, unsigned bottom = 0 ) -> void;
    auto cropWidth() -> unsigned;
    auto cropHeight() -> unsigned;
    auto cropTop() -> unsigned;
    auto cropLeft() -> unsigned;
    auto cropData16() -> uint16_t*;
    auto cropPitch() -> unsigned;

    // jit
    auto setInputSampling(uint8_t mode) -> void;

    auto fastForward(unsigned config) -> void;
    auto getForward() -> unsigned;

    auto sendKeyChange(bool pressed, Device::Input* input) -> void;
    auto informAboutKeyUpdate() -> void;
    auto setLineCallback(bool state, unsigned scanline) -> void;
    auto requestImmediateReturn() -> void;

private:
    auto prepareDevices() -> void;
    auto prepareMedia() -> void;
    auto preparePalettes() -> void;
    auto prepareModels() -> void;
    auto prepareFirmware() -> void;
    auto prepareExpansions() -> void;
};

}
