
#pragma once

struct ImageEncoder {

    enum class Type { PNG, JPG, TGA, BMP };

    struct Chunk {
        uint8_t* data;
        unsigned size;
    };

    std::vector<Chunk> chunks;
    
    uint8_t* data = nullptr;
    int size = 0;    

    auto encode(Type type, const uint8_t* src, unsigned width, unsigned height) -> bool;

    auto encodeBMPWithColorTable(std::vector<uint32_t>& colorTable, const uint8_t* src, unsigned width, unsigned height) -> bool;

    auto mergeChunks() -> void;

    static auto func(void* context, void* data, int len) -> void;

    template<typename T> static auto copyIntToBuffer(uint8_t* buf, T value) -> void;
};

