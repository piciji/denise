
#pragma once

#include <cstdint>
#include <vector>

namespace Emulator {

struct UUID {

    auto get() -> std::vector<uint8_t>;
};

}
