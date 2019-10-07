
#include "m6510.h"

/**
 * the exact discharge time for bit 6 and 7 is temperature dependant.
 * it's hard to emulate in a digital environment
 */
#define FALL_OFF_CYCLES 350000

namespace MOS65FAMILY {
    
auto M65Model::create6510() -> M65Model* {
	return new M6510;
}
	
auto M6510::reset() -> void {
    
	ctx->ddr = 0; //input mode
	ctx->por = 0;
	ctx->ioLines = 0;
	
	ctx->bit6.cycles = ctx->bit7.cycles = 0;
	ctx->bit6.charge = ctx->bit7.charge = 0;
	
	M6502::reset();
}

auto M6510::updateIoLines( uint8_t pullup, uint8_t pulldown ) -> void {    
    ctx->pullup = pullup;
    ctx->pulldown = pulldown;
    
    updateLines();
}

auto M6510::setSo(bool state) -> void {
    // external overflow is not supported by 6510
}

inline auto M6510::busRead( uint16_t addr ) -> uint8_t {        
    
	advanceCounter();
    
	if (addr == 0x0000) {
        // read without updating data bus ?
        ctx->readSelect();
        
		return ctx->ddr;   
    }
    
	if (addr == 0x0001) {
        // read without updating data bus ?
        ctx->readSelect();
        
		uint8_t data = ctx->ioLines;
		
		if ( !(ctx->ddr & 0x40) ) {
			data &= ~0x40;
			data |= ctx->bit6.charge;
		}

		if ( !(ctx->ddr & 0x80) ) {
			data &= ~0x80;
			data |= ctx->bit7.charge;
		}
		
		return data;
	}		
	  
	return M6502::busRead( addr );
}

inline auto M6510::busWrite( uint16_t addr, uint8_t data ) -> void {
    
	advanceCounter();
	
    // places write on bus for $00 and $01 too. for these two addresses
    // it seems only the address is selected on bus in write mode but not the data.
    // so the write happens with last data on external bus and not this data.
    // it's video data in case of c64, readed in first half cycle.
    
	if (addr == 0x0000) {						
		
		chargeUndefinedBits( data );
		ctx->ddr = data;		
		updateLines();
        // don't update data bus, see eplanation above.        
        ctx->writeSelect(addr);
		
	} else if (addr == 0x0001) {
		
		ctx->por = data;
		updateLines();			
        // don't update data bus, see eplanation above
        ctx->writeSelect(addr);
        
	} else        
        M6502::busWrite( addr, data );
}

inline auto M6510::busWatch() -> uint8_t {
    
    advanceCounter();
    
    return M6502::busWatch();    
}

auto M6510::updateLines() -> void {			

	ctx->ioLines = ( ctx->por & ctx->ddr ) | ( ~ctx->ddr & ( (ctx->pullup | ctx->ioLines) & ~ctx->pulldown ) );
	
    //external device can distingish between input and output because of voltage level 
	ctx->updatePort( ctx->ioLines, ctx->ddr );
}

auto M6510::chargeUndefinedBits( uint8_t newDdr ) -> void {
	
	if ( (ctx->ddr & 0x80) && !(newDdr & 0x80) ) {
	
		ctx->bit7.cycles = FALL_OFF_CYCLES;
		ctx->bit7.charge = ctx->por & 0x80;
	}
	
	if ( (ctx->ddr & 0x40) && !(newDdr & 0x40) ) {
	
		ctx->bit6.cycles = FALL_OFF_CYCLES;
		ctx->bit6.charge = ctx->por & 0x40;
	}
}

auto M6510::advanceCounter() -> void {
	
	if ( ctx->bit6.cycles )
		if ( --ctx->bit6.cycles == 0 )
			ctx->bit6.charge = 0;
	
	if ( ctx->bit7.cycles  )
		if ( --ctx->bit7.cycles == 0 )
			ctx->bit7.charge = 0;
}

}

#undef FALL_OFF_CYCLES
