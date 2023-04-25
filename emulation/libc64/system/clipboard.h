
#pragma once
#include <string>

namespace LIBC64 {

    struct System;

    struct Clipboard {

        Clipboard(System* system) : system(system) {}

        System* system;
        auto getText() -> std::string;
    };

}
