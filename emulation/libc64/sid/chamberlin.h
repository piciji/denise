
#pragma once

#include "reSid.h"

namespace LIBC64 {

struct Chamberlin : ReSid {
    using ReSid::ReSid;

    auto clock() -> void override;

    auto clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int override;

    auto getSample() -> float override { return static_cast<float>(externalFilter.output()) * 2.0f / 3.0f; }
};

}