
#pragma once

#include <assert.h>
#include "range.h"
#include "../model.h"

namespace M68FAMILY {
	
struct Base : M68Model {
	Base();
		
	enum : uint8_t { Byte, Word, Long };
	enum : uint8_t { BusError, AddressError };
	enum : uint8_t { None, NoCyclesPreDec, NoCyclesD8, NoLastPrefetch };
	enum : uint8_t {
		DataRegisterDirect = 0,
		AddressRegisterDirect = 1,
		AddressRegisterIndirect = 2,
		AddressRegisterIndirectWithPostIncrement = 3,
		AddressRegisterIndirectWithPreDecrement = 4,
		AddressRegisterIndirectWithDisplacement = 5,
		AddressRegisterIndirectWithIndex = 6,
		AbsoluteShort = 7 + 0,
		AbsoluteLong = 7 + 1,
		ProgramCounterIndirectWithDisplacement = 7 + 2,
		ProgramCounterIndirectWithIndex = 7 + 3,
		Immediate = 7 + 4,
	};
	
	auto setInterruptType( uint8_t type ) -> void;
	auto setContext( M68Context* context ) -> void;
	auto getFunctionCodes() -> uint8_t;

protected:
	M68Context* ctx;
	M68Context* ctxDefault;
	M68Context ctxError;
	uint8_t interruptType = AUTO_VECTOR;
	std::function<void ()> opTable[0x10000];
	bool optimized = false;
	
	struct { //no context relation in between opcodes 
		bool write = false; //read or write
		bool instruction = true; //instruction or not (group 0 / 1 exceptions)
		bool data = false;	//program or data
		bool wordTransfer = true;
		bool rmw = false; //read-modify-write cycle
		bool hbTransfer = false; //movep only
		uint32_t inputBuffer = 0;
		uint32_t outputBuffer = 0;
		uint8_t alternateFC; //alternate Function codes (68010+)
		bool overrideWithAlternateFC = false;
	} state; //usage state	
	
	struct EffectiveAddress {
		EffectiveAddress(uint8_t mode, uint8_t reg) : mode(mode), reg(reg) {
			if (mode == 7) mode += reg;
		}
		auto registerMode() -> bool { return mode == DataRegisterDirect || mode == AddressRegisterDirect; }
		auto immediateMode() -> bool { return mode == Immediate; }
		auto memoryMode() -> bool { return !registerMode() && !immediateMode(); }
		auto directMode() -> bool { return registerMode() || immediateMode(); }
		auto absMode() -> bool { return mode == AbsoluteShort || mode == AbsoluteLong; }
		auto indexMode() -> bool { return mode == AddressRegisterIndirectWithIndex || mode == ProgramCounterIndirectWithIndex; }
		auto pcMode() -> bool { return mode == ProgramCounterIndirectWithDisplacement || mode == ProgramCounterIndirectWithIndex; }
				
		uint8_t mode;
		uint8_t reg;
		bool calculated = false;
		uint32_t address;
	};

	struct AddressRegister {
		AddressRegister(uint8_t _pos) : pos(_pos & 7) {}
		uint8_t pos;
	};
	
	struct DataRegister {
		DataRegister(uint8_t _pos) : pos(_pos & 7) {}
		uint8_t pos;
	};
	
	auto getCCR() -> uint8_t;
	auto setCCR(uint8_t data) -> void;
	auto getSR() -> uint16_t;	
	auto setSR(uint16_t data) -> void;

	auto getInterruptVector(uint8_t level) -> uint8_t;
	auto getIllegalVector(uint8_t pattern) -> uint8_t;
	auto prepareIllegalExceptions() -> void;
	virtual auto illegalException(uint8_t vector) -> void = 0;
		
	template<uint8_t Size = Long> auto read(DataRegister reg) -> uint32_t {
		return clip<Size>( ctx->d[reg.pos] );
	}

	template<uint8_t Size = Long> auto write(DataRegister reg, uint32_t data) -> void {
		ctx->d[reg.pos] = (ctx->d[reg.pos] & ~mask<Size>()) | (data & mask<Size>());
	}

	template<uint8_t Size = Long> auto read(AddressRegister reg) -> uint32_t {
		return clip<Size>( ctx->a[reg.pos] );
	}

	auto write(AddressRegister reg, uint32_t data) -> void {
		ctx->a[reg.pos] = data;
	}

	auto testCondition(uint8_t code) -> bool;
	
	//logic
	enum : uint8_t { Asl, Asr, Lsl, Lsr, Rol, Ror, Roxl, Roxr };
	enum : uint8_t { Bchg, Bclr, Bset, Btst };
	enum : uint8_t { Add, And, Sub, Or, Eor, Abcd, Sbcd };
	enum : uint8_t { Bfchg, Bfclr, Bfexts, Bfextu, Bfffo, Bfins, Bfset, Bftst };
	#include "logic.t.h"
	auto abcd(uint8_t src, uint8_t dest) -> uint8_t;
	auto sbcd(uint8_t src, uint8_t dest) -> uint8_t;
	
	//helper
	template<uint8_t Size> auto clip(uint32_t data) -> uint32_t;
	template<uint8_t Size> auto mask() -> uint32_t;
	template<uint8_t Size> auto msb() -> uint32_t;
	template<uint8_t Size> auto sign(uint32_t data) -> int32_t;
	
	template<uint8_t Size> auto zero(uint32_t result) -> bool {
		return clip<Size>(result) == 0;
	}

	template<uint8_t Size> auto negative(uint32_t result) -> bool {
		return sign<Size>(result) < 0;
	}
	
	template<uint8_t Size> auto bytes() -> uint8_t;
	static auto parse(const char* s, uintmax_t sum = 0) -> uintmax_t;
	
	auto useDefaultContext() -> void { ctx = ctxDefault; }
	auto useErrorContext() -> void { ctx = &ctxError; }	
};

}