
#include "m6502custom.h"

#define A ctx->a
#define X ctx->x
#define Y ctx->y
#define S ctx->s
#define PC ctx->pc

#define C ctx->c
#define Z ctx->z
#define I ctx->i
#define D ctx->d
#define V ctx->v
#define N ctx->n

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

typedef MOS65FAMILY::M6502::Reg M6502Reg;
typedef MOS65FAMILY::M6502::Flag M6502Flag;

namespace LIBC64 {

auto M6502Custom::power() -> void {    
    
    step = 0;
    
    M6502::power();
}
    
auto M6502Custom::process() -> void {    
    
    if (step == 0) {     
        readNext = true; // will be reseted before write cycles
        M6502::process();        
    } else
        _decode( ctx->IR );       
}

auto M6502Custom::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( step );
    s.integer( readNext );    
}

auto M6502Custom::_decode( uint8_t IR ) -> void {
    #define COMMA ,
    #define op(id, name, ...) case id: return _##name(__VA_ARGS__);
    #define UO //undocumented opcode but always predictable
    /**
     * don't use these kind of opcodes
     * some results differs between visual6502 and real cpu
     * Visual6502 is a digital representation so it can not handle race conditions that good
     * real cpu could produce different results, depending on a lot of things like heat, cpu version, bus usage and so on
     */
    #define UUO //unstable undocumented opcode

    switch( IR ) {
        #include "../../../processor/m65xx/m6502/optable.cpp"
    }

    #undef op
    #undef UO
    #undef UUO
    #undef COMMA	
}

auto M6502Custom::detectIrq() -> void {

    ctx->irqPending = ctx->irqLine;
}

//indexed indirect
auto M6502Custom::_indexedIndirect( Alu alu ) -> void { 
	    
    switch(step++) {
        case 0:
            indexedIndirectAdr();            
            break;
        case 1:
            A = ALU( read( ctx->absolute, LAST ) );
            step = 0;
            break;
    }    	
}

template<M6502Reg reg> auto M6502Custom::_indexedIndirectW( ) -> void {
	   
    switch(step++) {
        case 0:
            indexedIndirectAdr();
            readNext = false;
            break;
        case 1:
            write( ctx->absolute, GET_REG(reg), LAST );
            step = 0;
            break;
    }       
}
    
auto M6502Custom::_indirectIndexed( Alu alu ) -> void {
	 
    switch(step++) {
        case 0:
            indirectIndexedAdr();
            break;
        case 1:
            A = ALU( read( ctx->absIndexed, LAST ) );
            step = 0;
            break;
    }       	
}

auto M6502Custom::_indirectIndexedW( ) -> void {
	
    switch(step++) {
        case 0:
            indirectIndexedAdr( true );
            readNext = false;
            break;
        case 1:
            write( ctx->absIndexed, A, LAST );
            step = 0;
            break;
    }      
}

//zero page
template<M6502Reg reg> auto M6502Custom::_zeroPage( Alu alu ) -> void {
    
    switch(step++) {
        case 0:
            ctx->zeroPage = readPCInc();
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU(loadZeroPage(ctx->zeroPage, LAST)) )
            } else
                loadZeroPage(ctx->zeroPage, LAST);

            step = 0;
            break;            
    }            
}


template<M6502Reg reg> auto M6502Custom::_zeroPageW( ) -> void {

    switch (step++) {
        case 0:
            ctx->zeroPage = readPCInc();
            readNext = false;
            break;
        case 1:
            storeZeroPage( ctx->zeroPage, GET_REG(reg), LAST );
            step = 0;
            break;
    }            
}

auto M6502Custom::_zeroPageM( Alu alu ) -> void {
    switch(step++) {
        case 0:
            // zero page addressing can not access via, so we don't need to jump out before a memory access
            ctx->zeroPage = readPCInc();
            ctx->data = loadZeroPage( ctx->zeroPage );
            storeZeroPage( ctx->zeroPage, ctx->data );
            readNext = false;
            break;
        case 1:
            storeZeroPage( ctx->zeroPage, ALU( ctx->data ), LAST );            
            step = 0;
            break;            
    }                   
}

//zero page indexed
template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_zeroPageIndexed( Alu alu ) -> void {
    
    switch(step++) {
        case 0:
            ctx->zeroPage = zeroPageIndexedAdr<regIndex>( );
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU( loadZeroPage( ctx->zeroPage, LAST ) ) )
            } else
                loadZeroPage( ctx->zeroPage, LAST );

            step = 0;
            break;            
    }       
}

