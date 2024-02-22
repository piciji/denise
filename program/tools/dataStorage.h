
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

struct DataStorage {
    DataStorage(unsigned maxSize) {
        this->maxSize = maxSize;
        this->size = 0;
        stores.reserve(100);
    }

    ~DataStorage() {
        unload();
    }

    struct Store {
        uint8_t* data;
        unsigned size;
        std::string ident;
    };

    std::vector<Store> stores;

    unsigned size;

    unsigned maxSize;

    auto get(std::string ident, unsigned& size) -> uint8_t* {
        for(auto& s : stores) {
            if (s.ident == ident) {
                size = s.size;
                return s.data;
            }
        }
        return nullptr;
    }

    auto add(std::string ident, uint8_t* data, unsigned _size) -> void {
        size += _size;
        stores.push_back({ data, _size, ident});
    }

    auto cleanUp() -> void {
        while(stores.size()) {
            if(size < maxSize)
                break;

            auto& first = stores.front();
            delete[] first.data;
            size -= first.size;
            stores.erase(stores.begin());
        }
    }

    auto unload() -> void {
        for(auto& s : stores) {
            delete[] s.data;
        }
        stores.clear();
    }
};