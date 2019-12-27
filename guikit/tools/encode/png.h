
#include "../crc32.h"

namespace ENCODE {

struct Png {
    static constexpr uint16_t DEFLATE_MAX_BLOCK_SIZE = 65535;
    
    uint32_t adler;
    unsigned fileSize = 0;

    auto generate( uint8_t* rgbData, unsigned width, unsigned height ) -> uint8_t* {

        uint8_t* out = nullptr;
        CRC32 crc32;
        crc32.init(~0);
        adler = 1;

        if (!rgbData || width == 0 || height == 0)
            return nullptr;

        unsigned lineSize = width * 3 + 1;
        unsigned size = lineSize * height;

        unsigned blocks = size / DEFLATE_MAX_BLOCK_SIZE;

        if ( (size % DEFLATE_MAX_BLOCK_SIZE) != 0)
            blocks++;

        fileSize = size + (blocks * 5) + 43 + 20;
        
        out = new uint8_t[fileSize];
        uint8_t* ptr = out;

        uint8_t header[] = {  // PNG header 43 bytes
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
            0x00, 0x00, 0x00, 0x0D,
            0x49, 0x48, 0x44, 0x52,
            0, 0, 0, 0,  // width
            0, 0, 0, 0,  // height
            0x08, 0x02, 0x00, 0x00, 0x00,
            0, 0, 0, 0,
            0, 0, 0, 0,  // size
            0x49, 0x44, 0x41, 0x54,
            0x08, 0x1D,
        };
        toBuffer( &header[16], width );
        toBuffer( &header[20], height );
        toBuffer( &header[33], size + (blocks * 5) + 6 );
        crc32.calc( &header[12], 17 );
        toBuffer( &header[29], crc32.value() );

        crc32.init(~0);
        crc32.calc( &header[37], 6 );

        std::memcpy( ptr, header, sizeof(header) );
        ptr += sizeof(header);

        unsigned x = 0;
        unsigned y = 0;
        unsigned blockPos = 0;
        unsigned todo = width * height * 3;
        unsigned remain = size;

        while (todo) {

            if (blockPos == 0) {  // new block
                unsigned blockSize = DEFLATE_MAX_BLOCK_SIZE;

                if (remain < blockSize)
                    blockSize = remain;

                uint8_t bheader[] = {  // 5 bytes long
                    (uint8_t)((remain <= DEFLATE_MAX_BLOCK_SIZE) ? 1 : 0),
                    (uint8_t)((blockSize >> 0) & 0xff),
                    (uint8_t)((blockSize >> 8) & 0xff),
                    (uint8_t)((blockSize >> 0) ^ 0xff),
                    (uint8_t)((blockSize >> 8) ^ 0xff),
                };

                std::memcpy( ptr, bheader, sizeof(bheader) );
                ptr += sizeof(bheader);

                crc32.calc( &bheader[0], sizeof(bheader) );
            }

            if (x == 0) {
                *ptr = 0;
                crc32.calc(ptr, 1);
                adler32(ptr, 1);
                ptr++;
                x++;
                blockPos++;
                remain--;

            } else {
                unsigned chunkSize = DEFLATE_MAX_BLOCK_SIZE - blockPos;

                if ((lineSize - x) < chunkSize)
                    chunkSize = lineSize - x;

                if (todo < chunkSize)
                    chunkSize = todo;

                std::memcpy(ptr, rgbData, chunkSize);

                crc32.calc( ptr, chunkSize );
                adler32( ptr, chunkSize );

                ptr += chunkSize;
                rgbData += chunkSize;
                todo -= chunkSize;
                x += chunkSize;
                blockPos += chunkSize;
                remain -= chunkSize;
            }

            if (blockPos == DEFLATE_MAX_BLOCK_SIZE) {
                blockPos = 0;
            }

            if (x == lineSize) {  // Increment line
                x = 0;
                y++;
            }
        }

        uint8_t footer[] = {  // 20 bytes long
            0, 0, 0, 0,
            0, 0, 0, 0,
            0x00, 0x00, 0x00, 0x00,
            0x49, 0x45, 0x4E, 0x44,
            0xAE, 0x42, 0x60, 0x82,
        };

        toBuffer(&footer[0], adler );
        crc32.calc(&footer[0], 4);
        toBuffer(&footer[4], crc32.value());

        std::memcpy( ptr, footer, sizeof(footer) );

        return out;
    }

    auto toBuffer( uint8_t* ptr, uint32_t val ) -> void {
    	for (unsigned i = 0; i < 4; i++)
    	    *(ptr + i) = val >> ( (3 - i) << 3 );
    }

    auto adler32(const uint8_t* data, unsigned size ) -> void {
        uint32_t s1 = adler & 0xFFFF;
        uint32_t s2 = adler >> 16;

        for (unsigned i = 0; i < size; i++) {
            s1 = (s1 + data[i]) % 65521;
            s2 = (s2 + s1) % 65521;
        }
        adler = (s2 << 16) | s1;
    }

};

}
