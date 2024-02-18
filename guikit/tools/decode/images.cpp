
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <cstdint>
namespace GUIKIT {

#include "images.h"

    auto ImageDecoder::decode(const uint8_t* src, unsigned size) -> bool {
        int n;
        data = (uint8_t*) stbi_load_from_memory((const unsigned char*) src, size, &width, &height, &n, 4);

        return data != nullptr;
    }

}
