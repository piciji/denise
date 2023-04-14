#pragma once

#include <cstdint>
#include <cstring>

template<typename A, typename V>
struct MemChangeTracker {

    struct MemChange {
        A address;
        V value;
    };

    MemChange* memChange;

    bool armed;
    unsigned pos;
    unsigned size;

    MemChangeTracker() {
        size = 10 * 1024;
        memChange = new MemChange[size];
        pos = 0;
        armed = false;
    }

    ~MemChangeTracker() {
        delete[] memChange;
    }

    auto enabled() -> bool const { return armed; }

    auto enable() -> void {
        pos = 0;
        armed = true;
    }

    auto disable() -> void {
        armed = false;
    }

    auto remember(A adr, uint8_t* src) -> void {
        MemChange* ptr = &memChange[pos++];

        ptr->address = adr;
        ptr->value = *(V*)(src + adr);

        if (pos == size)
            increase();
    }

    auto applyAndDisable(uint8_t* src) -> void {
        apply(src);
        disable();
    }

    auto apply(uint8_t* src) -> void {
        MemChange* ptr;
        if (!pos)
            return;

        for (int i = pos - 1; i >= 0; i--) {
            ptr = &memChange[i];
            *(V*)(src + ptr->address) = ptr->value;
        }
    }

    auto increase() -> void {
        MemChange* temp = new MemChange[size << 1];
        std::memcpy(temp, memChange, sizeof(MemChange) * size );
        size <<= 1;
        delete[] memChange;
        memChange = temp;
    }
};
