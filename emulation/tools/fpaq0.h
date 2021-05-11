
#pragma once

#include <cstring>
#include <functional>

#define FPAQ0_BUFFER_SIZE (100 * 1024)

namespace Emulator {

    struct Predictor {

        ~Predictor();

        uint16_t* model;
        unsigned modelSize;
        uint8_t prefix;
        unsigned ctx;
        unsigned ctxLimit;

        auto init() -> void;

        auto p() -> uint32_t const;

        auto update(bool bit) -> void;
    };

    struct PredictorEightBitWithPrefix : Predictor {
        PredictorEightBitWithPrefix();
    };

    struct PredictorNineBit : Predictor {
        PredictorNineBit();
    };

    struct PredictorOneBit : Predictor {
        PredictorOneBit();
    };

    struct Fpaq0 {

        Fpaq0();
        virtual ~Fpaq0();

        uint32_t x1, x2;
        uint32_t x;
        uint8_t* buffer;
        uint8_t* readBuffer;
        unsigned bufferPos = 0;
        unsigned bufferSize;

        std::function<void (uint8_t* buffer, unsigned length)> writeOut;

        std::function<unsigned (uint8_t*& buffer)> readIn;

        auto encode(Predictor* predictor, bool bit) -> void;

        auto decode(Predictor* predictor) -> bool;

        auto store( uint8_t byte ) -> void;

        auto flush( ) -> void;

        auto warmUp( ) -> void;

        auto read() -> uint8_t;

        auto init() -> void;
    };

}