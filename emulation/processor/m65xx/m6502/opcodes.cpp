
#include "m6502.h"

#define PAGE_CROSSED(x, y) ( ( (uint16_t)x >> 8 ) != ( (uint16_t)y >> 8) )
#define LAST true
#define ALU (this->*alu)

namespace MOS65FAMILY {

//indexed indirect
auto M6502::indexedIndirect( Alu alu ) -> void { 
	    
	A = ALU( read<5>( indexedIndirectAdr(), LAST ) );
}

auto M6502::indexedIndirectW( uint8_t data ) -> void {
	   
	write( indexedIndirectAdr(), data, LAST );
}

//indirect indexed
auto M6502::indirectIndexed( Alu alu ) -> void {
	    
	A = ALU( read<5>( indirectIndexedAdr(), LAST ) );
}

auto M6502::indirectIndexedW( uint8_t data ) -> void {
	
    write( indirectIndexedAdr( true ), data, LAST );    
}

//zero page
auto M6502::zeroPage( Alu alu, uint8_t& data ) -> void {
    
    ctx->mem.zeroPage = readPCInc<1>();
    
    if (alu)    
        data = ALU( loadZeroPage<2>( ctx->mem.zeroPage, LAST ) );
    else
        loadZeroPage<2>( ctx->mem.zeroPage, LAST );
}

auto M6502::zeroPage( Alu alu ) -> void {
	
	zeroPage( alu, A );
}

auto M6502::zeroPageW( uint8_t data ) -> void {
    
    auto zeroPage = readPCInc();
    storeZeroPage( zeroPage, data, LAST );
}

auto M6502::zeroPageM( Alu alu ) -> void {
    
    auto zeroPage = readPCInc();    
    auto data = loadZeroPage( zeroPage );
    
    storeZeroPage( zeroPage, data ); // needs this cycle for ALU
    storeZeroPage( zeroPage, ALU( data ), LAST );    
}

//zero page indexed
auto M6502::zeroPageIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void {
    
    uint8_t adr = zeroPageIndexedAdr( index );
    
    if (alu)
        data = ALU( loadZeroPage( adr, LAST ) );
    else
        loadZeroPage( adr, LAST );
}

auto M6502::zeroPageIndexed( uint8_t index, Alu alu ) -> void {
	
	zeroPageIndexed( index, alu, A );
}

auto M6502::zeroPageIndexedW( uint8_t index, uint8_t data ) -> void {
    
    uint8_t adr = zeroPageIndexedAdr( index );
    storeZeroPage( adr, data, LAST );    
}

auto M6502::zeroPageIndexedM( Alu alu ) -> void {
    
    uint8_t adr = zeroPageIndexedAdr( X );
    
    auto data = loadZeroPage( adr );
    storeZeroPage( adr, data );
    storeZeroPage( adr, ALU( data ), LAST );
}

//absolute
auto M6502::absolute( Alu alu, uint8_t& data ) -> void {
    
    uint16_t absolute = absoluteAdr();

	if( alu )
		data = ALU( read( absolute, LAST ) );
	else
		read( absolute, LAST );
}

auto M6502::absolute( Alu alu ) -> void {
	
	absolute( alu, A );
}

auto M6502::absoluteW( uint8_t data ) -> void {
    
    write( absoluteAdr(), data, LAST );
}

auto M6502::absoluteM( Alu alu ) -> void {

	uint16_t absolute = absoluteAdr();
    auto data = read( absolute );
    
    write( absolute, data );
    write( absolute, ALU( data ), LAST );
}

// absolute indexed
auto M6502::absoluteIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void {
    
    uint16_t absIndexed = absoluteIndexedAdr( index );

	if ( alu )
		data = ALU( read( absIndexed, LAST ) );
	else
		read( absIndexed, LAST );
}

auto M6502::absoluteIndexed( uint8_t index, Alu alu ) -> void {
	
	absoluteIndexed( index, alu, A);
}

auto M6502::absoluteIndexedW( uint8_t index, uint8_t data ) -> void {
    
	uint16_t absIndexed = absoluteIndexedAdr( index, true );
		
    write( absIndexed, data, LAST );
}

auto M6502::absoluteIndexedM( uint8_t index, Alu alu ) -> void {
	
	uint16_t absIndexed = absoluteIndexedAdr( index, true );
	
    auto data = read( absIndexed );

    write( absIndexed, data );

    write( absIndexed, ALU( data ), LAST );
}

//immediate
auto M6502::immediate( Alu alu, uint8_t& data ) -> void {
	
	data = ALU( readPCInc( LAST ) );
}

//implied
auto M6502::implied(Alu alu, uint8_t& data) -> void {
	
	readPC( LAST );
	data = ALU( data );
}

auto M6502::nop() -> void {
    
    readPC( LAST );
}

auto M6502::brk() -> void {
    
    interrupt( true );
}

auto M6502::rti() -> void {
    
    readPCInc();
    read( 0x100 | S ); //cycle to pre increment sp, fetched value is discarded
    
    setFlags( pullStack() );
    
    PC = pullStack();
    PC |= pullStack( LAST ) << 8;   
}

auto M6502::rts() -> void {
    
    readPCInc();
    read( 0x100 | S );
        
    PC = pullStack();
    PC |= pullStack() << 8;   
    
    readPCInc( LAST );
}

auto M6502::clear( bool& flag ) -> void {
    // I flag change is too late and not recognized for interrupt sampling at the end of this opcode
    // but when cpu enters rdy wait mode then the flag change is recognized in second cycle
    // we mark an upcomming state change

	ctx->memory.cli = &flag == &I;    
    readPC( LAST );
    ctx->memory.cli = false;
    flag = false; 
    // prevent external change for next two cycles
    ctx->memory.soBlock = &flag == &V ? 2 : 0;
}

auto M6502::set( bool& flag ) -> void {

	ctx->memory.sei = &flag == &I;
    readPC( LAST );
    ctx->memory.sei = false;
    flag = true; 
}

auto M6502::jmpAbsolute() -> void {
    
    uint16_t newPC = readPCInc();
    newPC |= readPC( LAST ) << 8;
    PC = newPC;
}

auto M6502::jmpIndirect() -> void {
    
    uint8_t absoluteLo = readPCInc();
    uint8_t absoluteHi = readPCInc();
    
    uint16_t newPC = read( absoluteHi << 8 | absoluteLo++ );        
    newPC |= read( absoluteHi << 8 | absoluteLo, LAST ) << 8;
    
    PC = newPC;
}

auto M6502::jsrAbsolute() -> void {

    uint16_t newPC = readPCInc();
    newPC |= readPC() << 8;   
    
    pushStack( PC >> 8 );
    pushStack( PC & 0xff );

    readPC( LAST );

    PC = newPC;
}

auto M6502::branch( bool& flag, bool state ) -> void {
    
    int8_t displacement = readPCInc( LAST );  //polls here for interrupts always, even if branch is taken
    
    // why so complicated? because of possible external change of overflow bit in third half cycle
    if ( flag != state )
        return;
        
    bool addCycle = PAGE_CROSSED( PC, PC + displacement );
    
    readPC( ); //don't polls here, even if this is final cycle
    
    uint16_t branchPC = PC + displacement;
    
    if ( addCycle ) {                
        
        setPCL( PC + displacement );
        
        readPC( LAST ); //polls here for a second time
    }
    
    PC = branchPC;
}

auto M6502::transfer(uint8_t src, uint8_t& target, bool flag) -> void {
    
    readPC( LAST );
    target = flag ? this->_ld( src ) : src;    
}
//pull stack
auto M6502::plp() -> void {
    
    readPC();
    read( 0x100 | S );
    
    setFlags( pullStack( LAST ) );
}

auto M6502::pla() -> void {
    
    readPC();
    read( 0x100 | S );
    
    A = this->_ld( pullStack( LAST ) );    
}
//push stack
auto M6502::php() -> void {
    
    readPC();
    ctx->memory.storeFlags = true;
    pushStack( getFlags() | 0x30, LAST );
    ctx->memory.storeFlags = false;
}

auto M6502::pha() -> void {
    
    readPC();    
    pushStack( A, LAST);
}

}

#undef PAGE_CROSSED
#undef LAST
#undef ALU