
#pragma once

#include "../expansionPort.h"

#include "../../../tools/event.h"

namespace LIBC64 {
    
struct Reu : ExpansionPort {   
    
    Reu(unsigned size);
    ~Reu();
    
    using Callback = std::function<void ()>;
    Emulator::Events* events;
    
    uint8_t status;      
    
    uint8_t command;
    
    uint16_t hostAddr;
    
    uint16_t reuAddr;
    
    uint8_t reuBank;
    
    unsigned size; // in kb
    uint8_t* data = nullptr;
    
    uint16_t transferLength;
    
    uint8_t intMask;
    
    uint8_t addrControl;
    
    Callback setIrq;
    Callback unsetIrq;
    Callback setDma;
    
    bool waitForStart;
    
    auto cycle() -> void;
    
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    
    auto readIo2( uint16_t addr ) -> uint8_t;
};    
    
}