
#pragma once

#include "../../system/memory.h"
#include "../via/via.h"
#include "../iec.h"
#include "../structure/structure.h"
#include "../../system/system.h"
#include "../cpu/m6502custom.h"
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
    unsigned romSize = 0;    
    
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
    M6502Custom* cpu;
    MOS65Context* cpuCtx;
    Structure1541 structure1541;
    int32_t cycleCounter;
    bool synced;
    uint8_t irqIncomming;
    uint8_t* ram = nullptr;
    uint32_t driveCycles;
    uint32_t accum;   
        
    Structure1541::GcrTrack* gcrTrack = new Structure1541::GcrTrack;
    
    uint8_t currentHalftrack = 17 * 2;    
    
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
    bool alternateRefTiming;
    
    uint8_t readValue;
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
    
    auto calculateRefCyclesPerRevolution() -> void;
    
    auto remap( bool romOnly = false ) -> void;
    auto power( ) -> void;
    auto powerOff( ) -> void;
    auto setViaTransition( bool state ) -> void;
    
    auto updateBus() -> void;
    auto setFirmware(uint8_t* rom, unsigned romSize) -> void;
    auto rotateD64() -> void;
    auto rotateG64( unsigned refCycles ) -> void;
    auto randomizeRpm() -> void;
    auto writeBit( bool state ) -> void;
    auto readBit() -> bool;
    auto changeHalfTrack( int step ) -> void;
    auto attach( uint8_t* data, unsigned size ) -> void;
    auto detach() -> void;
    auto setWriteProtect(bool state) -> void;
    
    auto processDelays() -> void;
    auto syncFound() -> uint8_t;
    auto writeprotectSense() -> uint8_t;
    auto writeTrack() -> void;
    
    auto getTrackState() -> TrackState;
    auto useAccuracy() -> bool;
    auto serialize(Emulator::Serializer& s) -> void;
};
  
}
