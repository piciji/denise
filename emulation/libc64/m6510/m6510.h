
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <optional>
#include "m65Debugger.h"
#include "../../tools/watcher.h"
#include "../system/memory.h"
#include "../../interface.h"

namespace Emulator {
    struct SystemTimer;
    struct Serializer;
}

namespace CIA {
    struct M6526;
}

namespace LIBC64 {
typedef Emulator::Interface::DebuggerAction DebuggerAction;
typedef Emulator::Interface::DebuggerTheme DebuggerTheme;

#define CPU_WRITE_CYCLE 0x80000000
#define CPU_RDY_CYCLE	0x40000000

struct System;
struct ExpansionPort;
struct VicIIBase;
struct IecBus;
struct Traps;
struct DebuggerSnapshot;

struct M6510 : M65Debugger {
    friend struct WatchPoints;
    friend struct ModifiedCodes;
    friend struct HistoryHandler;

    enum { Normal = 0, IRQ = 1, Halt = 2, ResetRoutine = 4};

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
    uint8_t lastBus;
	
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
	
	uint8_t bit6charge;
	uint8_t bit7charge;
	
	using Callback = std::function<void ()>;
	Callback unChargeBit6;
	Callback unChargeBit7;

    auto registerCallbacks() -> void;

	template<bool mhz2, bool busLogger> auto process() -> void;
	
	template<bool sampleInterrupt, bool rememberRdy, bool mhz2, bool busLogger, bool nextOp = false> auto busRead( uint16_t addr ) -> uint8_t;
	
	template<bool setI, bool mhz2, bool busLogger> auto busAccessUpdateFlagI( uint16_t addr ) -> void;
	
	template<bool mhz2, bool busLogger> auto busWrite( uint16_t addr, uint8_t value ) -> void;
	
	auto busWatch() -> uint8_t;
	
	template<bool software, bool mhz2, bool busLogger> auto interrupt() -> void;
	
	auto power() -> void;
	
	auto reset() -> void;
	
	template<bool mhz2, bool busLogger> auto resetRoutine() -> void;
	
	auto setIrq(bool state) -> void;
	
	auto setNmi(bool state) -> void;
	
	auto setRdy(bool state) -> void;
	
	auto addressBus() -> uint16_t { return busState & 0xffff; }
	
	auto isWriteCycle() -> bool { return busState & CPU_WRITE_CYCLE; }
	
	auto updateIoLines( uint8_t pullup, uint8_t pulldown = 0 ) -> void;
	
	auto updateLines() -> void;
	
	auto chargeUndefinedBits( uint8_t newDdr ) -> void;
	
	auto serialize(Emulator::Serializer& s) -> void;
    
	auto setClock(bool state, bool aggressive = false) -> void;

    auto getFlags() -> uint8_t;

    auto loadTrace(Emulator::HistoryEntry<uint8_t>& entry) -> void;

    auto flagDebugAction(int action, bool state) -> void;

    auto controlBreaks() -> void;

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;

    auto parseExpressionValue(const std::string& input, int& pos) -> uint32_t;

    auto peek(uint16_t addr) -> uint8_t;
};

}
