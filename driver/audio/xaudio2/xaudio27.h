
#pragma once

#define UNICODE

#include <cstdint>
#include <algorithm>

namespace DRIVER {
    
struct XAudio27 {
    
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto clear() -> void {}        
    virtual auto setFrequency(unsigned value) -> void {}
    virtual auto getFrequency() -> unsigned { return 48000u; }
    virtual auto setLatency(unsigned value) -> void {}
    virtual auto synchronize(bool state) -> void {}
    virtual auto hasSynchronized() -> bool { return false; }
    virtual auto addSamples( const uint8_t* buffer, unsigned size) -> void {}
    virtual auto getCenterBufferDeviation() -> double { return 0.0; }
    virtual auto getMinimumLatency() -> unsigned { return 1; }
	
	virtual ~XAudio27() = default;
    static auto create( ) -> XAudio27*;
};    
    
}
