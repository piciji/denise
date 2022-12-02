
#include "paula.h"
#include <cmath>

// The audio filters are analog and outside the Paula chip. For the sake of simplicity, these are described in the context of Paula.

namespace LIBAMI {

auto Paula::setLedFilter(bool state) -> void {
    useLedFilter = state;
}

auto Paula::setFilter() -> void {
    filters[0].reset();
    filters[1].reset();

    // input sample frequency (not host sample frequency, we apply this filter before we resample)
    float sampleFrequency = (float)agnus.frequency() / (float)sampleLimit;

    filter1A0 = calcFilter(sampleFrequency, 6200);
    filter2A0 = calcFilter(sampleFrequency, 20000);
    filterA0 = calcFilter(sampleFrequency, 7000);
}

auto Paula::calcFilter(float sampleFrequency, unsigned cutoffFrequency) -> float {
    if (cutoffFrequency >= (sampleFrequency / 2) )
        return 1.0;

    double omega = 2.0 * 3.14159265358979323846 * (double)cutoffFrequency / (double)sampleFrequency;
    return (float)(1.0 / (1.0 + 1.0 / ( std::tan (omega / 2.0) * 2.0) ));
}

template<uint8_t channel> auto Paula::lowPassfilter(int32_t sample) -> int32_t {
    float staticFilter, ledFilter;
    float _filterA0 = filterA0;
    float filterA0Inverse = 1.0 - _filterA0;

    Filter& filter = filters[channel];

    if (agnus.aga()) {
        staticFilter = (float)sample;

        filter.rc2 = _filterA0 * staticFilter + filterA0Inverse * filter.rc2 + (1E-10);
        filter.rc3 = _filterA0 * filter.rc2   + filterA0Inverse * filter.rc3;
        filter.rc4 = _filterA0 * filter.rc3   + filterA0Inverse * filter.rc4;
        ledFilter = filter.rc4;

    } else {
        filter.rc1 = filter1A0 * (float)sample + (1.0 - filter1A0) * filter.rc1 + (1E-10);
        filter.rc2 = filter2A0 * filter.rc1 + (1.0 - filter2A0) * filter.rc2;
        staticFilter = filter.rc2;

        filter.rc3 = _filterA0 * staticFilter + filterA0Inverse * filter.rc3;
        filter.rc4 = _filterA0 * filter.rc3   + filterA0Inverse * filter.rc4;
        filter.rc5 = _filterA0 * filter.rc4   + filterA0Inverse * filter.rc5;
        ledFilter = filter.rc5;
    }

    return useLedFilter ? (int32_t)ledFilter : (int32_t)staticFilter;
}

}