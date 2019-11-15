
#pragma once

#include "serializer.h"
#include <cstdlib>

namespace Emulator {
    
/**
 * purpose: quadrature encoder
 * a modern mouse periodically sends position updates. (digital)
 * most retro devices sends quadrature signals (analog) 
 * in a quadrature mouse, the output would be a constant flow of information
 * with the frequency of the pulses indicating how far the mouse has moved.
 * a modern pc updates input at a slower rate. OS layer, TFT screen latency
 * are main reasons for this. input polling more than one time per frame
 * isn't usefull.
 * doesn't matter which scanline emulated software is polling mouse movement,
 * we simply have only one value for the whole frame. this way we can't simulate
 * the analog character of quadrature signals. 
 * we need a way to calculate intermediate values when software is polling mid frame.
 * 
 * 1. calculate time delta and movement delta between input changes detectd by host
 * 2. translate this time delta in emulated cycles
 * 3. calculate the amount of emulated cycles for a movement of one pixel by knowing
 *    the movement delta and the emulated cycles for this delta from step 2.
 * 4. when emulation polls mouse movement mid frame we count the passed cycles.
 * 5. by knowing the amount of cycles for one pixel movement we can advance by the amount
 *    of pixel until current emulation cycle.
 * 
 * of course we can't detect speed and direction changes as fast as the real system.
 * that is the above mentioned limitation of modern machines we can't override 
 * by application software.
 * 
 */    
    
struct QuadratureEncoder {
    
    // final values for emulation
    int16_t X;
    int16_t Y;

protected:        
    unsigned cyclesPerSecond;
    unsigned cyclesPerFrame;
    
    // host coordinates and timestamp used last call
    int16_t latestHostX;
    int16_t latestHostY;
    unsigned latestPoll;
    
    // direction: 1 / -1
    int stepX;
    int stepY;
    
    // cycles which have to be passed for the movement of one pixel
    unsigned cyclesOneX; 
    unsigned cyclesSummedX;
    unsigned cyclesOneY;
    unsigned cyclesSummedY;
    
    unsigned cycleCounterX;
    unsigned cycleCounterY;
    
    // minimum cycle limit for the movement of one pixel
    unsigned limit;
    
public:   
    
    auto poll( int16_t hostX, int16_t hostY, unsigned pollTimestamp, unsigned deltaCycles ) -> void {
                    
        cycleCounterX += deltaCycles;
        cycleCounterY += deltaCycles;
        // advance to current cycle position or latest known host coordinates, when happens sooner
        // this way we can provide updated values multiple times per frame.
        updateX( latestHostX, cycleCounterX );        
        updateY( latestHostY, cycleCounterY );
        
        if (latestPoll == 0) { // first time only
            latestPoll = pollTimestamp;
            X = latestHostX = hostX;
            Y = latestHostY = hostY;
            return;
        }
        
        // should be ~20 ms for pal speed
        unsigned long deltaTime = pollTimestamp - latestPoll;
        
        if ( deltaTime == 0 || (hostX == latestHostX && hostY == latestHostY) )
            // host hasn't recognize any changes
            return;  
        
        // memory
        latestHostX = hostX;
        latestHostY = hostY;
        latestPoll = pollTimestamp;
        
        // now we need the amount of emulated cycles for this time delta.        
        // we solve this with a proportion: 
        // when one second (1.000.000 micro seconds) = machine cycles per second
        // then delta time                           = x emulated cycles
        // x = delta * machine cycles per second / 1.000.000 (micro seconds)
        unsigned emuCyclesToDo = (float)deltaTime * ((float)cyclesPerSecond / 1000000.0);
        
        if ( emuCyclesToDo > ( 2 * cyclesPerFrame ) ) 
            // no movement for more than 2 frames
            emuCyclesToDo = 2 * cyclesPerFrame;
        
        // calculates the difference between emulated (quadrature) and host coordinates
        int16_t diffX = hostX - X;
        int16_t diffY = hostY - Y;
        
        if (diffX != 0) {
            // calculate the direction to advance emulated coordinates to host coordinates.
            // a direction change of movement will be recognized here.
            stepX = diffX > 0 ? 1 : -1;
            // next we need to calculate the amount of emulated cycles for one pixel
            // movement by following proportion:
            // when: delta movement (xDiff) = emuCyclesToDo
            // then: 1 pixel of movement    = x emulated cycles
            // x = emuCyclesToDo / xDiff
            cyclesOneX = emuCyclesToDo / std::abs( diffX );

            cyclesSummedX = 0;
            
            cycleCounterX = 0;
            
        } else {
            stepX = 0;
            cyclesOneX = limit;
        }
        // same for y
        if (diffY != 0) {
            stepY = diffY > 0 ? -1 : 1;
            cyclesOneY = emuCyclesToDo / std::abs( diffY );
            cyclesSummedY = 0;
            cycleCounterY = 0;
            
        } else {
            stepY = 0;
            cyclesOneY = limit;
        }
        
        // check for underflows and correct it by keeping proportion between X and Y 
        if (cyclesOneX < limit) {
            if (cyclesOneX)
                cyclesOneY = cyclesOneY * limit / cyclesOneX;

            cyclesOneX = limit;
        }
        
        if (cyclesOneY < limit) {
            if (cyclesOneY)
                cyclesOneX = cyclesOneX * limit / cyclesOneY;

            cyclesOneY = limit;
        }
    }
    
    auto updateX( int16_t limitX, unsigned cyclesLimitX ) -> void {
        
        // lets advance coordinates until one of following conditions happen
        // 1. reach limitX (the latest known host position)
        // 2. catch up to current cycle position
        
        while (((limitX ^ X) & 0xffff) && (cyclesSummedX <= cyclesLimitX) ) {            
            // add / subtract single pixels
            X += stepX;
            // add the pre calculated cycle amount for the movement of one pixel.
            // each time host detects a change in movement, we update this precalculation
            // to take speed and direction changes into account.
            cyclesSummedX += cyclesOneX;
        }
    }
    
    auto updateY( int16_t limitY, unsigned cyclesLimitY ) -> void {
        
        while (((limitY ^ Y) & 0xffff) && (cyclesSummedY <= cyclesLimitY) ) {
            Y -= stepY;
            cyclesSummedY += cyclesOneY;
        }
    }
    
    auto setCyclesPerSecond( unsigned cycles ) -> void {
        
        this->cyclesPerSecond = cycles;
    }
    
    auto setCyclesPerFrame( unsigned cycles ) -> void {
        
        this->cyclesPerFrame = cycles;
        this->limit = cycles / 31 / 2;
    }   
    
    auto reset() -> void {
 
        latestHostX = latestHostY = 0;
        stepX = stepY = 0;
        X = Y = 0;
        latestPoll = 0;
        cycleCounterX = cycleCounterY = 0;
    }
    
    auto serialize(Serializer& s) -> void {
        
        s.integer( X );
        s.integer( Y );
        s.integer( cycleCounterX );
        s.integer( cycleCounterY );
        s.integer( cyclesOneX );
        s.integer( cyclesOneY );
        s.integer( cyclesPerFrame );
        s.integer( cyclesPerSecond );
        s.integer( cyclesSummedX );
        s.integer( cyclesSummedY );
        s.integer( latestHostX );
        s.integer( latestHostY );
        s.integer( latestPoll );
        s.integer( limit );
        s.integer( stepX );
        s.integer( stepY );
    }
    
};
    
}
