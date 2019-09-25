
#include "m6502.h"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )

#define GET_INDEX_REG regIndex == RegX ? X : ( regIndex == RegY ? Y : X )

namespace MOS65FAMILY {

// (d,x)
auto M6502::indexedIndirectAdr() -> uint16_t {
	
	ctx->zeroPage = readPCInc<1>();
	read<2>( ctx->zeroPage ); //need time for adding x register
    
	ctx->absolute = loadZeroPage<3>( ctx->zeroPage + ctx->x );
	ctx->absolute |= loadZeroPage<4>( ctx->zeroPage + ctx->x + 1 ) << 8;
	
	return ctx->absolute;
}

// (d), y
auto M6502::indirectIndexedAdr( bool forceExtraCycle ) -> uint16_t {
    
    ctx->zeroPage = readPCInc<1>();

    ctx->absolute = loadZeroPage<2>( ctx->zeroPage );
    ctx->absolute |= loadZeroPage<3>( ctx->zeroPage + 1 ) << 8;    

    ctx->absIndexed = ctx->absolute + ctx->y;
    
    ctx->boundaryCrossing = PAGE_CROSSED(ctx->absolute, ctx->absolute + ctx->y); 

    if (ctx->isDummy|| forceExtraCycle || ctx->boundaryCrossing)
        read<4>((ctx->absolute & 0xff00) | (ctx->absIndexed & 0xff));
	
    return ctx->absIndexed;
}

// d,x  d,y
template<M6502::Reg regIndex> auto M6502::zeroPageIndexedAdr( ) -> uint8_t {
    
    ctx->zeroPage = readPCInc<1>();
    loadZeroPage<2>( ctx->zeroPage );
    
    return ctx->zeroPage + (GET_INDEX_REG);
}

// a
auto M6502::absoluteAdr( ) -> uint16_t {
	
	ctx->absolute = readPCInc<1>();
	ctx->absolute |= readPCInc<2>() << 8;
	
	return ctx->absolute;
}

// a,x  a,y
template<M6502::Reg regIndex> auto M6502::absoluteIndexedAdr( bool forceExtraCycle ) -> uint16_t {
	
	absoluteAdr();
   
    ctx->boundaryCrossing = PAGE_CROSSED(ctx->absolute, ctx->absolute + (GET_INDEX_REG));
	
	ctx->absIndexed = ctx->absolute + (GET_INDEX_REG);

	if (ctx->isDummy || forceExtraCycle || ctx->boundaryCrossing)
		read<3>((ctx->absolute & 0xff00) | (ctx->absIndexed & 0xff));

	return ctx->absIndexed;
}

}

#undef PAGE_CROSSED
#undef GET_INDEX_REG
