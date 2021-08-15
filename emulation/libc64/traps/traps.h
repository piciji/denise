
#pragma once

#include <functional>

#define TRAP_OPCODE 0x02

namespace LIBC64 {
    
    struct Traps {
        
        struct Trap {
            std::string name;
            uint16_t address;
            uint16_t resumeAddress;
            uint8_t check[3];
            std::function<void ()> job;
        }
        
        std::vector<Trap> trapList;
        bool installed = false;
        
        auto add(Trap trap) -> void;
        auto install() -> void;
        auto install(Trap& t) -> bool;
        auto uninstall() -> void;
        auto uninstall(Trap& t) -> void;
        auto storeKernal(uint16_t addr, uint8_t value) -> void;
        auto readKernal(uint16_t addr) -> uint8_t;
        
        auto handler() -> bool;
        auto attention() -> void;
        auto send() -> void;
        auto receive() -> void;
        auto ready() -> void;
    }
    
    extern Traps* traps;
}
