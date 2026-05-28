
#pragma once

struct ImageResize {

    static auto decode(const uint8_t* in, unsigned inWidth, unsigned inHeight, uint8_t* out, unsigned outWidth, unsigned outHeight) -> bool;
};
