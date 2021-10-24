
/* Copyright  (C) 2010-2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (sinc_resampler.c).
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

/*
 * modification: same algorithm with modified context
 */

#include "sinc.h"
#include <cstring>

#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif

namespace Resampler {

    auto Sinc::build() -> void {
        double cutoff                  = 0.0;
        size_t phase_elems             = 0;
        size_t elems                   = 0;
        unsigned enable_avx            = 0;
        unsigned sidelobes             = 0;
        bool useKaiser;

        switch (quality) {
            case RESAMPLER_QUALITY_LOWEST:
                cutoff            = 0.98;
                sidelobes         = 2;
                ctx.phase_bits    = 12;
                ctx.subphase_bits = 10;
                useKaiser = false;
                break;
            case RESAMPLER_QUALITY_LOWER:
                cutoff            = 0.98;
                sidelobes         = 4;
                ctx.phase_bits    = 12;
                ctx.subphase_bits = 10;
                useKaiser = false;
                break;
            case RESAMPLER_QUALITY_HIGHER:
                cutoff            = 0.90;
                sidelobes         = 32;
                ctx.phase_bits    = 10;
                ctx.subphase_bits = 14;
                useKaiser = true;
                ctx.kaiser_beta   = 10.5;
                enable_avx        = 1;
                break;
            case RESAMPLER_QUALITY_HIGHEST:
                cutoff            = 0.962;
                sidelobes         = 128;
                ctx.phase_bits    = 10;
                ctx.subphase_bits = 14;
                useKaiser = true;
                ctx.kaiser_beta   = 14.5;
                enable_avx        = 1;
                break;
            case RESAMPLER_QUALITY_NORMAL:
            case RESAMPLER_QUALITY_DONTCARE:
                cutoff            = 0.825;
                sidelobes         = 8;
                ctx.phase_bits    = 8;
                ctx.subphase_bits = 16;
                useKaiser = true;
                ctx.kaiser_beta   = 5.5;
                break;
        }

        ctx.subphase_mask = (1 << ctx.subphase_bits) - 1;
        ctx.subphase_mod  = 1.0f / (1 << ctx.subphase_bits);
        ctx.taps          = sidelobes * 2;

        /* Downsampling, must lower cutoff, and extend number of
         * taps accordingly to keep same stopband attenuation. */
        if (rData->ratio < 1.0) {
            cutoff *= rData->ratio;
            ctx.taps = (unsigned)ceil(ctx.taps / rData->ratio);
        }

        ctx.taps     = (ctx.taps + 3) & ~3;

        phase_elems     = ((1 << ctx.phase_bits) * ctx.taps);
        if (useKaiser)
            phase_elems  = phase_elems * 2;

        elems           = phase_elems + 4 * ctx.taps;

        ctx.main_buffer = (float*)memalign_alloc(128, sizeof(float) * elems);

        std::memset(ctx.main_buffer, 0, sizeof(float) * elems);

        ctx.phase_table = ctx.main_buffer;
        ctx.buffer_l    = ctx.main_buffer + phase_elems;
        ctx.buffer_r    = ctx.buffer_l + 2 * ctx.taps;

        if (useKaiser) {
            init_table_kaiser(cutoff, ctx.phase_table, 1 << ctx.phase_bits, ctx.taps, true);

        } else {
            init_table_lanczos(cutoff, ctx.phase_table, 1 << ctx.phase_bits, ctx.taps, false);
        }

        if (useKaiser) {
            resampler = [this]() {

                unsigned phases                = 1 << (ctx.phase_bits + ctx.subphase_bits);

                uint32_t ratio                 = phases / rData->ratio;
                const float *input             = rData->in;
                float *output                  = rData->out;
                size_t frames                  = rData->inputFrames;
                size_t out_frames              = 0;

                {
                    if (rData->inChannels == 1) {
                        while (frames) {
                            while (frames && ctx.time >= phases) {
                                /* Push in reverse to make filter more obvious. */
                                if (!ctx.ptr)
                                    ctx.ptr = ctx.taps;
                                ctx.ptr--;

                                ctx.buffer_l[ctx.ptr + ctx.taps] =
                                ctx.buffer_l[ctx.ptr] = *input++;

                                ctx.time -= phases;
                                frames--;
                            }

                            {
                                const float *buffer_l = ctx.buffer_l + ctx.ptr;
                                unsigned taps = ctx.taps;
                                while (ctx.time < phases) {
                                    unsigned i;
                                    float sum_l = 0.0f;
                                    unsigned phase = ctx.time >> ctx.subphase_bits;
                                    float *phase_table = ctx.phase_table + phase * taps * 2;
                                    float *delta_table = phase_table + taps;
                                    float delta = (float)(ctx.time & ctx.subphase_mask) * ctx.subphase_mod;

                                    for (i = 0; i < taps; i++) {
                                        float sinc_val = phase_table[i] + delta_table[i] * delta;

                                        sum_l += buffer_l[i] * sinc_val;

                                    }

                                    output[0] = sum_l;
                                    output[1] = sum_l;

                                    output += 2;
                                    out_frames++;
                                    ctx.time += ratio;
                                }
                            }

                        }
                    } else {

                        while (frames) {
                            while (frames && ctx.time >= phases) {
                                /* Push in reverse to make filter more obvious. */
                                if (!ctx.ptr)
                                    ctx.ptr = ctx.taps;
                                ctx.ptr--;

                                ctx.buffer_l[ctx.ptr + ctx.taps] =
                                ctx.buffer_l[ctx.ptr] = *input++;

                                ctx.buffer_r[ctx.ptr + ctx.taps] =
                                ctx.buffer_r[ctx.ptr] = *input++;

                                ctx.time -= phases;
                                frames--;
                            }

                            {
                                const float *buffer_l = ctx.buffer_l + ctx.ptr;
                                const float *buffer_r = ctx.buffer_r + ctx.ptr;
                                unsigned taps = ctx.taps;
                                while (ctx.time < phases) {
                                    unsigned i;
                                    float sum_l = 0.0f;
                                    float sum_r = 0.0f;
                                    unsigned phase = ctx.time >> ctx.subphase_bits;
                                    float *phase_table = ctx.phase_table + phase * taps * 2;
                                    float *delta_table = phase_table + taps;
                                    float delta = (float)(ctx.time & ctx.subphase_mask) * ctx.subphase_mod;

                                    for (i = 0; i < taps; i++) {
                                        float sinc_val = phase_table[i] + delta_table[i] * delta;

                                        sum_l += buffer_l[i] * sinc_val;
                                        sum_r += buffer_r[i] * sinc_val;
                                    }

                                    output[0] = sum_l;
                                    output[1] = sum_r;

                                    output += 2;
                                    out_frames++;
                                    ctx.time += ratio;
                                }
                            }

                        }
                    }
                }

                rData->outputFrames = out_frames;
            };
        } else {
            resampler = [this]() {
                unsigned phases                = 1 << (ctx.phase_bits + ctx.subphase_bits);

                uint32_t ratio                 = phases / rData->ratio;
                const float *input             = rData->in;
                float *output                  = rData->out;
                size_t frames                  = rData->inputFrames;
                size_t out_frames              = 0;

                {
                    if (rData->inChannels == 1) {
                        while (frames) {
                            while (frames && ctx.time >= phases) {
                                /* Push in reverse to make filter more obvious. */
                                if (!ctx.ptr)
                                    ctx.ptr = ctx.taps;
                                ctx.ptr--;

                                ctx.buffer_l[ctx.ptr + ctx.taps] =
                                ctx.buffer_l[ctx.ptr] = *input++;

                                ctx.time -= phases;
                                frames--;
                            }

                            {
                                const float *buffer_l = ctx.buffer_l + ctx.ptr;
                                unsigned taps = ctx.taps;
                                while (ctx.time < phases) {
                                    unsigned i;
                                    float sum_l = 0.0f;
                                    unsigned phase = ctx.time >> ctx.subphase_bits;
                                    float *phase_table = ctx.phase_table + phase * taps;

                                    for (i = 0; i < taps; i++) {
                                        float sinc_val = phase_table[i];

                                        sum_l += buffer_l[i] * sinc_val;
                                    }

                                    output[0] = sum_l;
                                    output[1] = sum_l;

                                    output += 2;
                                    out_frames++;
                                    ctx.time += ratio;
                                }
                            }

                        }
                    } else {
                        while (frames) {
                            while (frames && ctx.time >= phases) {
                                /* Push in reverse to make filter more obvious. */
                                if (!ctx.ptr)
                                    ctx.ptr = ctx.taps;
                                ctx.ptr--;

                                ctx.buffer_l[ctx.ptr + ctx.taps] =
                                ctx.buffer_l[ctx.ptr] = *input++;

                                ctx.buffer_r[ctx.ptr + ctx.taps] =
                                ctx.buffer_r[ctx.ptr] = *input++;

                                ctx.time -= phases;
                                frames--;
                            }

                            {
                                const float *buffer_l = ctx.buffer_l + ctx.ptr;
                                const float *buffer_r = ctx.buffer_r + ctx.ptr;
                                unsigned taps = ctx.taps;
                                while (ctx.time < phases) {
                                    unsigned i;
                                    float sum_l = 0.0f;
                                    float sum_r = 0.0f;
                                    unsigned phase = ctx.time >> ctx.subphase_bits;
                                    float *phase_table = ctx.phase_table + phase * taps;

                                    for (i = 0; i < taps; i++) {
                                        float sinc_val = phase_table[i];

                                        sum_l += buffer_l[i] * sinc_val;
                                        sum_r += buffer_r[i] * sinc_val;
                                    }

                                    output[0] = sum_l;
                                    output[1] = sum_r;

                                    output += 2;
                                    out_frames++;
                                    ctx.time += ratio;
                                }
                            }

                        }
                    }
                }

                rData->outputFrames = out_frames;
            };
        }

    }

