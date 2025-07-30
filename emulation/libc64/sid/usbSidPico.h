
#pragma once

#include <functional>
#include "../../tools/serializer.h"

namespace Emulator {
    struct SystemTimer;
}

namespace LIBC64 {

struct System;

using Callback = std::function<void ()>;

struct USBSIDPico {
    USBSIDPico(System& system);

    System& system;
    Emulator::SystemTimer& sysTimer;

    Callback flush;

    bool enabled = false;
    unsigned buffSize = 8192;
    unsigned diffSize = 64;

    unsigned lastClock = 0;
    unsigned rasterRate = 0;

    auto open() -> int;

    auto close() -> void;

    auto setBuffSize(unsigned value) -> void;

    auto setDiffSize(unsigned value) -> void;

    auto updateStereo() -> void;

    auto store(uint8_t addr, uint8_t val, int chipNr) -> void;

    auto serialize(Emulator::Serializer& s) -> void;

    auto setInitialState() -> void;

};

}
