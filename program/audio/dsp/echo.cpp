/* Copyright  (C) 2010-2020 The RetroArch team
*
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (echo.c).
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

#include "echo.h"
#include <cstring>

namespace DSP {

auto Echo::process(Data* output, Data* input) -> void {
    if (mono)
        return processOneChannel(output, input);

    unsigned i;
    float* out;

    output->samples = input->samples;
    output->frames = input->frames;
    out = output->samples;

    for (i = 0; i < input->frames; i++, out += 2) {
        float left, right;
        float echo_left  = 0.0f;
        float echo_right = 0.0f;

        echo_left  += buffer[(ptr << 1) + 0];
        echo_right += buffer[(ptr << 1) + 1];

        echo_left     *= amp;
        echo_right    *= amp;

        left           = out[0] + echo_left;
        right          = out[1] + echo_right;

        float feedback_left  = out[0] + feedback * echo_left;
        float feedback_right = out[1] + feedback * echo_right;

        buffer[(ptr << 1) + 0] = feedback_left;
        buffer[(ptr << 1) + 1] = feedback_right;

        ptr = (ptr + 1) % frames;

        out[0] = left;
        out[1] = right;
    }
}

auto Echo::processOneChannel(Data* output, Data* input) -> void {
    unsigned i;
    float* out;

    output->samples = input->samples;
    output->frames = input->frames;
    out = output->samples;

    for (i = 0; i < input->frames; i++, out += 2) {
        float left;
        float echo_left  = 0.0f;

        echo_left  += buffer[(ptr << 1) + 0];

        echo_left     *= amp;

        left           = out[0] + echo_left;

        float feedback_left  = out[0] + feedback * echo_left;

        buffer[(ptr << 1) + 0] = feedback_left;

        ptr = (ptr + 1) % frames;

        out[0] = left;
        out[1] = left;
    }
}

auto Echo::init( float sampleRate, unsigned delay, float feedback, float amp) -> void {
    this->feedback = feedback;
    this->amp = amp;
    if (delay == 0)
        delay = 1;

    unsigned newFrames = (unsigned)((float)delay * (sampleRate / 1000.0f) + 0.5f);
    if (newFrames && (newFrames != this->frames)) {
        this->frames = newFrames;
        if (buffer)
            delete[] buffer;
        buffer = new float[newFrames * 2];
    }

    this->ptr = 0;
    std::memset(buffer, 0, this->frames * 2 * 4);
}

auto Echo::free() ->void {
    if (buffer)
        delete[] buffer;

    buffer = nullptr;
}

}
