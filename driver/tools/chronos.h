
#pragma once

#include <chrono>

// unsigned long (4 bytes) is good enough for seconds but too short for milliseconds.
// so use this function only for calculating deltas, but not absolute values.
  
struct Chronos {
    
    static auto getTimestampInMilliseconds() -> unsigned long {
        return std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::steady_clock::now().time_since_epoch()).count();
    }    

    static auto getTimestampInMicroseconds() -> unsigned long {
        return std::chrono::duration_cast<std::chrono::microseconds>
            (std::chrono::steady_clock::now().time_since_epoch()).count();
    } 

};
