
#pragma once

#include <string>
#include <exception>

struct Error : std::exception {

    std::string descr;

    Error(const std::string& str = "") {
        descr = str;
    }

    auto what() const noexcept -> const char* {        
        return descr.c_str();
    }
};

#define _inform(msg, ...) fprintf(stdout, msg "\n", ##__VA_ARGS__)

#define _warn(msg, ...) fprintf(stderr, "Warning: " msg "\n", ##__VA_ARGS__)

#define _error(msg, ...) fprintf(stderr, "Error: " msg "\n", ##__VA_ARGS__)
