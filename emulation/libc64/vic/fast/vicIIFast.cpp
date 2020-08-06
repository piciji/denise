#include "vicIIFast.h"
#include "serialization.cpp"
#include "scanline.cpp"
#include "silence.cpp"
#include "register.cpp"
#include "../../../tools/thread.h"

namespace LIBC64 {  

VicIIFast* vicIIFast = nullptr;    

VicIIFast::VicIIFast() : VicIIBase() {
    
    initMetaPattern();
    
    std::thread worker( [this] {     
        
        std::chrono::milliseconds duration(5);
        std::mutex cvM;
        std::unique_lock<std::mutex> lk(cvM);
            
        idle = true;
        
        while(1) {
            ready = false;
            
            while (!ready.load()) {
                
                if (idle.load()) {
                    if (cv.wait_for(lk, duration, [this]() { return ready.load(); }))
                        break;  
                } //else
                    //std::this_thread::yield();        
            }
            
            scanline();
        }
    } );
    
    Emulator::setThreadPriorityRealtime( worker );
    
    worker.detach();
}

auto VicIIFast::power() -> void {
    vcBase = vc = 0;
    rc = 0;
    color = 0x80;
    dmaDelay = 0;
    idle = useThread ? false : true;
    if (useThread)
        cv.notify_one();
    VicIIBase::power();
    setBorderDim();        
}

auto VicIIFast::powerOff() -> void {
    idle = true;
}

auto VicIIFast::setThreading( bool state) -> void {
    useThread = state;
    if (system->powerOn && (this == vicII) ) {
        idle = useThread ? false : true;
        if (useThread)
            cv.notify_one();
    }
}

inline auto VicIIFast::setLineInterrupt() -> void {
    
	if (vCounter == irqLine) {
		if (!lineIrqMatched) {
			updateIrq( Interrupt::Raster );
			lineIrqMatched = true;
		}
		return;
	}
	
	lineIrqMatched = false;		
}

auto VicIIFast::clock() -> void {

    if (irqLatchPending) {
        irqLatch |= irqLatchPending & 0x7f;
        updateIrq();
        irqLatchPending = 0;
    }    
    
    if (initVCounter) {
        vCounter = 0;
        initVCounter = false;
        lpLatched = false;
        if (!lpPin)
            triggerLightPen(false, 3);

        vcBase = vc = 0;
        allowBadlines = false;
    } 
    
    if (++cycle == lineCycles) {	
		cycle = 0; 

        if (vCounter == 0xf7)
            allowBadlines = false;                    

        if (++vCounter == lines) {
            vCounter -= 1;
            initVCounter = true;
        } else {
            if (!allowBadlines && (vCounter == 0x30) && den)
                allowBadlines = true;            
        }
        
        badLine = allowBadlines && (yScroll == (vCounter & 7));  
        
        if (badLine)
            idleMode = false;        

        if (vCounter == vStart) {
            updateBorderData();
            visibleLine = true;
            if (lineCallback.finishVblank)
                vblankCallback();
            
        } else if (lineVCounter == vHeight) {
            visibleLine = false;

            if (leftLineAnomaly.mode)
                insertVerticalLineAnomaly(lineCallback.line, lineVCounter);

            videoRefresh(frameBuffer + firstVisiblePixel, hWidth, lineVCounter, VIC_MAX_LINE_LENGTH - hWidth);
            lineVCounter = 0;
            
        } else if (lineCallback.use && (lineVCounter == lineCallback.line)) {
            if (leftLineAnomaly.mode)
                insertVerticalLineAnomaly( 0, lineVCounter );
            
            midScreenCallback();
        }

        if (visibleLine) {
            linePtr = frameBuffer + lineVCounter * VIC_MAX_LINE_LENGTH;
            lineVCounter++;
        }
        setRdy( spriteBa[8][ cycle ] );
        
    } else if (cycle == 11) {
                     
        setRdy( badLine );
        cAccessArea = true;
    } else if (cycle == 13) {
        
        if (badLine)
            rc = 0;
            
    } else if (cycle == 20) {
        dmaSpritesOff();
        
        if (visibleLine) {            
            if (useThread)
                ready.store(1);
            else
                scanline();
        }
                        
    } else if (cycle == 54) {    
        cAccessArea = false;
        dmaSprites();
        setRdy( spriteBa[8][ cycle ] );
        
        if (useThread && visibleLine)
            while ( ready.load() ) {}   
                            
        dmaDelay = 0;
        
        if ( spriteSpriteCollided)
            updateIrq( Interrupt::MMC );
	
        if ( spriteForegroundCollided)
            updateIrq( Interrupt::MBC );
        
    } else if (cycle == 57) {
        if (rc == 7) {
            vcBase = vc;
            idleMode = true;
        }

        if (!idleMode || badLine) {
            rc = (rc + 1) & 7; 
            idleMode = false;
        }
    } else if (!cAccessArea) {
        setRdy( spriteBa[8][ cycle ] );
    }      	
    
    setLineInterrupt();
    
    if (lpTrigger)
        checkLightPenNew();
}

inline auto VicIIFast::applyBorder() -> void {
    
    uint8_t _col = colorReg[ 0x20 ];
    
    std::fill_n(linePtr + firstVisiblePixel, borderLeft, _col);          
    
    linePos = firstVisiblePixel + hWidth - borderRight;
    
    std::fill_n(linePtr + linePos, borderRight, _col);    		
}

auto VicIIFast::setBorderDim() -> void {

    if (cSel) {
        borderLeft = ntscBorder ? 56 : 46;
        borderRight = ntscBorder ? 44 : 40;

    } else {
        borderLeft = ntscBorder ? 63 : 53;
        borderRight = ntscBorder ? 53 : 49;
    }
}

inline auto VicIIFast::calcSpriteX(Sprite* spr) -> void {
	    
	if (!ntscBorder) {
		if (spr->x < 0x194 )
			spr->xPos = firstVisiblePixel + spr->x + 22;
		else
			spr->xPos = spr->x - 0x194 + 8;
	} else {
		if (spr->x < 0x19c)
			spr->xPos = firstVisiblePixel + spr->x + 32;
		else
			spr->xPos = spr->x - 0x19c + 8;
	}			
}

inline auto VicIIFast::calcSpriteMask(Sprite* spr) -> void {
	
	if (spr->xPos >= VIC_MAX_LINE_LENGTH)
		spr->mask = 0;
	else {
		spr->mask = 0xffffff;

		unsigned diff = VIC_MAX_LINE_LENGTH - spr->xPos;

		if (spr->expandX)
			diff >>= 1;
		
		if (diff < 24 ) {
			spr->mask = 0;

			unsigned shift = 23;
			while (diff--) {
				spr->mask |= 1 << shift--;							
			}
		}
	}
}

auto VicIIFast::checkLightPenNew() -> void {

    if (lpPhi1)
        xCounter = xLookupPtrPhi1[cycle];
    else
        xCounter = xLookupPtrPhi2[cycle];
    

    lpTrigger = false;
    lpLatched = true;

    // last line doesn't latch lpx or lpy.
    if (vCounter == (lines - 1))
        return;

    // 2 adjacent pixel [4,5] [6,7] give the same value for lpx, because of
    // the last bit is shifted out. it's a division by 2.
    // this code fires between the half cycles in pixel 4.
    // for the 8565 the latch happens by pixel 7, btw. pixel 6 would give the same.
    // for the 6569 the latch happens one pixel later, but it's already the next
    // two pixel block. 

    lpxBefore = lpx;
    lpyBefore = lpy;
    
    lpx = xCounter >> 1;
    lpx += lpTriggerDelay;

    // vCounter is incremented in second half cycle of last line cycle.
    // if this latch happens in last pixel (like the 85xx) vCounter is already
    // incremented. I don't know if a latch in second to last pixel recognizes 
    // incremented vCounter too. From a CIA point of view it happens only in last or
    // first pixel of next cycle. From a Light Gun(Pen) point of view it could happen
    // an any cycle pixel, but vCounter increments in non visible area... means no problem

    if (!lpPhi1 && (cycle == (lineCycles - 1)))
        lpy = (vCounter + 1) & 0xff;
    else
        lpy = vCounter & 0xff;

    if (lpPhi1)
        updateIrq(Interrupt::LP);
    else
        // cpu mustn't recognize it this cycle, but next
        irqLatchPending |= 0x80 | (1 << Interrupt::LP);
}

auto VicIIFast::getCurrentLinePtr() -> uint16_t* {

    return linePtr + (cycle << 3);
}

inline auto VicIIFast::applyMeta() -> void {

    uint8_t* ptr = (uint8_t*) (linePtr + firstVisiblePixel);
    ptr += 1;

    uint8_t* src;

    if (ntscBorder)
        src = badLine ? patternBadlineNtsc : patternLineNtsc;
    else
        src = badLine ? patternBadline : patternLine;

    for (unsigned i = 0; i < 420; i++) {
        *ptr = *src++;
        ptr += 2;
    }  
}

auto VicIIFast::initMetaPattern() -> void {
    patternBadline = new uint8_t[420];
    patternLine = new uint8_t[420];
    patternBadlineNtsc = new uint8_t[420];
    patternLineNtsc = new uint8_t[420];
    
    std::memset(patternBadline, 0, 420);
    std::memset(patternLine, 0, 420);
    std::memset(patternBadlineNtsc, 0, 420);
    std::memset(patternLineNtsc, 0, 420);
        
    for (unsigned i = 0; i < 420; i++) {

        if (i < 24) {
            patternBadline[i] = 1;

            if (((i + 2) & 4) == 0) {
                patternBadline[i] |= 2;                
                patternLine[i] = 2;
            }
        }
        
        else if (i >= 24 && i < 342) {
            patternBadline[i] = 3;
            
            if (((i + 2) & 4) == 0) {
                patternLine[i] = 2;
            }
        }
        
        else if (i >= 342) {
            if (((i + 2) & 4) == 0) {
                patternLine[i] = 2;                
                patternBadline[i] = 2;
            }
        }
    }

    for (unsigned i = 0; i < 420; i++) {

        if (i >= 8 && i < 352)
            patternBadlineNtsc[i] = 1;
            
        if ((i & 4) == 0) {
            patternLineNtsc[i] = 2;
        }
                
        if (i < 32) {
            if ((i & 4) == 0) {
                patternBadlineNtsc[i] |= 2;
              
            }
        }
        else if (i >= 32 && i < 352) {
            patternBadlineNtsc[i] |= 2;
        }
        else if (i >= 352) {
            if ((i & 4) == 0) {
                patternBadlineNtsc[i] = 2;
            }
        }
    }
}

}

