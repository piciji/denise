
#include "analogControl.h"


namespace LIBAMI {

#define MOUSE_DELTA_LIMIT 126

struct Mouse : AnalogControl {

    Mouse( Emulator::Interface* interface, Emulator::Interface::Device* device ) : AnalogControl( interface, device ) {}

    auto poll( ) -> void {

        int16_t deltaX = interface->inputPoll( device->id, 0);
        int16_t deltaY = interface->inputPoll( device->id, 1);

        int _dx = std::abs(deltaX);
        int _dy = std::abs(deltaY);

        // limit movement
        if ( (_dx > _dy) && (_dx > MOUSE_DELTA_LIMIT)) {
            deltaY = (int)deltaY * MOUSE_DELTA_LIMIT / _dx;
            deltaX = (deltaX < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
        } else if (_dy > MOUSE_DELTA_LIMIT) {
            deltaX = (int)deltaX * MOUSE_DELTA_LIMIT / _dy;
            deltaY = (deltaY < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
        }

        posX += deltaX;
        posY += deltaY;
    }

    auto readButton1( ) -> uint8_t {

        return !interface->inputPoll( device->id, 2 );
    }

    auto getPotY() -> uint8_t {
        return interface->inputPoll( device->id, 3 ) ? 0 : 0xff;
    }

    auto readDirection( ) -> uint16_t {

        return (posY & 0xff) << 8 | (posX & 0xff);
    }


    auto reset() -> void {

        AnalogControl::reset();
    }

    auto serialize(Emulator::Serializer& s) -> void {

        AnalogControl::serialize( s );
    }
};

}
