
#include "m6502.h"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )

namespace MOS65FAMILY {

// (d,x)
auto M6502::indexedIndirectAdr() -> uint16_t {
	
	auto zeroPage = readPCInc();
	read( zeroPage ); //need time for adding x register
    
	uint16_t absolute = loadZeroPage( zeroPage + ctx->x );
	absolute |= loadZeroPage( zeroPage + ctx->x + 1 ) << 8;
	
	return absolute;
}

// (d), y
auto M6502::indirectIndexedAdr( bool forceExtraCycle ) -> uint16_t {
    
    auto zeroPage = readPCInc();

    uint16_t absolute = loadZeroPage( zeroPage );
    absolute |= loadZeroPage( zeroPage + 1 ) << 8;    

    uint16_t absIndexed = absolute + ctx->y;
    
	bool boundaryCrossing = PAGE_CROSSED(absolute, absolute + ctx->y); 

    if (forceExtraCycle | boundaryCrossing)
        read((absolute & 0xff00) | (absIndexed & 0xff));
	
	ctx->memory.absolute = absolute;
	ctx->memory.boundaryCrossing = boundaryCrossing;
	
    return absIndexed;
}

// d,x  d,y
auto M6502::zeroPageIndexedAdr( uint8_t index ) -> uint8_t {
    
    auto zeroPage = readPCInc();
    loadZeroPage( zeroPage );
    
    return zeroPage + index;
}

// a
auto M6502::absoluteAdr( ) -> uint16_t {
	
	uint16_t absolute = readPCInc();
	absolute |= readPCInc() << 8;
	
	return absolute;
}

// a,x  a,y
auto M6502::absoluteIndexedAdr( uint8_t index, bool forceExtraCycle ) -> uint16_t {
	
	uint16_t absolute = absoluteAdr();
    
	bool boundaryCrossing = PAGE_CROSSED(absolute, absolute + index);
	
	uint16_t absIndexed = absolute + index;

	if (forceExtraCycle | boundaryCrossing)
		read((absolute & 0xff00) | (absIndexed & 0xff));
	
	ctx->memory.absolute = absolute;
	ctx->memory.boundaryCrossing = boundaryCrossing;

	return absIndexed;
}

}

#undef PAGE_CROSSED