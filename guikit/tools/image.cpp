
#include "decode/png.h"
#include "encode/png.h"

Image::Image(unsigned width, unsigned height, uint8_t* src, Format format)
: format(format) {
    create(width, height, src);
}

Image::~Image() {
    free();
}

Image::Image(const Image& source) {
    operator=(source);
}

Image::Image(Image&& source) {
    operator=(std::forward<Image>(source));
}

Image& Image::operator=(const Image& source) {
    if(this == &source) return *this;

    format = source.format;
    resourceId = source.resourceId;
    alphaBlendApplied = source.alphaBlendApplied;
    create(source.width, source.height, source.data);
    return *this;
}

Image& Image::operator=(Image&& source) {
    if(this == &source) return *this;

    free();
    format = source.format;
    width = source.width;
    height = source.height;
    data = source.data;
    resourceId = source.resourceId;
    alphaBlendApplied = source.alphaBlendApplied;
    source.data = nullptr;
    return *this;
}

auto Image::loadPng(const uint8_t* src, unsigned size) -> bool {
    Png png;
    if (!png.load(src, size)) return false;

    create(png.info.width, png.info.height);
    format = RGBA;

    const uint8_t* sp = png.data;
    uint8_t* dp = data;

    auto decode = [&]() -> uint64_t {
        uint64_t p, r, g, b, a;

        switch(png.info.colorType) {
            case 0:  //L
                r = g = b = png.readbits(sp);
                a = (1 << png.info.bitDepth) - 1;
                break;
            case 2:  //R,G,B
                r = png.readbits(sp);
                g = png.readbits(sp);
                b = png.readbits(sp);
                a = (1 << png.info.bitDepth) - 1;
                break;
            case 3:  //P
                p = png.readbits(sp);
                r = png.info.palette[p][0];
                g = png.info.palette[p][1];
                b = png.info.palette[p][2];
                a = (1 << png.info.bitDepth) - 1;
                break;
            case 4:  //L,A
                r = g = b = png.readbits(sp);
                a = png.readbits(sp);
                break;
            case 6:  //R,G,B,A
                r = png.readbits(sp);
                g = png.readbits(sp);
                b = png.readbits(sp);
                a = png.readbits(sp);
                break;
        }

        a = normalize(a, png.info.bitDepth, 8);
        r = normalize(r, png.info.bitDepth, 8);
        g = normalize(g, png.info.bitDepth, 8);
        b = normalize(b, png.info.bitDepth, 8);

        return (r << 0) | (g << 8) | (b << 16) | (a << 24);
    };

    for(unsigned y = 0; y < height; y++) {
        for(unsigned x = 0; x < width; x++) {
            write(dp, decode());
            dp += 4;
        }
    }

    return true;
}

auto Image::generatePng( uint8_t* rgbData, unsigned width, unsigned height, unsigned& pngSize ) -> uint8_t* {
    
    ENCODE::Png png;
    
    auto pngData = png.generate( rgbData, width, height );
    
    pngSize = png.fileSize;
    
    return pngData;
}

auto Image::empty() -> bool {
    return height == 0 || width == 0 || data == nullptr || Application::isQuit;
}

auto Image::switchBetweenBGRandRGB() -> void {
    uint8_t* dp;

    for(unsigned y = 0; y < height; y++) {
        dp = data + width * y * 4;

        for(unsigned x = 0; x < width; x++) {
            dp[0] ^= dp[2]; dp[2] ^= dp[0]; dp[0] ^= dp[2];
            dp += 4;
        }
    }
    if (format == RGBA) format = BGRA;
    else format = RGBA;
}

auto Image::alphaBlend(unsigned alphaColor) -> void {
    uint8_t alphaR = (alphaColor >> 16) & 0xff;
    uint8_t alphaG = (alphaColor >> 8) & 0xff;
    uint8_t alphaB = (alphaColor >> 0) & 0xff;
    uint8_t* dp;

    for(unsigned y = 0; y < height; y++) {
        dp = data + width * y * 4;

        for(unsigned x = 0; x < width; x++) {
            uint8_t colorA = dp[3];
            uint8_t colorR = dp[2];
            uint8_t colorG = dp[1];
            uint8_t colorB = dp[0];
            double alphaScale = (double)colorA / (double)(255);

            colorR = (colorR * alphaScale) + (alphaR * (1.0 - alphaScale));
            colorG = (colorG * alphaScale) + (alphaG * (1.0 - alphaScale));
            colorB = (colorB * alphaScale) + (alphaB * (1.0 - alphaScale));

            dp[0] = colorB; dp[1] = colorG; dp[2] = colorR; dp[3] = 255;
            dp += 4;
        }
    }
    
    alphaBlendApplied = true;
}

