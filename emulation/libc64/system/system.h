
#pragma once

#include <functional>
#include "../../tools/branchPrediction.h"
#include "../interface.h"
#include "../m6510/m6510.h"
#include "../../cia/m6526.h"
#include "../vicII/vicII.h"
#include "memory.h"
#include "../../tools/systimer.h"
#include "../../tools/crop.h"

namespace Emulator {
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
	
	using Callback = std::function<void ()>;
	
    Memory::Read readUnmapped;
    Memory::Write writeUnmapped;
    Memory::Read readRam;
    Memory::Write writeRam;
    Memory::Write writeRamAt80To9F;
    Memory::Write writeRamAtA0ToBF;
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
    
    uint8_t* ram = nullptr;
    uint8_t* colorRam = nullptr;
    uint8_t* charRom = nullptr;
    uint8_t* kernalRom = nullptr;
    uint8_t* basicRom = nullptr;
    
    Interface* interface;  
    ExpansionPort* noExpansion;
    Prg* prgInUse = nullptr;
    
	Emulator::PowerSupply* powerSupply;
    Input* input;
    KeyBuffer* keyBuffer;
    GlueLogic* glueLogic;    
	
	Callback countDownPowerSupply;
	
	Emulator::Crop<uint8_t>* crop;
    unsigned serializationSize; 
    uint8_t requestedSids;
    
    uint8_t mode; //bit 4: exrom, bit 3: game, bit 2: charen, bit 1: hiram, bit 0: loram
    uint16_t vicBank;
    // the c64 shares multiple sources for irq and nmi
    // achknowledging e.g. an irq from vic side doesn't mean the irq line on cpu changes immediately
    // if cia1 holds up an irq too, the cpu irq pin goes hi if both sources are hi
    
    uint8_t irqIncomming; // bit 0: vicII, bit 1: cia1, bit 2: expansion port
    uint8_t nmiIncomming; // bit 0: keyboard, bit 1: cia2, bit 2: expansion port
    uint8_t rdyIncomming; // bit 0: vicII, bit 1: expansion port    
    
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
        bool performance = false;
        Emulator::MemSerializer serializer;
    } runAhead;
    
    struct {
        unsigned idleFrames = 0;
        bool idle = false;
        bool active = false;
    } diskSilence;
    
    struct {
        uint8_t value = 255;
        unsigned invertEvery = 64;
        unsigned randomPatternLength = 1;
        unsigned repeatRandomPattern = 256;
        unsigned randomChance = 0;
        
    } memoryInit;

    struct {
        bool enterRom = false;
        uint8_t memoryAccesses = 0;
    } observer;
    
    #include "testbench.h"
    
    auto setFirmware( unsigned typeId, uint8_t* data, unsigned size ) -> void;
    
    auto remapCpu() -> void;
	auto isUltimax() -> bool;
	auto changeExpansionPortMemoryMode(bool exrom, bool game, bool noUltimaxIfVicHasTheBus = false) -> void;
    
    auto power(bool softReset = false) -> void;
	auto powerOff() -> void;
    auto run() -> void;
    auto initRam(uint8_t*& mem) -> void;   
    auto useExtraSids(uint8_t requestedSids) -> void;
    
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
    
    auto setExpansion( Emulator::Interface::Expansion& expansion ) -> void;
    
    auto createExpansions() -> void;
    auto destroyExpansions() -> void;
    auto setExpansionCallbacks( ExpansionPort* expansionPtr ) -> void;
    auto analyzeExpansion(uint8_t* data, unsigned size, std::string suffix = "") -> Emulator::Interface::Expansion*;
    
    auto setFastForward( unsigned config ) -> void;    
    auto setRunAhead(unsigned frames) -> void;
    auto setRunAheadPerformance(bool state) -> void;
    auto runAheadEnableAudio() -> void;
    
    auto setCycleRenderer(bool state) -> void;
	auto updateStats() -> void;
    auto updateStatsStereo() -> void;
	
	auto updatePort(uint8_t lines, uint8_t ddr) -> void;
	
	auto videoRefresh( uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void;
	auto setVicIrq( bool state ) -> void;
	auto setVicRdy(bool state) -> void;
	auto VicMidScreenCallback() -> void;
	auto VicVblankCallback() -> void;

	auto pasteText( std::string buffer ) -> void;
    auto copyText( ) -> std::string;
    auto runAheadInProgress() -> bool { return runAhead.frames != 0; }

    auto checkForAutoStarter() -> bool;
};

extern System* system;
extern ExpansionPort* expansionPort;
extern CIA::M6526* cia1;
extern CIA::M6526* cia2; 
extern Emulator::SystemTimer sysTimer;

}
