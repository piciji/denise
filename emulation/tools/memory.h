#pragma once
#include <cstring>

namespace Emulator {

    template<typename T> static auto copyMemory(T*& target, unsigned& targetSize, T* src, unsigned srcSize) -> void {
        if (!src || !srcSize) {
            delete[] target;
            target = nullptr;
            targetSize = 0;
            return;
        }

        if (!target)
            target = new T[srcSize];
        else if (targetSize != srcSize) {
            delete[] target;
            target = new T[srcSize];
        }

        std::memcpy(target, src, srcSize * sizeof(T));
        targetSize = srcSize;
    }

}
