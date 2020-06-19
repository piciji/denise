
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
    
    IecBus();
    ~IecBus();        
    
    std::vector<Drive1541*> drives;    
    std::vector<Drive1541*> drivesEnabled;    
    
    bool atnOut;
    bool clockOut;
    bool dataOut;
    uint8_t lastByte;
    uint8_t port;
    int64_t syncPos;
    int64_t syncPosRead;
    int64_t syncPosWrite;
    int64_t cycleCounter;
    int64_t cpuCylcesPerSecond;
    std::atomic<bool> ready;
    std::atomic<bool> idle;
    bool threaded = false;
    uint8_t drivesConnected;
    std::condition_variable cv;
    bool cpuBurner;
    bool cpuBurnerRequested;
    bool powerOn;           
    
    auto writeCia( uint8_t byte ) -> bool;
    auto readCia() -> uint8_t;
    auto readVia() -> uint8_t;
    
    auto power() -> void;
    auto powerOff() -> void;
    auto run() -> void;
    
    auto updatePort() -> void;
    auto waitForDrives() -> void;
    auto syncDrives( int64_t _syncPos = 0, bool ciaAccess = false ) -> void;
    inline auto resetTicks() -> void { cycleCounter &= 0x3ff; }
    auto setDrivesEnabled( uint8_t count ) -> void;
    auto setDriveSpeed(double rpm, double wobble) -> void;
    auto setFirmware(uint8_t* rom, unsigned romSize) -> void;
    inline auto countTicks() -> void { cycleCounter++; }
    auto randomizeRpm() -> void;
    auto setCpuCyclesPerSecond( unsigned cycles ) -> void;
    auto attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size ) -> void;
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
};
   
extern IecBus* iecBus;
}
