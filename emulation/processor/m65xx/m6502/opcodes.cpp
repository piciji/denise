
#include "m6502.h"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )
#define LAST true
#define ALU (this->*alu)
#define SAVE_REG( _reg, cmd ) switch(_reg) {case RegA: A = cmd; break; case RegS: S = cmd; break; \
                                            case RegX: X = cmd; break; case RegY: Y = cmd; break; }

#define GET_REG( _reg ) _reg == RegA ? A : (_reg == RegS ? S : (_reg == RegX ? X : (_reg == RegY ? Y : (_reg == RegAX ? (A & X) : A ))))

#define SAVE_FLAG( val ) switch(flag) { case FlagC: C = val; break; case FlagN: N = val; break; \
                                        case FlagZ: Z = val; break; case FlagV: V = val; break; \
                                        case FlagI: I = val; break; case FlagD: D = val; break; }

#define GET_FLAG flag == FlagC ? C : (flag == FlagN ? N : (flag == FlagZ ? Z : (flag == FlagV ? V : (flag == FlagI ? I : (flag == FlagD ? D : D) ))))

namespace MOS65FAMILY {

//indexed indirect
auto M6502::indexedIndirect( Alu alu ) -> void { 
	    
	A = ALU( read<5>( indexedIndirectAdr(), LAST ) );
}

template<M6502::Reg reg> auto M6502::indexedIndirectW( ) -> void {
    
    indexedIndirectAdr();
    	       
	write( ctx->absolute, GET_REG(reg), LAST );
}

//indirect indexed
auto M6502::indirectIndexed( Alu alu ) -> void {
	    
	A = ALU( read<5>( indirectIndexedAdr(), LAST ) );
}

auto M6502::indirectIndexedW( ) -> void {
	
    write( indirectIndexedAdr( true ), A, LAST );    
}

//zero page
template<M6502::Reg reg> auto M6502::zeroPage( Alu alu ) -> void {
    
    ctx->zeroPage = readPCInc<1>();
    
    if (alu) {
        SAVE_REG( reg, ALU( loadZeroPage<2>( ctx->zeroPage, LAST ) ) )
    } else
        loadZeroPage<2>( ctx->zeroPage, LAST );
}

template<M6502::Reg reg> auto M6502::zeroPageW( ) -> void {
    
    ctx->zeroPage = readPCInc<1>();
    storeZeroPage( ctx->zeroPage, GET_REG(reg), LAST );
}

auto M6502::zeroPageM( Alu alu ) -> void {
    
    ctx->zeroPage = readPCInc<1>();    
    ctx->data = loadZeroPage<2>( ctx->zeroPage );
    
    storeZeroPage( ctx->zeroPage, ctx->data ); // needs this cycle for ALU
    storeZeroPage( ctx->zeroPage, ALU( ctx->data ), LAST );    
}

//zero page indexed
template<M6502::Reg regIndex, M6502::Reg reg> auto M6502::zeroPageIndexed( Alu alu ) -> void {
    
    ctx->zeroPage = zeroPageIndexedAdr<regIndex>( );
    
    if (alu) {
        SAVE_REG( reg, ALU( loadZeroPage<3>( ctx->zeroPage, LAST ) ) )
    } else
        loadZeroPage<3>( ctx->zeroPage, LAST );
}

template<M6502::Reg regIndex, M6502::Reg reg> auto M6502::zeroPageIndexedW() -> void {
    
    ctx->zeroPage = zeroPageIndexedAdr<regIndex>( );
    storeZeroPage( ctx->zeroPage, GET_REG(reg), LAST );    
}

auto M6502::zeroPageIndexedM( Alu alu ) -> void {
    
    ctx->zeroPage = zeroPageIndexedAdr<RegX>( );
    
    ctx->data = loadZeroPage<3>( ctx->zeroPage );
    storeZeroPage( ctx->zeroPage, ctx->data );
    storeZeroPage( ctx->zeroPage, ALU( ctx->data ), LAST );
}

//absolute
template<M6502::Reg reg> auto M6502::absolute( Alu alu ) -> void {
    
    absoluteAdr();

	if( alu ) {
		SAVE_REG( reg, ALU( read<3>( ctx->absolute, LAST ) ) )
	} else
		read<3>( ctx->absolute, LAST );
}

template<M6502::Reg reg> auto M6502::absoluteW( ) -> void {
    
    write( absoluteAdr(), GET_REG(reg), LAST );
}

auto M6502::absoluteM( Alu alu ) -> void {

	absoluteAdr();
    ctx->data = read<3>( ctx->absolute );
    
    write( ctx->absolute, ctx->data );
    write( ctx->absolute, ALU( ctx->data ), LAST );
}

// absolute indexed
template<M6502::Reg regIndex, M6502::Reg reg> auto M6502::absoluteIndexed( Alu alu ) -> void {
    
    absoluteIndexedAdr<regIndex>( );

	if ( alu )
		SAVE_REG( reg, ALU( read<4>( ctx->absIndexed, LAST ) ) )
	else
		read<4>( ctx->absIndexed, LAST );
}

template<M6502::Reg regIndex, M6502::Reg reg> auto M6502::absoluteIndexedW( ) -> void {
    
	absoluteIndexedAdr<regIndex>( true );
		
    write( ctx->absIndexed, GET_REG(reg), LAST );
}

template<M6502::Reg regIndex> auto M6502::absoluteIndexedM( Alu alu ) -> void {
	
	absoluteIndexedAdr<regIndex>( true );
	
    ctx->data = read<4>( ctx->absIndexed );

    write( ctx->absIndexed, ctx->data );

    write( ctx->absIndexed, ALU( ctx->data ), LAST );
}

//immediate
template<M6502::Reg reg> auto M6502::immediate( Alu alu ) -> void {
	
	SAVE_REG( reg, ALU( readPCInc<1>( LAST ) ) )
}

//implied
template<M6502::Reg reg> auto M6502::implied(Alu alu) -> void {
	
	readPC<1>( LAST );
	SAVE_REG( reg, ALU( GET_REG( reg ) ) )
}

auto M6502::nop() -> void {
    
    readPC<1>( LAST );
}

auto M6502::brk() -> void {
    
    interrupt( true );
}

auto M6502::rti() -> void {
    
    readPCInc<1>();
    read<2>( 0x100 | S ); //cycle to pre increment sp, fetched value is discarded
    
    setFlags( pullStack<3>() );
    
    PC = pullStack<4>();
    PC |= pullStack<5>( LAST ) << 8;   
}

auto M6502::rts() -> void {
    
    readPCInc<1>();
    read<2>( 0x100 | S );
        
    PC = pullStack<3>();
    PC |= pullStack<4>() << 8;   
    
    readPCInc<5>( LAST );
}

template<M6502::Flag flag> auto M6502::clear( ) -> void {
    // I flag change is too late and not recognized for interrupt sampling at the end of this opcode
    // but when cpu enters rdy wait mode then the flag change is recognized in second cycle
    // we mark an upcomming state change

	ctx->cli = flag == FlagI;    
    readPC<1>( LAST );
    ctx->cli = false;
    SAVE_FLAG( false )
    // prevent external change for next two cycles
    ctx->soBlock = flag == FlagV ? 2 : 0;
}

template<M6502::Flag flag> auto M6502::set( ) -> void {

	ctx->sei = flag == FlagI;
    readPC<1>( LAST );
    ctx->sei = false;
    SAVE_FLAG( true )
}

auto M6502::jmpAbsolute() -> void {
    
    ctx->dataW = readPCInc<1>();
    ctx->dataW |= readPC<2>( LAST ) << 8;
    PC = ctx->dataW;
}

auto M6502::jmpIndirect() -> void {
    
    ctx->data = readPCInc<1>();
    ctx->dataH = readPCInc<2>();
    
    ctx->dataW = read<3>( ctx->dataH << 8 | ctx->data++ );        
    ctx->dataW |= read<4>( ctx->dataH << 8 | ctx->data, LAST ) << 8;
    
    PC = ctx->dataW;
}

auto M6502::jsrAbsolute() -> void {

    ctx->dataW = readPCInc<1>();
    ctx->dataW |= readPC<2>() << 8;   
    
    pushStack( PC >> 8 );
    pushStack( PC & 0xff );

    readPC<3>( LAST );

    PC = ctx->dataW;
}

template<M6502::Flag flag> auto M6502::branch( bool state ) -> void {
    
    ctx->displacement = readPCInc<1>( LAST );  //polls here for interrupts always, even if branch is taken
    
    // why so complicated? because of possible external change of overflow bit in third half cycle
    if ( !ctx->isDummy && ((GET_FLAG) != state) )
        return;            
    
    readPC<2>( ); //don't polls here, even if this is final cycle
    
    bool addCycle = PAGE_CROSSED( PC, PC + ctx->displacement );
    
    ctx->dataW = PC + ctx->displacement;
    
    if ( ctx->isDummy || addCycle ) {                
        
        setPCL( PC + ctx->displacement );
        
        readPC<3>( LAST ); //polls here for a second time
    }
    
    PC = ctx->dataW;
}

template<M6502::Reg src, M6502::Reg target> auto M6502::transfer(bool flag) -> void {
    
    readPC<1>( LAST );
    SAVE_REG(target, ( flag ? this->_ld( GET_REG(src) ) : GET_REG(src) ) )   
}
//pull stack
auto M6502::plp() -> void {
    
    readPC<1>();
    read<2>( 0x100 | S );
    
    setFlags( pullStack<3>( LAST ) );
}

auto M6502::pla() -> void {
    
    readPC<1>();
    read<2>( 0x100 | S );
    
    A = this->_ld( pullStack<3>( LAST ) );    
}
//push stack
auto M6502::php() -> void {
    
    readPC<1>();
    ctx->storeFlags = true;
    pushStack( getFlags() | 0x30, LAST );
    ctx->storeFlags = false;
}

auto M6502::pha() -> void {
    
    readPC<1>();    
    pushStack( A, LAST);
}

}

#undef PAGE_CROSSED
#undef LAST
#undef ALU
#undef SAVE_REG
#undef GET_REG
#undef SAVE_FLAG
#undef GET_FLAG