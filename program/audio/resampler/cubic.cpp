
#include "cubic.h"

namespace Resampler {

    auto Cubic::process( ) -> void {

        FrameBuffer* outp = (FrameBuffer*) rData->out;
        float ratio = 1.0 / rData->ratio;
        float a, b, c, d;
        float A, B, C, D;
        float mu;

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

                    mu = fraction;

                    A = d - c - a + b;
                    B = a - b - A;
                    C = c - a;
                    D = b;

                    outp->l = A * (mu * 3) + B * (mu * 2) + C * mu + D;

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

                    mu = fraction;

                    A = d - c - a + b;
                    B = a - b - A;
                    C = c - a;
                    D = b;

                    outp->l = A * (mu * 3) + B * (mu * 2) + C * mu + D;

                    a = buffer[2].r;
                    b = buffer[1].r;
                    c = buffer[0].r;
                    d = inp->r;

                    buffer[2].r = b;
                    buffer[1].r = c;
                    buffer[0].r = d;

                    mu = fraction;

                    A = d - c - a + b;
                    B = a - b - A;
                    C = c - a;
                    D = b;

                    outp->r = A * (mu * 3) + B * (mu * 2) + C * mu + D;

                    fraction += ratio;

                    outp++;
                }

                fraction -= 1.0;

                inp++;
            }
        }

        rData->outputFrames = outp - (FrameBuffer*)rData->out;
    }

    auto Cubic::reset(float ratio, unsigned inChannels) -> void {
        rData->ratio = ratio;
        rData->inChannels = inChannels;
        for (unsigned i = 0; i < 3; i++) {
            buffer[i].l = 0.0;
            buffer[i].r = 0.0;
        }
        fraction = 0.0;
    }

}