    auto Sinc::init_table_kaiser(double cutoff, float *phase_table, int phases, int taps, bool calculate_delta) -> void {
        int i, j;
        double    window_mod = kaiser_window_function(0.0, ctx.kaiser_beta); /* Need to normalize w(0) to 1.0. */
        int           stride = calculate_delta ? 2 : 1;
        double     sidelobes = taps / 2.0;

        for (i = 0; i < phases; i++)
        {
            for (j = 0; j < taps; j++)
            {
                double sinc_phase;
                float val;
                int               n = j * phases + i;
                double window_phase = (double)n / (phases * taps); /* [0, 1). */
                window_phase        = 2.0 * window_phase - 1.0; /* [-1, 1) */
                sinc_phase          = sidelobes * window_phase;
                val                 = cutoff * _sinc(M_PI * sinc_phase * cutoff) *
                                      kaiser_window_function(window_phase, ctx.kaiser_beta) / window_mod;
                phase_table[i * stride * taps + j] = val;
            }
        }

        if (calculate_delta)
        {
            int phase;
            int p;

            for (p = 0; p < phases - 1; p++)
            {
                for (j = 0; j < taps; j++)
                {
                    float delta = phase_table[(p + 1) * stride * taps + j] -
                                  phase_table[p * stride * taps + j];
                    phase_table[(p * stride + 1) * taps + j] = delta;
                }
            }

            phase = phases - 1;
            for (j = 0; j < taps; j++)
            {
                float val, delta;
                double sinc_phase;
                int n               = j * phases + (phase + 1);
                double window_phase = (double)n / (phases * taps); /* (0, 1]. */
                window_phase        = 2.0 * window_phase - 1.0; /* (-1, 1] */
                sinc_phase          = sidelobes * window_phase;

                val                 = cutoff * _sinc(M_PI * sinc_phase * cutoff) *
                                      kaiser_window_function(window_phase, ctx.kaiser_beta) / window_mod;
                delta = (val - phase_table[phase * stride * taps + j]);
                phase_table[(phase * stride + 1) * taps + j] = delta;
            }
        }
    }

