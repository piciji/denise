
#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <condition_variable>

#include "drive/drive.h"
#include "../../interface.h"
#include "../../interface.h"
#include "../../tools/serializer.h"

namespace Emulator {
    struct SystemTimer;
}

namespace LIBC64 {

struct Drive;
struct System;

struct IecBus {
    
    IecBus(System* system, Emulator::Interface::MediaGroup* mediaGroup);
    ~IecBus();        

    System* system;
    Emulator::SystemTimer& sysTimer;
    std::vector<Drive*> drives;
    std::vector<Drive*> drivesEnabled;
    
    bool atnOut;
    bool clockOut;
    bool dataOut;
    uint8_t lastByte;
    uint8_t port;
    int64_t sysClock;
    int64_t cpuCylcesPerSecond;
    std::atomic<bool> ready;
    std::atomic<bool> idle;
    std::atomic<bool> updatePriority;
    bool threaded = false;
    uint8_t drivesConnected;
    std::condition_variable cv;
    bool cpuBurner;
    bool cpuBurnerRequested;
    bool powerOn;
    bool diskInsertInProgress = false;
    
    auto writeCia( uint8_t byte ) -> bool;
    auto readCia() -> uint8_t;
    auto readVia() -> uint8_t;
    auto serialShift(bool bit) -> void;
    auto readParallel() -> uint8_t;
    auto readParallelWithHandshake() -> uint8_t;
    auto writeParallelHandshake() -> void;
    
    auto power() -> void;
    auto powerOff() -> void;
    auto run() -> void;
    
    auto updatePort() -> void;
    auto waitForDrives() -> void;
    auto syncDrives( int direction = 0, bool ciaAccess = false ) -> bool;
    auto syncDrivesEachCycle( ) -> void;
    auto resetTicks() -> void;
    auto setDrivesEnabled( uint8_t count ) -> void;
	auto hideDrive( Emulator::Interface::Media* media ) -> void;
    auto resetDrive( Emulator::Interface::Media* media ) -> void;
    auto setDriveSpeed(unsigned rpmScaled) -> void;
    auto setDriveWobble(unsigned wobbleScaled) -> void;
    auto setStepperSeekTime( unsigned stepperSeekTimeScaled ) -> void;
    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
    auto randomizeRpm() -> void;
    auto setCpuCyclesPerSecond( unsigned cycles ) -> void;
    auto attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size, bool loadGracefully = false ) -> void;
    auto detach( Emulator::Interface::Media* media ) -> void;
    auto writeProtect( Emulator::Interface::Media* media, bool state ) -> void;    
    auto isWriteProtected( Emulator::Interface::Media* media ) -> bool;    
    auto getDiskListing(Emulator::Interface::Media* media) -> std::vector<Emulator::Interface::Listing>&;
    auto selectListing( Emulator::Interface::Media* media, unsigned pos, uint8_t options = 0 ) -> void;
    auto selectListing( Emulator::Interface::Media* media,  std::string fileName, uint8_t options = 0 ) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto serializeLight(Emulator::Serializer& s) -> void;
    auto setPowerThread( bool state ) -> void;
    auto setFastForward( bool state ) -> void;
    auto updateIdleState() -> void;
    auto resetDriveState() -> void;
    auto setExpandedMemory( Drive::ExpandedMemMode expandedMemMode, bool state ) -> void;
    auto getExpandedMemory( Drive::ExpandedMemMode expandedMemMode ) -> bool;
	inline auto checkForIdleWrite(uint8_t byte) -> bool  { return (byte & 0x38) == lastByte; }

    auto updateSerializationSize() -> void;
    auto insertDiskGracefully() -> void;

    auto setDriveType(Drive::Type type) -> void;
    auto emulateDxxMoreAccurate(bool state) -> void;
    auto disalignTracks(bool state) -> void;
    auto setSpeeder(uint8_t speeder) -> void;
    auto updateDriveSounds() -> void;
    auto wasAutostarted() -> bool;
};

}
