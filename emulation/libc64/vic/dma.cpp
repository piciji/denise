
#include "vicII.h"

namespace LIBC64 {    

auto VicIICycle::clock() -> void {
    advanceCycle();
    updateBadLine();
    setLineInterrupt();

    sequencer();  

	//ntscXLock -> true: NTSC(PAL-N), false: PAL, OLD-NTSC
	//ntscBorder -> true: NTSC(PAL-N), OLD-NTSC false: PAL
	
    switch (cycle) {
        case 0: if (ntscXLock)  { fetchSpriteS1<3>(); fetchSpriteS2(3); } 
                else			{ fetchSpriteP<3>(); fetchSpriteS0(3); }
                break;
        case 1: if (ntscXLock)  { fetchSpriteP<4>(); fetchSpriteS0(4); }
                else			{ fetchSpriteS1<3>(); fetchSpriteS2(3); }
                break;
        case 2: if (ntscXLock)  { fetchSpriteS1<4>(); fetchSpriteS2(4); }
                else			{ fetchSpriteP<4>(); fetchSpriteS0(4); }
                break;
        case 3: if(ntscXLock)   { fetchSpriteP<5>(); fetchSpriteS0( 5 ); }
                else			{ fetchSpriteS1<4>(); fetchSpriteS2( 4 ); }
                break;
        case 4: if(ntscXLock)   { fetchSpriteS1<5>(); fetchSpriteS2(5); }
                else			{ fetchSpriteP<5>(); fetchSpriteS0(5); }
                break;
        case 5: if(ntscXLock)   { fetchSpriteP<6>(); fetchSpriteS0(6); }
                else			{ fetchSpriteS1<5>(); fetchSpriteS2(5); }
                break;
        case 6: if(ntscXLock)   { fetchSpriteS1<6>(); fetchSpriteS2(6); }
                else			{ fetchSpriteP<6>(); fetchSpriteS0(6); }
                break;
        case 7: if(ntscXLock)   { fetchSpriteP<7>(); fetchSpriteS0(7); }
                else			{ fetchSpriteS1<6>(); fetchSpriteS2(6); }
                break;
        case 8: if(ntscXLock)   { fetchSpriteS1<7>(); fetchSpriteS2(7); }
                else			{ fetchSpriteP<7>(); fetchSpriteS0(7); }
                break;
        case 9: if(ntscXLock)   { idleCycle(); }
                else			{ fetchSpriteS1<7>(); fetchSpriteS2( 7 ); }
                break;
        case 10: refresh();
            cAccessArea = true;
            break;
        case 11: refresh();            
            break;
        case 12: refresh();
            break;
        case 13: refresh(); updateVc();
            break;
        case 14: refresh(); fetchC();
            display.enable = true;
            break;
        case 15:            
            fetchG();
            spriteUpdateBase();
            fetchC();
            onHalfCycle = [this]() { borderLeft<true>(); };
            break;
        case 16:
            fetchG();
            fetchC();
            onHalfCycle = [this]() { borderLeft<false>(); };
            break;
        case 17: case 18: case 19:
        case 20: case 21: case 22: case 23: case 24:
        case 25: case 26: case 27: case 28: case 29:
        case 30: case 31: case 32: case 33: case 34:
        case 35: case 36: case 37: case 38: case 39:
        case 40: case 41: case 42: case 43: case 44:
        case 45: case 46: case 47: case 48: case 49:
        case 50: case 51: case 52:
            fetchG();
            fetchC();            
            break;
        case 53:
            fetchG();
            fetchC();            
            cAccessArea = false;
            if (!ntscBorder)
                onHalfCycle = [this]() { spriteDmaCheck(); };
            break;
        case 54:    
            fetchG();
            display.enable = false;
            onHalfCycle = [this]() { borderRight<true>(); spriteDmaCheck(); };
            break;
        case 55:
            idleCycle();
            spriteFlip();
            onHalfCycle = [this]() { borderRight<false>(); if(ntscBorder) spriteDmaCheck(); };
            break;
        case 56:
            if (!ntscBorder)
                spriteDmaCycle1 = 0x80;
            idleCycle();
            break;
        case 57:    if (ntscBorder) { if(!ntscXLock) {spriteDisplayCheck();} idleCycle(); updateRc(); spriteDmaCycle1 = 0x80; }
                    else			{ spriteDisplayCheck(); fetchSpriteP<0>(); updateRc(); fetchSpriteS0(0); }            
                    break;
        case 58:    if (ntscBorder) { if(ntscXLock) {spriteDisplayCheck();} fetchSpriteP<0>(); fetchSpriteS0(0); }
                    else			{ fetchSpriteS1<0>(); fetchSpriteS2(0); }
                    break;
        case 59:    if (ntscBorder) { fetchSpriteS1<0>(); fetchSpriteS2(0); }
                    else			{ fetchSpriteP<1>(); fetchSpriteS0(1); }
                    break;
        case 60:    if (ntscBorder) { fetchSpriteP<1>(); fetchSpriteS0(1); }
                    else			{ fetchSpriteS1<1>(); fetchSpriteS2(1); }
                    break;
        case 61:    if (ntscBorder) { fetchSpriteS1<1>(); fetchSpriteS2(1); }
                    else			{ fetchSpriteP<2>(); fetchSpriteS0(2); }
                    break;
        case 62:    if (ntscBorder) { fetchSpriteP<2>(); fetchSpriteS0(2); }
                    else			{ fetchSpriteS1<2>(); fetchSpriteS2(2); }
                    break;
        // ntsc only
        case 63:    fetchSpriteS1<2>(); fetchSpriteS2(2);
                    break;
        case 64:    fetchSpriteP<3>(); fetchSpriteS0(3);
                    break;
    }        

    // copy state of ECM / BMM directly before a possible write in order
    // to delay it one cycle for DMA fetch logic
    modeEcmBmmDma = modeEcmBmm;
    // we reset last color reg a little bit later, because of
    // we have to find out for the new vics if it is accessed
    // in the fifth pixel, see grey dot bug
    lastColorReg = 0xff;	         
}    

__attribute__((always_inline)) auto VicIICycle::advanceCycle() -> void {    

    if (irqLatchPending) {
        irqLatch |= irqLatchPending & 0x7f;
        updateIrq();
        irqLatchPending = 0;
    } 
	
	// a written DEN bit in last cycle of 0x30 is recognized
    if ( !allowBadlines && (vCounter == 0x30) && den )
		allowBadlines = true;
	
	if(initVCounter) {
        vCounter = 0;
        initVCounter = false;
		lpLatched = false;	
		// retrigger happens in last pixel of second cycle for all Vic types,
        // if lp line is held low in beginning of cycle
        if (!lpPin)
            triggerLightPen( false, 3 );
        
		display.vcBase = display.vc = 0;
		refreshCounter = 0xff;
		allowBadlines = false;
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
		
		if ( vCounter == vStart ) {
            updateBorderData();
            // we buffer all pixel data in non blanking area, of course a CRT 
            // can not display the whole non blanking area.
            // cropping is done later and not within Vic emulation
            visibleLine = true; // non v-blank
            if (lineCallback.finishVblank)
                vblankCallback();
            			
        } else if ( lineVCounter == vHeight ) {
            visibleLine = false; // v-blank
            
            if (leftLineAnomaly.mode)
                insertVerticalLineAnomaly( lineCallback.line, lineVCounter );
            
            // push out the frame to host
            // we crop the h-blanking area before
            videoRefresh( frameBuffer + firstVisiblePixel, 
                hWidth, lineVCounter, VIC_MAX_LINE_LENGTH - hWidth
            );
			lineVCounter = 0;
		} else if (lineCallback.use && (lineVCounter == lineCallback.line)) {
            if (leftLineAnomaly.mode)
                insertVerticalLineAnomaly( 0, lineVCounter );
            
            midScreenCallback();
        }
        
	} else if (cycle == 1)
		setLineBuffer();  

    spriteOpenBus = nullptr;
    sprite0DmaLateBA = false;
}

inline auto VicIICycle::clearCollisions() -> void {

	// is cleared one cycle after read, means collisions in second half
	// and following first half cycle will be ignored
	if (clearCollision == 0x1e)
		spriteSpriteCollided = 0;
	else if (clearCollision == 0x1f)
		spriteForegroundCollided = 0;
	
	clearCollision = 0;

    spriteSpriteCollidedRead = spriteSpriteCollided;
    spriteForegroundCollidedRead = spriteForegroundCollided;
	
	if (canSpriteSpriteCollisionIrq && spriteSpriteCollided)
		updateIrq( Interrupt::MMC );
	
	if (canSpriteForegroundCollisionIrq && spriteForegroundCollided)
		updateIrq( Interrupt::MBC );
}

// cycle: 16-2
auto VicIICycle::spriteUpdateBase() -> void {

    for( uint8_t i = 0; i < 8; i++ ) {
		Sprite* spr = &sprite[i];
        
        if (spr->expandYFlop) {
            spr->mcBase = spr->mc;

            if (spr->mcBase == 63) {
                spr->dma = false;
                updateSpriteBaState(i, false);
            }
        }
    }
}
// cycle: 55-1 + 56-1
inline auto VicIICycle::spriteDmaCheck() -> void {
    
    for( uint8_t i = 0; i < 8; i++ ) {
        Sprite* spr = &sprite[i];
        
        if (spr->enabled && !spr->dma && ( (vCounter & 0xff) == spr->y ) ) {
            spr->dma = true;
            updateSpriteBaState(i, true);
            spr->mcBase = 0;
            spr->expandYFlop = 1;
            
            if (spr == sprite0)
                sprite0DmaLateBA = true;
        }
    }
}
// cycle: 56-2
auto VicIICycle::spriteFlip() -> void {

    for( uint8_t i = 0; i < 8; i++ ) {
		Sprite* spr = &sprite[i];

        if (spr->dma && spr->expandY)
            spr->expandYFlop ^= 1;
    }    
}
// cycle: 58-1
auto VicIICycle::spriteDisplayCheck() -> void {
            
    for( uint8_t i = 0; i < 8; i++ ) {
		Sprite* spr = &sprite[i];
        
        spr->mc = spr->mcBase;
        
        if (spr->dma) {
            if (spr->enabled && ( (vCounter & 0xff) == spr->y ) )  
                spritePending |= 1 << i;
        } else 
            spritePending &= ~(1 << i);
    }    
}

// cycle: 14-2
auto VicIICycle::updateVc() -> void {
	display.vc = display.vcBase;
	display.vmli = 0;
	if (badLine)
		display.rc = 0;
}
// cycle: 58-2
auto VicIICycle::updateRc() -> void {
	if (display.rc == 7) {
		display.vcBase = display.vc;
		idleMode = true;            
	} 
	if (!idleMode || badLine) {
		display.rc = (display.rc + 1) & 7;
		idleMode = false;            
	}		
}

inline auto VicIICycle::updateBAState() -> void {
	static bool _baLow;
    _baLow = baLow;
    
	if (badLine)
		idleMode = false;	
    
    if (cAccessArea) // 11 <= cycle <= 53
        baLow = badLine; // for "c" accesses, no sprites pos
        
    else
		baLow = spriteBa[8][ cycle ]; // for "s" accesses    
		
    if (_baLow != baLow)
        setRdy( baLow ); //update cpu rdy line
	
	if (baLow) {
		if(aecDelay)
			aecDelay--;		
	} else		
		aecDelay = 4;	
}

// a damn hack ... this is annoying
auto VicIICycle::reuBaLow() -> bool {
    // of course the expansion port sees the same BA state like CPU RDY line.
    // there is a known case, when BA calculation takes more time within cycle.
    // for cpu it doesn't matter, because it checks later in cycle.
    // REU seems to check this sooner and can't recognize BA in this special cycle.
    
    return baLow && !sprite0DmaLateBA;
}

inline auto VicIICycle::updateBadLine() -> void {
			
	badLine = allowBadlines && (yScroll == (vCounter & 7));
	
	idleModeTemp = idleMode;
}

inline auto VicIICycle::borderControl() -> void {
    
    if (den && (vCounter == borderTop))        
        vFlipFlop = vFlipFlopShadow = false;           
    
    else if (vCounter == borderBottom)
        vFlipFlopShadow = true;   
    
    if (cycle == 0)
        vFlipFlop = vFlipFlopShadow;
}

template<bool first> auto VicIICycle::borderLeft(  ) -> void {
	
	if ((cSel && first) || (!cSel && !first)) {
		if (vCounter == borderBottom) 
			vFlipFlopShadow = true;
		
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

auto VicIICycle::idleCycle() -> void {
    lastReadPhi1 = read( 0x3fff );
}

auto VicIICycle::refresh() -> void {
    lastReadPhi1 = read( (0x3f << 8) | refreshCounter-- );
}

template<uint8_t pos> auto VicIICycle::fetchSpriteP(  ) -> void {
    
    spriteDmaCycle2 = 0x80 | pos;    
	
    Sprite* spr = &sprite[pos];
    
    spr->dataP = lastReadPhi1 = read( (vm << 10) | 0x3f8 | (pos & 7) );	
}

auto VicIICycle::fetchSpriteS0( uint8_t pos ) -> void {    
    
    lastSpriteShift = 16;
    fetchSpriteSPhi2( pos );
}

auto VicIICycle::fetchSpriteS2( uint8_t pos ) -> void {    
    lastSpriteShift = 0;
    fetchSpriteSPhi2( pos );
}

inline auto VicIICycle::fetchSpriteSPhi2( uint8_t pos ) -> void {	
	
    Sprite* spr = &sprite[pos];
    spriteOpenBus = spr;
    
	uint8_t value = 0xff;    	
	
	if ( spr->dma ) {	
		if ( !aecDelay ) {
            value = read( (spr->dataP << 6) | spr->mc );
            spriteOpenBus = nullptr;
        }
        
        spr->mc++;
        spr->mc &= 63;
	}
    
    spr->dataS &= ~(0xff << lastSpriteShift);
    spr->dataS |= value << lastSpriteShift;        
}



template<uint8_t pos> auto VicIICycle::fetchSpriteS1( ) -> void {    
    
    if (pos != 7)
        spriteDmaCycle1 = 0x80 | (pos + 1);
	
    Sprite* spr = &sprite[pos];
	
	if ( spr->dma ) {		        
        lastReadPhi1 = read( (spr->dataP << 6) | spr->mc );
		
		spr->mc++;
		spr->mc &= 63;	
	} else
		idleCycle();
	
	spr->dataS &= 0xff00ff;
	spr->dataS |= lastReadPhi1 << 8;
}

auto VicIICycle::fetchC() -> void {
    if (!baLow)
        return;
        
	uint8_t color = !aecDelay ? readColor( display.vc ) : readCpu();
	
	uint8_t dataC = !aecDelay ? read( (vm << 10) | display.vc ) : 0xff;       
	
	display.cBuffer[ display.vmli ] = ((color & 0xf) << 8) | dataC;
}

auto VicIICycle::addrG( uint8_t useMode ) -> uint16_t {
    
    uint16_t addr = display.rc;

    if (VIC_MODE_BMM( useMode ) ) {
        addr |= display.vc << 3;
        addr |= (cb & 4) << 11;
		
    } else {        
        uint8_t dataC = display.cBuffer[ display.vmli ] & 0xff;    
        addr |= dataC << 3;
        addr |= cb << 11;
    }

    if (VIC_MODE_ECM( useMode ) )
        addr &= 0x39ff;   

    return addr;
}

auto VicIICycle::fetchG() -> void {        
    
    uint16_t addr;
    uint8_t useMode;
    
    if (rev65)
        useMode = modeEcmBmm;
    else
        useMode = modeEcmBmmDma; //is delayed one cycle for 85xx chips
    
    if ( idleModeTemp ) {
        addr = VIC_MODE_ECM(useMode) ? 0x39ff : 0x3fff;
		
    } else {
        
        if (rev65) {
			// if Bmm changes from 1 -> 0, keeps seted when used in first cycle after write
			// Ecm uses the new value directly after write for 65xx chips
            addr = addrG( useMode | (modeEcmBmmDma & 2) );
            
			// when Bmm changes
            if ( (useMode ^ modeEcmBmmDma) & 2 ) {
                uint16_t addrFrom = addrG( modeEcmBmmDma );
                uint16_t addrTo = addrG( useMode );
                
                if ( !isCharRomAccessed( addrFrom ) && isCharRomAccessed( addrTo ) ) 
                    addr = (addrFrom & 0xff) | (addrTo & 0x3f00);
            }
            
        } else
            addr = addrG( useMode );
		
		display.vmli++;
		display.vc++;
		display.vc &= 0x3ff;	
    }    
    
    lastReadPhi1 = read( addr );    
    
    display.gBuffer = lastReadPhi1; 
    
    if (display.gBufferUse) {
        display.gBufferPipe1 = display.gBuffer;
        display.gBufferUse = false;
    }
}

}
