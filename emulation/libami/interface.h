
#pragma once

#include "../interface.h"
#include "system/debuggerSnapshot.h"

namespace LIBAMI {

struct System;

struct Interface : Emulator::Interface  {

    Interface();
    ~Interface();
    System* system = nullptr;

    enum ModelId {
        ModelIdSystem,
        ModelIdAudioFilter,
        ModelIdRegion,
        ModelIdDiskDrivesConnected,
        ModelIdDiskDriveWobble,
        ModelIdDriveStepperDelay,
        ModelIdDriveStepperAccess,
        ModelIdDiskDriveSpeed,
        ModelIdDiskTurbo,
        ModelIdSampleFetch,
        ModelIdChipMem,
        ModelIdSlowMem,
        ModelIdFastMem,
        ModelIdRTC,
        ModelIdSerialPlugin,
        ModelIdJoyportDongle,
        ModelIdOverclock,
        ModelIdHardDrivesConnected,
        ModelIdHardDrivesBuiltInSlower,
    };

    enum MediaGroupId {
        MediaGroupIdDisk,
        MediaGroupIdHardDisk
    };

    enum ExpansionId {
        ExpansionIdNone,
        ExpansionIdHDController,
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

    // hard drive handling
    auto insertHardDisk(Media* media, uint8_t* data, uint64_t size) -> void;
    auto ejectHardDisk(Media* media) -> void;
    auto writeProtectHardDisk(Media* media, bool state) -> void;
    auto isWriteProtectedHardDisk(Media* media) -> bool;
    auto getHardDiskListing(Media* media) -> std::vector<Listing>;
    auto getHardDiskPreview(uint8_t* data, uint64_t size, Media* media) -> std::vector<Listing>;
    auto createHardDiskImage(uint64_t _size, bool vhd) -> Data;

    // disk drive handling
    auto insertDisk(Media* media, uint8_t* data, unsigned size) -> void;
    auto writeProtectDisk(Media* media, bool state) -> void;
    auto isWriteProtectedDisk(Media* media) -> bool;
    auto ejectDisk(Media* media) -> void;
    auto createDiskImage(unsigned typeId, std::string name = "", bool hd = false, bool ffs = false, bool bootable = false) -> Data;
    auto getDiskListing(Media* media) -> std::vector<Emulator::Interface::Listing>;
    auto getDiskPreview(uint8_t* data, unsigned size, Media* media = nullptr) -> std::vector<Emulator::Interface::Listing>;

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
    auto cropFrame( CropType type, Crop crop ) -> void;
    auto cropWidth() -> unsigned;
    auto cropHeight() -> unsigned;
    auto cropTop() -> unsigned;
    auto cropLeft() -> unsigned;
    auto cropCoordUpdated(unsigned& top, unsigned& left) -> bool;
    auto cropData16() -> uint16_t*;
    auto cropPitch() -> unsigned;
    auto cropOptions() -> uint8_t;
    auto cropAlternatively(unsigned& width, unsigned& height, unsigned& pitch) -> uint8_t*;

    // jit
    auto setInputSampling(uint8_t mode) -> void;

    auto setWarpMode(unsigned config) -> void;

    auto enableFloppySounds(bool state) -> void;

    auto sendKeyChange(bool pressed, Device::Input* input) -> void;
    auto informAboutKeyUpdate() -> void;
    auto requestImmediateReturn() -> void;

    auto needExternalKeyUpdates() -> bool { return true; }
    auto autoStartedByMediaGroup() -> MediaGroup*;

    // tools
    auto buildDisk(const std::string& name, std::vector<Item>& files) -> Data;
    auto buildHardDisk(const std::string& name, std::vector<Item>& files) -> Data;

    auto configRewind(unsigned steps, unsigned maxSizeInMb) -> void;
    auto setRewind(bool state) -> void;

    // debugger
    auto debuggerAdd(DebuggerTheme theme, DebuggerAction action, unsigned addr, unsigned addrTo = 0) -> void;
    auto debuggerRemove(DebuggerTheme theme, DebuggerAction action, std::optional<unsigned> addr = std::nullopt) -> void;
    auto setWatchpointCondition(DebuggerTheme theme, DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool;

    auto debuggerStepOver(DebuggerTheme theme) -> void;
    auto debuggerStepInto(DebuggerTheme theme) -> void;
    auto debuggerStepOut(DebuggerTheme theme) -> bool;

    // disassembler
    auto disassemble(DebuggerTheme theme, unsigned addr, unsigned& bytes) -> std::string;
    auto disassembleData(DebuggerTheme theme, unsigned addr, unsigned bytes) -> std::string;
    auto disassembleTrace(DebuggerTheme theme, unsigned i, uint16_t& flags) -> std::string;
    auto getCopperDump(unsigned addrFrom, unsigned addrTo) -> uint8_t*;
    auto getMemoryDumpBank(uint8_t bank, uint16_t* dump) -> void;
    auto getDmaDump() -> uint8_t*;

    auto editMemory(DebuggerTheme theme, uint32_t addr, std::vector<uint16_t> values) -> void;

private:
    auto prepareDevices() -> void;
    auto prepareMedia() -> void;
    auto preparePalettes() -> void;
    auto prepareModels() -> void;
    auto prepareFirmware() -> void;
    auto prepareExpansions() -> void;
};

}
