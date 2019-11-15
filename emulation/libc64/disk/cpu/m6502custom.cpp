
#include "m6502custom.h"

#include "serialization.cpp"

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
        decode( ctx->IR );       
}

auto M6502Custom::detectIrq() -> void {

    ctx->irqPending = ctx->irqLine;
}

//indexed indirect
auto M6502Custom::indexedIndirect( Alu alu ) -> void { 
	    
    switch(step++) {
        case 0:
            adrTemp = indexedIndirectAdr();            
            break;
        case 1:
            A = ALU( read( adrTemp, LAST ) );
            step = 0;
            break;
    }    	
}

auto M6502Custom::indexedIndirectW( uint8_t data ) -> void {
	   
    switch(step++) {
        case 0:
            adrTemp = indexedIndirectAdr();
            readNext = false;
            break;
        case 1:
            write( adrTemp, data, LAST );
            step = 0;
            break;
    }       
}
    
auto M6502Custom::indirectIndexed( Alu alu ) -> void {
	 
    switch(step++) {
        case 0:
            adrTemp = indirectIndexedAdr();
            break;
        case 1:
            A = ALU( read( adrTemp, LAST ) );
            step = 0;
            break;
    }       	
}

auto M6502Custom::indirectIndexedW( uint8_t data ) -> void {
	
    switch(step++) {
        case 0:
            adrTemp = indirectIndexedAdr( true );
            readNext = false;
            break;
        case 1:
            write( adrTemp, data, LAST );
            step = 0;
            break;
    }      
}

//zero page
auto M6502Custom::zeroPage( Alu alu, uint8_t& data ) -> void {
    
    switch(step++) {
        case 0:
            zeroAdrTemp = readPCInc();
            break;
        case 1:
            if (alu)
                data = ALU(loadZeroPage(zeroAdrTemp, LAST));
            else
                loadZeroPage(zeroAdrTemp, LAST);

            step = 0;
            break;            
    }            
}


auto M6502Custom::zeroPageW( uint8_t data ) -> void {

    switch (step++) {
        case 0:
            zeroAdrTemp = readPCInc();
            readNext = false;
            break;
        case 1:
            storeZeroPage( zeroAdrTemp, data, LAST );
            step = 0;
            break;
    }            
}

auto M6502Custom::zeroPageM( Alu alu ) -> void {
    switch(step++) {
        case 0:
            // zero page addressing can not access via, so we don't need to jump out before a memory access
            zeroAdrTemp = readPCInc();
            dataTemp = loadZeroPage( zeroAdrTemp );
            storeZeroPage( zeroAdrTemp, dataTemp );
            readNext = false;
            break;
        case 1:
            storeZeroPage( zeroAdrTemp, ALU( dataTemp ), LAST );            
            step = 0;
            break;            
    }                   
}

//zero page indexed
auto M6502Custom::zeroPageIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void {
    
    switch(step++) {
        case 0:
            zeroAdrTemp = zeroPageIndexedAdr( index );
            break;
        case 1:
            if (alu)
                data = ALU( loadZeroPage( zeroAdrTemp, LAST ) );
            else
                loadZeroPage( zeroAdrTemp, LAST );

            step = 0;
            break;            
    }       
}

auto M6502Custom::zeroPageIndexedW( uint8_t index, uint8_t data ) -> void {
    
    switch(step++) {
        case 0:
            zeroAdrTemp = zeroPageIndexedAdr( index );
            readNext = false;
            break;
        case 1:
            storeZeroPage( zeroAdrTemp, data, LAST );    
            step = 0;
            break;            
    }            
}

auto M6502Custom::zeroPageIndexedM( Alu alu ) -> void {

    switch(step++) {
        case 0:
            zeroAdrTemp = zeroPageIndexedAdr( X );
            dataTemp = loadZeroPage( zeroAdrTemp );
            storeZeroPage( zeroAdrTemp, dataTemp );
            readNext = false;
            break;
        case 1:
            storeZeroPage( zeroAdrTemp, ALU( dataTemp ), LAST );
            step = 0;
            break;            
    }    
}

