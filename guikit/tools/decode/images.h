
#pragma once

struct ImageDecoder {

    uint8_t* data = nullptr;
    int width;
    int height;

    auto decode(const uint8_t* src, unsigned size) -> bool;
};

