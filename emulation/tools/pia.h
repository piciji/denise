
// MC6821

#pragma once

#include <functional>
#include "serializer.h"

namespace Emulator {

struct Pia {

    Pia();

    enum class Port : unsigned { A = 0, B = 1 };

    std::function<uint8_t ( Port port )> readPort;
    std::function<void ( Port port, uint8_t data )> writePort;

    std::function<void (bool state)> irqCall;

    // call only if transition occurs
    std::function<void (bool direction)> ca2Out;
    std::function<void (bool direction)> cb2Out;
    auto ca1In( bool direction ) -> void;   // input only
    auto cb1In( bool direction ) -> void;   // input only
    auto ca2In( bool direction ) -> void;
    auto cb2In( bool direction ) -> void;

    uint8_t cra;
    uint8_t crb;
    uint8_t dataA;
    uint8_t dataB;
    uint8_t ddrA;
    uint8_t ddrB;

    uint8_t ioa;
    uint8_t iob;

    bool ca2;
    bool cb2;

    auto read(uint8_t adr) -> uint8_t;
    auto write(uint8_t adr, uint8_t value) -> void;
    auto reset() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
};

}
