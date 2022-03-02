
#pragma once

#ifdef _MSC_VER
    #define __unreachable __assume(false);
    #define likely(x)     (x)
    #define unlikely(x)   (x)
#else
    #define __unreachable   __builtin_unreachable();
    #define likely(x)	    __builtin_expect(!!(x), 1)
    #define unlikely(x)     __builtin_expect(!!(x), 0)
#endif
