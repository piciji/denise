

#include "chamberlin.h"

#include "../system/system.h"

namespace LIBC64 {

auto Chamberlin::clock() -> void {
    processEnvVoice();

    double _sample = chamberlinFilter.clock((double) voice[0].output() / 255.0,
                                            (double) voice[1].output() / 255.0,
                                            (double) voice[2].output() / 255.0);

    externalFilter.clock((int16_t)Emulator::sclamp(16, (int)_sample));
}

auto Chamberlin::clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int {
    float curSample;

    if (audioOut) {
        for (int c = 0; c < cycles; c++) {
            processEnvVoice();

            curSample = chamberlinFilter.clock(
                voice[0].output() / 255.0,
                voice[1].output() / 255.0,
                voice[2].output() / 255.0
            );

            externalFilter.clock((int16_t)Emulator::sclamp(16, (int)curSample));

            if (++sampleCounter == sampleLimit) {
                system->audioRefresh( (int16_t)Emulator::sclamp( 16,(externalFilter.output() * 4) / 5 ) );
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
