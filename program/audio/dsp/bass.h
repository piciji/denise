
#pragma once

#include "base.h"

namespace DSP {

// IIR Filter
struct Bass : Base {
    
    float b0, b1, b2;
    float a0, a1, a2;

    struct {
        float xn1, xn2;
        float yn1, yn2;
    } l, r;
    
    float reduceClipping;
    
    auto init( float sampleRate, float frequency, float gain, float reduceClipping ) -> void;
    
    auto process( Data* output, Data* input ) -> void;  
    
    auto processOneChannel( Data* output, Data* input ) -> void;
   
    
};

}
