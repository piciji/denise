
#pragma once

#include <cstdint>

namespace Emulator {

inline static auto atLeastTwoBitsSeted( uint8_t value ) -> bool {
    return ( value & (value - 1) ) != 0;
}

}