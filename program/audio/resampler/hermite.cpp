
#include "hermite.h"

namespace Resampler {

    auto Hermite::process( ) -> void {

        FrameBuffer* outp = (FrameBuffer*) rData->out;
        float ratio = 1.0 / rData->ratio;
        float a, b, c, d;
        float mu1, mu2, mu3, m0, m1, a0, a1, a2, a3;
        const float tension = 0.0;  //-1 = low, 0 = normal, +1 = high
        const float bias = 0.0;  //-1 = left, 0 = even, +1 = right

        if (rData->inChannels == 1) {
            float* inp = rData->in;

            float* inpMax = inp + rData->inputFrames;

            while (inp != inpMax) {

                while(fraction <= 1.0) {

                    a = buffer[2].l;
                    b = buffer[1].l;
                    c = buffer[0].l;
                    d = *inp;

                    buffer[2].l = b;
                    buffer[1].l = c;
                    buffer[0].l = d;

                    mu1 = fraction;
                    mu2 = mu1 * mu1;
                    mu3 = mu2 * mu1;

                    m0  = (b - a) * (1.0 + bias) * (1.0 - tension) / 2.0;
                    m0 += (c - b) * (1.0 - bias) * (1.0 - tension) / 2.0;
                    m1  = (c - b) * (1.0 + bias) * (1.0 - tension) / 2.0;
                    m1 += (d - c) * (1.0 - bias) * (1.0 - tension) / 2.0;

                    a0 = +2 * mu3 - 3 * mu2 + 1;
                    a1 =      mu3 - 2 * mu2 + mu1;
                    a2 =      mu3 -     mu2;
                    a3 = -2 * mu3 + 3 * mu2;

                    outp->l = (a0 * b) + (a1 * m0) + (a2 * m1) + (a3 * c);

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

                    a = buffer[2].l;
                    b = buffer[1].l;
                    c = buffer[0].l;
                    d = inp->l;

                    buffer[2].l = b;
                    buffer[1].l = c;
                    buffer[0].l = d;

                    mu1 = fraction;
                    mu2 = mu1 * mu1;
                    mu3 = mu2 * mu1;

                    m0  = (b - a) * (1.0 + bias) * (1.0 - tension) / 2.0;
                    m0 += (c - b) * (1.0 - bias) * (1.0 - tension) / 2.0;
                    m1  = (c - b) * (1.0 + bias) * (1.0 - tension) / 2.0;
                    m1 += (d - c) * (1.0 - bias) * (1.0 - tension) / 2.0;

                    a0 = +2 * mu3 - 3 * mu2 + 1;
                    a1 =      mu3 - 2 * mu2 + mu1;
                    a2 =      mu3 -     mu2;
                    a3 = -2 * mu3 + 3 * mu2;

                    outp->l = (a0 * b) + (a1 * m0) + (a2 * m1) + (a3 * c);

                    a = buffer[2].r;
                    b = buffer[1].r;
                    c = buffer[0].r;
                    d = inp->r;

                    buffer[2].r = b;
                    buffer[1].r = c;
                    buffer[0].r = d;

                    mu1 = fraction;
                    mu2 = mu1 * mu1;
                    mu3 = mu2 * mu1;

                    m0  = (b - a) * (1.0 + bias) * (1.0 - tension) / 2.0;
                    m0 += (c - b) * (1.0 - bias) * (1.0 - tension) / 2.0;
                    m1  = (c - b) * (1.0 + bias) * (1.0 - tension) / 2.0;
                    m1 += (d - c) * (1.0 - bias) * (1.0 - tension) / 2.0;

                    a0 = +2 * mu3 - 3 * mu2 + 1;
                    a1 =      mu3 - 2 * mu2 + mu1;
                    a2 =      mu3 -     mu2;
                    a3 = -2 * mu3 + 3 * mu2;

                    outp->r = (a0 * b) + (a1 * m0) + (a2 * m1) + (a3 * c);

                    fraction += ratio;

                    outp++;
                }

                fraction -= 1.0;

                inp++;
            }
        }

        rData->outputFrames = outp - (FrameBuffer*)rData->out;
    }

    auto Hermite::reset(float ratio, unsigned inChannels) -> void {
        rData->ratio = ratio;
        rData->inChannels = inChannels;
        for (unsigned i = 0; i < 3; i++) {
            buffer[i].l = 0.0;
            buffer[i].r = 0.0;
        }
        fraction = 0.0;
    }

}