// absolute
auto M6502Custom::absolute( Alu alu, uint8_t& data ) -> void {
    
    switch(step++) {
        case 0:
            adrTemp = absoluteAdr();
            break;
        case 1:
            if (alu)
                data = ALU(read(adrTemp, LAST));
            else
                read(adrTemp, LAST);
            
            step = 0;
            break;
    }          
}

auto M6502Custom::absoluteW( uint8_t data ) -> void {
    
    switch (step++) {
        case 0:
            adrTemp = absoluteAdr();
            readNext = false;
            break;
        case 1:
            write( adrTemp, data, LAST );
            step = 0;
            break;
    }      
}

auto M6502Custom::absoluteM( Alu alu ) -> void {

    switch (step++) {
        case 0:
            adrTemp = absoluteAdr();
            break;
        case 1:
            dataTemp = read( adrTemp ); 
            readNext = false;
            break;
        case 2:
            write( adrTemp, dataTemp );            
            break;
        case 3:
            write( adrTemp, ALU( dataTemp ), LAST );
            step = 0;
            break;
    }      
}

auto M6502Custom::absoluteIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void {

    switch (step++) {
        case 0:
            adrTemp = absoluteIndexedAdr( index );
            break;
        case 1:
            if (alu)
                data = ALU(read(adrTemp, LAST));
            else
                read(adrTemp, LAST);
            
            step = 0;
            break;
    } 
}

auto M6502Custom::absoluteIndexedW( uint8_t index, uint8_t data ) -> void {

    switch (step++) {
        case 0:
            adrTemp = absoluteIndexedAdr( index, true );
            readNext = false;
            break;
        case 1:
            write( adrTemp, data, LAST );
            step = 0;
            break;
    } 
}

auto M6502Custom::absoluteIndexedM( uint8_t index, Alu alu ) -> void {
    
    switch (step++) {
        case 0:
            adrTemp = absoluteIndexedAdr( index, true );
            break;
        case 1:
            dataTemp = read( adrTemp );
            readNext = false;
            break;
        case 2:
            write(adrTemp, dataTemp);
            break;
        case 3:
            write(adrTemp, ALU(dataTemp), LAST);
            step = 0;
            break;
    }   
}

//immediate
auto M6502Custom::immediate( Alu alu, uint8_t& data ) -> void {
    
	switch(step++) {
        case 0:
            break;
        case 1:
            data = ALU( readPCInc( LAST ) );
            step = 0;
            break;            
    }
	
}

//implied
auto M6502Custom::implied(Alu alu, uint8_t& data) -> void {
	
    switch(step++) {
        case 0:            
            break;
        case 1:
            readPC( LAST );
            data = ALU( data );
            step = 0;
            break;            
    }		
}

auto M6502Custom::nop() -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            readPC( LAST );
            step = 0;
            break;            
    }        
}

