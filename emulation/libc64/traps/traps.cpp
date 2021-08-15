
#include "traps.h"
#incluse "../system/system.h"
#include "../m6510/m6510.h"

namespace LIBC64 {
    
    Traps* traps = nullptr;
    
    auto Traps::add(Trap trap) -> void {
        trapList.push_back( trap );
    }

    auto Traps::install() -> void {
        for(auto& trap : trapList)
            install(trap);
        
        installed = true;
    }
    
    auto Traps::install(Trap& t) -> bool {
        for (uint8_t i = 0; i < 3; i++) {
            if (readKernal(t.address + i) != t.check[i]) {
                return false;
            }
        }
        
        storeKernal(t.address, TRAP_OPCODE);
        
        return true;
    }
    
    auto Traps::uninstall() -> void {
        for(auto& trap : trapList)
            uninstall(trap);
            
        installed = false;
    }
    
    auto Traps::uninstall(Trap& t) -> void {
        if (readKernal(t.address) != TRAP_OPCODE) {
            return;
        }
        
        storeKernal(t.address, t.check[0]);
    }
    
    auto Traps::handler() -> bool {
        if (!installed)
            return false;
            
        auto pc = cpu->pc;
        
        for(auto& trap : trapList) {
            if (trap.address == pc) {
                auto resumeAddr = trap.resumeAddress;
                trap.job();
                cpu->pc = resumeAddress;
                return true;
            }
        }
        
        return false;
    }

    auto Traps::storeKernal(uint16_t addr, uint8_t value) -> void {
        switch (addr & 0xf000) {
            case 0xe000:
            case 0xf000:
                system->kernalRom[addr & 0x1fff] = value;
                break;
        }
    }
    
    auto Traps::readKernal(uint16_t addr) -> uint8_t {
        switch (addr & 0xf000) {
            case 0xe000:
            case 0xf000:
                return system->kernalRom[addr & 0x1fff];
        }
        return 0;
    }

    
    auto Traps::attention() -> void {
    
    }
    
    auto Traps::send() -> void {
        
    }
    
    auto Traps::receive() -> void {
        
    }
    
    auto Traps::ready() -> void {
        
    }
}
