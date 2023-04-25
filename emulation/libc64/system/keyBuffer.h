
#pragma once

#include <functional>

#include "../prg/prg.h"
#include "../tape/tape.h"
#include "../../tools/petcii.h"

namespace LIBC64 {
   
struct System;
struct Tape;
    
struct KeyBuffer {

    KeyBuffer(System* system, Tape& tape) : system(system), tape(tape) {}

    const uint16_t countAdr = 198;
    
    const uint16_t bufferAdr = 631;

    const uint16_t currentScreenLineAdr = 0xd1;
    
    const uint16_t cursorColumnAdr = 0xd3;
    
    const uint16_t blinkCursorAdr = 0xcc;
    
    enum Found { Yes, No, NotYet };
    
    enum Mode : uint8_t { WaitDelay, WaitFor, Input };

    System* system;
    Tape& tape;
    
    struct Action {
        uint8_t callbackId = 0;
        Mode mode;
        std::vector<uint8_t> buffer;    
        std::vector<uint8_t> alternateBuffer;   
        unsigned delay = 0;
        bool blinkingCursor = false;
        std::function<void ( )> callback = nullptr;
        std::function<void (Action* action )> waitCallback = nullptr;
        
        unsigned position = 0;
    };
    
    std::vector<Action> queue;
    bool hasJobs = false;

    auto add( Action action, bool inSeconds = true ) -> void;
    auto forceDefaultKernalDelay() -> void;
    auto reset() -> void;
    auto isPrgInjectionInQueue() -> bool;
    auto serialize(Emulator::Serializer& s) -> void;
    auto serializeAction( Emulator::Serializer& s, Action& action ) -> void;
    auto process() -> void;
    auto feed( uint8_t c ) -> bool;
    auto checkFor( std::vector<uint8_t> buffer, bool withBlinkingCursor ) -> Found;
    auto paste( std::string str ) -> void;

};       
    
}
