
#if defined(_WIN32)
    #include <objbase.h>
#elif defined( __APPLE__ )
    #include <CoreFoundation/CFUUID.h>
#else
    #include <uuid/uuid.h>
#endif

#include "uuid.h"
#include <cstring>

namespace Emulator {

    auto UUID::get() -> std::vector<uint8_t> {
        std::vector<uint8_t> res;
        res.resize(16);

#if defined(_WIN32)
    GUID guid;
    if (CoCreateGuid(&guid) != S_OK)
        return res;

    res[0] = guid.Data1 >> 24;
    res[1] = (uint8_t)(guid.Data1 >> 16);
    res[2] = (uint8_t)(guid.Data1 >> 8);
    res[3] = (uint8_t)(guid.Data1 >> 0);
    res[4] = guid.Data2 >> 8;
    res[5] = guid.Data2 >> 0;
    res[6] = guid.Data3 >> 8;
    res[7] = guid.Data3 >> 0;

    std::memcpy(res.data() + 8, guid.Data4, 8);

#elif defined( __APPLE__ )
        auto newId = CFUUIDCreate(NULL);
        auto bytes = CFUUIDGetUUIDBytes(newId);
        CFRelease(newId);

        res[0] = bytes.byte0; res[4] = bytes.byte4; res[8] = bytes.byte8; res[12] = bytes.byte12;
        res[1] = bytes.byte1; res[5] = bytes.byte5; res[9] = bytes.byte9; res[13] = bytes.byte13;
        res[2] = bytes.byte2; res[6] = bytes.byte6; res[10] = bytes.byte10; res[14] = bytes.byte14;
        res[3] = bytes.byte3; res[7] = bytes.byte7; res[11] = bytes.byte11; res[15] = bytes.byte15;
#else
        uuid_generate(res.data());
#endif

        return res;
    }

}
