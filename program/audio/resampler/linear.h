
#pragma once

#include <math.h>

#include "data.h"

namespace Resampler {

struct Linear {

    struct FrameBuffer {
        float l;
        float r;
    };

    Data* rData = nullptr;
    float fraction;
    FrameBuffer buffer;

    auto setData( Data* rData ) -> void {
        this->rData = rData;
    }

    auto process( ) -> void;

    auto reset(float ratio, unsigned inChannels) -> void;

};

}
