
#include "m6502.h"

#define fp(name) &M6502::_##name
#define LAST true
#define ALU (this->*alu)
#define ALU2 (this->*alu2)

namespace MOS65FAMILY {

//lax
auto M6502::indexedIndirectLax( ) -> void {

	indexedIndirect( fp(ld) );
	
	X = A;
}

auto M6502::indirectIndexedLax( ) -> void {

	indirectIndexed( fp(ld) );
	
	X = A;
}

auto M6502::zeroPageLax() -> void {
	
	zeroPage( fp(ld), A );
	
	X = A;
}

auto M6502::zeroPageIndexedLax() -> void {
	
	zeroPageIndexed( Y, fp(ld), A );
	
	X = A;
}

auto M6502::absoluteLax() -> void {
	
	absolute( fp(ld), A );
	
	X = A;
}

auto M6502::absoluteIndexedLax() -> void {
	
	absoluteIndexed( Y, fp(ld), A);
	
	X = A;
}

auto M6502::immediate() -> void {
	
	readPCInc( LAST );
}

auto M6502::immediateLax() -> void {
    
	immediate( fp(lax), A );
	
	X = A;
}

//las
auto M6502::absoluteIndexedLas() -> void {
	
	absoluteIndexed( Y, fp(las), A);
	
	X = S = A;
}

//shx, shy
auto M6502::absoluteIndexedWSh( uint8_t index, uint8_t index2 ) -> void {
    
    uint16_t absIndexed = absoluteIndexedAdr( index, true );    
	
	H1AndedWrite( absIndexed, index2 );    
}
//ahx
auto M6502::absoluteIndexedWAhx() -> void {
    
    uint16_t absIndexed = absoluteIndexedAdr( Y, true );  
	
	H1AndedWrite( absIndexed, A & X );	
}

//tas
auto M6502::absoluteIndexedWTas() -> void {
    
    uint16_t absIndexed = absoluteIndexedAdr( Y, true );    		
	
	S = A & X;
	
	H1AndedWrite( absIndexed, A & X );
}

//anc
auto M6502::immediateAnc() -> void {
	
	immediate( fp(and), A );	
	
	C = N;
}
//alr
auto M6502::immediateAlr() -> void {
    
	A = (this->_lsr)( (this->_and)( readPCInc( LAST ) ) );
}
//arr
auto M6502::immediateArr() -> void {
    
	A = (this->_arr)( readPCInc( LAST ) );
}
//ane
auto M6502::immediateAne() -> void {
    
	ctx->memory.xaa = true;

    A = (this->_ane)( readPCInc( LAST ) );
	
	ctx->memory.xaa = false;
}
//sbx
auto M6502::immediateSbx() -> void {
	
	X = (this->_sbx)( readPCInc( LAST ) );
}

//kill
auto M6502::kill() -> void {

    readPCInc();
    
    read(0xffff);
    read(0xfffe);
    read(0xfffe);
    
    read(0xffff); //now reading from 0xffff endless
	
	ctx->killed = true;
}

auto M6502::indexedIndirectM( Alu alu, Alu alu2 ) -> void {
    
    auto absolute = indexedIndirectAdr();
    
	uint8_t data = read( absolute );
    write( absolute, data );
    
    data = ALU( data );
    
    write( absolute, data, LAST );
    
    A = ALU2( data );
}

auto M6502::indirectIndexedWAhx() -> void {
	
	uint16_t absIndexed = indirectIndexedAdr( true );
	
	H1AndedWrite( absIndexed, A & X );	
}

auto M6502::indirectIndexedM( Alu alu, Alu alu2 ) -> void {
	
    auto absolute = indirectIndexedAdr( true );
    
    uint8_t data = read( absolute );    
    write( absolute, data );
    
    data = ALU( data );
    
	write( absolute, data, LAST );
    
    A = ALU2( data );
}

auto M6502::zeroPageIndexedM( Alu alu, Alu alu2 ) -> void {
    
    zeroPageIndexedM( alu );
    
    A = ALU2( ctx->db ); 
}

auto M6502::zeroPageM( Alu alu, Alu alu2 ) -> void {
    
    zeroPageM( alu );
    
    A = ALU2( ctx->db );
}

auto M6502::absoluteM( Alu alu, Alu alu2 ) -> void {

    absoluteM( alu );
	
	A = ALU2( ctx->db );
}

auto M6502::absoluteIndexedM( uint8_t index, Alu alu, Alu alu2 ) -> void {

	absoluteIndexedM( index, alu );
			
	A = ALU2( ctx->db );
}

auto M6502::H1AndedWrite( uint16_t absIndexed, uint8_t anded ) -> void {
	
	uint8_t strange = (ctx->memory.absolute >> 8) + 1;
	strange &= anded;
	
	uint8_t data = anded;
	
	if ( !ctx->memory.rdyLastCycle )		
		data = strange;
	
	if (ctx->memory.boundaryCrossing)
		absIndexed = (strange << 8) | (absIndexed & 0xff);
    
    write( absIndexed, data, LAST );
}

}

#undef fp
#undef LAST
#undef ALU
#undef ALU2