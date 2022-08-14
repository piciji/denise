
#pragma once

#ifdef _MSC_VER
    #define _unreachable    __assume(false);
    #define likely(x)       (x)
    #define unlikely(x)     (x)
    #define _swapWord _byteswap_ushort
    #define _swapLong _byteswap_ulong
#else
    #define _unreachable    __builtin_unreachable();
    #define likely(x)	    __builtin_expect(!!(x), 1)
    #define unlikely(x)     __builtin_expect(!!(x), 0)
    #define _swapWord  __builtin_bswap16
    #define _swapLong  __builtin_bswap32
#endif
