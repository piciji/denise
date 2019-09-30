
#include "m6502custom.h"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )
#define LAST true
#define ALU (this->*alu)
#define ALU2 (this->*alu2)
#define fp(name) &M6502Custom::_##name

#define SAVE_REG( _reg, cmd ) switch(_reg) {case RegA: A = cmd; break; case RegS: S = cmd; break; \
                                            case RegX: X = cmd; break; case RegY: Y = cmd; break; }

#define GET_REG( _reg ) _reg == RegA ? A : (_reg == RegS ? S : (_reg == RegX ? X : (_reg == RegY ? Y : (_reg == RegAX ? (A & X) : A ))))

#define SAVE_FLAG( val ) switch(flag) { case FlagC: C = val; break; case FlagN: N = val; break; \
                                        case FlagZ: Z = val; break; case FlagV: V = val; break; \
                                        case FlagI: I = val; break; case FlagD: D = val; break; }

#define GET_FLAG flag == FlagC ? C : (flag == FlagN ? N : (flag == FlagZ ? Z : (flag == FlagV ? V : (flag == FlagI ? I : (flag == FlagD ? D : D) ))))

#define GET_INDEX_REG regIndex == RegX ? X : ( regIndex == RegY ? Y : X )


namespace LIBC64 {
    
//indexed indirect
auto M6502Custom::_indexedIndirect( Alu alu ) -> void { 
	    
    switch(step++) {
        case 0:
            _indexedIndirectAdr();            
            break;
        case 1:
            A = ALU( _read( ctx->absolute, LAST ) );
            step = 0;
            break;
    }    	
}

template<M6502Reg reg> auto M6502Custom::_indexedIndirectW( ) -> void {
	   
    switch(step++) {
        case 0:
            _indexedIndirectAdr();
            readNext = false;
            break;
        case 1:
            _write( ctx->absolute, GET_REG(reg), LAST );
            step = 0;
            break;
    }       
}
    
auto M6502Custom::_indirectIndexed( Alu alu ) -> void {
	 
    switch(step++) {
        case 0:
            _indirectIndexedAdr();
            break;
        case 1:
            A = ALU( _read( ctx->absIndexed, LAST ) );
            step = 0;
            break;
    }       	
}

auto M6502Custom::_indirectIndexedW( ) -> void {
	
    switch(step++) {
        case 0:
            _indirectIndexedAdr( true );
            readNext = false;
            break;
        case 1:
            _write( ctx->absIndexed, A, LAST );
            step = 0;
            break;
    }      
}

//zero page
template<M6502Reg reg> auto M6502Custom::_zeroPage( Alu alu ) -> void {
    
    switch(step++) {
        case 0:
            ctx->zeroPage = _readPCInc();
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU(_loadZeroPage(ctx->zeroPage, LAST)) )
            } else
                _loadZeroPage(ctx->zeroPage, LAST);

            step = 0;
            break;            
    }            
}


template<M6502Reg reg> auto M6502Custom::_zeroPageW( ) -> void {

    switch (step++) {
        case 0:
            ctx->zeroPage = _readPCInc();
            readNext = false;
            break;
        case 1:
            _storeZeroPage( ctx->zeroPage, GET_REG(reg), LAST );
            step = 0;
            break;
    }            
}

auto M6502Custom::_zeroPageM( Alu alu ) -> void {
    switch(step++) {
        case 0:
            // zero page addressing can not access via, so we don't need to jump out before a memory access
            ctx->zeroPage = _readPCInc();
            ctx->data = _loadZeroPage( ctx->zeroPage );
            _storeZeroPage( ctx->zeroPage, ctx->data );
            readNext = false;
            break;
        case 1:
            _storeZeroPage( ctx->zeroPage, ALU( ctx->data ), LAST );            
            step = 0;
            break;            
    }                   
}

//zero page indexed
template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_zeroPageIndexed( Alu alu ) -> void {
    
    switch(step++) {
        case 0:
            _zeroPageIndexedAdr<regIndex>( );
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU( _loadZeroPage( ctx->zeroPage, LAST ) ) )
            } else
                _loadZeroPage( ctx->zeroPage, LAST );

            step = 0;
            break;            
    }       
}

template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_zeroPageIndexedW( ) -> void {
    
    switch(step++) {
        case 0:
            _zeroPageIndexedAdr<regIndex>( );
            readNext = false;
            break;
        case 1:
            _storeZeroPage( ctx->zeroPage, GET_REG(reg), LAST );    
            step = 0;
            break;            
    }            
}

