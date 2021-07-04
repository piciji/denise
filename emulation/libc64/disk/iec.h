
#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <condition_variable>

#include "drive/drive1541.h"
#include "../../interface.h"
#include "../../tools/serializer.h"

namespace LIBC64 {

struct Drive1541;
    
struct IecBus {
    
    IecBus(Emulator::Interface::MediaGroup* mediaGroup);
    ~IecBus();        
    
    std::vector<Drive1541*> drives;    
    std::vector<Drive1541*> drivesEnabled;    
    
    bool atnOut;
    bool clockOut;
    bool dataOut;
    uint8_t lastByte;
    uint8_t port;
    int64_t sysClock;
    int64_t cpuCylcesPerSecond;
    std::atomic<bool> ready;
    std::atomic<bool> idle;
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
    
    auto power() -> void;
    auto powerOff() -> void;
    auto run() -> void;
    
    auto updatePort() -> void;
    auto waitForDrives() -> void;
    auto syncDrives( int direction = 0, bool ciaAccess = false ) -> void;
    auto syncDrivesEachCycle( ) -> void;
    auto resetTicks() -> void;
    auto setDrivesEnabled( uint8_t count ) -> void;
    auto setDriveSpeed(unsigned rpmScaled) -> void;
    auto setDriveWobble(unsigned wobbleScaled) -> void;
    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
    auto randomizeRpm() -> void;
    auto setCpuCyclesPerSecond( unsigned cycles ) -> void;
    auto attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size, bool loadGracefully = false ) -> void;
    auto detach( Emulator::Interface::Media* media ) -> void;
    auto writeProtect( Emulator::Interface::Media* media, bool state ) -> void;    
    auto isWriteProtected( Emulator::Interface::Media* media ) -> bool;    
    auto getDiskListing(Emulator::Interface::Media* media) -> std::vector<Emulator::Interface::Listing>&;
    auto selectListing( Emulator::Interface::Media* media, unsigned pos ) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto serializeLight(Emulator::Serializer& s) -> void;
    auto setPowerThread( bool state ) -> void;
    auto setFastForward( bool state ) -> void;
    auto updateIdleState() -> void;
    auto resetDriveState() -> void;
	inline auto checkForIdleWrite(uint8_t byte) -> bool  { return (byte & 0x38) == lastByte; }

    auto updateSerializationSize() -> void;
    auto insertDiskGracefully() -> void;

    auto setDriveType(Drive1541::Type type) -> void;
    auto emulateDxxMoreAccurate(bool state) -> void;
};
   
extern IecBus* iecBus;
}
