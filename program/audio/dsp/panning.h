
#pragma once

#include "base.h"

namespace DSP {

// IIR Filter
struct Panning : Base {
    
    float left0 = 1.0;
    float left1 = 0.0;
    float right0 = 0.0;
    float right1 = 1.0;    
    
    auto init( float left0, float left1, float right0, float right1 ) -> void;
    
    auto process( Data* output, Data* input ) -> void;     
    
};

}
