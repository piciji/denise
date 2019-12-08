
#inline "../crc32.h"

namespace ENCODE {

struct Png {
    static constexpr uint16_t DEFLATE_MAX_BLOCK_SIZE = 65535;
    CRC32 crc32;
    uint32_t adler;

    auto generate( uint8_t* rgbData, unsigned width, unsigned height ) -> uint8_t* {

        uint8_t* out = nullptr;
        crc32.checksum = 0;
        adler = 1;

        if (!data || width == 0 || height == 0)
            return nullptr;

        unsigned lineSize = width * 3 + 1;
        unsigned size = lineSize * height;

        unsigned blocks = size / DEFLATE_MAX_BLOCK_SIZE;

        if ( (size % DEFLATE_MAX_BLOCK_SIZE) != 0)
            blocks++;

        out = new uint8_t[size + (blocks * 5) + 43 + 20];
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
        crc32( &header[12], 17 );
        toBuffer( &header[29], crc32.checksum );

        crc32.checksum = 0;
        crc32( &header[37], 6 );

        std::memcpy( ptr, header, sizeof(header) );
        ptr += sizeof(header);

        unsigned x = 0;
        unsigned y = 0;
        unsigned blockPos = 0;
        unsigned todo = width * height * 3;

        while (todo) {

            if (blockPos == 0) {  // new block
                unsigned blockSize = DEFLATE_MAX_BLOCK_SIZE;

                if (blocks == 1) // last block
                    blockSize = todo + 1;

                const uint8_t bheader[] = {  // 5 bytes long
                    (blocks == 1) ? 1 : 0,
                    (blockSize >> 0) & 0xff,
                    (blockSize >> 8) & 0xff,
                    (blockSize >> 0) ^ 0xFF,
                    (blockSize >> 8) ^ 0xFF,
                };

                std::memcpy( ptr, bheader, sizeof(bheader) );
                ptr += sizeof(bheader);

                crc32( &bheader[0], sizeof(bheader) );
            }

            if (x == 0) {  // Beginning of line - write filter method byte
                *ptr = 0;
                crc32(ptr, 1);
                adler32(ptr, 1);
                ptr++;
                x++;
                blockPos++;

            } else {
                unsigned chunkSize = DEFLATE_MAX_BLOCK_SIZE - blockPos;

                if ((lineSize - x) < chunkSize)
                    chunkSize = lineSize - x;

                if (todo < chunkSize)
                    chunkSize = todo;

                std::memcpy(ptr, rgbData, chunkSize);

                crc32( ptr, chunkSize );
                adler32( ptr, chunkSize );

                ptr += chunkSize;
                rgbData += chunkSize;
                todo -= chunkSize;
                x += chunkSize;
                blockPos += chunkSize;
            }

            if (blockPos == DEFLATE_MAX_BLOCK_SIZE) {
                blockPos = 0;
                blocks--;
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
        crc32(&footer[0], 4);
        toBuffer(&footer[4], crc32.checksum);

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
