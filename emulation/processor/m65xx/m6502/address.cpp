
#include "m6502.h"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )

namespace MOS65FAMILY {

// (d,x)
auto M6502::indexedIndirectAdr() -> uint16_t {
	
	ctx->mem.zeroPage = readPCInc<1>();
	read<2>( ctx->mem.zeroPage ); //need time for adding x register
    
	ctx->mem.absolute = loadZeroPage<3>( ctx->mem.zeroPage + ctx->x );
	ctx->mem.absolute |= loadZeroPage<4>( ctx->mem.zeroPage + ctx->x + 1 ) << 8;
	
	return ctx->mem.absolute;
}

// (d), y
auto M6502::indirectIndexedAdr( bool forceExtraCycle ) -> uint16_t {
    
    ctx->mem.zeroPage = readPCInc<1>();

    ctx->mem.absolute = loadZeroPage<2>( ctx->mem.zeroPage );
    ctx->mem.absolute |= loadZeroPage<3>( ctx->mem.zeroPage + 1 ) << 8;    

    ctx->mem.absIndexed = ctx->mem.absolute + ctx->y;
    
    if (!ctx->jumpOut.active)
        ctx->mem.boundaryCrossing = PAGE_CROSSED(ctx->mem.absolute, ctx->mem.absolute + ctx->y); 

    if (forceExtraCycle | ctx->mem.boundaryCrossing)
        read<4>((ctx->mem.absolute & 0xff00) | (ctx->mem.absIndexed & 0xff));
	
    return ctx->mem.absIndexed;
}

// d,x  d,y
auto M6502::zeroPageIndexedAdr( uint8_t index ) -> uint8_t {
    
    ctx->mem.zeroPage = readPCInc<1>();
    loadZeroPage<2>( ctx->mem.zeroPage );
    
    return ctx->mem.zeroPage + index;
}

// a
auto M6502::absoluteAdr( ) -> uint16_t {
	
	ctx->mem.absolute = readPCInc<1>();
	ctx->mem.absolute |= readPCInc<2>() << 8;
	
	return ctx->mem.absolute;
}

// a,x  a,y
auto M6502::absoluteIndexedAdr( uint8_t index, bool forceExtraCycle ) -> uint16_t {
	
	absoluteAdr();
    
    if (!ctx->jumpOut.active)
        ctx->mem.boundaryCrossing = PAGE_CROSSED(ctx->mem.absolute, ctx->mem.absolute + index);
	
	ctx->mem.absIndexed = ctx->mem.absolute + index;

	if (forceExtraCycle | ctx->mem.boundaryCrossing)
		read<3>((ctx->mem.absolute & 0xff00) | (ctx->mem.absIndexed & 0xff));

	return ctx->mem.absIndexed;
}

}

#undef PAGE_CROSSED