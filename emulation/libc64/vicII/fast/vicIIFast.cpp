
#include "vicIIFast.h"
#include "serialization.cpp"
#include "scanline.cpp"
#include "silence.cpp"
#include "register.cpp"

namespace LIBC64 {  

VicIIFast::VicIIFast(System* system) : VicIIBase(system) {
    
    initMetaPattern();
	addMeta = false;

    std::fill_n(drawSprites, VIC_MAX_LINE_LENGTH << 1, nullptr);
}

auto VicIIFast::power() -> void {
    vcBase = vc = 0;
    rc = 0;
    color = 0x80;
    dmaDelay = 0;
	dataC = 0;
	dataG = 0;

	linePtr = frameBuffer;
    VicIIBase::power();
    setBorderDim();
}

auto VicIIFast::setMeta( bool state ) -> void {
    addMeta = state;
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
	if (!enableSequencer)
		return clockSilence();		
	
    if (irqLatchPending) {
        irqLatch |= irqLatchPending & 0x7f;
        updateIrq();
        irqLatchPending = 0;
    }    
    
    if (initVCounter) {
        if (debugger.storeSprites)
            debugger.resetSpriteStore();

        vCounter = 0;
        if (debugger.action == Emulator::Interface::DebuggerAction::Frame)
            oneTimeDebuggerAction();
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

        } else if (lineVCounter == vHeight) {
            visibleLine = false;

            system->videoRefresh(frameBuffer + firstVisiblePixel,
				hWidth, lineVCounter, VIC_MAX_LINE_LENGTH - hWidth
			);
			
            lineVCounter = 0;
        }

        if (visibleLine) {
            linePtr = frameBuffer + lineVCounter * VIC_MAX_LINE_LENGTH;
            lineVCounter++;
        }

        if (debugger.action == Emulator::Interface::DebuggerAction::Line) {
            if (debugger.stopLine == ~0 || debugger.stopLine == vCounter )
                oneTimeDebuggerAction();
        }

		flags = cycleTab[0];
		
		setRdy( spriteDma & getSpriteBA(flags) );   
		
    } else {
		
		flags = cycleTab[cycle];    

		if (isStartPhi2(flags)) {
			setRdy( badLine );		

		} else if (isUpdateVc( flags )) {
			if (badLine)
				rc = 0;

		} else if (isScanlineRender( flags )) {
			dmaSpritesOff();

			if (visibleLine)
                scanline();

		} else if (isScanlineRenderFin( flags )) {

			dmaSprites();

			setRdy( spriteDma & getSpriteBA( flags ) );

			dmaDelay = 0;

			if ( canSpriteSpriteCollisionIrq && spriteSpriteCollided) {
				canSpriteSpriteCollisionIrq = false;
			    if (!disallowSpriteSpriteCollisions)
				    updateIrq( Interrupt::MMC );
			}

			if ( canSpriteForegroundCollisionIrq && spriteForegroundCollided) {
				canSpriteForegroundCollisionIrq = false;
			    if (!disallowSpriteForegroundCollisions)
				    updateIrq( Interrupt::MBC );
			}
		} else if (isUpdateRc( flags )) {
			if (rc == 7) {
				vcBase = vc;
				idleMode = true;
			}

			if (!idleMode || badLine) {
				rc = (rc + 1) & 7; 
				idleMode = false;
			}
			
			setRdy( spriteDma & getSpriteBA( flags ) );
			
		} else if ( !isBgBA( flags ) ) {
			setRdy( spriteDma & getSpriteBA( flags ) );
		}
	}

    setLineInterrupt();
    
    if (unlikely(lpTrigger)) {
		xCounterLatch = getXpos( flags );	
        checkLightPen();
	}
}

inline auto VicIIFast::setRdy(bool _baLow) -> void {
	
	if (baLow != _baLow) {
		baLow = _baLow;
		system->setVicRdy( baLow );
	}
}

auto VicIIFast::triggerLightPen(bool state) -> void {
	// from CIA
	xCounterLatch = getXpos( flags );
	
	VicIIBase::triggerLightPen(state);
}

auto VicIIFast::triggerLightPen(bool state, uint8_t subCycle) -> void {
	
	if (!(subCycle & 2)	)			
		xCounterLatchBefore = getXpos( flags );
	
	VicIIBase::triggerLightPen( state, subCycle );
}

inline auto VicIIFast::applyBorder() -> void {
    
    uint8_t _col = colorReg[ 0x20 ];
    
    std::memset(linePtr + firstVisiblePixel, _col, borderLeft);          
    
    linePos = firstVisiblePixel + hWidth - borderRight;
    
    std::memset(linePtr + linePos, _col, borderRight);    		
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

auto VicIIFast::getCurrentLinePtr() -> uint8_t* {

    return linePtr + (cycle << 3);
}

auto VicIIFast::getCurrentFramePtr() -> uint8_t* {

    return linePtr + (cycle << 3);
}

inline auto VicIIFast::applyMeta() -> void {

    uint8_t* ptr = linePtr + firstVisiblePixel;    

    uint8_t* src;

    if (ntscBorder)
        src = badLine ? patternBadlineNtsc : patternLineNtsc;
    else
        src = badLine ? patternBadline : patternLine;

    for (unsigned i = 0; i < 420; i++) {
        *ptr |= *src++;
        ptr += 1;
    }  
}

auto VicIIFast::updateVideoSnapshot(DebuggerSnapshot& snap) -> void {
    auto& s = snap.vicII;
    s.vmli = 0;

    VicIIBase::updateVideoSnapshot(snap);
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
            patternBadline[i] = 1 << 4;

            if (((i + 2) & 4) == 0) {
                patternBadline[i] |= 2 << 4;                
                patternLine[i] = 2 << 4;
            }
        }
        
        else if (i >= 24 && i < 342) {
            patternBadline[i] = 3 << 4;
            
            if (((i + 2) & 4) == 0) {
                patternLine[i] = 2 << 4;
            }
        }
        
        else if (i >= 342) {
            if (((i + 2) & 4) == 0) {
                patternLine[i] = 2 << 4;                
                patternBadline[i] = 2 << 4;
            }
        }
    }

    for (unsigned i = 0; i < 420; i++) {

        if (i >= 8 && i < 352)
            patternBadlineNtsc[i] = 1 << 4;
            
        if ((i & 4) == 0) {
            patternLineNtsc[i] = 2 << 4;
        }
                
        if (i < 32) {
            if ((i & 4) == 0) {
                patternBadlineNtsc[i] |= 2 << 4;
              
            }
        }
        else if (i >= 32 && i < 352) {
            patternBadlineNtsc[i] |= 2 << 4;
        }
        else if (i >= 352) {
            if ((i & 4) == 0) {
                patternBadlineNtsc[i] = 2 << 4;
            }
        }
    }
}

}

