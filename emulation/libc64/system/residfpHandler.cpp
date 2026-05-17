
#include "residfpHandler.h"

namespace LIBC64 {

ResidfpHandler::ResidfpHandler(unsigned nr, System* system, SidManager& sidManager, Type type) :
Sid( nr, system, sidManager, type ) {

}

auto ResidfpHandler::reset() -> void {

}

auto ResidfpHandler::setType( Type type ) -> void {

}

auto ResidfpHandler::setDigiBoost( bool state ) -> void {

}

auto ResidfpHandler::hasDigiBoost() -> bool {
    return false;
}

auto ResidfpHandler::readIO( uint8_t addr ) -> uint8_t {
    return 0;
}

auto ResidfpHandler::peekIO( uint8_t addr ) -> uint8_t {
    return 0;
}

auto ResidfpHandler::writeIO( uint8_t addr, uint8_t value ) -> void {

}

auto ResidfpHandler::clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int {
    return 0;
}

auto ResidfpHandler::clock(unsigned options) -> void {

}

auto ResidfpHandler::serialize(Emulator::Serializer& s, bool light) -> void {

}

auto ResidfpHandler::setIoMask(uint8_t pos) -> void {

}

auto ResidfpHandler::useLeftChannel(bool state) -> void {

}

auto ResidfpHandler::useRightChannel(bool state) -> void {

}

auto ResidfpHandler::volumeCorrection(bool state) -> void {

}

auto ResidfpHandler::enableFilter( bool state ) -> void {

}

auto ResidfpHandler::filterEnabled() -> bool {
    return false;
}

auto ResidfpHandler::adjustFilterBias6581(int value) -> void {

}

auto ResidfpHandler::getFilterBias6581() -> int {
    return 0;
}

auto ResidfpHandler::adjustFilterBias8580(int value) -> void {

}

auto ResidfpHandler::getFilterBias8580() -> int {
    return 0;
}

}
