
#pragma once

namespace Emulator {
    
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
    
}
