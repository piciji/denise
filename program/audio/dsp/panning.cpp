
#include "panning.h"

namespace DSP {
    
auto Panning::init( float left0, float left1, float right0, float right1 ) -> void {
    
    this->left0 = left0;
    
    this->left1 = left1;
    
    this->right0 = right0;
    
    this->right1 = right1;
}
    
auto Panning::process( Data* output, Data* input ) -> void {
    
    output->samples = input->samples;
    output->frames = input->frames;
        
    float* out = output->samples;

    for (unsigned i = 0; i < input->frames; i++, out += 2) {

        float left = out[0];
        float right = out[1];
        out[0] = left * left0 + right * left1;
        out[1] = left * right0 + right * right1;
    }
}
    
}
