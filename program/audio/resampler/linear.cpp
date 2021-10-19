
#include "linear.h"

namespace Resampler {

auto Linear::process( ) -> void {

    FrameBuffer* outp = (FrameBuffer*) rData->out;
    float ratio = 1.0 / rData->ratio;
    float a;
    float b;
    float mu;

    if (rData->inChannels == 1) {
        float* inp = rData->in;

        float* inpMax = inp + rData->inputFrames;

        while (inp != inpMax) {

            while(fraction <= 1.0) {

                a = buffer.l;
                b = *inp;
                buffer.l = b;

                mu = fraction;

                outp->l = a * (1.0 - mu) + b * mu;

                outp->r = outp->l;

                fraction += ratio;

                outp++;
            }

            fraction -= 1.0;

            inp++;
        }

    } else {

        FrameBuffer* inp = (FrameBuffer*) rData->in;

        FrameBuffer* inpMax = (FrameBuffer*) (inp + rData->inputFrames);

        while (inp != inpMax) {

            while(fraction <= 1.0) {

                mu = fraction;

                a = buffer.l;
                b = inp->l;
                outp->l = a * (1.0 - mu) + b * mu;
                buffer.l = b;

                a = buffer.r;
                b = inp->r;
                outp->r = a * (1.0 - mu) + b * mu;
                buffer.r = b;

                fraction += ratio;

                outp++;
            }

            fraction -= 1.0;

            inp++;
        }
    }

    rData->outputFrames = outp - (FrameBuffer*)rData->out;
}

auto Linear::reset(float ratio, unsigned inChannels) -> void {
    rData->ratio = ratio;
    rData->inChannels = inChannels;
    buffer.l = 0.0;
    buffer.r = 0.0;
    fraction = 0.0;
}

}
