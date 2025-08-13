
#pragma once

#include <string>
#include <cstdint>
#include "../../../guikit/api.h"

namespace AudioRecord {

    struct BaseWriter {
        virtual ~BaseWriter() {
            if (sampleBuffer)
                delete[] sampleBuffer;
        }

        GUIKIT::File file;

        uint8_t* sampleBuffer = nullptr;

        unsigned bufferSize = 0;

        unsigned bufferPos = 0;

        virtual auto init(std::string& path, unsigned sampleRate, bool useFloat) -> bool { return false; }

        virtual auto write(uint8_t* buf, unsigned size) -> void {}

        virtual auto finish() -> void {}

        auto initSampleBuffer(unsigned size) -> void {
            if (!sampleBuffer) {
                sampleBuffer = new uint8_t[size];
                bufferSize = size;
            } else if (size != bufferSize) {
                delete[] sampleBuffer;
                sampleBuffer = new uint8_t[size];
                bufferSize = size;
            }
            bufferPos = 0;
        }
    };

}