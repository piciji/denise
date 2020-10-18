
/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2014-2017 - Ali Bouhlel ( aliaspider@gmail.com )
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * modification: same algorithm with modified context
 */

#include "cosine.h"

namespace Resampler {

auto Cosine::build() -> void {
    
    if (rData->ratio < 0.75) {
        // down sample
        distance = 0.0;       

        if (rData->inChannels == 1) {

            resampler = [this]() {               

                float* inp = rData->in;

                float* inpMax = inp + rData->inputFrames;

                FrameBuffer* outp = (FrameBuffer*) rData->out;

                float ratio = 1.0 / rData->ratio;

                float b = rData->ratio; /* cutoff frequency. */

                while (inp != inpMax) {

                    buffer[0].l += *inp * cc_kernel(distance, b);
                    buffer[1].l += *inp * cc_kernel(distance - ratio, b);
                    buffer[2].l += *inp * cc_kernel(distance - ratio - ratio, b);                          

                    distance++;
                    inp++;

                    if (distance > (ratio + 0.5)) {
                        // resue calculation
                        buffer[0].r = buffer[0].l;
                        *outp = buffer[0];

                        buffer[0] = buffer[1];
                        buffer[1] = buffer[2];

                        buffer[2].l = 0.0;
                        buffer[2].r = 0.0;

                        distance -= ratio;
                        outp++;
                    }
                }

                rData->outputFrames = outp - (FrameBuffer*)rData->out;
            };

        } else {

            resampler = [this]() {               

                FrameBuffer* inp = (FrameBuffer*) rData->in;

                FrameBuffer* inpMax = (FrameBuffer*) (inp + rData->inputFrames);

                FrameBuffer* outp = (FrameBuffer*) rData->out;

                float ratio = 1.0 / rData->ratio;

                float b = rData->ratio; /* cutoff frequency. */

                while (inp != inpMax) {
                    add_to(inp, buffer + 0, cc_kernel(distance, b));
                    add_to(inp, buffer + 1, cc_kernel(distance - ratio, b));
                    add_to(inp, buffer + 2, cc_kernel(distance - ratio - ratio, b));

                    distance++;
                    inp++;

                    if (distance > (ratio + 0.5)) {
                        *outp = buffer[0];

                        buffer[0] = buffer[1];
                        buffer[1] = buffer[2];

                        buffer[2].l = 0.0;
                        buffer[2].r = 0.0;

                        distance -= ratio;
                        outp++;
                    }
                }

                rData->outputFrames = outp - (FrameBuffer*)rData->out;
            };
        }                       
    } else {
        // up sample
        distance = 2.0;

        if (rData->inChannels == 1) {

            resampler = [this]() {

                float* inp = rData->in;

                float* inpMax = inp + rData->inputFrames;

                FrameBuffer* outp = (FrameBuffer*) rData->out;

                float b = fmin(rData->ratio, 1.00); /* cutoff frequency. */
                float ratio = 1.0 / rData->ratio;

                while (inp != inpMax) {
                    buffer[0] = buffer[1];
                    buffer[1] = buffer[2];
                    buffer[2] = buffer[3];
                    buffer[3].l = *inp;

                    while (distance < 1.0) {

                        outp->l = 0.0;
                        outp->r = 0.0;

                        for (int i = 0; i < 4; i++) {                                
                            float temp = cc_kernel(distance + 1.0 - i, b);
                            outp->l += buffer[i].l * temp;
                        }
                        // resue calculation
                        outp->r = outp->l;

                        distance += ratio;
                        outp++;
                    }

                    distance -= 1.0;
                    inp++;
                }

                rData->outputFrames = outp - (FrameBuffer*)rData->out;
            };

        } else {

            resampler = [this]() {

                FrameBuffer* inp = (FrameBuffer*) rData->in;

                FrameBuffer* inpMax = (FrameBuffer*) (inp + rData->inputFrames);

                FrameBuffer* outp = (FrameBuffer*) rData->out;

                float b = fminf(rData->ratio, 1.00); /* cutoff frequency. */
                float ratio = 1.0 / rData->ratio;

                while (inp != inpMax) {
                    buffer[0] = buffer[1];
                    buffer[1] = buffer[2];
                    buffer[2] = buffer[3];
                    buffer[3] = *inp;

                    while (distance < 1.0) {

                        outp->l = 0.0;
                        outp->r = 0.0;

                        for (int i = 0; i < 4; i++) {

                            float temp = cc_kernel(distance + 1.0 - i, b);
                            outp->l += buffer[i].l * temp;
                            outp->r += buffer[i].r * temp;
                        }

                        distance += ratio;
                        outp++;
                    }

                    distance -= 1.0;
                    inp++;
                }

                rData->outputFrames = outp - (FrameBuffer*)rData->out;
            };
        }        
    }
}

}