auto M6502Custom::rti() -> void {

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

auto M6502Custom::rts() -> void {

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

auto M6502Custom::clear( bool& flag ) -> void {
    // I flag change is too late and not recognized for interrupt sampling at the end of this opcode
    // but when cpu enters rdy wait mode then the flag change is recognized in second cycle
    // we mark an upcomming state change

    switch(step++) {
        case 0:
            ctx->memory.cli = &flag == &I;
            break;
        case 1:
            readPC( LAST );
            ctx->memory.cli = false;
            flag = false; 
            ctx->memory.soBlock = &flag == &V ? 2 : 0;
            step = 0;
            break;            
    }        	
}

auto M6502Custom::set( bool& flag ) -> void {

    switch(step++) {
        case 0:
            ctx->memory.sei = &flag == &I;
            break;
        case 1:
            readPC( LAST );
            ctx->memory.sei = false;
            flag = true; 
            step = 0;
            break;            
    }
}

auto M6502Custom::jmpAbsolute() -> void {
    
    switch(step++) {
        case 0:
            adrTemp = readPCInc();
            break;
        case 1:
            adrTemp |= readPC( LAST ) << 8;
            PC = adrTemp;
            step = 0;
            break;            
    }        
}

auto M6502Custom::jmpIndirect() -> void {

    switch(step++) {
        case 0:
            zeroAdrTemp = readPCInc();
            dataTemp = readPCInc();
            adrTemp = read( dataTemp << 8 | zeroAdrTemp++ );        
            break;
        case 1:
            adrTemp |= read( dataTemp << 8 | zeroAdrTemp, LAST ) << 8;
            PC = adrTemp;
            step = 0;
            break;            
    }
}

auto M6502Custom::jsrAbsolute() -> void {

    switch(step++) {
        case 0:
            adrTemp = readPCInc();
            adrTemp |= readPC() << 8;   

            pushStack( PC >> 8 );
            pushStack( PC & 0xff );
            break;
        case 1:
            readPC( LAST );
            PC = adrTemp;
            step = 0;
            break;            
    }
}

auto M6502Custom::branch( bool& flag, bool state ) -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            displacement = readPCInc( LAST );  //polls here for interrupts always, even if branch is taken                    
            
            // why so complicated? because of possible external change of overflow bit in third half cycle
            if ( flag != state ) 
                step = 0;
            break;            
        case 2: {
            bool addCycle = PAGE_CROSSED( PC, PC + displacement );
            readPC( ); //don't polls here, even if this is final cycle
            adrTemp = PC + displacement;
            if (!addCycle) {
                PC = adrTemp;
                step = 0;
            } else {
                setPCL( PC + displacement );    
            }
        } break;
        case 3:
            readPC( LAST ); //polls here for a second time
            PC = adrTemp;
            step = 0;
            break;
    }
}

