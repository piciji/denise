
#pragma once
#include <string>

namespace LIBC64 {

    struct Clipboard {

        auto getText() -> std::string;
    };

}
