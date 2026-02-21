
#pragma once

#include <functional>
#include "../../tools/macros.h"
#include "../interface.h"
#include "../input/input.h"
#include "../m6510/m6510.h"
#include "../../cia/m6526.h"
#include "../vicII/vicII.h"
#include "../vicII/fast/vicIIFast.h"
#include "../tape/tape.h"
#include "../disk/iec.h"
#include "memory.h"
#include "../../tools/systimer.h"
#include "../../tools/crop.h"
#include "../../tools/circularBuffer.h"
#include "gluelogic.h"
#include "sidManager.h"
#include "../traps/traps.h"
#include "../../tools/memchangetracker.h"
#include "../../tools/history.h"
#include "debuggerSnapshot.h"

namespace Emulator {
    struct PowerSupply;
    struct Serialzer;
}

namespace LIBC64 {

struct Prg;
struct KeyBuffer;
struct ExpansionPort;
struct DebugCart;
struct VicIIBase;
struct Sid;
struct Acia;
struct EasyFlash3;
struct EasyFlash;
struct Fastloader;
struct Freezer;
struct GameCart;
struct GeoRam;
struct Gmod2;
struct RetroReplay;
struct Reu;
struct FinalChessCard;
struct SuperCpu;
struct M6510;
struct IecBus;
struct Prg;

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
    Memory::Read peekVicReg;
    Memory::Write writeVicReg;
    Memory::Read readSidReg;
    Memory::Read peekSidReg;
    Memory::Write writeSidReg;
    Memory::Read readColorRam;
    Memory::Write writeColorRam;
    Memory::Read readIo1Reg;
    Memory::Read peekIo1Reg;
    Memory::Write writeIo1Reg;
    Memory::Read readIo2Reg;
    Memory::Read peekIo2Reg;
    Memory::Write writeIo2Reg;
    Memory::Read readCia1Reg;
    Memory::Read peekCia1Reg;
    Memory::Write writeCia1Reg;
    Memory::Read readCia2Reg;
    Memory::Read peekCia2Reg;
    Memory::Write writeCia2Reg;
    
    Memory::Write writeDebugReg;
    
    Memory::Read readCharRom;
    Memory::Read readKernalRom;
    Memory::Read peekKernalRom;
    Memory::Read readBasicRom; 
    Memory::Read readRomL;
    Memory::Read readRomH;
    Memory::Read peekRomL;
    Memory::Read peekRomH;
    Memory::Write writeRomL;
    Memory::Write writeRomH;
    // need this separated from writeRomL and writeRomH because of not writing to C64 memory
    Memory::Write writeUltimaxRomL;
    Memory::Write writeUltimaxRomH;
    // in ultimax mode a cartridge can map following areas freely.
    // will add more when emulating a cartridge which maps something in unmapped spaces
    Memory::Read readUltimaxA0;
    Memory::Read peekUltimaxA0;
    Memory::Write writeUltimaxA0;    
    
    Memory memoryCpu;
    
    uint8_t* ram = nullptr;
    uint8_t* colorRam = nullptr;
    uint8_t* charRom = nullptr;
    uint8_t* kernalRom = nullptr;
    uint8_t* basicRom = nullptr;

    DebugCart* debugCart = nullptr;
    Interface* interface;  
    ExpansionPort* noExpansion;
    Prg* prgInUse = nullptr;
    
	Emulator::PowerSupply* powerSupply;
    Input input;
    KeyBuffer* keyBuffer;
    GlueLogic glueLogic;
    Emulator::SystemTimer sysTimer;
    CIA::M6526 cia1;
    CIA::M6526 cia2;
    ExpansionPort* expansionPort;

    Acia* acia;
    EasyFlash3* easyFlash3;
    EasyFlash* easyFlash;
    Fastloader* fastloader;
    Freezer* freezer;
    GameCart* gameCart;
    GeoRam* geoRam;
    Gmod2* gmod2;
    RetroReplay* retroReplay;
    Reu* reu;
	FinalChessCard* finalChessCard;
	SuperCpu* superCpu;
    M6510 cpu;
    VicIIBase* vicII;
    VicIICycle vicIICycle;
    VicIIFast vicIIFast;

    SidManager sidManager;

    Traps traps;
    Tape tape;
    IecBus iecBus;
    std::vector<Prg*> prgs;
	
	Callback countDownPowerSupply;
    Callback debuggerAutoUpdater;
	
