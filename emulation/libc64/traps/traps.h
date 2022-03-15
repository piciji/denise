
#pragma once

#include <functional>
#include <vector>
#include <string>
#include "baseDevice.h"

namespace LIBC64 {

    struct Traps {
        
        struct Trap {
            std::string name;
            uint16_t address;
            uint16_t resumeAddress;
            uint8_t check[3];
            std::function<void ()> job;
        };
        std::vector<Trap> trapList;

        struct Serial {
            int inuse;  // has connected device
            int isopen[16];
            BaseDevice* device = nullptr;
            uint8_t nextbyte[16];
        } serialdevices[16];

        uint8_t SerialBuffer[256];
        int SerialPtr;

        bool installed = false;

        uint8_t device;
        uint8_t secondary;
        
        auto add(Trap trap) -> void;
        auto installSerial() -> void;
        auto installTape() -> void;
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

        auto tapeFindHeader() -> void;
        auto tapeReceive() -> void;

        auto setSt(uint8_t st) -> void;
        auto serialcommand(unsigned int device, uint8_t secondary) -> uint8_t;
        auto unlisten(unsigned int device, uint8_t secondary) -> void;
        auto untalk(unsigned int device, uint8_t secondary) -> void {}
        auto listentalk(unsigned int device, uint8_t secondary) -> void;
        auto close(unsigned int device, uint8_t secondary) -> void;
        auto open(unsigned int device, uint8_t secondary) -> void;
        auto write(unsigned int device, uint8_t secondary, uint8_t data) -> void;
        auto read(unsigned int device, uint8_t secondary) -> uint8_t;
        auto reset() -> void;

        auto listentalkSecondary(uint8_t b) -> void;
    };
    
    extern Traps* traps;
}