auto M6502Custom::_zeroPageIndexedM( Alu alu ) -> void {

    switch(step++) {
        case 0:
            _zeroPageIndexedAdr<RegX>( );
            ctx->data = _loadZeroPage( ctx->zeroPage );
            _storeZeroPage( ctx->zeroPage, ctx->data );
            readNext = false;
            break;
        case 1:
            _storeZeroPage( ctx->zeroPage, ALU( ctx->data ), LAST );
            step = 0;
            break;            
    }    
}

// absolute
template<M6502Reg reg> auto M6502Custom::_absolute( Alu alu ) -> void {
    
    switch(step++) {
        case 0:
            _absoluteAdr();
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU(_read(ctx->absolute, LAST)) )
            } else
                _read(ctx->absolute, LAST);
            
            step = 0;
            break;
    }          
}

template<M6502Reg reg> auto M6502Custom::_absoluteW( ) -> void {
    
    switch (step++) {
        case 0:
            _absoluteAdr();
            readNext = false;
            break;
        case 1:
            _write( ctx->absolute, GET_REG(reg), LAST );
            step = 0;
            break;
    }      
}

auto M6502Custom::_absoluteM( Alu alu ) -> void {

    switch (step++) {
        case 0:
            _absoluteAdr();
            break;
        case 1:
            ctx->data = _read( ctx->absolute ); 
            readNext = false;
            break;
        case 2:
            _write( ctx->absolute, ctx->data );            
            break;
        case 3:
            _write( ctx->absolute, ALU( ctx->data ), LAST );
            step = 0;
            break;
    }      
}

template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_absoluteIndexed( Alu alu ) -> void {

    switch (step++) {
        case 0:
            _absoluteIndexedAdr<regIndex>( );
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU(_read(ctx->absIndexed, LAST)) )
            } else
                _read(ctx->absIndexed, LAST);
            
            step = 0;
            break;
    } 
}

template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_absoluteIndexedW( ) -> void {

    switch (step++) {
        case 0:
            _absoluteIndexedAdr<regIndex>( true );
            readNext = false;
            break;
        case 1:
            _write( ctx->absIndexed, GET_REG(reg), LAST );
            step = 0;
            break;
    } 
}

template<M6502Reg regIndex> auto M6502Custom::_absoluteIndexedM( Alu alu ) -> void {
    
    switch (step++) {
        case 0:
            _absoluteIndexedAdr<regIndex>( true );
            break;
        case 1:
            ctx->data = _read( ctx->absIndexed );
            readNext = false;
            break;
        case 2:
            _write(ctx->absIndexed, ctx->data);
            break;
        case 3:
            _write(ctx->absIndexed, ALU(ctx->data), LAST);
            step = 0;
            break;
    }   
}

//immediate
template<M6502Reg reg> auto M6502Custom::_immediate( Alu alu ) -> void {
    
	switch(step++) {
        case 0:
            break;
        case 1:
            SAVE_REG( reg, ALU( _readPCInc( LAST ) ) )
            step = 0;
            break;            
    }
	
}

//implied
template<M6502Reg reg> auto M6502Custom::_implied(Alu alu) -> void {
	
    switch(step++) {
        case 0:            
            break;
        case 1:
            _readPC( LAST );
            SAVE_REG( reg, ALU( GET_REG( reg ) ) )
            step = 0;
            break;            
    }		
}

auto M6502Custom::_nop() -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            _readPC( LAST );
            step = 0;
            break;            
    }        
}

auto M6502Custom::_brk() -> void {
    
    _interrupt( true );
}

auto M6502Custom::_rti() -> void {

    switch(step++) {
        case 0:
            _readPCInc();
            _read( 0x100 | S ); //cycle to pre increment sp, fetched value is discarded
            setFlags( _pullStack() );
            PC = _pullStack();
            break;
        case 1:
            PC |= _pullStack( LAST ) << 8;   
            step = 0;
            break;            
    }        
}

auto M6502Custom::_rts() -> void {

    switch(step++) {
        case 0:
            _readPCInc();
            _read( 0x100 | S );

            PC = _pullStack();
            PC |= _pullStack() << 8;   
            break;
        case 1:
            _readPCInc( LAST );
            step = 0;
            break;            
    }           
}

template<M6502Flag flag> auto M6502Custom::_clear( ) -> void {
    // I flag change is too late and not recognized for interrupt sampling at the end of this opcode
    // but when cpu enters rdy wait mode then the flag change is recognized in second cycle
    // we mark an upcomming state change

    switch(step++) {
        case 0:
            ctx->cli = flag == FlagI;
            break;
        case 1:
            _readPC( LAST );
            ctx->cli = false;
            SAVE_FLAG( false )
            ctx->soBlock = flag == FlagV ? 2 : 0;
            step = 0;
            break;            
    }        	
}

