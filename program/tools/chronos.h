
#pragma once

#include <chrono>

struct Chronos {
    
    static auto getTimestampInMilliseconds() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }    

    static auto getTimestampInMicroseconds() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::microseconds>
            (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    } 

};
