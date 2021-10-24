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

#pragma once 

#include <functional>
#include <math.h>

#include "data.h"

namespace Resampler {
/* Convoluted Cosine Resampler */
struct Cosine {    
    
    struct FrameBuffer {
        float l;
        float r;
    };
    
    static inline auto cc_int(float x, float b) -> float {
        float val = x * b;

        val = val * (1 - 0.25 * val * val * (3.0 - val * val));

        return (val > 0.5) ? 0.5 : (val < -0.5) ? -0.5 : val;
    }

    static inline auto add_to(const FrameBuffer* source, FrameBuffer* target, float ratio) -> void {
        target->l += source->l * ratio;
        target->r += source->r * ratio;
    }
    
    auto setData( Data* rData ) -> void {
        this->rData = rData;
    }
    
    auto process( ) -> void {
        
        resampler(  );
    }
    
    auto buildSSE() -> void;
 
    auto build() -> void;
    
    auto reset(float ratio, unsigned inChannels) -> void {       

        for (int i = 0; i < 4; i++) {
            buffer[i].l = 0.0;
            buffer[i].r = 0.0;
        }
        
        rData->ratio = ratio;
        rData->inChannels = inChannels;
     
#ifdef __SSE__        
        if (ratio < 0.2)
            buildSSE();
        else
#endif            
            build();
    }
    
    float distance;    
    FrameBuffer buffer[4];
    std::function<void ()> resampler;
    Data* rData = nullptr;
};

}
