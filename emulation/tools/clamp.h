
#pragma once

namespace Emulator {

[[maybe_unused]] static auto sclamp(unsigned bits, const signed x) -> signed {
    const signed b = 1U << (bits - 1);
    const signed m = (1U << (bits - 1)) - 1;
    return (x > m) ? m : (x < -b) ? -b : x;
}

}