template<M6502Flag flag> auto M6502Custom::_set( ) -> void {

    switch(step++) {
        case 0:
            ctx->sei = flag == FlagI;
            break;
        case 1:
            _readPC( LAST );
            ctx->sei = false;
            SAVE_FLAG( true ) 
            step = 0;
            break;            
    }
}

auto M6502Custom::_jmpAbsolute() -> void {
    
    switch(step++) {
        case 0:
            ctx->dataW = _readPCInc();
            break;
        case 1:
            ctx->dataW |= _readPC( LAST ) << 8;
            PC = ctx->dataW;
            step = 0;
            break;            
    }        
}

auto M6502Custom::_jmpIndirect() -> void {

    switch(step++) {
        case 0:
            ctx->data = _readPCInc();
            ctx->dataH = _readPCInc();
            ctx->dataW = _read( ctx->dataH << 8 | ctx->data++ );        
            break;
        case 1:
            ctx->dataW |= _read( ctx->dataH << 8 | ctx->data, LAST ) << 8;
            PC = ctx->dataW;
            step = 0;
            break;            
    }
}

auto M6502Custom::_jsrAbsolute() -> void {

    switch(step++) {
        case 0:
            ctx->dataW = _readPCInc();
            ctx->dataW |= _readPC() << 8;   

            _pushStack( PC >> 8 );
            _pushStack( PC & 0xff );
            break;
        case 1:
            _readPC( LAST );
            PC = ctx->dataW;
            step = 0;
            break;            
    }
}

template<M6502Flag flag> auto M6502Custom::_branch( bool state ) -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            ctx->displacement = _readPCInc( LAST );  //polls here for interrupts always, even if branch is taken                    
            
            // why so complicated? because of possible external change of overflow bit in third half cycle
            if ( (GET_FLAG) != state ) 
                step = 0;
            break;            
        case 2: {
            bool addCycle = PAGE_CROSSED( PC, PC + ctx->displacement );
            _readPC( ); //don't polls here, even if this is final cycle
            ctx->dataW = PC + ctx->displacement;
            if (!addCycle) {
                PC = ctx->dataW;
                step = 0;
            } else {
                setPCL( PC + ctx->displacement );    
            }
        } break;
        case 3:
            _readPC( LAST ); //polls here for a second time
            PC = ctx->dataW;
            step = 0;
            break;
    }
}

template<M6502Reg src, M6502Reg target> auto M6502Custom::_transfer( bool flag) -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            _readPC( LAST );
            SAVE_REG(target, ( flag ? this->_ld( GET_REG(src) ) : GET_REG(src) ) ) 
            step = 0;
            break;            
    }    
}
//pull stack
auto M6502Custom::_plp() -> void {

    switch(step++) {
        case 0:
            _readPC();
            _read( 0x100 | S );
            break;
        case 1:
            setFlags( _pullStack( LAST ) );
            step = 0;
            break;            
    }            
}

auto M6502Custom::_pla() -> void {
    
    switch(step++) {
        case 0:
            _readPC();
            _read( 0x100 | S );
            break;
        case 1:
            A = this->_ld( _pullStack( LAST ) );    
            step = 0;
            break;            
    }       
}
//push stack
auto M6502Custom::_php() -> void {

    switch(step++) {
        case 0:
            _readPC();
            break;
        case 1:
            ctx->storeFlags = true;
            _pushStack( getFlags() | 0x30, LAST );
            ctx->storeFlags = false;
            step = 0;
            break;            
    }            
}

auto M6502Custom::_pha() -> void {

    switch(step++) {
        case 0:
            _readPC();    
            break;
        case 1:
            _pushStack( A, LAST);
            step = 0;
            break;            
    }            
}

// undocumented

