
// algorithm is from higan 

#pragma once

#include <vector>
#include "dsp.h"
#include "../../tools/circularBuffer.h"

namespace DSP {

struct Reverb : Base {
        
    std::vector<std::vector<CircularBuffer<double>>> reverb;
    
    auto init( uint8_t channels, uint8_t realChannels = 0 ) -> void {
        
        Base::init(channels, realChannels);
        
        reverb.clear();
        reverb.resize(this->realChannels);

        for (unsigned c = 0; c < this->realChannels; c++) {
            reverb[c].resize(7);
            reverb[c][0].resize(1229);
            reverb[c][1].resize(1559);
            reverb[c][2].resize(1907);
            reverb[c][3].resize(4057);
            reverb[c][4].resize(8117);
            reverb[c][5].resize(8311);
            reverb[c][6].resize(9931);
        }
    }
    
    inline auto core(double* sample, uint8_t channel) -> void {
        
        *sample *= 0.125;

        for (unsigned n = 0; n < 7; n++)
            *sample += 0.125 * reverb[ channel ][n].last();

        for (unsigned n = 0; n < 7; n++)
            reverb[ channel ][n].write(*sample);

        *sample *= 8.000;
    }    
};

}
