
#pragma once
#include <cstdint>
#include <cstring>
#include "../../interface.h"

namespace LIBAMI {

struct Disk {
    Disk();

    enum Type { ADF, RAW } type;

    struct Track {
        unsigned offset;
        bool formatted;
    };

    uint8_t* rawData;
    unsigned rawSize;

    auto attach(uint8_t* data, unsigned size) -> bool;
    auto readRawHeader() -> void;
    auto readAdfHeader() -> void;

    static auto create( Type type, bool hd = false, std::string name = "", bool ffs = false ) -> Emulator::Interface::Data;
    static auto getImageSize(Type type, bool hd) -> unsigned;
    static auto getRawTrackSize(bool hd) -> unsigned;
    static auto writeRootblock (uint8_t* data, int blocksize, std::string name, bool hd) -> void;

    bool hd;
    uint8_t trackCount;
    Track tracks[ 2 * 83 ];
};

}