//lax
auto M6502Custom::_indexedIndirectLax( ) -> void {

	_indexedIndirect( fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::_indirectIndexedLax( ) -> void {

	_indirectIndexed( fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::_zeroPageLax() -> void {
	
	_zeroPage<RegA>( fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::_zeroPageIndexedLax() -> void {
	
	_zeroPageIndexed<RegY, RegA>(fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::_absoluteLax() -> void {
	
	_absolute<RegA>( fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::_absoluteIndexedLax() -> void {
	
	_absoluteIndexed<RegY, RegA>( fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::_immediate() -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            _readPCInc( LAST );
            step = 0;
            break;            
    }		
}

auto M6502Custom::_immediateLax() -> void {
    
	_immediate<RegA>( fp(lax) );
	
    if (step == 0)
        X = A;
}

//las
auto M6502Custom::_absoluteIndexedLas() -> void {
	
	_absoluteIndexed<RegY, RegA>( fp(las) );
	
    if (step == 0)
        X = S = A;
}

//shx, shy
template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_absoluteIndexedWSh( ) -> void {

    switch(step++) {
        case 0:
            _absoluteIndexedAdr<regIndex>( true ); 
            readNext = false;
            break;
        case 1:
            H1AndedWrite( GET_REG( reg ) );    
            step = 0;
            break;            
    }        		
}
//ahx
auto M6502Custom::_absoluteIndexedWAhx() -> void {
    
    switch(step++) {
        case 0:
            _absoluteIndexedAdr<RegY>( true );  
            readNext = false;
            break;
        case 1:
            H1AndedWrite( A & X );	
            step = 0;
            break;            
    }	
}

//tas
auto M6502Custom::_absoluteIndexedWTas() -> void {

    switch(step++) {
        case 0:
            _absoluteIndexedAdr<RegY>( true ); 
            readNext = false;            
            break;
        case 1:
            S = A & X;
            H1AndedWrite( A & X );
            step = 0;
            break;            
    }
}

//anc
auto M6502Custom::_immediateAnc() -> void {
	
	_immediate<RegA>( fp(and) );	
    
	if (step == 0)
        C = N;
}
//alr
auto M6502Custom::_immediateAlr() -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            A = (this->_lsr)( (this->_and)( _readPCInc( LAST ) ) );
            step = 0;
            break;            
    }	
}
//arr
auto M6502Custom::_immediateArr() -> void {

    switch(step++) {
        case 0:
            break;
        case 1:
            A = (this->_arr)( _readPCInc( LAST ) );
            step = 0;
            break;            
    }    	
}
//ane
auto M6502Custom::_immediateAne() -> void {
   
    switch(step++) {
        case 0:
            break;
        case 1:
            ctx->xaa = true;
            A = (this->_ane)( _readPCInc( LAST ) );
            ctx->xaa = false;
            step = 0;
            break;            
    }    
}
//sbx
auto M6502Custom::_immediateSbx() -> void {
	
    switch(step++) {
        case 0:
            break;
        case 1:
            X = (this->_sbx)( _readPCInc( LAST ) );
            step = 0;
            break;            
    }
}

auto M6502Custom::_kill() -> void {
    _readPCInc();
    
    _read(0xffff);
    _read(0xfffe);
    _read(0xfffe);
    
    _read(0xffff); //now reading from 0xffff endless
	
	ctx->killed = true;
}

auto M6502Custom::_indexedIndirectM( Alu alu, Alu alu2 ) -> void {
    
    switch(step++) {
        case 0:
            _indexedIndirectAdr();
            break;
        case 1:
            ctx->data = _read( ctx->absolute );
            readNext = false;
            break;
        case 2:
            _write( ctx->absolute, ctx->data );                        
            ctx->data = ALU( ctx->data );
            break;
        case 3:            
            _write( ctx->absolute, ctx->data, LAST );
            A = ALU2( ctx->data );
            step = 0;
            break;            
    }    
}

auto M6502Custom::_indirectIndexedWAhx() -> void {
	
    switch(step++) {
        case 0:
            _indirectIndexedAdr( true );
            readNext = false;
            break;
        case 1:
            H1AndedWrite( A & X );	
            step = 0;
            break;            
    }	
}

auto M6502Custom::_indirectIndexedM( Alu alu, Alu alu2 ) -> void {

    switch(step++) {
        case 0:
            _indirectIndexedAdr( true );
            break;
        case 1:
            ctx->data = _read( ctx->absIndexed );    
            readNext = false;
            break;
        case 2:
            _write( ctx->absIndexed, ctx->data );
            ctx->data = ALU( ctx->data );
            break;
        case 3:
            _write( ctx->absIndexed, ctx->data, LAST );
            A = ALU2( ctx->data );
            step = 0;
            break;            
    }	    
}

auto M6502Custom::_zeroPageIndexedM( Alu alu, Alu alu2 ) -> void {
    
    _zeroPageIndexedM( alu );
    
    if (step == 0)
        A = ALU2( ctx->db ); 
}

auto M6502Custom::_zeroPageM( Alu alu, Alu alu2 ) -> void {
    
    _zeroPageM( alu );
    
    if (step == 0)
        A = ALU2( ctx->db );
}

auto M6502Custom::_absoluteM( Alu alu, Alu alu2 ) -> void {

    _absoluteM( alu );
	
    if (step == 0)
        A = ALU2( ctx->db );
}

template<M6502Reg regIndex> auto M6502Custom::_absoluteIndexedM( Alu alu, Alu alu2 ) -> void {

	_absoluteIndexedM<regIndex>( alu );
			
    if (step == 0)
        A = ALU2( ctx->db );
}    
    
}

#undef PAGE_CROSSED
#undef LAST
#undef fp
#undef ALU
#undef ALU2
#undef SAVE_REG
#undef GET_REG
#undef SAVE_FLAG
#undef GET_FLAG
#undef GET_INDEX_REG