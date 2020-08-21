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

#include <cstring>
#include "reverb.h"

namespace DSP {
    
auto Reverb::process( Data* output, Data* input ) -> void {

    if (mono)
        return processOneChannel(output, input);    
    
    unsigned i;
    float *out;

    output->samples = input->samples;
    output->frames = input->frames;
    out = output->samples;

    for (i = 0; i < input->frames; i++, out += 2) {
        float in[2] = {out[0], out[1]};

        out[0] = revmodel_process(&left, in[0]);
        out[1] = revmodel_process(&right, in[1]);
    }
} 

auto Reverb::processOneChannel(Data* output, Data* input) -> void {
    unsigned i;
    float *out;

    output->samples = input->samples;
    output->frames = input->frames;
    out = output->samples;

    for (i = 0; i < input->frames; i++, out += 2) {
        
        out[0] = revmodel_process(&left, out[0]);
        out[1] = out[0];
    }
}

float Reverb::revmodel_process(struct revmodel *rev, float in)
{
   int i;
   float mono_out = 0.0f;
   float mono_in  = in;
   float input    = mono_in * rev->gain;

   for (i = 0; i < numcombs; i++)
      mono_out += comb_process(&rev->combL[i], input);

   for (i = 0; i < numallpasses; i++)
      mono_out = allpass_process(&rev->allpassL[i], mono_out);

   return mono_in * rev->dry + mono_out * rev->wet1;
}
    
auto Reverb::init( float sampleRate, float drytime, float wettime, float damping, float roomwidth, float roomsize ) -> void {

    revmodel_init(&left, sampleRate);
    revmodel_init(&right, sampleRate);

    revmodel_setdamp(&left, damping);
    revmodel_setdry(&left, drytime);
    revmodel_setwet(&left, wettime);
    revmodel_setwidth(&left, roomwidth);
    revmodel_setroomsize(&left, roomsize);

    revmodel_setdamp(&right, damping);
    revmodel_setdry(&right, drytime);
    revmodel_setwet(&right, wettime);
    revmodel_setwidth(&right, roomwidth);
    revmodel_setroomsize(&right, roomsize);
}

void Reverb::revmodel_init(struct revmodel *rev, int srate) {

    static const int comb_lengths[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static const int allpass_lengths[4] = {225, 341, 441, 556};
    double r = srate * (1 / 44100.0);
    
    unsigned c;

    for (c = 0; c < numcombs; ++c) {
        rev->combL[c].bufsize = r * comb_lengths[c];
        rev->bufcomb[c] = new float[rev->combL[c].bufsize];
        rev->combL[c].buffer = rev->bufcomb[c];
        std::memset(rev->combL[c].buffer, 0, rev->combL[c].bufsize * sizeof (float));        
        rev->combL[c].bufidx = 0;
        rev->combL[c].filterstore = 0;
    }

    for (c = 0; c < numallpasses; ++c) {
        rev->allpassL[c].bufsize = r * allpass_lengths[c];        
        rev->bufallpass[c] = new float[rev->allpassL[c].bufsize];
        rev->allpassL[c].buffer = rev->bufallpass[c];
        std::memset(rev->allpassL[c].buffer, 0, rev->allpassL[c].bufsize * sizeof (float));        
        rev->allpassL[c].feedback = 0.5f;
        rev->allpassL[c].bufidx = 0;
    }

    revmodel_setwet(rev, initialwet);
    revmodel_setroomsize(rev, initialroom);
    revmodel_setdry(rev, initialdry);
    revmodel_setdamp(rev, initialdamp);
    revmodel_setwidth(rev, initialwidth);
    revmodel_setmode(rev, initialmode);
}

void Reverb::revmodel_update(struct revmodel *rev)
{
   int i;
   rev->wet1 = rev->wet * (rev->width / 2.0f + 0.5f);

   if (rev->mode >= freezemode)
   {
      rev->roomsize1 = 1.0f;
      rev->damp1 = 0.0f;
      rev->gain = muted;
   }
   else
   {
      rev->roomsize1 = rev->roomsize;
      rev->damp1 = rev->damp;
      rev->gain = fixedgain;
   }

   for (i = 0; i < numcombs; i++)
   {
      rev->combL[i].feedback = rev->roomsize1;
      rev->combL[i].damp1 = rev->damp1;
      rev->combL[i].damp2 = 1.0f - rev->damp1;
   }
}

void Reverb::revmodel_setroomsize(struct revmodel *rev, float value)
{
   rev->roomsize = value * scaleroom + offsetroom;
   revmodel_update(rev);
}

void Reverb::revmodel_setdamp(struct revmodel *rev, float value)
{
   rev->damp = value * scaledamp;
   revmodel_update(rev);
}

void Reverb::revmodel_setwet(struct revmodel *rev, float value)
{
   rev->wet = value * scalewet;
   revmodel_update(rev);
}

void Reverb::revmodel_setdry(struct revmodel *rev, float value)
{
   rev->dry = value * scaledry;
   revmodel_update(rev);
}

void Reverb::revmodel_setwidth(struct revmodel *rev, float value)
{
   rev->width = value;
   revmodel_update(rev);
}

void Reverb::revmodel_setmode(struct revmodel *rev, float value)
{
   rev->mode = value;
   revmodel_update(rev);
}

inline float Reverb::comb_process(struct comb *c, float input)
{
   float output         = c->buffer[c->bufidx];
   c->filterstore       = (output * c->damp2) + (c->filterstore * c->damp1);

   c->buffer[c->bufidx] = input + (c->filterstore * c->feedback);

   c->bufidx++;
   if (c->bufidx >= c->bufsize)
      c->bufidx = 0;

   return output;
}

inline float Reverb::allpass_process(struct allpass *a, float input)
{
   float bufout         = a->buffer[a->bufidx];
   float output         = -input + bufout;
   a->buffer[a->bufidx] = input + bufout * a->feedback;

   a->bufidx++;
   if (a->bufidx >= a->bufsize)
      a->bufidx = 0;

   return output;
}

void Reverb::reverb_free()
{
    unsigned i;

    for (i = 0; i < numcombs; i++) {
        delete[] left.bufcomb[i];
        delete[] right.bufcomb[i];
    }

    for (i = 0; i < numallpasses; i++) {
        delete[] left.bufallpass[i];
        delete[] right.bufallpass[i];
    }        
}
    
}