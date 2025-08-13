
#pragma once

#include <lame/lame.h>
#include "base.h"

namespace AudioRecord {

    struct MP3Writer : BaseWriter {
        ~MP3Writer();

        lame_t lame;

        bool useFloat;

        uint8_t* mp3Buffer = nullptr;
        
        unsigned mp3BufferSize = 0;

        auto init(std::string& path, unsigned sampleRate, bool useFloat) -> bool;

        auto write(uint8_t* buf, unsigned size) -> void;

        auto finish() -> void;

        auto encode() -> int;
    };

}
