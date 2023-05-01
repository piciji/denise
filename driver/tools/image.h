
#pragma once

#include <cstdint>
#include <cstring>
#include <algorithm>

struct Image {
    Image() {
        data = nullptr;
        scaledData = nullptr;
    }

    ~Image() {
        if (data)
            delete[] data;
        if (scaledData)
            delete[] scaledData;
    }

    auto empty() -> bool { return data == nullptr; }

    uint8_t* data = nullptr;
    unsigned width = 0;
    unsigned height = 0;

    uint8_t* scaledData = nullptr;
    int scaledWidth;
    int scaledHeight;

    auto setData(uint8_t* data, unsigned width, unsigned height, bool toogleRGB_BGR = false) -> void {
        this->width = width;
        this->height = height;

        if (this->data) {
            delete[] this->data;
            this->data = nullptr;
        }

        if (!data || !width || !height)
            return;

        this->data = new uint8_t[width * height * 4];

        if (toogleRGB_BGR) {
            for (int h = 0; h < height; h++) {
                uint32_t* dest = (uint32_t*)(data + h * width * 4);

                for (int w = 0; w < width; w++) {
                    uint32_t color = *dest;
                    *dest++ = (color & 0xff00ff00) | ((color >> 16) & 0xff) | ((color & 0xff) << 16);
                }
            }
        }

        std::memcpy(this->data, data, width * height * 4);
    }

    auto scale(int outWidth, int outHeight) -> void {
        if (scaledData && (scaledData != data) )
            delete[] scaledData;

        scaledData = nullptr;

        if (!outWidth || !outHeight)
            return;

        scaledWidth = width;
        scaledHeight = height;
        scaledData = data;

        if (outWidth == width && outHeight == height)
            return;

        if(outWidth == width )
            return scaleLinearHeight(outHeight);
        if(outHeight == height)
            return scaleLinearWidth(outWidth);

        int d1wh = (width  * outWidth ) + (outWidth * outHeight);
        int d1hw = (height * outHeight) + (outWidth * outHeight);
        int d2wh = (outWidth * outHeight) * 3;

        if(d1wh <= d1hw && d1wh <= d2wh)
            return scaleLinearWidth(outWidth), scaleLinearHeight(outHeight);
        if(d1hw <= d2wh)
            return scaleLinearHeight(outHeight), scaleLinearWidth(outWidth);

        scaleLinear(outWidth, outHeight);
    }

    auto scaleLinearWidth(unsigned outWidth) -> void {
        uint8_t* outData = allocate( outWidth, scaledHeight );
        uint64_t xstride = ((uint64_t)(scaledWidth - 1) << 32) / std::max(1u, outWidth - 1);

        for(int y = 0; y < scaledHeight; y++) {
            uint64_t xfraction = 0;

            uint8_t* sp = scaledData + scaledWidth * 4 * y;
            uint8_t* dp = outData + outWidth * 4 * y;

            uint64_t a = read(sp);
            uint64_t b = read(sp + 4);
            sp += 4;

            unsigned x = 0;
            while(true) {
                while(xfraction < 0x100000000 && x++ < outWidth) {
                    write(dp, interpolate4i(a, b, xfraction));
                    dp += 4;
                    xfraction += xstride;
                }
                if(x >= outWidth) break;

                sp += 4;
                a = b;
                b = read(sp);
                xfraction -= 0x100000000;
            }
        }

        if (scaledData && (scaledData != data) )
            delete[] scaledData;
        scaledData = outData;
        scaledWidth = outWidth;
    }

    inline auto scaleLinearHeight(unsigned outHeight) -> void {
        uint8_t* outData = allocate( scaledWidth, outHeight );
        uint64_t ystride = ((uint64_t)(scaledHeight - 1) << 32) / std::max(1u, outHeight - 1);

        for(int x = 0; x < scaledWidth; x++) {
            uint64_t yfraction = 0;

            uint8_t* sp = scaledData + 4 * x;
            uint8_t* dp = outData + 4 * x;

            uint64_t a = read(sp);
            uint64_t b = read(sp + scaledWidth * 4);
            sp += scaledWidth * 4;

            int y = 0;
            while(true) {
                while(yfraction < 0x100000000 && y++ < outHeight) {
                    write(dp, interpolate4i(a, b, yfraction));
                    dp += scaledWidth * 4;
                    yfraction += ystride;
                }
                if(y >= outHeight) break;

                sp += scaledWidth * 4;
                a = b;
                b = read(sp);
                yfraction -= 0x100000000;
            }
        }
        if (scaledData && (scaledData != data) )
            delete[] scaledData;
        scaledData = outData;
        scaledHeight = outHeight;
    }

