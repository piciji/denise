
#pragma once

#include "base.h"

namespace DSP {

struct Echo : Base {
    Echo() {
        buffer = nullptr;
    }
    virtual ~Echo() {
        free();
    }

    unsigned frames;
    float feedback;
    float amp;
    float* buffer = nullptr;
    unsigned ptr;

    auto process(Data* output, Data* input) -> void;
    auto processOneChannel(Data* output, Data* input) -> void;
    auto init( float sampleRate, unsigned delay, float feedback, float amp) -> void;
    auto free() -> void;
};

}