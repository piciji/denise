
#pragma once

#include <string>
#include <cstring>
#include <cstdint>

#define ToU32LE(buf) Emulator::copyBufferToInt<uint32_t>(buf)
#define ToU16LE(buf) Emulator::copyBufferToInt<uint16_t>(buf)
#define FromU32LE(buf, val) Emulator::copyIntToBuffer<uint32_t>(buf, val)
#define FromU16LE(buf, val) Emulator::copyIntToBuffer<uint16_t>(buf, val)

#define ToU32BE(buf) Emulator::copyBufferToIntBigEndian<uint32_t>(buf)
#define ToU16BE(buf) Emulator::copyBufferToIntBigEndian<uint16_t>(buf)
#define FromU32BE(buf, val) Emulator::copyIntToBufferBigEndian<uint32_t>(buf, val)
#define FromU16BE(buf, val) Emulator::copyIntToBufferBigEndian<uint16_t>(buf, val)

namespace Emulator {
    
// little endian
template<typename T> static auto copyIntToBuffer( uint8_t* buf, T value ) -> void {    

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        buf[i] = value & 0xff;

        value >>= 8;
    }
}

template<typename T> static auto copyBufferToInt( const uint8_t* buf ) -> T {    

    T value = 0;

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        value |= buf[i] << ( i << 3 );
    }

    return value;
}

// big endian
template<typename T> static auto copyIntToBufferBigEndian( uint8_t* buf, T value ) -> void {    

    constexpr unsigned shift = (sizeof(T) - 1) << 3;
    
    for( unsigned i = 0; i < sizeof(T); i++ ) {        
        
        buf[i] = (value >> shift) & 0xff;

        value <<= 8;
    }
}

template<typename T> static auto copyBufferToIntBigEndian( const uint8_t* buf ) -> T {    

    T value = 0;

    for( unsigned i = 0; i < sizeof(T); i++ ) {

        unsigned shift = sizeof(T) - 1 - i;     
        
        value |= buf[i] << ( shift << 3 );
    }

    return value;
}

static auto replaceInBuffer(uint8_t* buf, unsigned size, const std::string& needle, const std::string& replace) -> bool {
    unsigned _len = needle.size();
    auto _char = needle.c_str();

    for (unsigned i = 0; i < size - _len; i++) {

        if (std::strncmp((const char*)buf + i, _char, _len) == 0) {
            std::memcpy((void*)(buf + i), (void*)replace.c_str(), _len);
            return true;
        }
    }
    return false;
}
    
}
