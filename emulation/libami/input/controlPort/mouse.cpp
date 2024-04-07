
#include "analogControl.h"


namespace LIBAMI {

#define MOUSE_DELTA_LIMIT (100)

struct Mouse : AnalogControl {

    Mouse( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device ) : AnalogControl( interface, input, device ) {}

    auto poll( ) -> void {

//        int16_t deltaX = interface->inputPoll( device->id, 0);
//        int16_t deltaY = interface->inputPoll( device->id, 1);
//
//        int _dx = std::abs(deltaX);
//        int _dy = std::abs(deltaY);
//
//        // limit movement
//        if ( (_dx > _dy) && (_dx > MOUSE_DELTA_LIMIT)) {
//            deltaY = (int)deltaY * MOUSE_DELTA_LIMIT / _dx;
//            deltaX = (deltaX < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
//        } else if (_dy > MOUSE_DELTA_LIMIT) {
//            deltaX = (int)deltaX * MOUSE_DELTA_LIMIT / _dy;
//            deltaY = (deltaY < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
//        }
//
//        posX += deltaX;
//        posY += deltaY;


        deltaX += interface->inputPoll( device->id, 0);
        deltaY += interface->inputPoll( device->id, 1);

//        if (_dx > 1 || _dy > 1)
//            if (input.sampling.mode != Input::SamplingMode::Static_Sampling)
//                input.setJitLock();
    }

    auto readButton1( ) -> uint8_t {
        return interface->inputPoll( device->id, 2 );
    }

    auto observePot(uint8_t& x, uint8_t& y) -> void {
        y = interface->inputPoll( device->id, 3 ) ? 0 : 0xff;
    }

    auto readDirection( ) -> uint16_t {
        if (deltaX != 0 || deltaY != 0) {
            int _dx = std::abs(deltaX);
            int _dy = std::abs(deltaY);

            // limit movement
            if ((_dx > _dy) && (_dx > MOUSE_DELTA_LIMIT)) {
                deltaY = (int) deltaY * MOUSE_DELTA_LIMIT / _dx;
                deltaX = (deltaX < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
            } else if (_dy > MOUSE_DELTA_LIMIT) {
                deltaX = (int) deltaX * MOUSE_DELTA_LIMIT / _dy;
                deltaY = (deltaY < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
            }

            posX += deltaX;
            posY += deltaY;

            deltaX = deltaY = 0;
        }

        return (posY & 0xff) << 8 | (posX & 0xff);
    }

    auto writeJoytest(uint16_t data) -> void {
        posX = (data & 0xfc) | (posX & 3);
        posY = ((data >> 8) & 0xfc) | (posY & 3);
    }

    auto reset() -> void {

        AnalogControl::reset();
    }

    auto serialize(Emulator::Serializer& s) -> void {

        AnalogControl::serialize( s );
    }
};

}
