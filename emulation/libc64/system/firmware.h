
#pragma once

#include <cstdint>
#include <cstring>
#include <algorithm>

namespace Firmware {

extern uint8_t basicRom[8192];

extern uint8_t charRom[4096];

extern uint8_t kernalRom[8192]; // rev3 default kernal

extern uint8_t kernalRomRev1[8192];

extern uint8_t kernalRomRev2[8192];

extern uint8_t drive1541IIRom[16384];

extern uint8_t drive1541Rom[16384];

extern uint8_t drive1541CRom[16384];

extern uint8_t drive1570Rom[32768];

extern uint8_t drive1571Rom[32768];

};