auto Image::alphaMultiply() -> void {
	unsigned divisor = (1 << 8) - 1;

	for(unsigned y = 0; y < height; y++) {
		uint8_t* dp = data + width * y * 4;
		
		for(unsigned x = 0; x < width; x++) {
			uint8_t colorA = dp[3];
			uint8_t colorR = dp[2];
			uint8_t colorG = dp[1];
			uint8_t colorB = dp[0];

			colorR = (colorR * colorA) / divisor;
			colorG = (colorG * colorA) / divisor;
			colorB = (colorB * colorA) / divisor;

			dp[0] = colorB; dp[1] = colorG; dp[2] = colorR; dp[3] = colorA;
			dp += 4;
		}
	}
}

auto Image::free() -> void {
    if(data) delete[] data;
    data = nullptr;
}

auto Image::create(unsigned _width, unsigned _height, uint8_t* src) -> void {
    free();
    width = _width;
    height = _height;
    unsigned size = _width * _height * 4;
    data = new uint8_t[size];
    if (src) memcpy(data, src, size);
}

auto Image::read(uint8_t* p) -> unsigned {
    unsigned result = 0;
    for(signed n = 3; n >= 0; n--) result = (result << 8) | p[n];
    return result;
}

auto Image::write(uint8_t* p, unsigned value) -> void {
    for(unsigned n = 0; n < 4; n++) {
        p[n] = value & 0xff;
        value >>= 8;
    }
}

auto Image::scaleNearest(unsigned outputWidth, unsigned outputHeight) -> void {
    if ( (outputWidth == width) && (outputHeight == height) ) return;
    uint8_t* outputData = new uint8_t[outputWidth * outputHeight * 4];

    uint64_t xstride = ((uint64_t)width  << 32) / outputWidth;
    uint64_t ystride = ((uint64_t)height << 32) / outputHeight;

    for(unsigned y = 0; y < outputHeight; y++) {
        uint64_t yfraction = ystride * y;
        uint64_t xfraction = 0;

        uint8_t* sp = data + (width * 4) * (yfraction >> 32);
        uint8_t* dp = outputData + (outputWidth * 4) * y;

        uint64_t a = read(sp);
        unsigned x = 0;

        while(true) {
            while(xfraction < 0x100000000 && x++ < outputWidth) {
                write(dp, a);
                dp += 4;
                xfraction += xstride;
            }
            if(x >= outputWidth) break;

            sp += 4;
            a = read(sp);
            xfraction -= 0x100000000;
        }
    }

    free();
    data = outputData;
    width = outputWidth;
    height = outputHeight;
}

auto Image::normalize(uint64_t color, unsigned sourceDepth, unsigned targetDepth) -> uint64_t {
    if(sourceDepth == 0 || targetDepth == 0) return 0;
    while(sourceDepth < targetDepth) {
        color = (color << sourceDepth) | color;
        sourceDepth += sourceDepth;
    }
    if(targetDepth < sourceDepth) color >>= (sourceDepth - targetDepth);
    return color;
}

auto Image::getCharDataStringFromBinary(std::string inFile, std::string outFile) -> bool {
    bool error = false;
    File fileIn( inFile );
    File fileOut( outFile );

    try {
        if (!fileOut.open(File::Mode::Write, true) ) throw "";
        if (!fileIn.open(File::Mode::Read) ) throw "";

        unsigned char* data = fileIn.read();
        if (data == nullptr) throw "";

        auto fp = fileOut.getHandle();
        std::string out = "";
        char hex[8];

        for(int i = 0; i < fileIn.getSize(); i++) {

            //sprintf( hex, "%2x", data[i] );
            //out += "0x" + (std::string)hex;
            out += std::to_string( data[i] );

            if ( (i+1) != fileIn.getSize()) {
                out += ", ";
            }
        }
        std::string elementCount = "size: " + std::to_string( fileIn.getSize() ) + "\n";
        fputs( elementCount.c_str(), fp );
        fputs( out.c_str(), fp );
    } catch(...) {
        error = true;
    }

    fileIn.unload();
    fileOut.unload();
    return !error;
}

auto Image::setResourceId( int rId ) -> void {
    
    resourceId = rId;
}