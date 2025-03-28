#pragma once

namespace GUIKIT {

struct Lha {
       
    struct File {
        std::string name;
        std::string date;
        int id;
        unsigned size;
        unsigned csize;
        unsigned cmode;
        unsigned crc32;
        uint8_t* data = nullptr; //uncompressed data
        bool isDirectory;
    };

    std::vector<File> files;
    FILE* fp;

    auto open(FILE* fp, unsigned size) -> bool;
    auto extract(File& file) -> uint8_t*;

    auto hasPath(std::vector<std::string>& dirPaths, std::string& path) -> bool;
};

}