template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_zeroPageIndexedW( ) -> void {
    
    switch(step++) {
        case 0:
            ctx->zeroPage = zeroPageIndexedAdr<regIndex>( );
            readNext = false;
            break;
        case 1:
            storeZeroPage( ctx->zeroPage, GET_REG(reg), LAST );    
            step = 0;
            break;            
    }            
}

auto M6502Custom::_zeroPageIndexedM( Alu alu ) -> void {

    switch(step++) {
        case 0:
            ctx->zeroPage = zeroPageIndexedAdr<RegX>( );
            ctx->data = loadZeroPage( ctx->zeroPage );
            storeZeroPage( ctx->zeroPage, ctx->data );
            readNext = false;
            break;
        case 1:
            storeZeroPage( ctx->zeroPage, ALU( ctx->data ), LAST );
            step = 0;
            break;            
    }    
}

// absolute
template<M6502Reg reg> auto M6502Custom::_absolute( Alu alu ) -> void {
    
    switch(step++) {
        case 0:
            absoluteAdr();
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU(read(ctx->absolute, LAST)) )
            } else
                read(ctx->absolute, LAST);
            
            step = 0;
            break;
    }          
}

template<M6502Reg reg> auto M6502Custom::_absoluteW( ) -> void {
    
    switch (step++) {
        case 0:
            absoluteAdr();
            readNext = false;
            break;
        case 1:
            write( ctx->absolute, GET_REG(reg), LAST );
            step = 0;
            break;
    }      
}

auto M6502Custom::_absoluteM( Alu alu ) -> void {

    switch (step++) {
        case 0:
            absoluteAdr();
            break;
        case 1:
            ctx->data = read( ctx->absolute ); 
            readNext = false;
            break;
        case 2:
            write( ctx->absolute, ctx->data );            
            break;
        case 3:
            write( ctx->absolute, ALU( ctx->data ), LAST );
            step = 0;
            break;
    }      
}

template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_absoluteIndexed( Alu alu ) -> void {

    switch (step++) {
        case 0:
            absoluteIndexedAdr<regIndex>( );
            break;
        case 1:
            if (alu) {
                SAVE_REG( reg, ALU(read(ctx->absIndexed, LAST)) )
            } else
                read(ctx->absIndexed, LAST);
            
            step = 0;
            break;
    } 
}

template<M6502Reg regIndex, M6502Reg reg> auto M6502Custom::_absoluteIndexedW( ) -> void {

    switch (step++) {
        case 0:
            absoluteIndexedAdr<regIndex>( true );
            readNext = false;
            break;
        case 1:
            write( ctx->absIndexed, GET_REG(reg), LAST );
            step = 0;
            break;
    } 
}

template<M6502Reg regIndex> auto M6502Custom::_absoluteIndexedM( Alu alu ) -> void {
    
    switch (step++) {
        case 0:
            absoluteIndexedAdr<regIndex>( true );
            break;
        case 1:
            ctx->data = read( ctx->absIndexed );
            readNext = false;
            break;
        case 2:
            write(ctx->absIndexed, ctx->data);
            break;
        case 3:
            write(ctx->absIndexed, ALU(ctx->data), LAST);
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
            SAVE_REG( reg, ALU( readPCInc( LAST ) ) )
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
            readPC( LAST );
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
            readPC( LAST );
            step = 0;
            break;            
    }        
}

auto M6502Custom::_brk() -> void {
    
    interrupt( true );
}

auto M6502Custom::_rti() -> void {

    switch(step++) {
        case 0:
            readPCInc();
            read( 0x100 | S ); //cycle to pre increment sp, fetched value is discarded
            setFlags( pullStack() );
            PC = pullStack();
            break;
        case 1:
            PC |= pullStack( LAST ) << 8;   
            step = 0;
            break;            
    }        
}

auto M6502Custom::_rts() -> void {

    switch(step++) {
        case 0:
            readPCInc();
            read( 0x100 | S );

            PC = pullStack();
            PC |= pullStack() << 8;   
            break;
        case 1:
            readPCInc( LAST );
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
            readPC( LAST );
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
            readPC( LAST );
            ctx->sei = false;
            SAVE_FLAG( true ) 
            step = 0;
            break;            
    }
}

auto M6502Custom::_jmpAbsolute() -> void {
    
    switch(step++) {
        case 0:
            ctx->dataW = readPCInc();
            break;
        case 1:
            ctx->dataW |= readPC( LAST ) << 8;
            PC = ctx->dataW;
            step = 0;
            break;            
    }        
}

