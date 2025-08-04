
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "cgif.h"

#include <cstdint>
#include <vector>
#include <cstring>

namespace GUIKIT {

#include "images.h"

    ImageEncoder::~ImageEncoder() {
        for (auto& chunk : chunks) {
            if (chunk.data)
                delete[] chunk.data;
        }
    }

    auto ImageEncoder::encode(Type type, const uint8_t* src, unsigned width, unsigned height) -> bool {
        size = 0;
        usedType = type;

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
            case Type::GIF:
                if (encodeGIF(src, width, height))
                    mergeChunks();                    
                else {
                    usedType = Type::PNG;
                    data = stbi_write_png_to_mem((const unsigned char*)src, width * 3, width, height, 3, &size);                    
                }
                break;
        }

        return data != nullptr;
    }

    auto ImageEncoder::encodeWithColorTable(Type type, std::vector<uint32_t>& colorTable, const uint8_t* src, const uint8_t* src2, unsigned width, unsigned height) -> bool {
        size = 0;
        usedType = type;
        
        switch (type) {
            case Type::BMP:
                encodeBMPWithColorTable(colorTable, src, width, height);
                break;
            case Type::GIF:
                encodeGIFWithColorTable(colorTable, src, src2, width, height);
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
        if (!size)
            return;

        data = new uint8_t[size];

        unsigned offset = 0;
        for(auto& chunk : chunks) {
            std::memcpy(data + offset, chunk.data, chunk.size);
            offset += chunk.size;
            delete[] chunk.data;
        }

        chunks.clear();
    }

    template<typename T> auto ImageEncoder::copyIntToBuffer(uint8_t* buf, T value) -> void {

        for (unsigned i = 0; i < sizeof(T); i++) {

            buf[i] = value & 0xff;

            value >>= 8;
        }
    }

    auto ImageEncoder::encodeBMPWithColorTable(std::vector<uint32_t>& colorTable, const uint8_t* src, unsigned width, unsigned height) -> bool {
        unsigned tableEntries = colorTable.size();
        if ((tableEntries > 256) || !height || !width)
            return false;

        uint16_t depth = tableEntries > 16 ? 8 : 4;
        unsigned maxEntriesPerDepth = depth == 8 ? 256 : 16;
        unsigned rowSize = depth == 8 ? width : ( (width & 1) ? ((width >> 1) + 1) : (width >> 1));

        while ((rowSize & 3) != 0) // 4 byte allignment
            rowSize++;

        size = 14 + 40 + maxEntriesPerDepth * 4 + (rowSize * height);
        data = new uint8_t[size];
        std::memset(data, 0, size);

        uint8_t* ptr = data;
        *ptr++ = 'B';
        *ptr++ = 'M';
        copyIntToBuffer<uint32_t>(ptr, (unsigned)size);
        ptr += 8;
        copyIntToBuffer<uint32_t>(ptr, 14 + 40 + maxEntriesPerDepth * 4);
        ptr += 4;

        copyIntToBuffer<uint32_t>(ptr, 40);
        ptr += 4;
        copyIntToBuffer<uint32_t>(ptr, width);
        ptr += 4;
        copyIntToBuffer<uint32_t>(ptr, height);
        ptr += 4;
        copyIntToBuffer<uint16_t>(ptr, 1);
        ptr += 2;
        copyIntToBuffer<uint16_t>(ptr, depth);
        ptr += 2;
        copyIntToBuffer<uint32_t>(ptr, 0);
        ptr += 4;
        copyIntToBuffer<uint32_t>(ptr, 0);
        ptr += 4;
        copyIntToBuffer<uint32_t>(ptr, 0x0b13);
        ptr += 4;
        copyIntToBuffer<uint32_t>(ptr, 0x0b13);
        ptr += 4;
        copyIntToBuffer<uint32_t>(ptr, tableEntries);
        ptr += 4;
        copyIntToBuffer<uint32_t>(ptr, tableEntries);
        ptr += 4;

        uint8_t* tablePtr = ptr;
        for(auto& col : colorTable) {
            copyIntToBuffer<uint32_t>(tablePtr, col);
            tablePtr += 4;
        }
        ptr += maxEntriesPerDepth * 4;

        if (depth == 8) {
            for (int y = height - 1; y >= 0; y--) {
                for (unsigned x = 0; x < width; x++)
                    *ptr++ = *(src + y * width + x);
                
                ptr += rowSize - width;
            }
        } else {
            for (int y = height - 1; y >= 0; y--) {
                uint8_t* _p = ptr;

                for (unsigned x = 0; x < width; x++) {
                    if (x & 1) {
                        *_p++ |= *(src + y * width + x) & 0xf;
                    } else {
                        *_p = *(src + y * width + x) << 4;
                    }
                }
                ptr += rowSize;
            }
        }

        return true;
    }

    static auto writeGIF(void* pContext, const uint8_t* pData, const size_t numBytes) -> int {
        ImageEncoder::func(pContext, (void*)pData, numBytes);

        return 0;
    }

    auto ImageEncoder::encodeGIF(const uint8_t* src, unsigned width, unsigned height) -> bool {
        std::vector<uint32_t> colorTable;
        colorTable.reserve(256);
        
        unsigned _size = width * height;
        uint8_t* _ptr = (uint8_t*)src;
        unsigned _col;
        bool match;
        int colRef;
        uint8_t* imgSrc = new uint8_t[_size];
        uint8_t* _pImg = imgSrc;

        for (int i = 0; i < _size; i++) {
            _col = (*_ptr++ << 16) | (*_ptr++ << 8) | *_ptr++;
            match = false;

            for (colRef = 0; colRef < colorTable.size(); colRef++) {
                if (colorTable[colRef] == _col) {
                    match = true;
                    *_pImg++ = colRef;
                    break;
                }
            }

            if (match)
                continue;

            colorTable.push_back(_col);
            colRef = colorTable.size() - 1;

            if (colRef >= 256) {
                delete[] imgSrc;
                return false;
            }
            *_pImg++ = colRef;
        }

        CGIF* pGIF;
        CGIF_Config gConfig;
        CGIF_FrameConfig fConfig;

        uint16_t numColors = colorTable.size();
        uint8_t* aPalette = new uint8_t[numColors * 3];

        uint8_t* ptr = aPalette;
        for (auto& col : colorTable) {
            *ptr++ = (col >> 16) & 0xff;
            *ptr++ = (col >> 8) & 0xff;
            *ptr++ = (col >> 0) & 0xff;
        }

        std::memset(&gConfig, 0, sizeof(CGIF_Config));
        gConfig.width = width;
        gConfig.height = height;
        gConfig.pGlobalPalette = aPalette;
        gConfig.numGlobalPaletteEntries = numColors;
        gConfig.path = nullptr;
        gConfig.pWriteFn = writeGIF;
        gConfig.pContext = this;

        pGIF = cgif_newgif(&gConfig);
        delete[] aPalette;

        std::memset(&fConfig, 0, sizeof(CGIF_FrameConfig));
        fConfig.pImageData = imgSrc;

        cgif_addframe(pGIF, &fConfig);
        cgif_close(pGIF);
        delete[] imgSrc;

        return true;
    }

    auto ImageEncoder::encodeGIFWithColorTable(std::vector<uint32_t>& colorTable, const uint8_t* src, const uint8_t* src2, unsigned width, unsigned height) -> bool {
        CGIF* pGIF;
        CGIF_Config gConfig;
        CGIF_FrameConfig fConfig;
        
        uint16_t numColors = colorTable.size();
        uint8_t* aPalette = new uint8_t[numColors * 3];
        
        uint8_t* ptr = aPalette;
        for (auto& col : colorTable) {
            *ptr++ = (col >> 16) & 0xff;
            *ptr++ = (col >> 8) & 0xff;
            *ptr++ = (col >> 0) & 0xff;
        }

        std::memset(&gConfig, 0, sizeof(CGIF_Config));
        gConfig.width = width;
        gConfig.height = height;
        gConfig.pGlobalPalette = aPalette;
        gConfig.numGlobalPaletteEntries = numColors;
        gConfig.path = nullptr;
        gConfig.pWriteFn = writeGIF;
        gConfig.pContext = this;
        if (src2)
            gConfig.attrFlags = CGIF_ATTR_IS_ANIMATED;

        pGIF = cgif_newgif(&gConfig);
        delete[] aPalette;

        std::memset(&fConfig, 0, sizeof(CGIF_FrameConfig));
        fConfig.pImageData = (uint8_t*)src;
        if (src2)
            fConfig.delay = 2; // 0.02 => 50 FPS
        
        cgif_addframe(pGIF, &fConfig);

        if (src2) {
            fConfig.pImageData = (uint8_t*)src2;
            fConfig.delay = 2;
            cgif_addframe(pGIF, &fConfig);
        }

        cgif_close(pGIF);       
        mergeChunks();

        return true;
    }
}
