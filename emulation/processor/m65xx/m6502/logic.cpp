
#include "m6502.h"

#define zero( data ) data == 0
#define negative( data ) ((data >> 7) & 1)
#define overflow( data ) ((data >> 6) & 1)

namespace MOS65FAMILY {

auto M6502::_and( uint8_t data ) -> uint8_t {
    
    data &= A;
    
    Z = zero( data );
    N = negative( data );
    
    return data;
}

auto M6502::_ora( uint8_t data ) -> uint8_t {
    
    data |= A;
    
    Z = zero( data );
    N = negative( data );
    
    return data;
}

auto M6502::_eor( uint8_t data ) -> uint8_t {
    
    data ^= A;
    
    Z = zero( data );
    N = negative( data );
    
    return data;
}

auto M6502::_ror( uint8_t data ) -> uint8_t {
    
    bool c = C;    
    C = data & 1;
    
    data = c << 7 | data >> 1;
    
    Z = zero( data );
    N = negative( data );

    return data;
}

auto M6502::_rol( uint8_t data ) -> uint8_t {
    
    bool c = C;    
    C = negative( data );
    
    data = data << 1 | c;
    
    Z = zero( data );
    N = negative( data );

    return data;
}

auto M6502::_asl( uint8_t data ) -> uint8_t {
    
    C = negative( data );
    
    data <<= 1;
    
    Z = zero( data );
    N = negative( data );

    return data;
}

auto M6502::_lsr( uint8_t data ) -> uint8_t {
    
    C = data & 1;
    
    data >>= 1;
    
    Z = zero( data );
    N = negative( data );

    return data;
}


auto M6502::_bit( uint8_t data ) -> uint8_t {
    
    Z = (A & data) == 0;
    V = (data >> 6) & 1;
    N = negative( data );
    ctx->soBlock = 1;
    
    return A;
}

auto M6502::_cmp( uint8_t data ) -> uint8_t {
    
    uint16_t res = A - data;
    C = ( (res >> 8) & 1 ) == 0;
    Z = zero( uint8_t( res ) );
    N = negative( res );
    
    return A;
}

auto M6502::_cpx( uint8_t data ) -> uint8_t {
    
    uint16_t res = X - data;
    C = ( (res >> 8) & 1 ) == 0;
    Z = zero( uint8_t( res ) );
    N = negative( res );
    
    return X;
}

auto M6502::_cpy( uint8_t data ) -> uint8_t {
    
    uint16_t res = Y - data;
    C = ( (res >> 8) & 1 ) == 0;
    Z = zero( uint8_t( res ) );
    N = negative( res );
    
    return Y;
}

auto M6502::_dec( uint8_t data ) -> uint8_t {
    
    data--;
    
    Z = zero( data );
    N = negative( data );

    return data;
}

auto M6502::_inc( uint8_t data ) -> uint8_t {
    
    data++;
    
    Z = zero( data );
    N = negative( data );

    return data;
}

auto M6502::_ld( uint8_t data ) -> uint8_t {
    
    Z = zero( data );
    N = negative( data );

    return data;
}

auto M6502::_adc( uint8_t data ) -> uint8_t {
    
    uint16_t res = A + data + C;
	Z = zero( (uint8_t) res ); //calced same way in decimal mode
    
    if( !D ) {				
		N = negative( res );
        V = ~(A ^ data) & (A ^ res) & 0x80;
		
    } else {					
        res = (A & 0x0f) + (data & 0x0f) + C;
        if( res > 0x09 )
            res += 0x06;
        
        bool c = res > 0x0f;
        res = (A & 0xf0) + (data & 0xf0) + (c << 4) + (res & 0x0f);
		// n and v calced same way like binary mode but before adjustment of hi nibble
		N = negative( res );
        V = ~(A ^ data) & (A ^ res) & 0x80;
        
        if( res > 0x9f )
            res += 0x60;
    }    
    
    C = res >= 0x100;
    ctx->soBlock = 1;
	
    return (uint8_t) res;
}

auto M6502::_sbc( uint8_t data ) -> uint8_t {
    
    bool oldC = C;    
    uint8_t dataInv = ~data;	
    
	uint16_t res = A + dataInv + C;
	Z = zero( (uint8_t) res );
	C = res >= 0x100;
    N = negative( res );
	V = ~(A ^ dataInv) & (A ^ res) & 0x80;
    
    if( D ) {
        
        int8_t AL = (A & 0x0f) - (data & 0x0f) + (oldC ? 0 : -1);
        
        if (AL < 0) AL = ((AL - 0x06) & 0x0F) - 0x10;
        
        int16_t res2 = (A & 0xf0) - (data & 0xf0) + AL;
        
        if (res2 < 0) res2 -= 0x60;
        
        res = res2;
    }
    ctx->soBlock = 1;
       
    return (uint8_t)res;
}

//undocumented
auto M6502::_sbx( uint8_t data ) -> uint8_t {
	//sbc with the flag behaviour of cmp
	uint16_t res = (A & X) - data;

	C = ((res >> 8) & 1) == 0;
	Z = zero( uint8_t(res) );
	N = negative(res);

	return (uint8_t)res;
}

auto M6502::_arr( uint8_t data ) -> uint8_t {

    data = _and( data );    
    
	uint8_t res = C << 7 | data >> 1;
	
    if ( D ) { //adc influence
        //todo recheck this garbage algorithm
        uint8_t AH = data >> 4;
        uint8_t AL = data & 15;

        N = C;                
        
        Z = zero( res );
                
        V = ((data ^ res) & 0x40) != 0;

        if (AL + (AL & 1) > 5)
            res = (res & 0xf0) | ((res + 6) & 0xf);

        C = (AH + (AH & 1)) > 5;
        
        if ( C )
            res += 0x60;
        
        return res;
    }    
	
    C = negative( data );    
    V = negative( data ) ^ overflow( data );        
    
    Z = zero( res );
    N = negative( res );
    
    ctx->soBlock = 1;

    return res;
}

auto M6502::_las( uint8_t data ) -> uint8_t {
    
    data &= S;
    
    Z = zero( data );
    N = negative( data );
    
    return data;
}

auto M6502::_ane( uint8_t data ) -> uint8_t { //xaa
    
    /**
     * highly unstable, depends on temperature or ee ( everything else )
     * unsimulatable behaviour in a digital environment
     * NOTE + todo: c64 Mastertronic Burner loader expects 0xff for the magic value
     * it seems it differs between different cpu models
	 *	- always working
	 *  - never working
	 *  - sometimes working
	 * ok the best we can do is to inject this value from outside instead of trying to
	 * assign fixed values for different cpu manufacturers
     */
    
    data = ( A | ctx->magicAne ) & X & data;
    
    Z = zero( data );
    N = negative( data );
   
    return data;
}

auto M6502::_lax( uint8_t data ) -> uint8_t {
    
    //uint8_t magic = 0xee;
    
    data = (A | ctx->magicAne) & data;
    
    Z = zero( data );
    N = negative( data );
    
    return data;
}

}

#undef zero
#undef negative
#undef overflow