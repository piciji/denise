
#pragma once

#include "data.h"

namespace DSP {

struct Base {
    
    bool mono = false;
    
    virtual auto process( Data* output, Data* input ) -> void {}
    
    auto setMono(bool state) -> void {
        
        // expects a two channel source
        // mono simly copies result of left channel to right channel to reduce calculation time
        this->mono = state;
    }
};

}
