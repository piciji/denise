
#include "keyboard.h"
#include "../../../tools/serializer.h"


namespace LIBAMI {


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