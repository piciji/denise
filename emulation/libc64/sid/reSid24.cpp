
#include "reSid24.h"

#include "../system/system.h"

namespace LIBC64 {

auto ReSid24::clock() -> void {
    processEnvVoice();

    filter.clock24(voice[0].output(), voice[1].output(), voice[2].output());

    externalFilter.clock(filter.output24());
}

auto ReSid24::clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int {
    if (audioOut) {
        for (int c = 0; c < cycles; c++) {
            processEnvVoice();

            filter.clock24(voice[0].output(), voice[1].output(), voice[2].output());
            externalFilter.clock(filter.output24());

            if (++sampleCounter == sampleLimit) {
                system->audioRefresh( (int16_t)Emulator::sclamp( 16, externalFilter.output() ) );
                sampleCounter = 0;
            }
        }
    } else {
        for (int c = 0; c < cycles; c++) {
            processEnvVoice();
        }
    }

    return sampleCounter;
}

}
