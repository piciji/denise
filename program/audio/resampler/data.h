
#pragma once

namespace Resampler {
    
struct Data {
    float* in;
    float* out;

    // 1 or 2 incomming channels supported
    // in case of one channel, resampler result is simply copied in second channel
    unsigned inChannels;
    unsigned inputFrames;
    unsigned outputFrames; // always 2 channels in interleaved format: l,r,l,r ...
    
    double ratio;
};
    
}
