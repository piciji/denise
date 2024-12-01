
/* Copyright  (C) 2010-2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (reverb.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "base.h"

#define numcombs 8
#define numallpasses 4

namespace DSP {

    
struct Reverb : Base {
    Reverb();

    virtual ~Reverb() {
        reverb_free();
    }
    
    const float muted = 0;
    const float fixedgain = 0.015f;
    const float scalewet = 3;
    const float scaledry = 2;
    const float scaledamp = 0.4f;
    const float scaleroom = 0.28f;
    const float offsetroom = 0.7f;
    const float initialroom = 0.5f;
    const float initialdamp = 0.5f;
    const float initialwet = 1.0f / 3.0f;
    const float initialdry = 0;
    const float initialwidth = 1;
    const float initialmode = 0;
    const float freezemode = 0.5f;
    
    struct comb {
        float *buffer;
        unsigned bufsize;
        unsigned bufidx;

        float feedback;
        float filterstore;
        float damp1, damp2;
    };

    struct allpass {
        float *buffer;
        float feedback;
        unsigned bufsize;
        unsigned bufidx;
    };
    
    struct revmodel {
        struct comb combL[numcombs];
        struct allpass allpassL[numallpasses];

        float *bufcomb[numcombs];
        float *bufallpass[numallpasses];

        float gain;
        float roomsize, roomsize1;
        float damp, damp1;
        float wet, wet1, wet2;
        float dry;
        float width;
        float mode;
    } left, right;

    auto process(Data* output, Data* input) -> void;

    auto processOneChannel(Data* output, Data* input) -> void;
    
    float revmodel_process(struct revmodel *rev, float in);
        
    auto init( float sampleRate, float drytime, float wettime, float damping, float roomwidth, float roomsize ) -> void;
    
    void revmodel_init(struct revmodel *rev, int srate);        
    
    void revmodel_update(struct revmodel *rev);
    
    void revmodel_setroomsize(struct revmodel *rev, float value);
    
    void revmodel_setdamp(struct revmodel *rev, float value);
    
    void revmodel_setwet(struct revmodel *rev, float value);
        
    void revmodel_setdry(struct revmodel *rev, float value);
    
    void revmodel_setwidth(struct revmodel *rev, float value);
    
    void revmodel_setmode(struct revmodel *rev, float value);
    
    float comb_process(struct comb *c, float input);
    
    float allpass_process(struct allpass *a, float input);
    
    void reverb_free();
};

}