	Emulator::Crop<uint8_t>* crop;
    unsigned serializationSize;
    unsigned serializationSizeLight;
    uint8_t requestedSids;
    
    uint8_t mode; //bit 4: exrom, bit 3: game, bit 2: charen, bit 1: hiram, bit 0: loram
    // the c64 shares multiple sources for irq and nmi
    // acknowledging e.g. an irq from vic side doesn't mean the irq line on cpu changes immediately
    // if cia1 holds up an irq too, the cpu irq pin goes hi if both sources are hi
    
    uint8_t irqIncomming; // bit 0: vicII, bit 1: cia1, bit 2: expansion port
    uint8_t nmiIncomming; // bit 0: keyboard, bit 1: cia2, bit 2: expansion port
    uint8_t rdyIncomming; // bit 0: vicII, bit 1: expansion port    
    
    bool leaveEmulation = false;
    Emulator::Serializer serializer;    
    bool kernalBootComplete = false;
    bool powerOn = false;
    bool cycleRendererNextBoot = false;
    uint8_t mhz2 = 0;

    // petscii will be converted to ascii or screen codes to be viewed in host
    bool convertToScreencode = false;
    bool loadWithColumn = false;
    
    struct {
        unsigned config = 0;
        unsigned frameCounter;
        bool renderNext;
    } warp;
    
    struct {
        unsigned frames = 0;
        unsigned pos = 0;
        bool performance = false;
        bool preventJit = true;
		bool active = false;
        MemState<uint32_t, uint8_t, 2> memState;
    } runAhead;

    History<uint32_t, uint8_t, 2> history;

    struct {
        unsigned idleFrames = 0;
        bool idle = false;
        bool active = false;
    } diskSilence;

    struct {
        bool requestFloppy;
        bool useFloppy;
		bool requestTape;
		bool useTape;
	} driveSounds;

    struct {
        bool burstRequested = false;
        bool parallelRequested = false;
        bool burstPossible = false;
        bool parallelPossible = false;
        bool burstUse = false;
        bool parallelUse = false;
        bool parallelUserport = false;
        bool parallelExpansion = false;
        bool cycleSyncing = false;
    } secondDriveCable;

    struct {
        bool enterRom = false;
        uint8_t memoryAccesses = 0;
        bool stateChange = false;
        bool motor;
        uint8_t inputFetches = 0;
        bool inputLock = true;
    } observer;

    struct TapeNoise {
        bool enabled = false;
        bool active = false;
        int duration = 0;
        int amplitude = 0;
        CircularBuffer<unsigned> circularBuffer;

        auto reset() -> void;
    } tapeNoise;

    struct {
        Emulator::Interface::DebuggerAction action = Emulator::Interface::DebuggerAction::None;
        uint32_t addr;
        uint32_t dmaWatchers[4] = { 0 };
    } debugger;

    DebuggerSnapshot debuggerSnapshot;

	Emulator::Interface::MemoryPattern memoryInit;
    
    auto setFirmware( unsigned typeId, uint8_t* data, unsigned size, bool allowPatching ) -> void;
    
    auto remapCpu(bool speedHack = false) -> void;
    auto memoryDump(uint8_t page, uint8_t* dump) -> void;
    auto editMemory(uint32_t addr, std::vector<uint16_t> values) -> void;
	auto isUltimax() -> bool;
	auto changeExpansionPortMemoryMode(bool exrom, bool game, bool noUltimaxIfVicHasTheBus = false, bool speedHack = false) -> void;
    
    auto power(bool softReset = false) -> void;
	auto powerOff() -> void;
    auto run() -> void;
    auto initRam(uint8_t*& mem) -> void;   
    auto useExtraSids(uint8_t requestedSids) -> void;
    
    auto calcSerializationSize() -> void;
    auto serialize(unsigned& size) -> uint8_t*;
    auto serializeLight(MemState<uint32_t, uint8_t, 2>& memState, bool fromHistory = false) -> void;
    auto unserializeLight(MemState<uint32_t, uint8_t, 2>& memState, bool fromHistory = false) -> void;
    auto checkSerialization(uint8_t* data, unsigned size) -> bool;
    auto unserialize(uint8_t* data, unsigned size) -> bool;
    auto serializeAll(Emulator::Serializer& s) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto serializeDiskIdle(Emulator::Serializer& s) -> void;
    auto serializeExpansion(Emulator::Serializer& s) -> void;
    
    auto setExpansion( Interface::ExpansionId id ) -> void;
    
