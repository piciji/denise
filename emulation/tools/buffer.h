
#pragma once

namespace Emulator {
    
// little endian
template<typename T> static auto copyIntToBuffer( uint8_t* buf, T value ) -> void {    

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        buf[i] = value & 0xff;

        value >>= 8;
    }
}

template<typename T> static auto copyBufferToInt( uint8_t* buf ) -> T {    

    T value = 0;

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        value |= buf[i] << ( i << 3 );
    }

    return value;
}

// big endian
template<typename T> static auto copyIntToBufferBigEndian( uint8_t* buf, T value ) -> void {    

    unsigned shift = (sizeof(T) - 1) << 3;
    
    for( unsigned i = 0; i < sizeof(T); i++ ) {        
        
        buf[i] = (value >> shift) & 0xff;

        value <<= 8;
    }
}

template<typename T> static auto copyBufferToIntBigEndian( uint8_t* buf ) -> T {    

    T value = 0;

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        unsigned shift = sizeof(T) - 1 - i;     
        
        value |= buf[i] << ( shift << 3 );
    }

    return value;
}
    
}
