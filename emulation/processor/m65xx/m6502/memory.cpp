
#include "m6502.h"

namespace MOS65FAMILY {

template<uint8_t cycle> auto M6502::read( uint16_t addr, bool lastCycle ) -> uint8_t {         
    
    ctx->addrBus = addr;  
    
    ctx->syncLo(); //put addr on bus (first half cycle)                
    
#ifdef SUPPORT_SO    
    handleSo();
#endif	
    if (lastCycle)
        sampleInterrupt(); 
    
    ctx->rdyLastCycle = false;
    
    if(workCtx->useDummy) {
        
        if ((workCtx->resumeCycle & 0xf) == cycle) {
            
            restoreContext(); 
            
            addr = ctx->addrBus;
        } else
            return 0;
    }  
    
    while( ctx->rdyLine ) { //cpu is halted in read mode only
        
		ctx->rdyLastCycle = true;
		        		
        ctx->syncHi();                
        
        detectInterrupt(); 
        
        ctx->db = busWatch(); //on falling edge
        
        if ( ctx->xaa )
            A &= ctx->db;
                
		// rdy prolongs complete cycles, not half cycles
        ctx->syncLo();
#ifdef SUPPORT_SO            
        handleSo();
#endif       
        // cli or sei instruction executes now
        if (ctx->cli) {
            I = false;
            ctx->cli = false;
        } else if (ctx->sei) {
            I = true;
            ctx->sei = false;
        }
        
        // this behavior was observed in countless visual6502 tests
		// normally a detected interrupt in last cycle can not be sampled the same cycle, so irq happens one opcode later
		// in rdy repeated last cycle this is possible, because there is at least one cycle more running
        if (lastCycle)
            sampleInterrupt();    
       
        if (dontBlockExecution) {
            dontBlockExecution = false;
            ctx->resumeCycle = cycle;
            ctx->useDummy = true;
            // swap dummy in
            ctx = ctx->dummyCtx;                                           
            return 0;
        }
    }        
    
    ctx->db = busRead( addr ); //read bus (second half cycle)
        
    ctx->syncHi();    
	
    detectInterrupt(); //happens during second half cycle ( falling edge of phi2 )
    
    return ctx->db;
}

auto M6502::write( uint16_t addr, uint8_t data, bool lastCycle ) -> void {    
    
    ctx->addrBus = addr;
    
    ctx->syncLo();      
    
    if (lastCycle)
        sampleInterrupt();    
    
    ctx->db = data; 
#ifdef SUPPORT_SO        
    handleSo();
#endif            
	// Beware: bus write and synchronisation happen in parallel 
    // doesn't necessary mean internal logic of other bus participant can use written value after second half cycle
    // thats why we do the write before syncHi. in the context of another bus participant the write should be pipelined
	// now and executed to a proper time within syncHi
	busWrite( addr, ctx->db );          
	
	ctx->syncHi();    
    
    detectInterrupt();	
}

template<uint8_t cycle> inline auto M6502::loadZeroPage( uint8_t addr, bool lastCycle ) -> uint8_t {
    
    return read<cycle>( 0x0000 | addr, lastCycle );
}

inline auto M6502::storeZeroPage( uint8_t addr, uint8_t data, bool lastCycle ) -> void {
    
    write( 0x0000 | addr, data, lastCycle );
}

template<uint8_t cycle> inline auto M6502::readPCInc( bool lastCycle ) -> uint8_t {
    
    return read<cycle>( ctx->pc++, lastCycle );
}

template<uint8_t cycle> inline auto M6502::readPC( bool lastCycle ) -> uint8_t {
    
    return read<cycle>( ctx->pc, lastCycle );
}

inline auto M6502::pushStack( uint8_t data, bool lastCycle ) -> void {
    
    write( 0x100 | ctx->s--, data, lastCycle );
}

template<uint8_t cycle> inline auto M6502::pullStack( bool lastCycle ) -> uint8_t {
    
    /**
     * because of pre incrementing an idle cycle is needed before a series of pull requests.
     * bus is accessed with current sp for this idle cycle and value is discarded (rti, rts)
     */
    
    return read<cycle>( 0x100 | ++ctx->s, lastCycle );
}

}