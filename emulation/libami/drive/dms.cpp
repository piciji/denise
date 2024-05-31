
#include "dms/dms.h"

namespace LIBAMI {

auto DiskStructure::analyzeDMS(uint8_t*& data, unsigned& size) -> bool {
    unsigned foSize = 0;
    unsigned char* fo = nullptr;

    auto result = AMI_DMS::Process_File(data, size, &fo, foSize);

    if (result == 0 || result == 1) {
        if (analyzeADF(fo, foSize)) {
            data = fo;
            size = foSize;
            virtualCreated = true;
            return true;
        }
    }
    if (fo)
        delete[] fo;

    return false;
}

}
