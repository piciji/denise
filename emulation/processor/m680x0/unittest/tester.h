
#pragma once

#include "../m68000/m68000.h"

namespace M68FAMILY {	
	
struct Results;
struct Memory;

struct M68000Tester : M68000 {
	M68000Tester();
	Results* calced = nullptr;
	Memory* memory = nullptr;
	unsigned adrCounter;
	unsigned errorCounter = 0;
	
	auto run() -> void;
	auto setUp() -> void;	
	auto group0exception(uint32_t addr, uint8_t type) -> void;
	auto illegalException(uint8_t vector) -> void;
	auto trapException(uint8_t vector) -> void;
	auto check(std::string ident) -> void;
	auto setContext( M68Context* context ) -> void;
	auto getEA(uint8_t mode, uint8_t reg = 0) -> unsigned;
	auto errorCount() -> void;
	
	auto addWord(uint16_t word) -> void;

	auto setRegA(unsigned reg, unsigned value) -> void {
		ctx->a[reg & 7] = value;
	}

	auto setRegD(unsigned reg, unsigned value) -> void {
		ctx->d[reg & 7] = value;
	}
	
	void testNbcd();
	void sampleNbcd();
	void testSbcd();
	void sampleSbcd();
};
	
}
