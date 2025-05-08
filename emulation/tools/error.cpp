#include "error.h"

namespace Emulator {

    Error::Error(Type t, const std::string& info) {
        buildMessage(t, info);
    }

    Error::Error(Type t, int info) {
        buildMessage(t, std::to_string(info));
    }

    auto Error::buildMessage(Type t, const std::string& add) -> void {
        switch (t) {
            case Type::DRIVER_NO_SEGLIST:
                descr = "driver has no seg list";
                break;
            case Type::DRIVER_NOT_FOUND:
                descr = "invalid driver " + add;
                break;
            case Type::DRIVER_WRONG_HUNK_COUNT:
                descr = "driver has " + add + " hunks, 3 expected";
                break;
            case Type::HUNK_SEGMENT_INITIALIZATION_FAIL:
                descr = "hunk segment could not be initialized";
                break;
            case Type::HUNK_BAD_RELOC_TARGET:
                descr = "hunk relocation target " + add + " is outside of hunk list";
                break;
            case Type::HUNK_BAD_HEADER:
                descr = "hunk has bad header";
                break;
            case Type::HUNK_UNEXPECTED_END:
                descr = "access outside of hunk data";
                break;
            case Type::HUNK_BAD_SIZE:
                descr = "section exceeds hunk size";
                break;
            case Type::HUNK_BAD_RELOC:
                descr = "points to non existing hunk";
                break;
            case Type::HUNK_UNIMPLEMENTED:
                descr = "unimplemented hunk " + add;
                break;
            case Type::HDD_BAD_FILE_OFFSET:
                descr = "cant fetch from HDD at offset " + add;
                break;
            default:
                descr = "generic error";
                break;
        }
    }
}
