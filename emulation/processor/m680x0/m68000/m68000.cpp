
#include "m68000.h"

namespace M68FAMILY {	
	
auto M68Model::create68000() -> M68Model* {
	return new M68000;
}
	
enum : uint8_t { Byte, Word, Long };

#include "memory.cpp"
#include "address.cpp"
#include "exception.cpp"
#include "opcodes.cpp"

M68000::M68000( bool derived ) : Base() {
	if(derived) return;
	
	optimized = false;
	uint16_t opcode;	
	#include "../optables/68000.h"

	Base::prepareIllegalExceptions();
}
	
auto M68000::power() -> void {
	ctx->usp = 0;
	for(auto& dreg : ctx->d) dreg = 0;
	for(auto& areg : ctx->a) areg = 0;
	
	reset();
}

auto M68000::reset() -> void {
	ctx->irqPendingLevel = ctx->irqSamplingLevel = 0;
	ctx->level7SamplingTrigger = ctx->level7PendingTrigger = false;
	ctx->stop = false;
	ctx->halt = false;
	ctx->trace = false;
	state.instruction = false;
	/** reset vector fetches are from program space
	 * all other vector fetches are from user space
	 */
	state.data = false;	
	
	ctx->c = 0;
	ctx->v = 0;
	ctx->z = 0;
	ctx->n = 0;
	ctx->x = 0;
	ctx->i = 7;
	ctx->s = 1;
	ctx->t = 0;

	ctx->sync(14);	
	ctx->a[7] = ctx->ssp = read<Long>(0);
	ctx->pc = read<Long>(4);
	fullPrefetch<true>();
}

auto M68000::process() -> void {
	useDefaultContext(); //recover context from group 0 exception, if needed	
	
	if ( ctx->halt ) {
        ctx->cpuHalted();
        ctx->sync(4);
        return; //cpu can recover from reset only
    }
	//maybe another interrupt was recognized while stacking
	if(group1exceptions()) return;
	
	if ( ctx->stop ) {
		sampleIrq();
		ctx->sync(2);
		return; //cpu can recover from interrupt or reset only
	}
	state.instruction = true;
	ctx->trace = ctx->t;
	opTable[ctx->ird]();
}

auto M68000::setInterrupt( uint8_t level, unsigned cycle ) -> void {
	if (level == 7 && ctxDefault->irqPendingLevel < 7) { //edge sensitive
		ctxDefault->level7PendingTrigger = true;
	}
	ctxDefault->irqPendingLevel = level & 7; //level sensitive
}

auto M68000::raiseBusError(uint32_t addr) -> void {
    group0exception( addr, BusError );
}

auto M68000::openBus() -> uint32_t {
	return ctx->irc;
}

auto M68000::sampleIrq() -> void {
	ctx->sampleIrq();
	ctx->irqSamplingLevel = ctx->level7PendingTrigger ? 7 : ctx->irqPendingLevel;
	ctx->level7SamplingTrigger = ctx->level7PendingTrigger;
	ctx->level7PendingTrigger = false;	
}

auto M68000::switchToSupervisor() -> void {
	if (ctx->s) return;	
	ctx->usp = ctx->a[7];	
	ctx->a[7] = ctx->ssp;
	ctx->s = true;	
}

auto M68000::cyclesDivu(uint32_t dividend, uint16_t divisor) -> unsigned {
    if ((dividend >> 16) >= divisor) return 10; //quotient is bigger than 16bit
    uint32_t hdivisor = divisor << 16;

    unsigned mcycles = !optimized ? 38 : 10;
    for (int i = 0; i < 15; i++) {
        if ((int32_t)dividend < 0) {
            dividend <<= 1;
            dividend -= hdivisor;
        } else {
            dividend <<= 1;
            mcycles += 2;
            if (dividend >= hdivisor) {
                dividend -= hdivisor;
                mcycles--;
            }
        }
    }
    return mcycles * 2;
}

auto M68000::cyclesDivs(int32_t dividend, int16_t divisor) -> unsigned {
    unsigned mcycles = 6;
    if (dividend < 0) mcycles++;

    if ((std::abs(dividend) >> 16) >= std::abs(divisor)) return (mcycles + 2) * 2;

    mcycles += !optimized ? 55 : 21;

    if (divisor >= 0) {
        if (dividend >= 0) mcycles--;
        else mcycles++;
    }

    uint32_t aquot = std::abs(dividend) / std::abs(divisor);
    for (int i = 0; i < 15; i++) {
        if ( (int16_t)aquot >= 0) mcycles++;
        aquot <<= 1;
    }

    return mcycles * 2;
}

template<uint8_t Mode> auto M68000::cyclesBit(uint8_t bit) -> void {
    uint8_t cycles = 0;
    
    switch(Mode) {
        case Btst: cycles = 2; break;
        case Bclr: cycles = 2;
        case Bset:
        case Bchg:
            cycles += bit > 15 ? 4 : 2;
            break;
    }
    ctx->sync( cycles );
}


auto M68000::cyclesJmp(uint8_t mode) -> void {
	
	switch(mode) {
		case AddressRegisterIndirectWithIndex:
		case ProgramCounterIndirectWithIndex:
			ctx->sync(4); return;
		case AbsoluteShort:
		case AddressRegisterIndirectWithDisplacement:
		case ProgramCounterIndirectWithDisplacement:
			ctx->sync(2); return;			
	}	
}

}