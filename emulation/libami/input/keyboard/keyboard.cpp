
#include "keyboard.h"
#include "../../../tools/serializer.h"
#include "../../agnus/agnus.h"

namespace LIBAMI {

Keyboard::Keyboard(Emulator::Interface* interface, Agnus& agnus) : agnus(agnus) {
    this->interface = interface;

    callback = [this](uint8_t id) {


    };
}

auto Keyboard::poll() -> void {

}

auto Keyboard::reset() -> void {

}

auto Keyboard::setDevice( Emulator::Interface::Device* device ) -> void {

    if (!device->isKeyboard())
        return;

    this->device = device;
}

auto Keyboard::serialize( Emulator::Serializer& s ) -> void {


}

}
