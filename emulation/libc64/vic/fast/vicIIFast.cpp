#include "vicIIFast.h"
#include "serialization.cpp"
#include "scanline.cpp"
#include "silence.cpp"

namespace LIBC64 {  

VicIIFast* vicIIFast = nullptr;    

VicIIFast::VicIIFast() : VicIIBase() {
    
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
                } else
                    std::this_thread::yield();        
            }
            
            scanline();
        }
    } );
    
    worker.detach();
}

auto VicIIFast::power() -> void {
    vcBase = vc = 0;
    rc = 0;
    color = 0x80;
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

        if (++vCounter == (ntsc ? 263 : 312)) {
            vCounter -= 1;
            initVCounter = true;
        }

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
    } else if (cycle == 10) {
        if (!allowBadlines && (vCounter == 0x30) && den)
            allowBadlines = true;
    
        badLine = allowBadlines && (yScroll == (vCounter & 7));
        
        setRdy( badLine );
        cAccessArea = true;
    } else if (cycle == 15) {
        dmaSpritesOff();
        
        if (visibleLine) {            
            if (useThread)
                ready.store(1);
            else
                scanline();
        }                               
        
    } else if (cycle == 53) {
        setRdy( false );
        cAccessArea = false;
        dmaSprites();    
        if (useThread)
            while ( ready.load() ) {}
        
    } else if (!cAccessArea) {
        setRdy( spriteBa[8][ cycle ] );
    }
    
    setLineInterrupt();
    
    if (lpTrigger)
        checkLightPenNew();
}


auto VicIIFast::applyBorder() -> void {
    
    uint8_t _col = colorReg[ 0x20 ];
    
    std::fill_n(linePtr + firstVisiblePixel, borderLeft, _col);          
    
    linePos = firstVisiblePixel + hWidth - borderRight;
    
    std::fill_n(linePtr + linePos, borderRight, _col);    
}

auto VicIIFast::setBorderDim() -> void {

    if (cSel) {
        borderLeft = ntsc ? 56 : 46;
        borderRight = ntsc ? 44 : 40;

    } else {
        borderLeft = ntsc ? 63 : 53;
        borderRight = ntsc ? 53 : 49;
    }
}

auto VicIIFast::readReg( uint8_t addr ) -> uint8_t {
    
    addr &= 0x3f;
    
    if (addr == 0x1e) {
        uint8_t value = spriteSpriteCollided;
        spriteSpriteCollided = 0;
        return value;
    } else if (addr == 0x1f) {
        uint8_t value = spriteForegroundCollided;
        spriteForegroundCollided = 0;
        return value;
    }
    
    return VicIIBase::readReg( addr );
}


auto VicIIFast::checkLightPenNew() -> void {

    if (lpPhi1)
        xCounter = xLookupPtrPhi1[cycle];
    else
        xCounter = xLookupPtrPhi1[cycle];
    

    lpTrigger = false;
    lpLatched = true;

    // last line doesn't latch lpx or lpy.
    if (vCounter == (ntsc ? 262 : 311))
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

}

