
// fpaq0 - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt

#include <algorithm>
#include "fpaq0.h"

namespace Emulator {

    PredictorEightBitWithPrefix::PredictorEightBitWithPrefix() {
        // custom predictor for P64
        modelSize = 65536;

        model = new uint16_t[modelSize];

        ctxLimit = 256;

        init();
    }

    PredictorNineBit::PredictorNineBit() {
        modelSize = 512;

        model = new uint16_t[modelSize];

        ctxLimit = 512;

        init();
    }

    PredictorOneBit::PredictorOneBit() {
        modelSize = 2;

        model = new uint16_t[modelSize];

        ctxLimit = 0;

        init(0 );
    }

    Predictor::~Predictor() {
        delete[] model;
    }

    auto Predictor::init(unsigned _ctx) -> void {

        std::fill_n(model, modelSize, 2048);

        prefix = 0;

        ctx = _ctx;
    }

    auto Predictor::p() -> uint16_t const {

        return *(model + (ctx | (prefix << 8)));
    }

    auto Predictor::update(bool bit) -> void {

        uint16_t* ptr = model + (ctx | (prefix << 8));

        if (bit)
            *ptr += (0xfff - *ptr) >> 4;
        else
            *ptr -= *ptr >> 4;

        if (!ctxLimit)
            ctx = bit;
        else {
            if ((ctx += ctx + bit) >= ctxLimit)
                ctx = 1;
        }
    }


    Fpaq0::Fpaq0() {
        buffer = new uint8_t[ FPAQ0_BUFFER_SIZE ];

        writeOut = [](uint8_t* buffer, unsigned length) {};
        readIn = [](uint8_t*& buffer) { return 0; };

        init();
    }

    Fpaq0::~Fpaq0() {
        delete[] buffer;
    }

    auto Fpaq0::init() -> void {
        bufferPos = 0;
        x1 = 0;
        x2 = ~0;
        x = 0;
    }

    auto Fpaq0::encode(Predictor* predictor, bool bit) -> void {

        uint32_t xmid = x1 + ((x2 - x1) >> 12) * (uint32_t)predictor->p();

        if (bit)
            x2 = xmid;
        else
            x1 = xmid + 1;

        predictor->update(bit);

        while ((( x1 ^ x2) & 0xff000000) == 0) {

            store( x2 >> 24 );

            x1 <<= 8;
            x2 = (x2 << 8) + 255;
        }
    }

    auto Fpaq0::decode(Predictor* predictor) -> bool {

        uint32_t xmid = x1 + ((x2 - x1) >> 12) * (uint32_t)predictor->p();

        bool bit = false;

        if ( x <= xmid ) {
            bit = true;
            x2 = xmid;

        } else
            x1 = xmid + 1;

        predictor->update(bit);

        while ((( x1 ^ x2) & 0xff000000) == 0) {
            x1 <<= 8;
            x2 = (x2 << 8) + 255;
            x = (x << 8) + read();
        }

        return bit;
    }

    auto Fpaq0::store(uint8_t byte ) -> void {

        buffer[bufferPos++] = byte;

        if (bufferPos == FPAQ0_BUFFER_SIZE) {
            writeOut( buffer, FPAQ0_BUFFER_SIZE );
            bufferPos = 0;
        }
    }

    auto Fpaq0::flush( ) -> void {

        for( uint8_t i = 0; i < 4; i++ ) {
            store( x2 >> 24 );
            x2 <<= 8;
        }

        if ( bufferPos )
            writeOut( buffer, bufferPos );

        bufferPos = 0;
    }

    auto Fpaq0::warmUp( ) -> void {

        for( uint8_t i = 0; i < 4; i++ ) {
            x = (x << 8) | read();
        }
    }

    auto Fpaq0::read() -> uint8_t {

        if (!bufferPos || (bufferPos == bufferSize)) {
            bufferSize = readIn( readBuffer );

            if (!bufferSize)
                return 0;

            bufferPos = 0;
        }

        return readBuffer[bufferPos++];
    }



}