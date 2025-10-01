#pragma once

#include <cstdint>
#include <cstring>

#define TRACKER_SIZE_INC (512)

template<typename A, typename V>
struct MemChangeTracker {

    struct MemChange {
        A address;
        V value;
    };

    MemChange* memChange;

    unsigned pos;
    unsigned size;
    uint8_t* src;

    MemChangeTracker() {
        memChange = nullptr;
        src = nullptr;
        size = 0;
        initSize();
    }

    virtual ~MemChangeTracker() {
        delete[] memChange;
    }

    auto reset(uint8_t* _src) -> void {
        pos = 0;
        src = _src;
    }

    auto reset() -> void {
        pos = 0;
    }

    auto remember(A adr) -> void {
        MemChange* ptr = &memChange[pos++];

        ptr->address = adr;
        ptr->value = *(V*)(src + adr);

        if (pos == size)
            increase();
    }

    auto apply() -> void {
        MemChange* ptr;
        if (!pos)
            return;

        for (int i = pos - 1; i >= 0; i--) {
            ptr = &memChange[i];
            *(V*)(src + ptr->address) = ptr->value;
        }
        pos = 0;
    }

    auto increase() -> void {
        MemChange* temp = new MemChange[size << 1];
        std::memcpy(temp, memChange, sizeof(MemChange) * size );
        size <<= 1;
        delete[] memChange;
        memChange = temp;
    }

    auto initSize() -> void {
        if (size == TRACKER_SIZE_INC)
            return;
        if (memChange)
            delete[] memChange;
        size = TRACKER_SIZE_INC;
        memChange = new MemChange[size];
        pos = 0;
    }
};
