
#pragma once

namespace DSP {

struct Base {
    
    // channels per audio frame
    uint8_t channels;
    
    // simply to reduce calculation time, if multiple channels have same content
    uint8_t realChannels; 
    
    double volume = 1.0;
    
    auto setVolume( double volume ) -> void {
        this->volume = volume;
    }
    
    virtual auto init( uint8_t channels, uint8_t realChannels = 0 ) -> void {
        this->channels = channels;
        this->realChannels = realChannels == 0 ? channels : realChannels;
    }
    
    inline virtual auto core(double* sample, uint8_t channel) -> void {}    
    
    virtual auto process(double* stream, unsigned frames) -> void {

        uint8_t channelMask = channels - 1;

        for (unsigned i = 0; i < (frames << channelMask); i++) {           
            
            uint8_t channel = i & channelMask;

            if (channel >= realChannels) {
                // reuse dsp calculation for other channels of current frame.
                *stream = *(stream - realChannels);
                
            } else {
                
                *stream = std::max(-1.0, std::min(+1.0, *stream * volume));
                
                core( stream, channel );
            }
            
            stream++;
        }
    }
};

}
