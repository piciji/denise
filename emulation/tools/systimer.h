
#pragma once

#include <vector>
#include <algorithm>
#include <functional>

#include "serializer.h"
#include "buffer.h"

namespace Emulator {
    
struct Event {
	
	using Callback = std::function<void()>;
	
	Callback* callback;
	
	unsigned clock;
	
	bool remove;
};

struct SystemTimer {

	using Callback = std::function<void()>;
	
	enum Action { Normal = 0, BeforeOthers = 1, UpdateExisting = 2, WhenNotExistsOnly = 4 };
	
	unsigned clock = ~0;
	
	std::vector<Event> events;

	struct RegisteredCallback {
		Callback* callback;

		unsigned maxUsage;
	};

	std::vector<RegisteredCallback> registeredCallbacks;

	auto registerCallback(std::vector<RegisteredCallback> regCallbacks) -> void {

		for (auto& r : regCallbacks)
			registerCallback(r);
	}

	auto registerCallback(RegisteredCallback regCallback) -> void {

		registeredCallbacks.push_back(regCallback);
	}
	
	auto process() -> void {
		static bool remove;
		
		clock++;	
		
		size_t _size = events.size();

		remove = false;

        for( unsigned i = 0; i < _size; i++ ) {
			
            Event& event = events[i];

			if (clock == event.clock) {

				event.remove = true;
				
				(*event.callback)();				

				remove = true;
			}
		}

		if (remove) {
			events.erase(std::remove_if(events.begin(), events.end(), [] (Event & e) {
				return e.remove;
			}), events.end());
		}
	}
	
	auto add( Callback* callback, unsigned delay, Action action = Normal ) -> void {
		
		if (action & UpdateExisting) {
			
			auto event = get( callback );
			
			if (event) {
				
				event->clock = clock + delay;
				
				event->remove = false;
				
				return;
			}					
		} else if ( action & Action::WhenNotExistsOnly ) {
			
			auto event = get( callback );
			
			if ( event )
				return;
		}
		
        if ( action & Action::BeforeOthers )            
            events.insert( events.begin(), { callback, clock + delay, false } ); 
        else
            events.push_back({ callback, clock + delay, false });
	}
	
	auto get(Callback* callback) -> Event* {
		
		for(auto& event : events) {
			if (event.callback == callback)
				return &event;
		}
		
		return nullptr;
	}
    
    inline auto has( Callback* callback ) -> bool {
        for( auto& event : events )
            if ( event.callback == callback )
                return true;
        
        return false;        
    }

    inline auto delay(Callback* callback) -> unsigned {
        for (auto& event : events)
            if (event.callback == callback)
                return (event.clock - clock) & 0xffffffff;

        return 0;
    }
    
    auto remove( Callback* callback ) -> void {

        events.erase( std::remove_if (
            events.begin(), events.end(),
                
                [callback](Event& e) { 
                    return e.callback == callback;
                }), events.end());
    } 
    
    auto clear() -> void {
        clock = ~0;
		events.clear();
	}

	auto fallBackCycles( unsigned _last ) -> unsigned {
		return (clock - _last) & 0xffffffff;
	}
	
    auto serialize( Serializer& s ) -> void {
        
        unsigned _size = events.size() * 6;        
        unsigned delay = 0;
        uint16_t id = 0;
        
		s.integer( clock );
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

            for (auto& event : events) {

                for (uint16_t i = 0; i < registeredCallbacks.size(); i++) {

                    Callback* callback = registeredCallbacks[i].callback;

                    if (callback == event.callback) {

                        copyIntToBuffer(ptr, i);

                        copyIntToBuffer(ptr + 2, event.clock);

                        ptr += 6;

                        break;
                    }
                }
            }
        }
        
        s.array( temp, _size );
        
        if (s.mode() == Emulator::Serializer::Mode::Load) {
            
            events.clear();
            
            for ( unsigned i = 0; i < (_size / 6); i++ ) {
            
                id = copyBufferToInt<uint16_t>( ptr );
                
                delay = copyBufferToInt<unsigned>( ptr + 2 );
                
                ptr += 6;           
                                
                events.push_back( { registeredCallbacks[id].callback, delay, false } );
            }
        }
        
        delete temp;
    }	
};

}
