
#include "thread.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace Emulator {  
    
auto setThreadPriorityRealtime( std::thread& th ) -> void {

#if defined(_WIN32)

    std::thread::native_handle_type h = th.native_handle();
    SetPriorityClass( (HANDLE)h, REALTIME_PRIORITY_CLASS);
    SetThreadPriority( (HANDLE)h, THREAD_PRIORITY_TIME_CRITICAL);

#else
    sched_param sch_params;
    sch_params.sched_priority = 99;

    if (pthread_setschedparam( th.native_handle(), SCHED_RR, &sch_params)) {

    }

#endif
}

}
