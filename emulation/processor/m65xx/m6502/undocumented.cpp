
#include "m6502.h"

#define fp(name) &M6502::_##name
#define LAST true
#define ALU (this->*alu)
#define ALU2 (this->*alu2)

#define GET_REG( _reg ) _reg == RegA ? A : (_reg == RegS ? S : (_reg == RegX ? X : (_reg == RegY ? Y : (_reg == RegAX ? (A & X) : A ))))

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
	
	zeroPage<RegA>( fp(ld) );
	
	X = A;
}

auto M6502::zeroPageIndexedLax() -> void {
	
	zeroPageIndexed<RegY, RegA>( fp(ld) );
	
	X = A;
}

auto M6502::absoluteLax() -> void {
	
	absolute<RegA>( fp(ld) );
	
	X = A;
}

auto M6502::absoluteIndexedLax() -> void {
	
	absoluteIndexed<RegY, RegA>( fp(ld) );
	
	X = A;
}

auto M6502::immediate() -> void {
	
	readPCInc<1>( LAST );
}

auto M6502::immediateLax() -> void {
    
	immediate<RegA>( fp(lax) );
	
	X = A;
}

//las
auto M6502::absoluteIndexedLas() -> void {
	
	absoluteIndexed<RegY, RegA>( fp(las));
	
	X = S = A;
}

//shx, shy
template<M6502::Reg regIndex, M6502::Reg reg> auto M6502::absoluteIndexedWSh( ) -> void {
    
    absoluteIndexedAdr<regIndex>(  true );    
	
	H1AndedWrite( GET_REG( reg ) );    
}
//ahx
auto M6502::absoluteIndexedWAhx() -> void {
    
    absoluteIndexedAdr<RegY>( true );  
	
	H1AndedWrite( A & X );	
}

//tas
auto M6502::absoluteIndexedWTas() -> void {
    
    absoluteIndexedAdr<RegY>( true );    		
	
	S = A & X;
	
	H1AndedWrite( A & X );
}

//anc
auto M6502::immediateAnc() -> void {
	
	immediate<RegA>( fp(and) );	
	
	C = N;
}
//alr
auto M6502::immediateAlr() -> void {
    
	A = (this->_lsr)( (this->_and)( readPCInc<1>( LAST ) ) );
}
//arr
auto M6502::immediateArr() -> void {
    
	A = (this->_arr)( readPCInc<1>( LAST ) );
}
//ane
auto M6502::immediateAne() -> void {
    
	ctx->xaa = true;

    A = (this->_ane)( readPCInc<1>( LAST ) );
	
	ctx->xaa = false;
}
//sbx
auto M6502::immediateSbx() -> void {
	
	X = (this->_sbx)( readPCInc<1>( LAST ) );
}

//kill
auto M6502::kill() -> void {

    readPCInc<1>();
    
    read<2>(0xffff);
    read<3>(0xfffe);
    read<4>(0xfffe);
    
    read<5>(0xffff); //now reading from 0xffff endless
	
	ctx->killed = true;
}

auto M6502::indexedIndirectM( Alu alu, Alu alu2 ) -> void {
    
    indexedIndirectAdr();
    
	ctx->data = read<5>( ctx->absolute );
    write( ctx->absolute, ctx->data );
    
    ctx->data = ALU( ctx->data );
    
    write( ctx->absolute, ctx->data, LAST );
    
    A = ALU2( ctx->data );
}

auto M6502::indirectIndexedWAhx() -> void {
	
	indirectIndexedAdr( true );
	
	H1AndedWrite( A & X );	
}

auto M6502::indirectIndexedM( Alu alu, Alu alu2 ) -> void {
	
    indirectIndexedAdr( true );
    
    ctx->data = read<5>( ctx->absIndexed );    
    write( ctx->absIndexed, ctx->data );
    
    ctx->data = ALU( ctx->data );
    
	write( ctx->absIndexed, ctx->data, LAST );
    
    A = ALU2( ctx->data );
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

template<M6502::Reg regIndex> auto M6502::absoluteIndexedM( Alu alu, Alu alu2 ) -> void {

	absoluteIndexedM<regIndex>( alu );
			
	A = ALU2( ctx->db );
}

auto M6502::H1AndedWrite( uint8_t anded ) -> void {
	
	uint8_t strange = (ctx->absolute >> 8) + 1;
	strange &= anded;
	
	uint8_t data = anded;
	
	if ( !ctx->rdyLastCycle )		
		data = strange;
	
	if (ctx->boundaryCrossing)
		ctx->absIndexed = (strange << 8) | (ctx->absIndexed & 0xff);
    
    write( ctx->absIndexed, data, LAST );
}

}

#undef fp
#undef LAST
#undef ALU
#undef ALU2