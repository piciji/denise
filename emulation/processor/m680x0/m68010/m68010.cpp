
#include "m68010.h"

namespace M68FAMILY {	
	
auto M68Model::create68010() -> M68Model* {
	return new M68010;
}

#include "exception.cpp"
#include "opcodes.cpp"

M68010::M68010() : M68000( true ) {
	uint16_t opcode;
	optimized = true;
	
	#include "../optables/68000.h"
	#include "../optables/68010.h"
	#include "../optables/68010LoopMode.h"	

	Base::prepareIllegalExceptions();
}

auto M68010::reset() -> void {	
	ctx->vbr = 0;
	ctx->sfc = ctx->dfc = 0;
	ctx->loopMode = false;
	state.overrideWithAlternateFC = false;
	state.rmw = state.hbTransfer = false;
	M68000::reset();
}

auto M68010::getFunctionCodes() -> uint8_t {
	if (state.overrideWithAlternateFC) {
		return state.alternateFC;
	}
	return Base::getFunctionCodes();
}

auto M68010::cyclesMoves(uint8_t mode) -> void {	
	switch(mode) {
		case AddressRegisterIndirect:
		case AddressRegisterIndirectWithPreDecrement:
		case AddressRegisterIndirectWithIndex:
			ctx->sync(6); return;
			
		case AddressRegisterIndirectWithPostIncrement:
			ctx->sync(8); return;
			
		case AddressRegisterIndirectWithDisplacement:
		case AbsoluteShort:
		case AbsoluteLong:
			ctx->sync(4); return;
	}	
}

template<uint8_t Size> auto M68010::noFetchTail(EffectiveAddress& ea) -> void {
	switch(ea.mode) {
		case AddressRegisterIndirect:
		case AddressRegisterIndirectWithPreDecrement:
		case AddressRegisterIndirectWithIndex:
			ctx->sync(2); break;
		case AddressRegisterIndirectWithPostIncrement:
			ctx->sync(4); break;
	}	
	Base::updateRegA<Size>(ea);
	ea.calculated = true;
}

}
