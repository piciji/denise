
#include "m6502.h"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )

#define GET_INDEX_REG regIndex == RegX ? X : ( regIndex == RegY ? Y : X )

namespace MOS65FAMILY {

// (d,x)
auto M6502::indexedIndirectAdr() -> void {
	
	ctx->zeroPage = std::move( readPCInc<1>() );
	read<2>( ctx->zeroPage ); //need time for adding x register
    
	ctx->absolute = std::move( loadZeroPage<3>( ctx->zeroPage + ctx->x ) );
	ctx->absolute |= std::move( loadZeroPage<4>( ctx->zeroPage + ctx->x + 1 ) ) << 8;	
}

// (d), y
auto M6502::indirectIndexedAdr( bool forceExtraCycle ) -> void {
    
    ctx->zeroPage = std::move( readPCInc<1>() );

    ctx->absolute = std::move( loadZeroPage<2>( ctx->zeroPage ) );
    ctx->absolute |= std::move( loadZeroPage<3>( ctx->zeroPage + 1 ) ) << 8;    

    ctx->absIndexed = ctx->absolute + ctx->y;
    
    ctx->boundaryCrossing = PAGE_CROSSED(ctx->absolute, ctx->absolute + ctx->y); 

    if (workCtx->useDummy|| forceExtraCycle || ctx->boundaryCrossing)
        read<4>((ctx->absolute & 0xff00) | (ctx->absIndexed & 0xff));	
}

// d,x  d,y
template<M6502::Reg regIndex> auto M6502::zeroPageIndexedAdr( ) -> void {
    
    ctx->zeroPage = std::move( readPCInc<1>() );
    loadZeroPage<2>( ctx->zeroPage );
    
    ctx->zeroPage += (GET_INDEX_REG);
}

// a
auto M6502::absoluteAdr( ) -> void {
	
	ctx->absolute = std::move( readPCInc<1>() );
	ctx->absolute |= std::move( readPCInc<2>() ) << 8;	
}

// a,x  a,y
template<M6502::Reg regIndex> auto M6502::absoluteIndexedAdr( bool forceExtraCycle ) -> void {
	
	absoluteAdr();
   
    ctx->boundaryCrossing = PAGE_CROSSED(ctx->absolute, ctx->absolute + (GET_INDEX_REG));
	
	ctx->absIndexed = ctx->absolute + (GET_INDEX_REG);

	if (workCtx->useDummy || forceExtraCycle || ctx->boundaryCrossing)
		read<3>((ctx->absolute & 0xff00) | (ctx->absIndexed & 0xff));
}

}

#undef PAGE_CROSSED
#undef GET_INDEX_REG
