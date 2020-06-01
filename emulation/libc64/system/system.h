
#pragma once

#include <functional>
#include "../interface.h"
#include "../../processor/m65xx/model.h"
#include "../../cia/m6526.h"
#include "memory.h"
#include "../../tools/event.h"

#define C64_FREQUENCY_PAL 985248
#define C64_FREQUENCY_NTSC 1022727
#define C64_CYCLES_FRAME_PAL (63 * 312)
#define C64_CYCLES_FRAME_NTSC (65 * 263)

namespace Emulator {
    struct Crop;
    struct PowerSupply;
    struct Serialzer;
}

namespace LIBC64 {

struct Prg;
struct Input;    
struct KeyBuffer;	
struct GlueLogic;
struct ExpansionPort;

struct System {   
    
    System(Interface* interface);
    ~System();
	   
    Memory::Read readUnmapped;
    Memory::Write writeUnmapped;
    Memory::Read readRam;
    Memory::Write writeRam;
    Memory::Write writeRamAt80To9F;
    Memory::Read readVicReg;
    Memory::Write writeVicReg;
    Memory::Read readSidReg;
    Memory::Write writeSidReg;
    Memory::Read readColorRam;
    Memory::Write writeColorRam;
    Memory::Read readIo1Reg;
    Memory::Write writeIo1Reg;
    Memory::Read readIo2Reg;
    Memory::Write writeIo2Reg;
    Memory::Read readCia1Reg;
    Memory::Write writeCia1Reg;
    Memory::Read readCia2Reg;
    Memory::Write writeCia2Reg;
    
    Memory::Write writeDebugReg;
    
    Memory::Read readCharRom;
    Memory::Read readKernalRom;
    Memory::Read readBasicRom; 
    Memory::Read readRomL;
    Memory::Read readRomH;
    Memory::Write writeRomL;
    Memory::Write writeRomH;
    // need this separated from writeRomL and writeRomH because of not writing to C64 memory
    Memory::Write writeUltimaxRomL;
    Memory::Write writeUltimaxRomH;
    // in ultimax mode a cartridge can map following areas freely.
    // will add more when emulating a cartridge which maps something in unmapped spaces
    Memory::Read readUltimaxA0;
    Memory::Write writeUltimaxA0;    
    
    Memory memoryCpu;
    Memory memoryVic;
    
    uint8_t* ram = nullptr;
    uint8_t* colorRam = nullptr;
    uint8_t* charRom = nullptr;
    unsigned charRomSize = 0;
    uint8_t* kernalRom = nullptr;
    unsigned kernalRomSize = 0;
    uint8_t* basicRom = nullptr;
    unsigned basicRomSize = 0;
    
    Interface* interface;
    MOS65Model* cpu;
    MOS65Context* cpuCtx;
    CIA::M6526* cia1;
    CIA::M6526* cia2;
    ExpansionPort* expansionPort;
    ExpansionPort* noExpansion;
    Prg* prgInUse = nullptr;
    
	Emulator::PowerSupply* powerSupply;
    Input* input;
    KeyBuffer* keyBuffer;
    GlueLogic* glueLogic;
    Emulator::Events events;
	
	Emulator::Crop* crop;
    unsigned serializationSize; 
    
    uint8_t mode; //bit 4: exrom, bit 3: game, bit 2: charen, bit 1: hiram, bit 0: loram
    uint8_t vicBank;
    // the c64 shares multiple sources for irq and nmi
    // achknowledging e.g. an irq from vic side doesn't mean the irq line on cpu changes immediately
    // if cia1 holds up an irq too, the cpu irq pin goes hi if both sources are hi
    
    uint8_t irqIncomming; // bit 0: vicII, bit 1: cia1, bit 2: expansion port
    uint8_t nmiIncomming; // bit 0: keyboard, bit 1: cia2, bit 2: expansion port
    uint8_t rdyIncomming; // bit 0: vicII, bit 1: expansion port
    
    bool ntsc = false;
    bool frameComplete = false;
    Emulator::Serializer serializer;    
    bool kernalBootComplete = false;
    bool powerOn = false;
    bool cycleRendererNextBoot = false;
    
    struct {
        unsigned config;
        unsigned frameCounter;
        bool renderNext;
    } fastForward;
    
    struct {
        unsigned frames = 0;
        unsigned pos = 0;
        bool accuracy = false;
        Emulator::MemSerializer serializer;
    } runAhead;
    
    struct {
        unsigned idleFrames = 0;
        bool idle = false;
        bool active = false;
    } diskSilence;
    
    #include "testbench.h"
    
    auto setFirmware( unsigned typeId, uint8_t* data, unsigned size ) -> void;
    auto setNtsc(bool state) -> void;
    auto isNtsc() -> bool;
    
    auto remapCpu() -> void;
    auto remapVic() -> void;
	auto isUltimax() -> bool;
	auto changeExpansionPortMemoryMode(bool exrom, bool game) -> void;
    
    auto power(bool softReset = false) -> void;
	auto powerOff() -> void;
    auto run() -> void;
    auto initRam() -> void;
    
    auto getCyclesPerSecond() -> unsigned {
        return ntsc ? C64_FREQUENCY_NTSC : C64_FREQUENCY_PAL;
    }
    
    auto getCyclesPerFrame() -> unsigned {
        return ntsc ? C64_CYCLES_FRAME_NTSC : C64_CYCLES_FRAME_PAL;
    }
    
    auto calcSerializationSize() -> void;
    auto serialize(unsigned& size) -> uint8_t*;
    auto serializeLight() -> void;
    auto unserializeLight() -> void;
    auto checkSerialization(uint8_t* data, unsigned size) -> bool;
    auto unserialize(uint8_t* data, unsigned size) -> bool;
    auto serializeAll(Emulator::Serializer& s) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto serializeDiskIdle(Emulator::Serializer& s) -> void;
    auto serializeExpansion(Emulator::Serializer& s) -> void;
    static auto serialize6502( Emulator::Serializer& s, MOS65Context* cpuCtx ) -> void;        
    
    auto setExpansion( Emulator::Interface::Expansion& expansion ) -> void;
    
    auto createExpansions() -> void;
    auto destroyExpansions() -> void;
    auto setExpansionCallbacks( ExpansionPort* expansionPtr ) -> void;
    auto analyzeExpansion(uint8_t* data, unsigned size) -> Emulator::Interface::Expansion*;
    
    auto dispatcha() -> void;
    auto setFastForward( unsigned config ) -> void;    
    auto setRunAhead(unsigned frames) -> void;
    auto setRunAheadAccuracy(bool state) -> void;
    
    auto setCycleRenderer(bool state) -> void;
};

extern System* system;

}
