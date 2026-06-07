
#pragma once

template<typename T> static auto copyBufferToInt( const uint8_t* buf ) -> T {

    T value = 0;

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        value |= buf[i] << ( i << 3 );
    }

    return value;
}

template<typename T> static auto copyIntToBuffer( uint8_t* buf, T value ) -> uint8_t* {

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        buf[i] = value & 0xff;

        value >>= 8;
    }

    return buf + sizeof(T);
}

static auto copyStringToBuffer(uint8_t* buf, uint8_t length, uint8_t* input) -> uint8_t* {
    buf[0] = length;
    memcpy(&buf[1], input, length);

    return buf + length + 1;
}
