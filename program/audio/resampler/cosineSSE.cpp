
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

#ifdef __SSE__
    #include <xmmintrin.h>
#endif
#include "cosine.h"

namespace Resampler {

auto Cosine::buildSSE() -> void {
#ifdef __SSE__    
    distance = 0.0;
    
    if (rData->inChannels == 1) {
        
        resampler = [this]() {

            float* inp = rData->in;

            float* inpMax = inp + rData->inputFrames;

            FrameBuffer* outp = (FrameBuffer*) rData->out;

            float ratio = 1.0 / rData->ratio;
            float b = rData->ratio; /* cutoff frequency. */

            __m128 vec_previous = _mm_loadu_ps((float*) &buffer[0]);
            __m128 vec_current = _mm_loadu_ps((float*) &buffer[2]);

            while (inp != inpMax) {
                __m128 vec_ww1, vec_ww2;
                __m128 vec_w_previous;
                __m128 vec_w_current;
                __m128 vec_in;
                __m128 vec_ratio =
                        _mm_mul_ps(_mm_set_ps1(ratio), _mm_set_ps(3.0, 2.0, 1.0, 0.0));
                __m128 vec_w = _mm_sub_ps(_mm_set_ps1(distance), vec_ratio);

                __m128 vec_w1 = _mm_add_ps(vec_w, _mm_set_ps1(0.5));
                __m128 vec_w2 = _mm_sub_ps(vec_w, _mm_set_ps1(0.5));

                __m128 vec_b = _mm_set_ps1(b);

                vec_w1 = _mm_mul_ps(vec_w1, vec_b);
                vec_w2 = _mm_mul_ps(vec_w2, vec_b);

                (void) vec_ww1;
                (void) vec_ww2;

                vec_ww1 = _mm_mul_ps(vec_w1, vec_w1);
                vec_ww2 = _mm_mul_ps(vec_w2, vec_w2);

                vec_ww1 = _mm_mul_ps(vec_ww1, _mm_sub_ps(_mm_set_ps1(3.0), vec_ww1));
                vec_ww2 = _mm_mul_ps(vec_ww2, _mm_sub_ps(_mm_set_ps1(3.0), vec_ww2));

                vec_ww1 = _mm_mul_ps(_mm_set_ps1(1.0 / 4.0), vec_ww1);
                vec_ww2 = _mm_mul_ps(_mm_set_ps1(1.0 / 4.0), vec_ww2);

                vec_w1 = _mm_mul_ps(vec_w1, _mm_sub_ps(_mm_set_ps1(1.0), vec_ww1));
                vec_w2 = _mm_mul_ps(vec_w2, _mm_sub_ps(_mm_set_ps1(1.0), vec_ww2));

                vec_w1 = _mm_min_ps(vec_w1, _mm_set_ps1(0.5));
                vec_w2 = _mm_min_ps(vec_w2, _mm_set_ps1(0.5));
                vec_w1 = _mm_max_ps(vec_w1, _mm_set_ps1(-0.5));
                vec_w2 = _mm_max_ps(vec_w2, _mm_set_ps1(-0.5));
                vec_w = _mm_sub_ps(vec_w1, vec_w2);

                vec_w_previous =
                        _mm_shuffle_ps(vec_w, vec_w, _MM_SHUFFLE(1, 1, 0, 0));
                vec_w_current =
                        _mm_shuffle_ps(vec_w, vec_w, _MM_SHUFFLE(3, 3, 2, 2));

                vec_in = _mm_loadl_pi(_mm_setzero_ps(), (__m64*) inp);
                vec_in = _mm_shuffle_ps(vec_in, vec_in, _MM_SHUFFLE(1, 0, 1, 0));

                vec_previous =
                        _mm_add_ps(vec_previous, _mm_mul_ps(vec_in, vec_w_previous));
                vec_current =
                        _mm_add_ps(vec_current, _mm_mul_ps(vec_in, vec_w_current));

                distance++;
                inp++;

                if (distance > (ratio + 0.5)) {
                    _mm_storel_pi((__m64*) outp, vec_previous);
                    outp->r = outp->l; // resuse
                    vec_previous =
                            _mm_shuffle_ps(vec_previous, vec_current, _MM_SHUFFLE(1, 0, 3, 2));
                    vec_current =
                            _mm_shuffle_ps(vec_current, _mm_setzero_ps(), _MM_SHUFFLE(1, 0, 3, 2));

                    distance -= ratio;
                    outp++;
                }
            }

            _mm_storeu_ps((float*) &buffer[0], vec_previous);
            _mm_storeu_ps((float*) &buffer[2], vec_current);

            rData->outputFrames = outp - (FrameBuffer*) rData->out;
        };
    } else {
        
        resampler = [this]() {

            FrameBuffer* inp = (FrameBuffer*) rData->in;

            FrameBuffer* inpMax = (FrameBuffer*) (inp + rData->inputFrames);

            FrameBuffer* outp = (FrameBuffer*) rData->out;

            float ratio = 1.0 / rData->ratio;
            float b = rData->ratio; /* cutoff frequency. */

            __m128 vec_previous = _mm_loadu_ps((float*) &buffer[0]);
            __m128 vec_current = _mm_loadu_ps((float*) &buffer[2]);

            while (inp != inpMax) {
                __m128 vec_ww1, vec_ww2;
                __m128 vec_w_previous;
                __m128 vec_w_current;
                __m128 vec_in;
                __m128 vec_ratio =
                        _mm_mul_ps(_mm_set_ps1(ratio), _mm_set_ps(3.0, 2.0, 1.0, 0.0));
                __m128 vec_w = _mm_sub_ps(_mm_set_ps1(distance), vec_ratio);

                __m128 vec_w1 = _mm_add_ps(vec_w, _mm_set_ps1(0.5));
                __m128 vec_w2 = _mm_sub_ps(vec_w, _mm_set_ps1(0.5));

                __m128 vec_b = _mm_set_ps1(b);

                vec_w1 = _mm_mul_ps(vec_w1, vec_b);
                vec_w2 = _mm_mul_ps(vec_w2, vec_b);

                (void) vec_ww1;
                (void) vec_ww2;

                vec_ww1 = _mm_mul_ps(vec_w1, vec_w1);
                vec_ww2 = _mm_mul_ps(vec_w2, vec_w2);

                vec_ww1 = _mm_mul_ps(vec_ww1, _mm_sub_ps(_mm_set_ps1(3.0), vec_ww1));
                vec_ww2 = _mm_mul_ps(vec_ww2, _mm_sub_ps(_mm_set_ps1(3.0), vec_ww2));

                vec_ww1 = _mm_mul_ps(_mm_set_ps1(1.0 / 4.0), vec_ww1);
                vec_ww2 = _mm_mul_ps(_mm_set_ps1(1.0 / 4.0), vec_ww2);

                vec_w1 = _mm_mul_ps(vec_w1, _mm_sub_ps(_mm_set_ps1(1.0), vec_ww1));
                vec_w2 = _mm_mul_ps(vec_w2, _mm_sub_ps(_mm_set_ps1(1.0), vec_ww2));

                vec_w1 = _mm_min_ps(vec_w1, _mm_set_ps1(0.5));
                vec_w2 = _mm_min_ps(vec_w2, _mm_set_ps1(0.5));
                vec_w1 = _mm_max_ps(vec_w1, _mm_set_ps1(-0.5));
                vec_w2 = _mm_max_ps(vec_w2, _mm_set_ps1(-0.5));
                vec_w = _mm_sub_ps(vec_w1, vec_w2);

                vec_w_previous =
                        _mm_shuffle_ps(vec_w, vec_w, _MM_SHUFFLE(1, 1, 0, 0));
                vec_w_current =
                        _mm_shuffle_ps(vec_w, vec_w, _MM_SHUFFLE(3, 3, 2, 2));

                vec_in = _mm_loadl_pi(_mm_setzero_ps(), (__m64*) inp);
                vec_in = _mm_shuffle_ps(vec_in, vec_in, _MM_SHUFFLE(1, 0, 1, 0));

                vec_previous =
                        _mm_add_ps(vec_previous, _mm_mul_ps(vec_in, vec_w_previous));
                vec_current =
                        _mm_add_ps(vec_current, _mm_mul_ps(vec_in, vec_w_current));

                distance++;
                inp++;

                if (distance > (ratio + 0.5)) {
                    _mm_storel_pi((__m64*) outp, vec_previous);
                    vec_previous =
                            _mm_shuffle_ps(vec_previous, vec_current, _MM_SHUFFLE(1, 0, 3, 2));
                    vec_current =
                            _mm_shuffle_ps(vec_current, _mm_setzero_ps(), _MM_SHUFFLE(1, 0, 3, 2));

                    distance -= ratio;
                    outp++;
                }
            }

            _mm_storeu_ps((float*) &buffer[0], vec_previous);
            _mm_storeu_ps((float*) &buffer[2], vec_current);

            rData->outputFrames = outp - (FrameBuffer*)rData->out;                    

        };
    }
#endif    
}

}