auto M6502Custom::_jmpIndirect() -> void {

    switch(step++) {
        case 0:
            ctx->data = readPCInc();
            ctx->dataH = readPCInc();
            ctx->dataW = read( ctx->dataH << 8 | ctx->data++ );        
            break;
        case 1:
            ctx->dataW |= read( ctx->dataH << 8 | ctx->data, LAST ) << 8;
            PC = ctx->dataW;
            step = 0;
            break;            
    }
}

auto M6502Custom::_jsrAbsolute() -> void {

    switch(step++) {
        case 0:
            ctx->dataW = readPCInc();
            ctx->dataW |= readPC() << 8;   

            pushStack( PC >> 8 );
            pushStack( PC & 0xff );
            break;
        case 1:
            readPC( LAST );
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
            ctx->displacement = readPCInc( LAST );  //polls here for interrupts always, even if branch is taken                    
            
            // why so complicated? because of possible external change of overflow bit in third half cycle
            if ( (GET_FLAG) != state ) 
                step = 0;
            break;            
        case 2: {
            bool addCycle = PAGE_CROSSED( PC, PC + ctx->displacement );
            readPC( ); //don't polls here, even if this is final cycle
            ctx->dataW = PC + ctx->displacement;
            if (!addCycle) {
                PC = ctx->dataW;
                step = 0;
            } else {
                setPCL( PC + ctx->displacement );    
            }
        } break;
        case 3:
            readPC( LAST ); //polls here for a second time
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
            readPC( LAST );
            SAVE_REG(target, ( flag ? this->_ld( GET_REG(src) ) : GET_REG(src) ) ) 
            step = 0;
            break;            
    }    
}
//pull stack
auto M6502Custom::_plp() -> void {

    switch(step++) {
        case 0:
            readPC();
            read( 0x100 | S );
            break;
        case 1:
            setFlags( pullStack( LAST ) );
            step = 0;
            break;            
    }            
}

auto M6502Custom::_pla() -> void {
    
    switch(step++) {
        case 0:
            readPC();
            read( 0x100 | S );
            break;
        case 1:
            A = this->_ld( pullStack( LAST ) );    
            step = 0;
            break;            
    }       
}
//push stack
auto M6502Custom::_php() -> void {

    switch(step++) {
        case 0:
            readPC();
            break;
        case 1:
            ctx->storeFlags = true;
            pushStack( getFlags() | 0x30, LAST );
            ctx->storeFlags = false;
            step = 0;
            break;            
    }            
}

auto M6502Custom::_pha() -> void {

    switch(step++) {
        case 0:
            readPC();    
            break;
        case 1:
            pushStack( A, LAST);
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
            readPCInc( LAST );
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
            absoluteIndexedAdr<regIndex>( true ); 
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
            absoluteIndexedAdr<RegY>( true );  
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
            absoluteIndexedAdr<RegY>( true ); 
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
            A = (this->_lsr)( (this->_and)( readPCInc( LAST ) ) );
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
            A = (this->_arr)( readPCInc( LAST ) );
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
            A = (this->_ane)( readPCInc( LAST ) );
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
            X = (this->_sbx)( readPCInc( LAST ) );
            step = 0;
            break;            
    }
}

auto M6502Custom::_kill() -> void {
    kill();
}

auto M6502Custom::_indexedIndirectM( Alu alu, Alu alu2 ) -> void {
    
    switch(step++) {
        case 0:
            indexedIndirectAdr();
            break;
        case 1:
            ctx->data = read( ctx->absolute );
            readNext = false;
            break;
        case 2:
            write( ctx->absolute, ctx->data );                        
            ctx->data = ALU( ctx->data );
            break;
        case 3:            
            write( ctx->absolute, ctx->data, LAST );
            A = ALU2( ctx->data );
            step = 0;
            break;            
    }    
}

auto M6502Custom::_indirectIndexedWAhx() -> void {
	
    switch(step++) {
        case 0:
            indirectIndexedAdr( true );
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
            indirectIndexedAdr( true );
            break;
        case 1:
            ctx->data = read( ctx->absIndexed );    
            readNext = false;
            break;
        case 2:
            write( ctx->absIndexed, ctx->data );
            ctx->data = ALU( ctx->data );
            break;
        case 3:
            write( ctx->absIndexed, ctx->data, LAST );
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
