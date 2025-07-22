
#pragma once

#include <cstdint>
#include <functional>

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

struct M6510 {
	
	M6510(System* system, Emulator::SystemTimer& sysTimer, CIA::M6526& cia1, CIA::M6526& cia2, IecBus& iecBus, Traps& traps);

    System* system;
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
	
	bool interruptSampled;
	bool callResetRoutine;
	
	bool killed;
    bool oddCycle;
    bool reg2mhz;
	
	unsigned busState;
	
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

    auto registerCallbacks() -> void;

	template<bool mhz2> auto process() -> void;
	
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
    
    auto setClock(bool state) -> void;
};

}
