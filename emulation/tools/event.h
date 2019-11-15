
#pragma once

#include <vector>

#include <algorithm>
#include <functional>

#include "serializer.h"
#include "buffer.h"

namespace Emulator {
   
struct Events {

    using Callback = std::function<void ()>;
    
    // BeforeOthers: inserted before others, usefull if more events have same delay
    // UpdateExisting: update if exists, create if not exists
    // hint: options can be combined by |
    // no options means: event is added always after existing ones
    enum Action { Normal = 0, BeforeOthers = 1, UpdateExisting = 2, WhenNotExistsOnly = 4 };
    
    struct Event {
                                        
        Callback* callback;        
        
        unsigned delay;                
    };
    
    std::vector<Event> pipeline;
    
    struct RegisteredCallback {
        
        Callback* callback;
        
        unsigned maxUsage;
    };   
    
    std::vector<RegisteredCallback> registeredCallbacks;
    
    auto registerCallback( std::vector<RegisteredCallback> regCallbacks ) -> void {
        
        for( auto& r : regCallbacks )
            registerCallback( r );
    }
    
    auto registerCallback( RegisteredCallback regCallback ) -> void {
        
        registeredCallbacks.push_back( regCallback );
    }

    auto serialize( Serializer& s ) -> void {
        
        unsigned _size = pipeline.size() * 6;        
        unsigned delay = 0;
        uint16_t id = 0;
        
        s.integer( _size );

        if (s.mode() == Emulator::Serializer::Mode::Size) {
            
            for( auto& r : registeredCallbacks ) {
                    
                for( unsigned i = 0; i < r.maxUsage; i++ ) {
                    s.integer( delay );
                    s.integer( id );   
                }
            }
            
            return;
        }
        
        uint8_t* temp = new uint8_t[ _size ];
        
        uint8_t* ptr = temp;
        
        if (s.mode() == Emulator::Serializer::Mode::Save) {

            for (auto& event : pipeline) {

                for (uint16_t i = 0; i < registeredCallbacks.size(); i++) {

                    Callback* callback = registeredCallbacks[i].callback;

                    if (callback == event.callback) {

                        copyIntToBuffer(ptr, i);

                        copyIntToBuffer(ptr + 2, event.delay);

                        ptr += 6;

                        break;
                    }
                }
            }
        }
        
        s.array( temp, _size );
        
        if (s.mode() == Emulator::Serializer::Mode::Load) {
            
            pipeline.clear();
            
            for ( unsigned i = 0; i < (_size / 6); i++ ) {
            
                id = copyBufferToInt<uint16_t>( ptr );
                
                delay = copyBufferToInt<unsigned>( ptr + 2 );
                
                ptr += 6;           
                                
                pipeline.push_back( { registeredCallbacks[id].callback, delay } );
            }
        }
        
        delete temp;
    }
    
    auto add( Callback* callback, unsigned delay, Action action = Normal ) -> void {
    
        if ( action & Action::UpdateExisting ) {
            
            auto event = get( callback );
            
            if ( event ) {
                
                event->delay = delay;
                
                return;
            }
        } else if ( action & Action::WhenNotExistsOnly ) {
			
			auto event = get( callback );
			
			if ( event )
				return;
		}
                    
        if ( action & Action::BeforeOthers )            
            pipeline.insert( pipeline.begin(), { callback, delay } );            
            
        else            
            pipeline.push_back( { callback, delay } );
    }
    
    inline auto has( Callback* callback ) -> bool {
        for( auto& event : pipeline )
            if ( event.callback == callback )
                return true;
        
        return false;        
    }
    
    inline auto get( Callback* callback ) -> Event* {
        for( auto& event : pipeline )
            if ( event.callback == callback )
                return &event;
        
        return nullptr;        
    }

	inline auto delay( Callback* callback ) -> unsigned {
		for( auto& event : pipeline )
            if ( event.callback == callback )
				return event.delay;
		
		return 0;
	}
    
    inline auto process() -> void {
		
        size_t _size = pipeline.size();
        
        if (_size == 0)
            return;

        for( unsigned i = 0; i < _size; i++ ) {        
            Event& event = pipeline[i];
            
            if ( --event.delay == 0 ) {
				(*event.callback)();   
            }
        }
        
        remove( );
    }
    
    inline auto remove( ) -> void {
        
        pipeline.erase( std::remove_if (
            pipeline.begin(), pipeline.end(),
                
                [](Event& e) { 
                    return e.delay == 0;
                }), pipeline.end());
    }          
	
	auto remove( Callback* callback ) -> void {

        pipeline.erase( std::remove_if (
            pipeline.begin(), pipeline.end(),
                
                [callback](Event& e) { 
                    return e.callback == callback ;
                }), pipeline.end());
    } 
	
	auto clear() -> void {
		pipeline.clear();
	}
    
};

}
