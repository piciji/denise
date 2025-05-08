
#include <string>
#include <vector>

namespace Emulator {

struct Error : std::exception {
    enum class Type {
        DRIVER_NO_SEGLIST,
        DRIVER_NOT_FOUND,
        DRIVER_WRONG_HUNK_COUNT,
        
        HUNK_SEGMENT_INITIALIZATION_FAIL,
        HUNK_BAD_RELOC_TARGET,
        HUNK_BAD_HEADER,
        HUNK_UNEXPECTED_END,
        HUNK_BAD_SIZE,
        HUNK_BAD_RELOC,
        HUNK_UNIMPLEMENTED,

        HDD_BAD_FILE_OFFSET,
    };

    std::string descr;

    Error(Type t, const std::string& add = "");

    Error(Type t, int info);

    auto buildMessage(Type t, const std::string& info) -> void;

    auto what() const noexcept -> const char* { return descr.c_str(); }
};

#define inform(msg, ...) fprintf(stderr, msg "\n", ##__VA_ARGS__)

#define warn(msg, ...) fprintf(stderr, "Warning: " msg "\n", ##__VA_ARGS__)

#define error(msg, ...) fprintf(stderr, "Error: " msg "\n", ##__VA_ARGS__)


}
