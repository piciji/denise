
#pragma once

#include <functional>
#include "serializer.h"

namespace Emulator {

using EventCallback = std::function<void(uint8_t, uint16_t)>;

template<uint8_t Channels>
struct Events {

    struct Event {

        EventCallback* callback = nullptr;

        unsigned clock = 0;

        uint8_t job = 0;

        uint16_t data = 0;
    };

    Event eventStore[Channels];

    unsigned clock = 0;

    unsigned nextClock = 0;

    inline auto processEvents() -> void {
        if (++clock == nextClock) {
            unroll();
        }
    }

    template<uint8_t Channel>
    auto addEvent(EventCallback* callback) -> void {
        eventStore[Channel].callback = callback;
    }

    template<uint8_t Channel>
    auto updateEvent(uint8_t job, unsigned delay, uint16_t data = 0) -> void {
        delay += clock;

        Event& event = eventStore[Channel];
        event.job = job;
        event.clock = delay;
        event.data = data;

        if (delay < nextClock)
            nextClock = delay;
    }

    template<uint8_t Channel>
    auto updateEventAndExecuteExistingBefore(uint8_t job, unsigned delay, uint16_t data = 0) -> void {
        Event& event = eventStore[Channel];

        if (event.job)
            (*event.callback)(event.job, event.data);

        updateEvent<Channel>(job, delay, data);
    }

    template<uint8_t Channel>
    auto forceEvent() -> void {
        Event& event = eventStore[Channel];

        if (event.job) {
            (*event.callback)(event.job, event.data);
            event.job = 0;
        }
    }

    template<uint8_t Channel>
    auto getActiveEvent() -> uint8_t {
        Event& event = eventStore[Channel];
        return event.job;
    }

    template<uint8_t Channel>
    auto getEventDelay() -> unsigned {
        Event& event = eventStore[Channel];
        return (event.clock - clock) & 0xffffffff;
    }

    template<uint8_t Channel>
    auto setEventInactive() -> void {
        Event& event = eventStore[Channel];
        event.job = 0;
    }

    auto fallBackCycles( unsigned _last ) -> unsigned {
        return (clock - _last) & 0xffffffff;
    }

    auto clearEvents(std::vector<uint8_t> exceptions) -> void {
        for(uint8_t Channel = 0; Channel < Channels; Channel++) {
            if (std::find(exceptions.begin(), exceptions.end(), Channel) != exceptions.end())
                continue;

            Event& event = eventStore[Channel];
            event.job = 0;
        }
    }

protected:
    auto serialize( Serializer& s ) -> void {
        s.integer( clock );
        s.integer( nextClock );

        for(unsigned Channel = 0; Channel < Channels; Channel++) {
            Event& event = eventStore[Channel];
            s.integer( event.clock );
            s.integer( event.job );
        }
    }

private:
    template<uint8_t Channel = Channels>
    inline auto unroll() -> void {
        Event& event = eventStore[Channel];

        if (event.job) {
            if (event.clock == clock) {
                (*event.callback)(event.job, event.data);
                event.job = 0;
            } else {
                if (clock == nextClock)
                    nextClock = event.clock;
                else if (event.clock < nextClock)
                    nextClock = event.clock;
            }
        }

        if constexpr (Channel)
            unroll<Channel - 1>();
    }
};


}
