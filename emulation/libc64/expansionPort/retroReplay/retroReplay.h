#pragma once

#include "../../interface.h"
#include "../../../tools/flash040.h"
#include "../cart/freezeButton.h"

namespace LIBC64 {
    
struct RetroReplay : FreezeButton {
    
    RetroReplay(System* system);
    ~RetroReplay();

    Emulator::Flash040 flash;
    std::function<void ()> flashModeReset;
    uint8_t* flashData;
    uint8_t* ram = nullptr;
    bool flashJumper;
    bool bankJumper;
    bool enabled;
    uint8_t bank;
    bool frozen;
    bool ramMode;
    
    bool nordicPower;
    bool allowBank;
    bool noFreeze;
    bool reuMapping;
    bool writeOnce;
    bool writeProtect;
    
    bool requestedGame;
    bool requestedExRom;
    
    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    auto readIo1( uint16_t addr ) -> uint8_t;
    auto peekIo1( uint16_t addr ) -> uint8_t;
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto peekIo2( uint16_t addr ) -> uint8_t;
    auto setJumper( unsigned jumperId, bool state ) -> void;
    auto getJumper( unsigned jumperId ) -> bool;
    template<bool specialCase = false> auto getFlashAddr( uint32_t addr ) -> uint32_t;
    template<bool ignoreAllowBank = false> auto getRamAddr( uint16_t addr ) -> uint16_t;
    auto init() -> void;
    
    auto readRomL( uint16_t addr ) -> uint8_t;
    auto peekRomL( uint16_t addr ) -> uint8_t;
    auto writeRomL( uint16_t addr, uint8_t data ) -> void;
    auto listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void;
    
    auto readRomH( uint16_t addr ) -> uint8_t;
    auto peekRomH( uint16_t addr ) -> uint8_t;
    auto peekUltimaxA0( uint16_t addr ) -> uint8_t;
    auto readUltimaxA0( uint16_t addr ) -> uint8_t;
    
    
    auto writeRomH( uint16_t addr, uint8_t data ) -> void;
    auto writeUltimaxA0( uint16_t addr, uint8_t data ) -> void;
    
    auto clock() -> void;
    
    auto didFreeze() -> void;
    auto blockFreeze() -> bool;
    auto reset(bool softReset = false) -> void;
    auto setWriteProtect(bool state) -> void;
    auto isWriteProtected() -> bool;
    auto write() -> void;
    //auto isBootable( ) -> bool;
    auto serialize(Emulator::Serializer& s) -> void;
    auto createImage(Emulator::Interface::Media* media, unsigned& imageSize) -> uint8_t*;

    auto getSizeNotConsideredForMemorySerialization() -> unsigned;
};

}
