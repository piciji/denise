
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include <cstdint>

namespace GUIKIT {

#include "resize.h"

auto ImageResize::decode(const uint8_t* in, unsigned inWidth, unsigned inHeight, uint8_t* out, unsigned outWidth, unsigned outHeight) -> bool {

    auto res = stbir_resize_uint8_srgb(
        in,
        static_cast<int>(inWidth),
        static_cast<int>(inHeight),
        0,
        out,
        static_cast<int>(outWidth),
        static_cast<int>(outHeight),
        0,
        STBIR_RGBA
    );

    return res == out;
}

}
