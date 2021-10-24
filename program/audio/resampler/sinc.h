
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

#pragma once

#include <functional>
#include <math.h>

#include "data.h"

namespace Resampler {

    struct Sinc {

        ~Sinc();

        enum Quality { RESAMPLER_QUALITY_LOWEST, RESAMPLER_QUALITY_LOWER, RESAMPLER_QUALITY_HIGHER, RESAMPLER_QUALITY_HIGHEST, RESAMPLER_QUALITY_NORMAL, RESAMPLER_QUALITY_DONTCARE };

        struct {
            float *main_buffer = nullptr;
            float *phase_table = nullptr;
            float *buffer_l = nullptr;
            float *buffer_r = nullptr;
            unsigned phase_bits = 0;
            unsigned subphase_bits = 0;
            unsigned subphase_mask = 0;
            unsigned taps = 0;
            unsigned ptr = 0;
            uint32_t time = 0;
            float subphase_mod = 0.0;
            float kaiser_beta = 0.0;
        } ctx;

        auto setData( Data* rData ) -> void {
            this->rData = rData;
        }

        auto process( ) -> void {
            resampler(  );
        }

        auto build() -> void;


        auto reset(float ratio, unsigned inChannels, Quality _quality) -> void {

            rData->ratio = ratio;
            rData->inChannels = inChannels;
            this->quality = _quality;

            freeData();
            build();
        }

        auto freeData() -> void;

        static inline auto _sinc(double val) -> double;

        static inline auto kaiser_window_function(double index, double beta) -> double;

        static inline auto lanzcos_window_function(double index) -> double;

        auto init_table_kaiser(double cutoff, float *phase_table, int phases, int taps, bool calculate_delta) -> void;
        auto init_table_lanczos(double cutoff, float *phase_table, int phases, int taps, bool calculate_delta) -> void;

        static auto memalign_alloc(size_t boundary, size_t size) -> void*;
        static auto memalign_free(void *ptr) -> void;
        static auto besseli0(double x) -> double;

        std::function<void ()> resampler;
        Data* rData = nullptr;
        Quality quality;
    };
}