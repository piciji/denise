
#pragma once

#include "../context.h"
#include "memory.h"

namespace M68FAMILY {	
	
struct Results {
						
	std::string ident = "";
	M68Context* ctx = nullptr;
	Memory* memory = nullptr;
		
	bool illegalException = false;
    bool privilegeException = false;
	bool group2Exception = false;
    bool addressError = false;
    bool busError = false;    
	bool irqChecked = false;
	
	int cycles = 0;
	
	static std::vector<Results*> list;
	
	Results(std::string ident, bool add = true);
	~Results();
	
	static auto add(Results* result) -> void;
	static auto get(std::string ident) -> Results*;
	static auto compare( Results* a, Results* b, std::string& error ) -> bool;
	static auto compareMemory(Memory* a, Memory* b, std::string& error) -> bool;
	static auto toHex(int value) -> std::string;
	
	auto setMemory(Memory* memory) -> void {
		this->memory = memory;
	}
	
	auto setContext(M68Context* ctx) -> void {
		this->ctx = ctx;
	}
	
	auto setX() -> Results* {
		ctx->x = true;
		return this;
    }	
    auto setN() -> Results* {
		ctx->n = true;
		return this;
    }
    auto setZ() -> Results* {
		ctx->z = true;
		return this;
    }
    auto setV() -> Results* {
		ctx->v = true;
		return this;
    }
    auto setC() -> Results* {
		ctx->c = true;
		return this;
    }
	auto resetCCR() -> Results* {
		ctx->x = ctx->n = ctx->z = ctx->v = ctx->c = false;
		return this;
	}
	auto resetSR() -> Results* {
		resetCCR();
		ctx->i = 7;
		ctx->s = 1;
		ctx->t = 0;
	}
	
	auto setRegA(unsigned reg, unsigned value) -> Results* {
        ctx->a[reg & 7] = value; 
        return this;
    }

    auto setRegD(unsigned reg, unsigned value) -> Results* {
        ctx->d[reg & 7] = value;
        return this;
    }

	auto setCycleCount(unsigned value) -> void {
        cycles = value;
    }	
};

}