    auto Sinc::init_table_lanczos(double cutoff, float *phase_table, int phases, int taps, bool calculate_delta) -> void {
        int i, j;
        double    window_mod = lanzcos_window_function(0.0); /* Need to normalize w(0) to 1.0. */
        int           stride = calculate_delta ? 2 : 1;
        double     sidelobes = taps / 2.0;

        for (i = 0; i < phases; i++)
        {
            for (j = 0; j < taps; j++)
            {
                double sinc_phase;
                float val;
                int               n = j * phases + i;
                double window_phase = (double)n / (phases * taps); /* [0, 1). */
                window_phase        = 2.0 * window_phase - 1.0; /* [-1, 1) */
                sinc_phase          = sidelobes * window_phase;
                val                 = cutoff * _sinc(M_PI * sinc_phase * cutoff) *
                                      lanzcos_window_function(window_phase) / window_mod;
                phase_table[i * stride * taps + j] = val;
            }
        }

        if (calculate_delta)
        {
            int phase;
            int p;

            for (p = 0; p < phases - 1; p++)
            {
                for (j = 0; j < taps; j++)
                {
                    float delta = phase_table[(p + 1) * stride * taps + j] -
                                  phase_table[p * stride * taps + j];
                    phase_table[(p * stride + 1) * taps + j] = delta;
                }
            }

            phase = phases - 1;
            for (j = 0; j < taps; j++)
            {
                float val, delta;
                double sinc_phase;
                int n               = j * phases + (phase + 1);
                double window_phase = (double)n / (phases * taps); /* (0, 1]. */
                window_phase        = 2.0 * window_phase - 1.0; /* (-1, 1] */
                sinc_phase          = sidelobes * window_phase;

                val                 = cutoff * _sinc(M_PI * sinc_phase * cutoff) *
                                      lanzcos_window_function(window_phase) / window_mod;
                delta = (val - phase_table[phase * stride * taps + j]);
                phase_table[(phase * stride + 1) * taps + j] = delta;
            }
        }
    }