    auto createExpansions() -> void;
    auto destroyExpansions() -> void;
    auto setExpansionCallbacks( ExpansionPort* expansionPtr ) -> void;
    auto analyzeExpansion(uint8_t* data, unsigned size, std::string suffix = "") -> Emulator::Interface::Expansion*;
    auto isExpansionUnsupported() -> bool;

    auto hintSlowSpeed(bool state) -> void;
    auto setWarpMode( unsigned config ) -> void;
	auto setRunAhead(unsigned frames) -> void;
    auto setFloppySounds(bool state) -> void;
	auto setTapeSounds(bool state) -> void;
    auto updateDriveSounds() -> void;
    auto setTapeLoadingNoise(unsigned volume) -> void;
	auto runAheadPreventJit() -> bool { return runAhead.preventJit && runAhead.frames; }
    
    auto setCycleRenderer(bool state = true) -> void;
	auto updateStats() -> void;
    auto updateStatsStereo() -> void;
	
	auto updatePort(uint8_t lines, uint8_t ddr) -> void;
	
	auto videoRefresh( uint8_t* frame, unsigned width, unsigned height, unsigned linePitch) -> void;
	auto setVicIrq( bool state ) -> void;
	auto setVicRdy(bool state) -> void;

	auto pasteText( std::string buffer ) -> void;
    auto copyText( ) -> std::string;
    auto runAheadInProgress() -> bool { return runAhead.frames != 0; }

    auto checkForAutoStarter() -> bool;
    auto hintObserverMotorChange(bool state) -> void;
    auto hintObserverLEDChange(bool state) -> void;
    auto informAboutStateChange() -> void;

    auto burstOrParallelUpdate() -> void;
    auto driveCycleSyncingUpdate() -> void;
    auto diskIdleOff() -> void;

    auto readParallelWithHandshake() -> uint8_t;
    auto readParallel() -> uint8_t;
    auto writeParallelHandshake() -> void;
    auto isC64C() -> bool;
    auto activateDebugCart(unsigned limitCycles) -> void;
    auto enabledDebugCart() -> bool;

    auto tapeNoiseEnabled() -> bool { return tapeNoise.enabled && !runAhead.pos; }
    auto tapeNoiseSetSample( unsigned duration ) -> void;
    auto audioRefresh(int16_t sample) -> void;
    auto audioRefreshStereo(int16_t sampleL, int16_t sampleR) -> void;

    auto autoStartFinish(bool soft) -> void;
    auto jam(Emulator::Interface::Media* media = nullptr) -> void;
    auto displayFrame() -> const bool { return !runAhead.pos; }
	auto processFrame() -> const bool { return !runAhead.active || (runAhead.frames == runAhead.pos); }
    auto getPrgInstance(Emulator::Interface::Media* media) -> Prg*;

    auto set2Mhz(bool state) -> void;
    auto toggle2Mhz() -> bool;

    auto debuggerAdd(Emulator::Interface::DebuggerTheme theme, Emulator::Interface::DebuggerAction action, uint32_t addr, uint32_t addrTo) -> void;
	auto debuggerRemove(Emulator::Interface::DebuggerTheme theme, Emulator::Interface::DebuggerAction action, std::optional<unsigned> addr) -> void;
    auto setWatchpointCondition(Emulator::Interface::DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool;

	auto debuggerStepOver() -> void;
	auto debuggerStepInto() -> void;
	auto debuggerStepOut() -> bool;
	auto getMemoryDumpBank(uint8_t bank, uint8_t* dump) -> void;
	auto getMemoryDumpPage(uint8_t page, uint8_t* dump) -> void;

    auto debugPointReached(Emulator::Interface::DebuggerAction action, unsigned addr) -> void;
    auto updateDebuggerSnapshot() -> void;
    auto debuggerUpdate() -> void;
    auto debuggerUpdateEvent() -> void;

	auto disassemble(unsigned addr, unsigned& bytes) -> std::string;
	auto disassembleData(unsigned addr, unsigned bytes) -> std::string;
	auto disassembleTrace(unsigned i, uint8_t& flags) -> std::string;

    auto updateCiaDebuggerSnapshot(DebuggerSnapshot& snap) -> void;
    auto updateMemorySnapshot(DebuggerSnapshot& snap) -> void;

    auto logCpu(uint16_t addr, uint8_t data) -> void;
    auto cropFrame( Emulator::Interface::CropType type, Emulator::Interface::Crop _crop ) -> void;
};

}
