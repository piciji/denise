
#pragma once

#include <cstdint>

namespace Emulator {

inline static auto atLeastTwoBitsSeted( uint8_t value ) -> bool {
    return ( value & (value - 1) ) != 0;
}

template<typename T> inline static auto copyMasked(T& target, T src, unsigned mask) -> void {
    target = (target & ~mask) | (src & mask);
}

}