    inline auto scaleLinear(unsigned outWidth, unsigned outHeight) -> void {
        uint8_t* outData = allocate( outWidth, outHeight );
        uint64_t xstride = ((uint64_t)(scaledWidth  - 1) << 32) / std::max(1u, outWidth  - 1);
        uint64_t ystride = ((uint64_t)(scaledHeight - 1) << 32) / std::max(1u, outHeight - 1);

        for(int y = 0; y < outHeight; y++) {
            uint64_t yfraction = ystride * y;
            uint64_t xfraction = 0;

            uint8_t* sp = scaledData + scaledWidth * 4 * (yfraction >> 32);
            uint8_t* dp = outData + outWidth * 4 * y;

            uint64_t a = read(sp);
            uint64_t b = read(sp + 4);
            uint64_t c = read(sp + scaledWidth * 4);
            uint64_t d = read(sp + scaledWidth * 4 + 4);
            sp += 4;

            int x = 0;
            while(true) {
                while(xfraction < 0x100000000 && x++ < outWidth) {
                    write(dp, interpolate4i(a, b, c, d, xfraction, yfraction));
                    dp += 4;
                    xfraction += xstride;
                }
                if(x >= outWidth) break;

                sp += 4;
                a = b;
                c = d;
                b = read(sp);
                d = read(sp + scaledWidth * 4);
                xfraction -= 0x100000000;
            }
        }
        if (scaledData && (scaledData != data) )
            delete[] scaledData;
        scaledData = outData;
        scaledWidth = outWidth;
        scaledHeight = outHeight;
    }

    auto allocate(unsigned w, unsigned h) -> uint8_t* {
        int padding = w * 4 + 4; // to prevent bounds checking
        int size = w * h * 4;
        uint8_t* newData = new uint8_t[size + padding];
        std::memset(newData + size, 0, padding);
        return newData;
    }

    auto read(uint8_t* p) -> unsigned {
        unsigned result = 0;
        for(signed n = 3; n >= 0; n--)
            result = (result << 8) | p[n];
        return result;
    }

    auto write(uint8_t* p, unsigned value) -> void {
        for(unsigned n = 0; n < 4; n++) {
            p[n] = value & 0xff;
            value >>= 8;
        }
    }

    inline auto isplit(uint64_t* c, uint64_t color) -> void {
        c[0] = (color & (255u << 24)) >> 24;
        c[1] = (color & (255u << 16)  ) >> 16;
        c[2] = (color & (255u << 8)) >> 8;
        c[3] = (color & (255u << 0) ) >> 0;
    }

    inline auto imerge(const uint64_t* c) -> uint64_t {
        return c[0] << 24 | c[1] << 16 | c[2] << 8 | c[3] << 0;
    }

    inline auto interpolate1i(int64_t a, int64_t b, uint32_t x) -> uint64_t {
        return a + (((b - a) * x) >> 32);  //a + (b - a) * x
    }

    inline auto interpolate1i(int64_t a, int64_t b, int64_t c, int64_t d, uint32_t x, uint32_t y) -> uint64_t {
        a = a + (((b - a) * x) >> 32);     //a + (b - a) * x
        c = c + (((d - c) * x) >> 32);     //c + (d - c) * x
        return a + (((c - a) * y) >> 32);  //a + (c - a) * y
    }

    inline auto interpolate4i(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint32_t x, uint32_t y) -> uint64_t {
        uint64_t o[4], pa[4], pb[4], pc[4], pd[4];
        isplit(pa, a), isplit(pb, b), isplit(pc, c), isplit(pd, d);
        for(int n = 0; n < 4; n++) o[n] = interpolate1i(pa[n], pb[n], pc[n], pd[n], x, y);
        return imerge(o);
    }

    inline auto interpolate4i(uint64_t a, uint64_t b, uint32_t x) -> uint64_t {
        uint64_t o[4], pa[4], pb[4];
        isplit(pa, a), isplit(pb, b);
        for(int n = 0; n < 4; n++) o[n] = interpolate1i(pa[n], pb[n], x);
        return imerge(o);
    }

};