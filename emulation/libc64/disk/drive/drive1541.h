
#pragma once

#include "../../system/memory.h"
#include "../via/via.h"
#include "../iec.h"
#include "../structure/structure.h"
#include "../../system/system.h"
//#include "../cpu/m6502custom.h"
#include "../cpu/m6502.h"
#include "../../../tools/rand.h"
#include "../../../tools/serializer.h"
#include <cstdlib>

namespace LIBC64 {
   
struct IecBus;
    
struct Drive1541 {   
        
    Drive1541( uint8_t number );
    ~Drive1541();
    
    const unsigned rotSpeedBps[4] = { 250000, 266667, 285714, 307692 };
    const unsigned DISC_DELAY = 600000;

    enum TrackState : uint8_t { NoOperation = 0, Read = 1, Write = 2, ReadHalf = 4, WriteHalf = 5 };
    
    std::function<void ( )> updateState = [](){};
    
    uint8_t number;
    uint8_t* rom = nullptr;
   
    Emulator::Interface::Media* media;

    struct MotorOff {
        bool slowDown = false;
        const unsigned CHUNKS = 20;
        std::vector<unsigned> chunkSize;
        unsigned decelerationPoint;
        unsigned delay;
        unsigned pos;
    } motorOff;
    
    Memory memory;
    Memory::Read readRam;
    Memory::Write writeRam;
    Memory::Read readVia1Reg;
    Memory::Write writeVia1Reg;
    Memory::Read readVia2Reg;
    Memory::Write writeVia2Reg;
    Memory::Read readRom;
    Memory::Read readUnmapped;
    Memory::Write writeUnmapped;
    
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
    unsigned bitCounter;
    
    uint64_t refCyclesPerRevolution300rpm;
    uint32_t refCyclesPerRevolution;
    bool filter;
    bool lastFilter;
    uint8_t ue7Counter;
    uint8_t uf4Counter;
    Emulator::Rand randomizer;
    unsigned randCounter;
    
    uint8_t writeValue;
    unsigned readBuffer;
    unsigned writeBuffer;
    
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
    
    auto calculateRefCyclesPerRevolution() -> void;
    
    auto sync() -> void;
    auto cpuWrite(uint16_t addr, uint8_t data) -> void;
    auto cpuRead(uint16_t addr) -> uint8_t;
    auto remap( ) -> void;
    auto power( ) -> void;
    auto powerOff( ) -> void;
    auto setViaTransition( bool state ) -> void;
    auto getMedia() -> Emulator::Interface::Media* { return media; }
    
    auto updateBus() -> void;
    auto setFirmware(uint8_t* rom) -> void;
    auto rotateD64() -> void;
    auto rotateG64( bool irqNextCycle ) -> void;
    auto randomizeRpm() -> void;
    auto writeBit( bool state ) -> void;
    auto readBit() -> bool;
    auto changeHalfTrack( uint8_t step ) -> void;
    auto attach( Emulator::Interface::Media* media, uint8_t* data, unsigned size ) -> void;
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
    
    auto getTrackState() -> TrackState;
    auto useAccuracy() -> bool;
    auto serialize(Emulator::Serializer& s) -> void;
};
  
}
