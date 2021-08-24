
#pragma once

// 1541 drive runs with 16 MHz, called reference cycles.
// clock is divided by 16 for cpu, means 1.000.000 cpu cycles per second.
// drive speed is 300 rounds per minute, means 300 / 60 = 5 rounds per second.
// one revolution has 16.000.000 / 5 reference cycles
#define CyclesPerRevolution300Rpm 3200000

#include "../via/via.h"

#include "../structure/structure.h"
#include "../../system/system.h"
#include "../cpu/m6502.h"
#include "../../../tools/rand.h"
#include "../../../tools/serializer.h"
#include "../cia/cia8520.h"
#include "../../../tools/pia.h"
#include <cstdlib>

#define USERDATA_LEVEL 1u
#define ENCODEDDATA_LEVEL 2u
#define FLUXDATA_LEVEL 4u

#define DRIVE_MODE_154x 8u
#define DRIVE_MODE_157x 16u

#define DRIVE_HAS_PIA 32u

namespace LIBC64 {

struct Drive {
        
    Drive( uint8_t number, Emulator::Interface::Media* mediaConnected );
    ~Drive();

    enum class Type { D1541, D1541II, D1541C, D1570, D1571 } type;

    enum class ExpandedMemMode  { M20 = 1, M40 = 2, M60 = 4, M80 = 8, MA0 = 16 };

    unsigned rotSpeedBps[4] = { 250000, 266667, 285714, 307692 };
    const unsigned DISC_DELAY = 600000;
    
    uint8_t number;
    uint8_t* rom = nullptr;
    uint16_t romMask;

    uint8_t* rom1541II = nullptr;
    uint16_t rom1541IISize = 0;
    uint8_t* rom1541 = nullptr;
    uint16_t rom1541Size = 0;
    uint8_t* rom1541C = nullptr;
    uint16_t rom1541CSize = 0;
    uint8_t* rom1571 = nullptr;
    uint16_t rom1571Size = 0;
    uint8_t* rom1570 = nullptr;
    uint16_t rom1570Size = 0;
    uint8_t* romExpanded = nullptr;
    uint16_t romExpandedMask = 0;

    uint8_t* ram20To3F = nullptr;
    uint8_t* ram40To5F = nullptr;
    uint8_t* ram60To7F = nullptr;
    uint8_t* ram80To9F = nullptr;
    uint8_t* ramA0ToBF = nullptr;
    uint8_t* turboTrans = nullptr;

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
    Cia8520* cia;
    Emulator::Pia* pia;
    M6502* cpu;
    DiskStructure structure;
    int64_t cycleCounter;
    bool synced;
    uint8_t irqIncomming;
    uint8_t* ram = nullptr;
    uint32_t driveCycles;
    uint32_t accum;
    unsigned frequency;
    int64_t syncPosRead;
    int64_t syncPosWrite;
    int64_t syncPos;
    uint8_t refCyclesInCpuCycle;
    uint8_t operation;
    uint8_t expandMemory;
    uint8_t speeder = 0;
    uint8_t nibble = 0;
    bool profDosAutoSpeed;
    bool prologic40TrackMode;
    uint8_t prologic2Mhz;
    bool extendedMemoryMap;
    uint8_t turboTransVisible; // 0: rom, 1: ram
    uint8_t turboTransPage;
        
    DiskStructure::GcrTrack* gcrTrack;
    DiskStructure::GcrTrack* dummyTrack;

    bool emulateDxxMoreAccurate = false;
    uint8_t currentHalftrack;
    int stepDirection = 0;

    bool byteReady = false;
    bool ca1Line = false;
    uint8_t side = 0;
    bool dataDirection = 0;
    
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
    bool wasAttachDetached;
    
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
    auto setSyncPos(int direction) -> void;
    auto cpuWrite(uint16_t addr, uint8_t data) -> void;
    auto cpuRead(uint16_t addr) -> uint8_t;
    auto power( ) -> void;
    auto powerOff( ) -> void;
    auto setViaTransition( bool direction ) -> void;
    auto getMedia() -> Emulator::Interface::Media* { return media; }
	auto getMediaConnected() -> Emulator::Interface::Media* { return mediaConnected; }
	auto setType( Type type ) -> void;
    
    auto updateBus() -> void;
    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
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
    auto setSpeed( unsigned rpmScaled ) -> void;
    auto setWobble( unsigned wobbleScaled ) -> void;

    auto syncFound() -> uint8_t;
    auto writeprotectSense() -> uint8_t;
    auto write() -> void;
    auto updateStepper( uint8_t step ) -> bool;
    auto motorRun() -> bool;
    auto motorOffInit() -> void;

    auto serialize(Emulator::Serializer& s) -> void;
    auto updateDeviceState() -> void;
    auto updateIdleDeviceState() -> void;

    auto byteFetched( bool overflowNotThisCycle ) -> void;

    auto updateCycleSpeed(bool mhz2x, bool init = true) -> void;
    auto setFirmwareByType( ) -> void;
    auto use2Mhz() -> bool { return frequency == 2000000; }
    auto setExpandedMemory( ExpandedMemMode& expandedMemMode, bool state  ) -> void;
    auto setSpeeder(uint8_t speeder) -> void;

    auto readProfDosEncoder(uint16_t addr) -> uint8_t;
    auto readProfDosEncoderV1(uint16_t addr) -> uint8_t;

    auto turboTransWriteControl(uint16_t addr, uint8_t data) -> bool;

    auto profDosClockControl(uint16_t addr) -> void;
    auto profDosAutoClockControl(uint16_t addr) -> void;
    auto prologicControlClassic(uint8_t addr, uint8_t data) -> void;
    auto prologicControl(uint16_t addr) -> void;
};
  
}
