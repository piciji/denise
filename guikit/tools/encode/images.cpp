
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstdint>
#include <vector>
#include <cstring>

namespace GUIKIT {

#include "images.h"

    auto ImageEncoder::encode(Type type, const uint8_t* src, unsigned width, unsigned height) -> bool {
        size = 0;

        switch(type) {
            case Type::PNG:
                data = stbi_write_png_to_mem((const unsigned char*)src, width * 3, width, height, 3, &size);
                break;
            case Type::BMP:
                stbi_write_bmp_to_func(func, this, width, height, 3, (const void*)src);
                mergeChunks();
                break;
            case Type::JPG:
                stbi_write_jpg_to_func(func, this, width, height, 3, (const void*)src, 0);
                mergeChunks();
                break;
            case Type::TGA:
                stbi_write_tga_to_func(func, this, width, height, 3, (const void*)src);
                mergeChunks();
                break;
        }

        return data != nullptr;
    }

    auto ImageEncoder::func(void* context, void* data, int len) -> void {
        ImageEncoder* decoder = (ImageEncoder*)context;

        if (!len)
            return;

        Chunk chunk;
        chunk.size = len;
        chunk.data = new uint8_t[len];

        std::memcpy(chunk.data, (uint8_t*)data, len);

        decoder->chunks.push_back(chunk);
        decoder->size += len;
    }

    auto ImageEncoder::mergeChunks() -> void {
        data = new uint8_t[size];

        unsigned offset = 0;
        for(auto& chunk : chunks) {
            std::memcpy(data + offset, chunk.data, chunk.size);
            offset += chunk.size;
            delete[] chunk.data;
        }

        chunks.clear();
    }

}
