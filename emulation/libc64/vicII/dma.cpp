
#include "vicII.h"
#include "../system/system.h"

#define DEBUG_SCROLL_MAX 30

namespace LIBC64 {    

auto VicIICycle::clockLogged() -> void {
    clockCycle<true>();
}

auto VicIICycle::clock() -> void {
    clockCycle<false>();
}

auto VicIICycle::clockMaybeLogged() -> void {
    if (debugger.dmaLog)    clockCycle<true>();
    else                    clockCycle<false>();
}

template<bool logDma> inline auto VicIICycle::clockCycle() -> void {
	
	if (!enableSequencer)
		return clockSilence();	

	// fetch sprite data of second half cycle this late (this way a possible register access will not be missed)
	// a value from a VIC register access will be read back in sprite fetch logic, if BUS is not available for VIC or if
	// sprite DMA is off. In such cases the VIC reads 0xff or value from a possible register access in this cycle.
	// under normal conditions the VIC reads sprite data and the CPU is waiting (BA + AEC).
	fetchSprPhi2( flags );
	
    advanceCycle();
	
	flags = cycleTab[cycle];
    updateBadLine();
    setLineInterrupt();

    sequencer( flags );  
	
	if (isSprDisp( flags ))
		spriteDisplayCheck();
	else if (isSprMcBase( flags ))
		spriteUpdateBase();	
	else if (isSprExp( flags ))
		spriteExpand();

    lastReadPhi1 = fetchPhi1<logDma>( flags );
	
	if (isUpdateVc( flags ))
		updateVc();	
	else if (isUpdateRc( flags ))
		updateRc();
	
    // copy state of ECM / BMM directly before a possible "write" in order
    // to delay it one cycle for DMA fetch logic
    modeEcmBmmDma = modeEcmBmm;   
	
	lastBusPhi2 = 0xff;
}    

inline auto VicIICycle::advanceCycle() -> void {    

    if (irqLatchPending) {
        irqLatch |= irqLatchPending & 0x7f;
        updateIrq();
        irqLatchPending = 0;
    } 
	
	// a written DEN bit in last cycle of 0x30 is recognized
    if ( !allowBadlines && (vCounter == 0x30) && den )
		allowBadlines = true;
	
	if(initVCounter) {
	    if (debugger.storeSprites)
	        debugger.resetSpriteStore();
        vCounter = 0;
        initVCounter = false;
		lpLatched = false;	
		// retrigger happens in last pixel of second cycle for all Vic types,
        // if lp line is held low in beginning of cycle
        if (!lpPin)
            triggerLightPen( false, 3 );
        
		vcBase = vc = 0;
		refreshCounter = 0xff;
		allowBadlines = false;
	    if (debuggerAction == Emulator::Interface::DebuggerAction::Frame)
	        oneTimeDebuggerAction();
    }
    
	if (++cycle == lineCycles) {
		cycle = 0;

		// Note: line complete but vcounter is not incremented at this point
		if (vCounter == 0xf7)
			allowBadlines = false;		
		
		if (++vCounter == lines ) {
			// last line is not reseted this cycle but next
			vCounter -= 1;
			initVCounter = true;
		} else {
			// when vCounter increments to 0x30 we check for DEN
            // the above check in this function would miss the first cycle in line
			if ( !allowBadlines && (vCounter == 0x30) && den )
				allowBadlines = true;
		}

	    if (debugger.dmaView) {
	        nextLineWithDmaView();
	    } else {
	        setLineBuffer();

	        if ( vCounter == vStart ) {
	            updateBorderData();
	            // we buffer all pixel data in non blanking area, of course a CRT
	            // can not display the whole non blanking area.
	            // cropping is done later and not within Vic emulation
	            visibleLine = true; // non v-blank

	        } else if ( lineVCounter == vHeight ) {
	            visibleLine = false; // v-blank

	            // push out the frame to host
	            // we crop the h-blanking area before
	            system->videoRefresh( frameBuffer + firstVisiblePixel,
                    hWidth, lineVCounter, VIC_MAX_LINE_LENGTH - hWidth
                );
	            lineVCounter = 0;
	        }
	    }

	    if (debuggerAction == Emulator::Interface::DebuggerAction::Line)
	        oneTimeDebuggerAction();
	}

    sprite0DmaLateBA = false;
}

auto VicIICycle::nextLineWithDmaView() -> void {
    visibleLine = true;
	setLineBuffer();
	uint8_t usage;
	for (int c = 0; c < lineCycles; c++ ) {
	    usage = debugger.dma[c].usage;
	    std::memset( debugger.frameLine, usage, 4 );

	    usage = debugger.dma[c].usageCpu;
	    if (usage != DMA_CHARACTER && usage != DMA_SPR_DATA)
	        usage = DMA_CPU;

	    std::memset( debugger.frameLine + 4, usage, 4 );

	    debugger.frameLine += 8;
	}

	if (initVCounter) {
	    auto& _crop = debugger.crop;
	    unsigned _pitch = VIC_MAX_LINE_LENGTH - hWidth;
	    uint8_t* _ptr = frameBuffer + firstVisiblePixel;
	    unsigned _height = vHeight;
	    unsigned _width = hWidth;
	    _crop.apply( _ptr, _width, _height, _pitch);
	    unsigned width = lineCycles * 8;

	    int offsetX = 0;
	    int offsetY = 0;
	    int offsetFrameX = 0;
	    int offsetFrameY = 0;
	    bool endDmaView = false;

	    if (debugger.scrollDirection != 0) {
	        if (((debugger.scrollDirection == 1) && (debugger.scrollCounter < DEBUG_SCROLL_MAX))
            || ((debugger.scrollDirection == -1) && debugger.scrollCounter)) {
	            if (_crop.latest.width < width) {
	                offsetX = width - _crop.latest.width;
	                offsetX = offsetX - (offsetX * debugger.scrollCounter) / DEBUG_SCROLL_MAX;
	            }

	            if (_crop.latest.height < lineVCounter) {
	                offsetY = lineVCounter - _crop.latest.height;
	                offsetY = offsetY - (offsetY * debugger.scrollCounter) / DEBUG_SCROLL_MAX;
	            }

	            unsigned lastLeft = firstVisiblePixel + _crop.latest.left;
	            offsetFrameX = lastLeft - ((lastLeft * debugger.scrollCounter) / DEBUG_SCROLL_MAX);

	            unsigned lastTop = vStart + _crop.latest.top;
	            offsetFrameY = lastTop - ((lastTop * debugger.scrollCounter) / DEBUG_SCROLL_MAX);

	            debugger.scrollCounter += debugger.scrollDirection;
            } else {
                if (debugger.scrollDirection == -1)
                    endDmaView = true;

                debugger.scrollDirection = 0;
            }
	    }

	    if (!endDmaView) {
	        system->videoRefresh( frameBuffer + (offsetFrameY * VIC_MAX_LINE_LENGTH) + offsetFrameX,
                width - offsetX, lineVCounter - offsetY, VIC_MAX_LINE_LENGTH - (width - offsetX)
            );
	    }

	    lineVCounter = 0;

	    if (endDmaView) {
	        debugger.dmaView = false;
	        debugger.dmaLog = debugger.requestDmaLog;
	        visibleLine = false;
	    }
	}
	debugger.frameLine = debugger.dmaFrame + lineVCounter * VIC_MAX_LINE_LENGTH;
}

inline auto VicIICycle::clearCollisions() -> void {
	// collisions in the second half of the cycle are lost
	if (clearCollision == 0x1e) {
		spriteSpriteCollided = 0;
		canSpriteSpriteCollisionIrq = true;
	} else {
		/* if (clearCollision == 0x1f) */
		spriteForegroundCollided = 0;
		canSpriteForegroundCollisionIrq = true;
	}

	clearCollision = 0;
}

// cycle: 16-2
auto VicIICycle::spriteUpdateBase() -> void {
    Sprite* spr;
	
    for( uint8_t i = 0; i < 8; i++ ) {
		spr = &sprite[i];
        
        if (spr->expandYFlop) {
            spr->mcBase = spr->mc;

            if (spr->mcBase == 63)
				spriteDma &= ~(1 << i);     
        }
    }    
}
// cycle: 55-1 + 56-1
inline auto VicIICycle::spriteDmaCheck() -> void {
    Sprite* spr;
	
    for( uint8_t i = 0; i < 8; i++ ) {
        spr = &sprite[i];
        
        if (spr->enabled && !sprHasDma(i) && ( (vCounter & 0xff) == spr->y ) ) {
            spriteDma |= 1 << i;
            
            spr->mcBase = 0;
            spr->expandYFlop = true;
            
            if (spr == sprite0)
				sprite0DmaLateBA = true;
        }
    }
}
// cycle: 56-2
auto VicIICycle::spriteExpand() -> void {
	Sprite* spr;
	
    for( uint8_t i = 0; i < 8; i++ ) {
		spr = &sprite[i];

        if (sprHasDma(i) && spr->expandY)
            spr->expandYFlop ^= 1;
    }    
}
// cycle: 58-1
auto VicIICycle::spriteDisplayCheck() -> void {
	Sprite* spr;    
	
    for( uint8_t i = 0; i < 8; i++ ) {
		spr = &sprite[i];
        
        spr->mc = spr->mcBase;
        
        if (sprHasDma(i)) {
            if (spr->enabled && ( (vCounter & 0xff) == spr->y ) )  
                spritePending |= 1 << i;
        } else 
            spritePending &= ~(1 << i);
    }    
}

// cycle: 14-2
auto VicIICycle::updateVc() -> void {
	vc = vcBase;
	vmli = 0;
	if (badLine)
		rc = 0;
}
// cycle: 58-2
auto VicIICycle::updateRc() -> void {
	if (rc == 7) {
		vcBase = vc;
		idleMode = true;            
	} 
	if (!idleMode || badLine) {
		rc = (rc + 1) & 7;
		idleMode = false;            
	}		
}

inline auto VicIICycle::updateBAState( uint32_t flags ) -> void {
    bool _baLow = baLow;
    
	if (badLine)
		idleMode = false;	
    
    if (isBgBA(flags) ) // 11 <= cycle <= 53
        baLow = badLine; // for "c" accesses, no sprites pos
        
    else
		baLow = spriteDma & getSpriteBA( flags );       
		
    if (_baLow != baLow)
        system->setVicRdy( baLow ); //update cpu rdy line
	
	if (baLow) {
		if(aecDelay)
			aecDelay--;
	} else
		aecDelay = 4;
}

inline auto VicIICycle::updateBadLine() -> void {
			
	badLine = allowBadlines && (yScroll == (vCounter & 7));
	
	idleModeTemp = idleMode;
}

inline auto VicIICycle::borderControl() -> void {
    
    if (den && (vCounter == borderTop)) {
        vFlipFlop = vFlipFlopShadow = false;
    
	} else if (vCounter == borderBottom) {
		if (!vFlipFlopShadow)
        	vFlipFlopShadow = true;
	}
    
    if (cycle == 0)
        vFlipFlop = vFlipFlopShadow;
}

template<bool first> auto VicIICycle::borderLeft(  ) -> void {
	
	if ((cSel && first) || (!cSel && !first)) {
		if (vCounter == borderBottom) {
			if (!vFlipFlopShadow)
				vFlipFlopShadow = true;
		}
		
		vFlipFlop = vFlipFlopShadow;
		if (!vFlipFlop) {
            hFlipFlop = 0;
        }			
	}
}

template<bool first> auto VicIICycle::borderRight( ) -> void {
	
	if ((!cSel && first) || (cSel && !first))
		hFlipFlop = 1;
}

}
