
#pragma once

#include <functional>

#include "system.h"
#include "../prg/prg.h"
#include "../tape/tape.h"

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
        
        unsigned position = 0;
    };
    
    std::vector<Action> queue;
    
    auto add( Action action ) -> void {
        
        action.position = 0;
        
        action.delay = (system->ntsc ? 60 : 50) * action.delay;
        
        if (action.delay)
            action.delay++;
        
        queue.push_back( action );
        
        Emulator::Serializer s;
        
        serializeAction( s, action );
        
        system->serializationSize += s.size();
    }
    
    auto reset() -> void {
      
        queue.clear();

	}   
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        uint8_t vSize = queue.size();
        s.integer( vSize );
        
        if ( s.mode() == Emulator::Serializer::Mode::Load ) {
            queue.clear();
            
            for(unsigned i = 0; i < vSize; i++) {
                Action action;
                
                serializeAction( s, action );
                
                if (action.callbackId == 1)
                    action.callback = []() { system->kernalBootComplete = true; };
                    
                else if (action.callbackId == 2)
                    action.callback = []() { prg->inject(); };
                    
                else if (action.callbackId == 3)
                    action.callback = []() { tape->setMode( Tape::Mode::Play ); };
                
                queue.push_back( action );
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
        
        if (queue.size() == 0)
            return;
        
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
                    queue.clear();
                    return;
                }
                
                if ((action.position & 7) != 0)
                    // don't check each frame
                    return;
                
                result = checkFor( action.buffer, action.blinkingCursor );                
                
                if ( result == Found::No ) {
                    
                    if (action.alternateBuffer.size() != 0) {
                        
                        if ( checkFor( action.alternateBuffer, false ) == Found::Yes ) {
                            return;
                        }
                    }
                    
                    // don't do any further actions                    
                    queue.clear();
                    return;
                    
                } else if ( result == Found::NotYet ) {
                    // wait some more time               
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

        for( int i = 0; i < buffer.size(); i++) {
            
            if (ram[adr + i] != (buffer[i] % 64) ) {
                if (ram[adr + i] != 32)
                    return Found::No;
                
                return Found::NotYet;
            }
        }

        return Found::Yes;
    }
           
};       
    
}
