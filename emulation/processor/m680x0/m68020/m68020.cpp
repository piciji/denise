
#include "m68020.h"

namespace M68FAMILY {
	
auto M68Model::create68020() -> M68Model* {
	return new M68020;
}
	
enum : uint8_t { Byte, Word, Long };

#include "memory.cpp"
#include "address.cpp"
#include "exception.cpp"
#include "opcodes.cpp"
	
M68020::M68020() : Base() {
	uint16_t opcode;
	#include "../optables/68000.h"
	#include "../optables/68010.h"
	#include "../optables/68020.h"

	Base::prepareIllegalExceptions();
}

auto M68020::power() -> void {
	ctx->usp = ctx->msp = 0;
	ctx->ca_c = ctx->ca_ce = false;
	for(auto& dreg : ctx->d) dreg = 0;
	for(auto& areg : ctx->a) areg = 0;
	
	reset();
}

auto M68020::reset() -> void {
	ctx->irqPendingLevel = ctx->irqSamplingLevel = 0;
	ctx->stop = false;
	ctx->halt = false;
	ctx->trace = false;
	state.instruction = false;
	state.data = false;	
	ctx->intrHis.clear();
	ctx->busCycles = ctx->absCycles = ctx->lastBusAccessCycles = 0;
	
    ctx->caar = 0;
	ctx->ca_f = ctx->ca_e = false;
	ctx->vbr = ctx->sfc = ctx->dfc = 0;
	for(auto i : range(64)) ctx->cache.valid[i] = false;

	ctx->c = 0;
	ctx->v = 0;
	ctx->z = 0;
	ctx->n = 0;
	ctx->x = 0;
	ctx->i = 7;
	ctx->s = 1;
	ctx->t = 0;
	ctx->t0 = 0;
	ctx->m = 0;
	addSequencer(14);	
	
	ctx->a[7] = ctx->ssp = read<Long>(0);
	executeAt( 1 );
}

auto M68020::process() -> void {
	useDefaultContext(); //recover context from group 0 exception if needed	
	
	if ( ctx->halt ) {
        ctx->cpuHalted();
        ctx->sync(4);
        return; //cpu can recover from reset only
    }
	
	sampleIrq();
	//maybe another interrupt was recognized while stacking
	if(group1exceptions()) return;
	
	if ( ctx->stop ) {
		addSequencer(2);
		return; //cpu can recover from interrupt or reset only
	}
	state.instruction = true;
	ctx->trace = ctx->t;
	opTable[ctx->stageD]();
}

auto M68020::raiseBusError(uint32_t addr) -> void {
//    group0exception( addr, BUS_ERROR );
}

auto M68020::openBus() -> uint32_t {
	return ctx->cacheHolding;
}

auto M68020::getFunctionCodes() -> uint8_t {
	if (state.overrideWithAlternateFC) {
		return state.alternateFC;
	}
	return Base::getFunctionCodes();
}

   
auto M68020::setInterrupt( uint8_t level, unsigned cycle ) -> void {
	if (cycle > ctxDefault->absCycles) cycle = 0;
	else cycle = ctxDefault->absCycles - cycle;
	
	if (ctxDefault->irqPendingLevel != 7 && level == 7) {
		ctxDefault->intrHis.insert(ctxDefault->intrHis.begin(), {cycle, 7, true} );
	} else {
		ctxDefault->intrHis.insert(ctxDefault->intrHis.begin(), {cycle, level & 7, false} );
	}
	ctxDefault->irqPendingLevel = level & 7;
}

auto M68020::sampleIrq() -> void {
    ctx->sampleIrq();
	
	int consumed = ctx->absCycles - ctx->busCycles; //abs sequencer
	consumed -= 2; //latest possible detection is two cycles before opcode edge
	bool intrIncomming = false;
	
	if (consumed > 0) {
		while( ctx->intrHis.size() ) {
			auto& intr = ctx->intrHis[0];
			if (intr.cycle <= consumed) {
				intrIncomming = true;
				if (intr.l7Trigger || intr.level > ctx->i) {
					if (intr.level > ctx->irqSamplingLevel)
						ctx->irqSamplingLevel = intr.level;
				}
				ctx->intrHis.erase(ctx->intrHis.begin());				
			} else
				break;
		}		
	}
	if (!intrIncomming) {
		if (ctx->irqPendingLevel > ctx->i) {
			ctx->irqSamplingLevel = ctx->irqPendingLevel;
		}
	}
	
	if (ctx->absCycles > 1000) {
		for(auto& intr : ctx->intrHis) {
			intr.cycle -= consumed;
		}
		ctx->absCycles = ctx->busCycles + 2;
	}
}
    
auto M68020::addBus(unsigned cycles) -> void {
	ctx->lastBusAccessCycles += cycles;
	ctx->busCycles += cycles;
	ctx->absCycles += cycles;
	ctx->sync( cycles );	
}

auto M68020::addSequencer(unsigned cycles, bool syncBus) -> void {	
	ctx->busCycles -= cycles;

	if (ctx->busCycles < 0) {
		ctx->absCycles += cycles;
		ctx->sync( -ctx->busCycles );
		ctx->busCycles = 0;
	}
	
	if (syncBus) syncToBus();
}

auto M68020::syncToBus() -> void {	
	if (ctx->lastBusAccessCycles && (ctx->busCycles > ctx->lastBusAccessCycles)) {
		ctx->busCycles = ctx->lastBusAccessCycles;
	}
	ctx->lastBusAccessCycles = 0;
}

auto M68020::catchUpSequencer() -> void {
	ctx->lastBusAccessCycles = ctx->busCycles = 0;
}

auto M68020::addWaitStates(unsigned cycles) -> void {
	ctx->busCycles += cycles;
	ctx->lastBusAccessCycles += cycles;
	ctx->absCycles += cycles;
}

}