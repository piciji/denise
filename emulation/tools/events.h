
#pragma once

#include <functional>
#include "serializer.h"

namespace Emulator {

using EventCallback = std::function<void(uint8_t id)>;

template<uint8_t Slots>
struct Events {

    static EventCallback dummy;

    struct Event {

        EventCallback& callback = dummy;

        unsigned clock = 0;

        uint8_t id = 0;
    };

    Event eventStore[Slots];

    unsigned clock = 0;

    unsigned nextClock = 0;

    inline auto processEvents() -> void {
        if (++clock == nextClock) {
            unroll();
        }
    }

    template<uint8_t Slot>
    auto addEvent(EventCallback& callback) -> void {
        eventStore[Slot] = { callback, 0, 0 };
    }

    template<uint8_t Slot>
    auto updateEvent(uint8_t id, unsigned delay) -> void {
        delay += clock;

        Event& event = eventStore[Slot];
        event.id = id;
        event.clock = delay;

        if (delay < nextClock)
            nextClock = delay;
    }

    template<uint8_t Slot>
    auto hasActiveEvent() -> bool {
        Event& event = eventStore[Slot];
        return event.id != 0;
    }

    template<uint8_t Slot>
    auto getEventDelay() -> unsigned {
        Event& event = eventStore[Slot];
        return (event.clock - clock) & 0xffffffff;
    }

    template<uint8_t Slot>
    auto setEventInactive() -> void {
        Event& event = eventStore[Slot];
        event.id = 0;
    }

protected:
    auto serialize( Serializer& s ) -> void {
        s.integer(clock);
        s.integer( nextClock );

        for(unsigned slot = 0; slot < Slots; slot++) {
            Event& event = eventStore[slot];
            s.integer( event.clock );
            s.integer( event.id );
        }
    }

private:
    template<uint8_t Slot = Slots>
    inline auto unroll() -> void {
        Event& event = eventStore[Slot];

        if (event.id) {
            if (event.clock == clock) {
                (event.callback)(event.id);
                event.id = 0;
            } else {
                if (clock == nextClock)
                    nextClock = event.clock;
                else if (event.clock < nextClock)
                    nextClock = event.clock;
            }
        }

        if constexpr (Slot)
            unroll<Slot - 1>();
    }
};

template<uint8_t Slots>
EventCallback Events<Slots>::dummy = [](uint8_t){};

}
