
#pragma once

// 1541 drive runs with 16 MHz, called reference cycles.
// clock is divided by 16 for cpu, means 1.000.000 cpu cycles per second.
// drive speed is 300 rounds per minute, means 300 / 60 = 5 rounds per second.
// one revolution has 16.000.000 / 5 reference cycles
#define CyclesPerRevolution300Rpm 3200000

#include "../via/via.h"
#include "../iec.h"
#include "../structure/structure.h"
#include "../../system/system.h"
#include "../cpu/m6502.h"
#include "../../../tools/rand.h"
#include "../../../tools/serializer.h"
#include <cstdlib>

namespace LIBC64 {
   
struct IecBus;
    
struct Drive1541 {   
        
    Drive1541( uint8_t number, Emulator::Interface::Media* mediaConnected );
    ~Drive1541();
    
    const unsigned rotSpeedBps[4] = { 250000, 266667, 285714, 307692 };
    const unsigned DISC_DELAY = 600000;
    
    uint8_t number;
    uint8_t* rom = nullptr;
   
    Emulator::Interface::Media* media;
	Emulator::Interface::Media* mediaConnected; // update status LED if there was no disk inserted

    struct MotorOff {
        bool slowDown = false;
        const unsigned CHUNKS = 20;
        std::vector<unsigned> chunkSize;
        unsigned decelerationPoint;
        unsigned delay;
        unsigned pos;
    } motorOff;
        
    Via* via1;
    Via* via2;
    M6502* cpu;
    Structure1541 structure1541;
    int64_t cycleCounter;
    bool synced;
    uint8_t irqIncomming;
    uint8_t* ram = nullptr;
    uint32_t driveCycles;
    uint32_t accum;   
        
    Structure1541::GcrTrack* gcrTrack = new Structure1541::GcrTrack;
    
    uint8_t currentHalftrack;
    int stepDirection = 0;
    
    unsigned speedZone = 0;
    bool byteReadyOverflow = true; // random initialization ?
    bool readMode = true; // random initialization ?
    unsigned headOffset = 0; // one and only initialization
    uint8_t ue3Counter;

    uint32_t refCyclesPerRevolution;

    uint8_t ue7Counter;
    uint8_t uf4Counter;
    Emulator::Rand randomizer;
    unsigned randCounter;
    int pulseIndex;
    unsigned pulseDelta;

    bool comperatorFlipFlop; // detect flux reversal
    bool uf6aFlipFlop;
    unsigned pulseDuration;
    
    uint8_t writeValue;
    unsigned readBuffer;
    uint8_t writeBuffer;
    uint8_t latchedByte;
    
    unsigned attachDelay = 0;
    unsigned detachDelay = 0;
    unsigned attachDetachDelay = 0;
    
    bool motorOn = false;
    bool written = false;
    bool writeProtected = true;
    bool loaded = false;
    
    bool clockOut;
    bool dataOut;
    bool atnOut;    
    
    unsigned rpm = 30000;
    unsigned wobble = 50;

    auto sync() -> void;
    auto cpuWrite(uint16_t addr, uint8_t data) -> void;
    auto cpuRead(uint16_t addr) -> uint8_t;
    auto power( ) -> void;
    auto powerOff( ) -> void;
    auto setViaTransition( bool state ) -> void;
    auto getMedia() -> Emulator::Interface::Media* { return media; }
	auto getMediaConnected() -> Emulator::Interface::Media* { return mediaConnected; }
    
    auto updateBus() -> void;
    auto setFirmware(uint8_t* rom) -> void;
    auto rotateD64() -> void;
    auto rotateG64() -> void;
    auto rotateP64(  ) -> void;
    auto randomizeRpm() -> void;
    auto writeBit( bool state ) -> void;
    auto readBit() -> bool;
    auto changeHalfTrack( uint8_t step ) -> void;
    auto attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size, bool loadGracefully = false ) -> void;
    auto postAttach() -> void;
    auto detach() -> void;
    auto setWriteProtect(bool state) -> void;
    auto setSpeed( double rpm, double wobble ) -> void;
    
    auto processDelays() -> void;
    auto syncFound() -> uint8_t;
    auto writeprotectSense() -> uint8_t;
    auto write() -> void;
    auto informUserToRemoveWriteProtection() -> void;
    auto updateStepper( uint8_t step ) -> bool;
    auto motorRun() -> bool;
    auto motorOffInit() -> void;

    auto serialize(Emulator::Serializer& s) -> void;
    auto updateDeviceState() -> void;
    auto updateIdleDeviceState() -> void;

    auto byteFetched( bool overflowNotThisCycle ) -> void;
};
  
}