auto M6502Custom::transfer(uint8_t src, uint8_t& target, bool flag) -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            readPC( LAST );
            target = flag ? this->_ld( src ) : src;    
            step = 0;
            break;            
    }    
}
//pull stack
auto M6502Custom::plp() -> void {

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

auto M6502Custom::pla() -> void {
    
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
auto M6502Custom::php() -> void {

    switch(step++) {
        case 0:
            readPC();
            break;
        case 1:
            ctx->memory.storeFlags = true;
            pushStack( getFlags() | 0x30, LAST );
            ctx->memory.storeFlags = false;
            step = 0;
            break;            
    }            
}

auto M6502Custom::pha() -> void {

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
auto M6502Custom::indexedIndirectLax( ) -> void {

	indexedIndirect( fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::indirectIndexedLax( ) -> void {

	indirectIndexed( fp(ld) );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::zeroPageLax() -> void {
	
	zeroPage( fp(ld), A );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::zeroPageIndexedLax() -> void {
	
	zeroPageIndexed( Y, fp(ld), A );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::absoluteLax() -> void {
	
	absolute( fp(ld), A );
	
    if (step == 0)
        X = A;
}

auto M6502Custom::absoluteIndexedLax() -> void {
	
	absoluteIndexed( Y, fp(ld), A);
	
    if (step == 0)
        X = A;
}

auto M6502Custom::immediate() -> void {
    
    switch(step++) {
        case 0:
            break;
        case 1:
            readPCInc( LAST );
            step = 0;
            break;            
    }		
}

auto M6502Custom::immediateLax() -> void {
    
	immediate( fp(lax), A );
	
    if (step == 0)
        X = A;
}

//las
auto M6502Custom::absoluteIndexedLas() -> void {
	
	absoluteIndexed( Y, fp(las), A);
	
    if (step == 0)
        X = S = A;
}

//shx, shy
auto M6502Custom::absoluteIndexedWSh( uint8_t index, uint8_t index2 ) -> void {

    switch(step++) {
        case 0:
            adrTemp = absoluteIndexedAdr( index, true ); 
            readNext = false;
            break;
        case 1:
            H1AndedWrite( adrTemp, index2 );    
            step = 0;
            break;            
    }        		
}
//ahx
auto M6502Custom::absoluteIndexedWAhx() -> void {
    
    switch(step++) {
        case 0:
            adrTemp = absoluteIndexedAdr( Y, true );  
            readNext = false;
            break;
        case 1:
            H1AndedWrite( adrTemp, A & X );	
            step = 0;
            break;            
    }	
}

//tas
auto M6502Custom::absoluteIndexedWTas() -> void {

    switch(step++) {
        case 0:
            adrTemp = absoluteIndexedAdr( Y, true ); 
            readNext = false;            
            break;
        case 1:
            S = A & X;
            H1AndedWrite( adrTemp, A & X );
            step = 0;
            break;            
    }
}

//anc
auto M6502Custom::immediateAnc() -> void {
	
	immediate( fp(and), A );	
    
	if (step == 0)
        C = N;
}
//alr
auto M6502Custom::immediateAlr() -> void {
    
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
auto M6502Custom::immediateArr() -> void {

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
auto M6502Custom::immediateAne() -> void {
   
    switch(step++) {
        case 0:
            break;
        case 1:
            ctx->memory.xaa = true;
            A = (this->_ane)( readPCInc( LAST ) );
            ctx->memory.xaa = false;
            step = 0;
            break;            
    }    
}
//sbx
auto M6502Custom::immediateSbx() -> void {
	
    switch(step++) {
        case 0:
            break;
        case 1:
            X = (this->_sbx)( readPCInc( LAST ) );
            step = 0;
            break;            
    }
}

auto M6502Custom::indexedIndirectM( Alu alu, Alu alu2 ) -> void {
    
    switch(step++) {
        case 0:
            adrTemp = indexedIndirectAdr();
            break;
        case 1:
            dataTemp = read( adrTemp );
            readNext = false;
            break;
        case 2:
            write( adrTemp, dataTemp );                        
            dataTemp = ALU( dataTemp );
            break;
        case 3:            
            write( adrTemp, dataTemp, LAST );
            A = ALU2( dataTemp );
            step = 0;
            break;            
    }    
}

auto M6502Custom::indirectIndexedWAhx() -> void {
	
    switch(step++) {
        case 0:
            adrTemp = indirectIndexedAdr( true );
            readNext = false;
            break;
        case 1:
            H1AndedWrite( adrTemp, A & X );	
            step = 0;
            break;            
    }	
}

auto M6502Custom::indirectIndexedM( Alu alu, Alu alu2 ) -> void {

    switch(step++) {
        case 0:
            adrTemp = indirectIndexedAdr( true );
            break;
        case 1:
            dataTemp = read( adrTemp );    
            readNext = false;
            break;
        case 2:
            write( adrTemp, dataTemp );
            dataTemp = ALU( dataTemp );
            break;
        case 3:
            write( adrTemp, dataTemp, LAST );
            A = ALU2( dataTemp );
            step = 0;
            break;            
    }	    
}

auto M6502Custom::zeroPageIndexedM( Alu alu, Alu alu2 ) -> void {
    
    zeroPageIndexedM( alu );
    
    if (step == 0)
        A = ALU2( ctx->db ); 
}

auto M6502Custom::zeroPageM( Alu alu, Alu alu2 ) -> void {
    
    zeroPageM( alu );
    
    if (step == 0)
        A = ALU2( ctx->db );
}

auto M6502Custom::absoluteM( Alu alu, Alu alu2 ) -> void {

    absoluteM( alu );
	
    if (step == 0)
        A = ALU2( ctx->db );
}

auto M6502Custom::absoluteIndexedM( uint8_t index, Alu alu, Alu alu2 ) -> void {

	absoluteIndexedM( index, alu );
			
    if (step == 0)
        A = ALU2( ctx->db );
}

}

#undef PAGE_CROSSED
#undef LAST
#undef fp
#undef ALU
#undef ALU2
