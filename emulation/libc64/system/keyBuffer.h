
#pragma once

#include <functional>

#include "system.h"
#include "../prg/prg.h"
#include "../tape/tape.h"
#include "../../tools/petcii.h"

namespace LIBC64 {
   
struct System;
    
struct KeyBuffer {
    
    const uint16_t countAdr = 198;
    
    const uint16_t bufferAdr = 631;

    const uint16_t currentScreenLineAdr = 0xd1;
    
    const uint16_t cursorColumnAdr = 0xd3;
    
    const uint16_t blinkCursorAdr = 0xcc;
    
    enum Found { Yes, No, NotYet };
    
    enum Mode : uint8_t { WaitDelay, WaitFor, Input };
    
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
    
    auto add( Action action, bool inSeconds = true ) -> void {
        
        action.position = 0;
        
        if (inSeconds)
            action.delay = (unsigned)(system->interface->stats.fps * (double)action.delay);
        
        if (action.delay)
            action.delay++;
        
        queue.push_back( action );
        
        Emulator::Serializer s;
        
        serializeAction( s, action );
        
        system->serializationSize += s.size();

        hasJobs = true;
    }

    auto forceDefaultKernalDelay() -> void {
        if (!queue.size())
            return;

        queue[0].delay = (unsigned)(system->interface->stats.fps * 2.2);
    }

    auto reset() -> void {
      
        queue.clear();

        hasJobs = false;
	}   
    
    auto isPrgInjectionInQueue() -> bool {
        
        if (!hasJobs)
            return false;
        
        for(auto& action : queue) {
            if (action.callbackId == 2)
                return true;
        }
        
        return false;
    }
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        uint8_t vSize = queue.size();
        s.integer( vSize );
        
        if ( s.mode() == Emulator::Serializer::Mode::Load ) {
            reset();
            
            for(unsigned i = 0; i < vSize; i++) {
                Action action;
                
                serializeAction( s, action );
                
                if (action.callbackId == 1)
                    action.callback = []() { system->kernalBootComplete = true; };
                  
                // state generation before prg injection will be prevented now
                // else if (action.callbackId == 2)
                //    action.callback = []() { system->prgInUse->inject(); };
                    
                else if (action.callbackId == 3)
                    action.callback = []() { tape->setMode( Tape::Mode::Play ); };

                else if (action.callbackId == 4)
                    action.waitCallback = [](Action* action) {
                        if (system->checkForAutoStarter()) {
                            system->keyBuffer->reset();
                            system->interface->autoStartFinish(true);
                        } };

                else if (action.callbackId == 5)
                    action.callback = [this]() {
                        system->interface->autoStartFinish(false);
                    };
                
                queue.push_back( action );

                hasJobs = true;
            }
            
        } else {

            for (auto& action : queue)
                serializeAction(s, action);            
        }  
      
    }    

    auto serializeAction( Emulator::Serializer& s, Action& action ) -> void {

        s.integer((uint8_t&) action.mode);
        s.vector(action.buffer);
        s.vector(action.alternateBuffer);
        s.integer(action.delay);
        s.integer(action.blinkingCursor);
        s.integer(action.position);
        s.integer(action.callbackId);
    }
    
    auto process() -> void {
        
        if (queue.size() == 0) {
            hasJobs = false;
            return;
        }
        
        Found result;
        
        auto& action = queue[0];
        
        switch (action.mode) {
            
            case Mode::WaitDelay: {
                
                if (!action.delay)
                    break;

                if (++action.position != action.delay )
                    return;
                                                        
            } break;
            
            case Mode::Input: {
                
                while( action.position < action.buffer.size() ) {
                    
                    if ( !feed( action.buffer[ action.position ] ) )
                        // input buffer hasn't an empty space
                        return;
                    
                    action.position++;                                        
                }                
                
            } break;
            
            case Mode::WaitFor: {
                
                if ( ++action.position == action.delay ) {
                    // autostarted programs would check forever
                    reset();
                    return;
                }
                
                result = checkFor( action.buffer, action.blinkingCursor );                
                
                if ( result == Found::No ) {
                    
                    if (action.alternateBuffer.size() != 0) {
                        
                        if ( checkFor( action.alternateBuffer, false ) == Found::Yes )
                            return;
                    }
                    
                    // don't do any further actions
                    system->interface->autoStartFinish(true);
                    reset();
                    return;
                    
                } else if ( result == Found::NotYet ) {
                    // wait some more time
                    if (action.waitCallback)
                        action.waitCallback(&action);
                    return;
                }
                // result == Yes
                
            } break;
        }

        if (action.callback)
            action.callback();
        
        // delete queue entry
        queue.erase( queue.begin() );
    }    
    
    auto feed( uint8_t c ) -> bool {
        
        uint8_t* ram = system->ram;
        
        uint8_t pos = ram[ countAdr ];
        // max 10 char to buffer 
        if (pos >= 9)
            return false;
        
        ram[ bufferAdr + pos ] = c;
        
        ram[ countAdr ] = pos + 1;        
        
        return true;
    }
    
    auto checkFor( std::vector<uint8_t> buffer, bool withBlinkingCursor ) -> Found {

        uint8_t* ram = system->ram;
        
        uint16_t screenAdr = ram[currentScreenLineAdr] | (ram[currentScreenLineAdr + 1] << 8);
        
        uint16_t column = ram[cursorColumnAdr];
                
        uint16_t length = ram[0xd5] + 1;

        if (ram[ countAdr ] != 0)
            // input buffer contains data
            return Found::NotYet;
        
        if (withBlinkingCursor && (column > 1) )
            // blinking cursor is not in first or second column
            return Found::NotYet;
        
        if (withBlinkingCursor && (ram[blinkCursorAdr] != 0) )
            return Found::NotYet;

        uint16_t adr;
        
        if (withBlinkingCursor)
            adr = screenAdr - length;
        else 
            adr = screenAdr;        

        if (buffer.size()) {
            for (int i = 0; i < buffer.size(); i++) {

                if (ram[adr + i] != (buffer[i] & 63)) {
                    return Found::NotYet;
                }
            }
        }

        return Found::Yes;
    }

    auto paste( std::string str ) -> void {

        KeyBuffer::Action action;
        action.mode = KeyBuffer::Mode::Input;

        Emulator::PetciiConversion petciiConversion;
        petciiConversion.encodeWithLineEnding( str, action.buffer );

        add( action );
    }
};       
    
}