    auto Sinc::memalign_alloc(size_t boundary, size_t size) -> void* {
        void **place   = NULL;
        uintptr_t addr = 0;
        void *ptr      = (void*)malloc(boundary + size + sizeof(uintptr_t));
        if (!ptr)
            return NULL;

        addr           = ((uintptr_t)ptr + sizeof(uintptr_t) + boundary)
                         & ~(boundary - 1);
        place          = (void**)addr;
        place[-1]      = ptr;

        return (void*)addr;
    }

    auto Sinc::memalign_free(void *ptr) -> void {
        void **p = NULL;
        if (!ptr)
            return;

        p = (void**)ptr;
        free(p[-1]);
    }

    /* Modified Bessel function of first order.
 * Check Wiki for mathematical definition ... */
    inline auto Sinc::besseli0(double x) -> double {
        unsigned i;
        double sum            = 0.0;
        double factorial      = 1.0;
        double factorial_mult = 0.0;
        double x_pow          = 1.0;
        double two_div_pow    = 1.0;
        double x_sqr          = x * x;

        /* Approximate. This is an infinite sum.
         * Luckily, it converges rather fast. */
        for (i = 0; i < 18; i++) {
            sum += x_pow * two_div_pow / (factorial * factorial);

            factorial_mult += 1.0;
            x_pow *= x_sqr;
            two_div_pow *= 0.25;
            factorial *= factorial_mult;
        }

        return sum;
    }

    inline auto Sinc::_sinc(double val) -> double {
        if (fabs(val) < 0.00001)
            return 1.0;
        return sin(val) / val;
    }

    inline auto Sinc::kaiser_window_function(double index, double beta) -> double {
        return besseli0(beta * sqrtf(1 - index * index));
    }

    inline auto Sinc::lanzcos_window_function(double index) -> double {
        return _sinc(M_PI * index);
    }

    auto Sinc::freeData() -> void {

        if (ctx.main_buffer)
            memalign_free(ctx.main_buffer);

        ctx.main_buffer = nullptr;
        ctx.phase_table = nullptr;
        ctx.buffer_l = nullptr;
        ctx.buffer_r = nullptr;
        ctx.phase_bits = 0;
        ctx.subphase_bits = 0;
        ctx.subphase_mask = 0;
        ctx.taps = 0;
        ctx.ptr = 0;
        ctx.time = 0;
        ctx.subphase_mod = 0;
        ctx.kaiser_beta = 0;
    }

    Sinc::~Sinc() {
        freeData();
    }
}