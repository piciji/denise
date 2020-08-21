
// modified version of Bass Boost (IIR filter) from RetroArch

/* Copyright  (C) 2010-2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (iir.c).
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

#include "bass.h"

#include <cmath>

#ifndef M_PI 
#define M_PI    3.14159265358979323846f 
#endif

namespace DSP {
    
    auto Bass::init(float sampleRate, float frequency, float gain, float reduceClipping ) -> void {
        
        this->reduceClipping = reduceClipping;
        
        double omega = 2.0 * M_PI * frequency / sampleRate;
        double cs = std::cos(omega);
        double sn = std::sin(omega);
        double A = std::exp(std::log(10.0) * gain / 40.0);
        double beta = std::sqrt((A * A + 1) / 1.0 - (std::pow((A - 1), 2)));
        
        b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
        b1 = 2 * A * ((A - 1) - (A + 1) * cs);
        b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
        a0 = ((A + 1) + (A - 1) * cs + beta * sn);
        a1 = -2 * ((A - 1) + (A + 1) * cs);
        a2 = (A + 1) + (A - 1) * cs - beta * sn;

        l.xn1 = 0;
        l.xn2 = 0;
        l.yn1 = 0;
        l.yn2 = 0;
        
        r.xn1 = 0;
        r.xn2 = 0;
        r.yn1 = 0;
        r.yn2 = 0;
    }
    
    auto Bass::processOneChannel( Data* output, Data* input ) -> void {
        
        float _xn1_l = l.xn1;
        float _xn2_l = l.xn2;
        float _yn1_l = l.yn1;
        float _yn2_l = l.yn2;

        output->samples = input->samples;
        output->frames = input->frames;
        
        float* out = output->samples;

        for (unsigned i = 0; i < input->frames; i++, out += 2) {
            float in_l = out[0];

            float _l = (b0 * in_l + b1 * _xn1_l + b2 * _xn2_l - a1 * _yn1_l - a2 * _yn2_l) / a0;

            _xn2_l = _xn1_l;
            _xn1_l = in_l;
            _yn2_l = _yn1_l;
            _yn1_l = _l;

            out[0] = _l * reduceClipping;
            out[1] = out[0]; // copy left to right channel
        }

        l.xn1 = _xn1_l;
        l.xn2 = _xn2_l;
        l.yn1 = _yn1_l;
        l.yn2 = _yn2_l;
    }
    
    auto Bass::process( Data* output, Data* input ) -> void {

        if (mono)
            return processOneChannel( output, input );                

        float _xn1_l = l.xn1;
        float _xn2_l = l.xn2;
        float _yn1_l = l.yn1;
        float _yn2_l = l.yn2;

        float _xn1_r = r.xn1;
        float _xn2_r = r.xn2;
        float _yn1_r = r.yn1;
        float _yn2_r = r.yn2;

        output->samples = input->samples;
        output->frames = input->frames;
        
        float* out = output->samples;

        for (unsigned i = 0; i < input->frames; i++, out += 2) {
            float in_l = out[0];
            float in_r = out[1];

            float _l = (b0 * in_l + b1 * _xn1_l + b2 * _xn2_l - a1 * _yn1_l - a2 * _yn2_l) / a0;
            float _r = (b0 * in_r + b1 * _xn1_r + b2 * _xn2_r - a1 * _yn1_r - a2 * _yn2_r) / a0;

            _xn2_l = _xn1_l;
            _xn1_l = in_l;
            _yn2_l = _yn1_l;
            _yn1_l = _l;

            _xn2_r = _xn1_r;
            _xn1_r = in_r;
            _yn2_r = _yn1_r;
            _yn1_r = _r;

            out[0] = _l;
            out[1] = _r;
        }

        l.xn1 = _xn1_l;
        l.xn2 = _xn2_l;
        l.yn1 = _yn1_l;
        l.yn2 = _yn2_l;

        r.xn1 = _xn1_r;
        r.xn2 = _xn2_r;
        r.yn1 = _yn1_r;
        r.yn2 = _yn2_r;
    }
}