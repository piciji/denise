
#pragma once

#include <thread>

#if defined(_WIN32)
    #include <windows.h>
#elif defined( __APPLE__ )
    #include <pthread.h>
    #include <mach/mach.h>
    #include <mach/mach_time.h>
#else
    #include <pthread.h>
#endif

struct ThreadPriority {

    enum Mode { Normal, High, Realtime };

    static auto setPriority( ThreadPriority::Mode mode, float typicalProcessingTimeInMilliSeconds = 0, float maxProcessingTimeInMilliSeconds = 0 ) -> bool {

        if (!_setPriority( mode, typicalProcessingTimeInMilliSeconds, maxProcessingTimeInMilliSeconds )) {

            if ( mode == Mode::Realtime)
                return _setPriority( ThreadPriority::Mode::High );

            return false;
        }

        return true;
    }

#if defined(_WIN32)
    static auto _setPriority( ThreadPriority::Mode mode, float typicalProcessingTimeInMilliSeconds = 0, float maxProcessingTimeInMilliSeconds = 0 ) -> bool {

        int prio;
        switch(mode) {
            default:
            case Mode::Normal:
                prio = THREAD_PRIORITY_NORMAL;
                break;
            case Mode::High:
                prio = THREAD_PRIORITY_HIGHEST;
                break;
            case Mode::Realtime:
                prio = THREAD_PRIORITY_TIME_CRITICAL;
                break;
        }

        return SetThreadPriority(GetCurrentThread(), prio) != 0;
    }

#elif defined( __APPLE__ )
    static auto _setPriority( ThreadPriority::Mode mode, float typicalProcessingTimeInMilliSeconds = 0, float maxProcessingTimeInMilliSeconds = 0 ) -> bool {
        kern_return_t result;
        mach_port_t machId = pthread_mach_thread_np( pthread_self() );

        thread_extended_policy_data_t extended;
        extended.timeshare = (mode == ThreadPriority::Mode::Normal) ? 1 : 0;
        result = thread_policy_set( machId, THREAD_EXTENDED_POLICY, (thread_policy_t)&extended, THREAD_EXTENDED_POLICY_COUNT);

        if (result != KERN_SUCCESS) {
            mach_error("thread policy error", result);
        }

        switch(mode) {
            default:
            case ThreadPriority::Mode::Normal: {
                thread_standard_policy_data_t standard;
                result = thread_policy_set( machId, THREAD_STANDARD_POLICY, (thread_policy_t)&standard, THREAD_STANDARD_POLICY_COUNT);
            } break;
            case ThreadPriority::Mode::High:
                thread_precedence_policy_data_t precedence;
                precedence.importance = 63;
                result = thread_policy_set( machId, THREAD_PRECEDENCE_POLICY, (thread_policy_t)&precedence, THREAD_PRECEDENCE_POLICY_COUNT);
                break;
            case ThreadPriority::Mode::Realtime: {
                mach_timebase_info_data_t tb_info;
                mach_timebase_info(&tb_info);
                double _clock = ((double)tb_info.denom / (double)tb_info.numer) * 1000.0;

                thread_time_constraint_policy_data_t timeConstraints;
                timeConstraints.period = 0;
                timeConstraints.computation = (unsigned)(typicalProcessingTimeInMilliSeconds * _clock * 1000.0);
                timeConstraints.constraint = (unsigned)(maxProcessingTimeInMilliSeconds * _clock * 1000.0);
                timeConstraints.preemptible = false;

                result = thread_policy_set( machId, THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&timeConstraints, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
            } break;
        }

        if (result != KERN_SUCCESS) {
            mach_error("thread policy error", result);
            return false;
        }

        return true;
    }
#else
    static auto _setPriority( ThreadPriority::Mode mode, float minProcessingTimeInMilliSeconds = 0, float maxProcessingTimeInMilliSeconds = 0 ) -> bool {
        // setcap cap_sys_nice=ep /usr/local/bin/your-program

        const int policy = SCHED_RR;
        const int minPrio = sched_get_priority_min(policy);
        const int maxPrio = sched_get_priority_max(policy);
        sched_param sch_params;

        switch(mode) {
            default:
            case Mode::Normal:
                sch_params.sched_priority = (minPrio + maxPrio) / 2;
                break;
            case Mode::High:
                sch_params.sched_priority = maxPrio - 3;
                break;
            case Mode::Realtime:
                sch_params.sched_priority = maxPrio - 1;
                break;
        }

        return pthread_setschedparam( pthread_self(), policy, &sch_params) == 0;
    }
#endif
};
