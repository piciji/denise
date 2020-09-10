
#pragma once

#include <chrono>

struct Chronos {
    
    // steady_clock -> boot time of PC
    
    static auto getTimestampInSeconds() -> uint32_t {
        return std::chrono::duration_cast<std::chrono::seconds>
            (std::chrono::steady_clock::now().time_since_epoch()).count();
    } 
    
    static auto getTimestampInMilliseconds() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::steady_clock::now().time_since_epoch()).count();
    }    

    static auto getTimestampInMicroseconds() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::microseconds>
            (std::chrono::steady_clock::now().time_since_epoch()).count();
    } 

};
