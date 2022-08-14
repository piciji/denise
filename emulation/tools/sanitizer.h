
#pragma once

namespace Emulator {

    static auto powerOfTwo(unsigned value) -> unsigned {
        unsigned data = 1;
        if (value == 0)
            return 0;

        while (1) {
            if (data < value) {
                data <<= 1;
            } else if (data == value) {
                return value;
            } else {
                break;
            }
        }

        return data >> 1;
    }

}
