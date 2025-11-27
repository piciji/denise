
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <optional>
#include "watcher.h"
#include "../system/memory.h"

namespace Emulator {
    struct SystemTimer;
    struct Serializer;
}

namespace CIA {
    struct M6526;
}

namespace LIBC64 {

#define CPU_WRITE_CYCLE 0x80000000
#define CPU_RDY_CYCLE	0x40000000

struct System;
struct ExpansionPort;
struct VicIIBase;
struct IecBus;
struct Traps;
struct DebuggerSnapshot;

struct M6510 {
    friend struct WatchPoints;
    friend struct ModifiedCodes;
    friend struct HistoryHandler;

    enum { Normal = 0, IRQ = 1, Halt = 2, ResetRoutine = 4,
        WatchPoint = 8, BreakPoint = 0x10, ExceptionPoint = 0x20, SoftStop = 0x40, ModifiedCode = 0x80,
        History = 0x100
    };

    enum class DebuggerAction { None, Breakpoint, Watchpoint, ExceptionPoint, Softstop, ModifiedCode, History };

	M6510(System* system, Emulator::SystemTimer& sysTimer, CIA::M6526& cia1, CIA::M6526& cia2, IecBus& iecBus, Traps& traps);

    System* system;
    Memory& memory;
    Emulator::SystemTimer& sysTimer;
    CIA::M6526& cia1;
    CIA::M6526& cia2;
    IecBus& iecBus;
    Traps& traps;

    ExpansionPort* expansionPort;
    VicIIBase* vicII;

	bool rdyLine;
	
	bool irqPending;
	bool nmiPending;
	bool nmiDetect;

    bool oddCycle;
    uint8_t reg2mhz;
	
	unsigned busState;
    int control;
	
	uint16_t pc;
	
	uint8_t regX;
	
	uint8_t regY;
	
	uint8_t regA;
	
	uint8_t regS;
	
	uint8_t regP;
	
	uint8_t flagZ;
	
	uint8_t flagN;
	
	uint8_t ddr;
	
	uint8_t por;
	
	uint8_t ioLines;
	
	uint8_t pullup;
	
	uint8_t pulldown;
	
	uint8_t magicAne = 0xef;
	uint8_t magicLax = 0xee;
	
	uint8_t bit6charge;	
	uint8_t bit7charge;
	
	using Callback = std::function<void ()>;
	Callback unChargeBit6;
	Callback unChargeBit7;

    WatchPoints watchPoints = WatchPoints(*this, WatchPoint);
    WatchPoints breakPoints = WatchPoints(*this, BreakPoint);
    WatchPoints exceptionPoints = WatchPoints(*this, ExceptionPoint);
    ModifiedCodes modifiedCode = ModifiedCodes(*this, ModifiedCode);
    HistoryHandler historyHandler = HistoryHandler(*this, History);
    std::optional<uint16_t> softStep = std::nullopt;

    auto registerCallbacks() -> void;

	template<bool mhz2, bool postBreakCheck = false> auto process() -> void;
	
	template<bool sampleInterrupt, bool rememberRdy, bool mhz2> auto busRead( uint16_t addr ) -> uint8_t;
	
	template<bool setI, bool mhz2> auto busAccessUpdateFlagI( uint16_t addr ) -> void;
	
	template<bool mhz2> auto busWrite( uint16_t addr, uint8_t value ) -> void;
	
	auto busWatch() -> uint8_t;
	
	template<bool software, bool mhz2> auto interrupt() -> void;
	
	auto power() -> void;
	
	auto reset() -> void;
	
	template<bool mhz2> auto resetRoutine() -> void;
	
	auto setIrq(bool state) -> void;
	
	auto setNmi(bool state) -> void;
	
	auto setRdy(bool state) -> void;
	
	auto addressBus() -> uint16_t { return busState & 0xffff; }
	
	auto isWriteCycle() -> bool { return busState & CPU_WRITE_CYCLE; }
	
	auto updateIoLines( uint8_t pullup, uint8_t pulldown = 0 ) -> void;
	
	auto updateLines() -> void;
	
	auto chargeUndefinedBits( uint8_t newDdr ) -> void;
	
	auto setMagicForAne(uint8_t magicAne) -> void;

    auto getMagicForAne() -> uint8_t { return magicAne; }
	
	auto setMagicForLax(uint8_t magicLax) -> void;

    auto getMagicForLax() -> uint8_t { return magicLax; }
	
	auto serialize(Emulator::Serializer& s) -> void;
    
	auto setClock(bool state, bool aggressive = false) -> void;

    auto inDebugMode() -> bool {
        return control & (WatchPoint | BreakPoint | ExceptionPoint | SoftStop | History);
    }

    auto getFlags() -> uint8_t;

    auto flagDebugAction(int action, bool state) -> void;

    auto disassemble(uint16_t addr, unsigned& bytes, const uint8_t* memSnap = nullptr) -> std::string;

    auto disassembleData(uint16_t addr, unsigned bytes) -> std::string;

    auto controlBreaks() -> void;

    auto checkSoftStop(uint16_t addr) -> bool;

    auto disassembleTrace(unsigned i, uint8_t& flags) -> std::string;

    auto debuggerStepOver() -> void;
    auto debuggerStepInto() -> void;
    auto debuggerAdd(DebuggerAction action, uint16_t addr, uint16_t addrTo = 0) -> void;
    auto debuggerRemove(DebuggerAction action, uint16_t addr) -> void;
    auto debuggerEnable(DebuggerAction action, uint16_t addr) -> void;
    auto debuggerDisable(DebuggerAction action, uint16_t addr) -> void;
    auto debuggerDisableAll() -> void;

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;

    auto hasModifiedCode() -> bool { return modifiedCode.getAndForget(); }
};

}
