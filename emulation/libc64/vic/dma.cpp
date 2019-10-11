
#include "vicII.h"

namespace LIBC64 {

auto VicII::phase1() -> void {	
    advanceCycle();
    checkLightPen<true>();
    setLineInterrupt();	
    sequencer<true>(  );    		
            
    switch( cycle )  {
        case 0: ntsc ? fetchSpriteS1( 3 )		: fetchSpriteP( 3 );	break;		
		case 1: ntsc ? fetchSpriteP( 4 )		: fetchSpriteS1( 3 );	break;
		case 2: ntsc ? fetchSpriteS1( 4 )		: fetchSpriteP( 4 );	break;	
		case 3: ntsc ? fetchSpriteP( 5 )		: fetchSpriteS1( 4 );	break;		
		case 4: ntsc ? fetchSpriteS1( 5 )		: fetchSpriteP( 5 );	break;		
		case 5: ntsc ? fetchSpriteP( 6 )		: fetchSpriteS1( 5 );	break;		
		case 6: ntsc ? fetchSpriteS1( 6 )		: fetchSpriteP( 6 );	break;		
		case 7: ntsc ? fetchSpriteP( 7 )		: fetchSpriteS1( 6 );	break;		
		case 8: ntsc ? fetchSpriteS1( 7 )		: fetchSpriteP( 7 );	break;		
		case 9: ntsc ? idleCycle()              : fetchSpriteS1( 7 );	break;		
		case 10: refresh();                             				break;		
		case 11: refresh();	cAccessArea = 1;                			break;		
		case 12: refresh();                                 			break;		
		case 13: refresh();                                     		break;		
		case 14: refresh();                                     		break;		
		case 15: 
			display.enable = true;
			fetchG();
			break;
		case 16:
			borderLeft( true );
			fetchG();
			break;
		case 17:
			borderLeft( false );
			fetchG();
			break;		
		case 18: case 19:
		case 20: case 21: case 22: case 23: case 24:
		case 25: case 26: case 27: case 28: case 29:
		case 30: case 31: case 32: case 33: case 34:
		case 35: case 36: case 37: case 38: case 39:
		case 40: case 41: case 42: case 43: case 44:
		case 45: case 46: case 47: case 48: case 49:
		case 50: case 51: case 52: case 53:
            fetchG();
            break;
		case 54: 
            cAccessArea = 0;
			if(!ntsc)
				spriteDmaCheck();
			fetchG();
			break;
			
		case 55: 
			borderRight(true);
			spriteDmaCheck();
			idleCycle();
			break;					
		case 56:
			borderRight(false);
			if(ntsc)
				spriteDmaCheck();
			idleCycle();	
			break;						
		case 57:
			if(ntsc) {
				idleCycle();
			} else {
				spriteDisplayCheck();
				fetchSpriteP( 0 );
			}			
			break;
		
		case 58:
			if(ntsc) {
				spriteDisplayCheck();
				fetchSpriteP( 0 );
			} else {
				fetchSpriteS1( 0 );
			}
			break;
				
		case 59: ntsc ? fetchSpriteS1( 0 )		: fetchSpriteP( 1 );	break;	
		case 60: ntsc ? fetchSpriteP( 1 )		: fetchSpriteS1( 1 );	break;		
		case 61: ntsc ? fetchSpriteS1( 1 )		: fetchSpriteP( 2 );	break;		
		case 62: ntsc ? fetchSpriteP( 2 )		: fetchSpriteS1( 2 );	break;		
		case 63: fetchSpriteS1( 2 ); break;		
		case 64: fetchSpriteP( 3 ); break;
    }
	
	borderControl();
	updateBAState();
	clearCollisions();
}

auto VicII::phase2() -> void {
	checkLightPen<false>();
    sequencer<false>(  );
    
    switch( cycle )  {
		case 0: ntsc ? fetchSpriteS2( 3 )		: fetchSpriteS0( 3 );	break;		
		case 1: ntsc ? fetchSpriteS0( 4 )		: fetchSpriteS2( 3 );	break;
		case 2: ntsc ? fetchSpriteS2( 4 )		: fetchSpriteS0( 4 );	break;
		case 3: ntsc ? fetchSpriteS0( 5 )		: fetchSpriteS2( 4 );	break;		
		case 4: ntsc ? fetchSpriteS2( 5 )		: fetchSpriteS0( 5 );	break;		
		case 5: ntsc ? fetchSpriteS0( 6 )		: fetchSpriteS2( 5 );	break;		
		case 6: ntsc ? fetchSpriteS2( 6 )		: fetchSpriteS0( 6 );	break;	
		case 7: ntsc ? fetchSpriteS0( 7 )		: fetchSpriteS2( 6 );	break;		
		case 8: ntsc ? fetchSpriteS2( 7 )		: fetchSpriteS0( 7 );	break;		
		case 9: if (!ntsc) fetchSpriteS2( 7 );                          break;		
		case 10:														break;		
		case 11:														break;		
		case 12:														break;		
		case 13: updateVc();											break;			
		case 14: fetchC();												break;				
		case 15:
			spriteUpdateBase();
			fetchC();
			break;
		case 16: case 17: case 18: case 19:
		case 20: case 21: case 22: case 23: case 24:
		case 25: case 26: case 27: case 28: case 29:
		case 30: case 31: case 32: case 33: case 34:
		case 35: case 36: case 37: case 38: case 39:
		case 40: case 41: case 42: case 43: case 44:
		case 45: case 46: case 47: case 48: case 49:
		case 50: case 51: case 52: case 53:			
			fetchC();
			break;			
		case 54: display.enable = false;								break;			
		case 55: spriteFlip();											break;			
		case 56:														break;			
		case 57: updateRc();
			if (!ntsc) fetchSpriteS0( 0 );								break;		
		case 58: ntsc ? fetchSpriteS0( 0 )		: fetchSpriteS2( 0 );	break;		
		case 59: ntsc ? fetchSpriteS2( 0 )		: fetchSpriteS0( 1 );	break;		
		case 60: ntsc ? fetchSpriteS0( 1 )		: fetchSpriteS2( 1 );	break;		
		case 61: ntsc ? fetchSpriteS2( 1 )		: fetchSpriteS0( 2 );	break;		
		case 62: ntsc ? fetchSpriteS0( 2 )		: fetchSpriteS2( 2 );	break;		
		case 63: fetchSpriteS2( 2 ); break;		
		case 64: fetchSpriteS0( 3 ); break;
    }
	
    // copy state of ECM / BMM directly before a possible write in order
    // to delay it one cycle for DMA fetch logic
    modeEcmBmmDma = modeEcmBmm;	
}

inline auto VicII::advanceCycle() -> void {    
    // first we apply a possible register write at beginning of a new cycle
    // instead of cycle end, because of irq state changes by writing to 0x19 
    // or 0x1a mustn't be recognized by cpu in previous cycle.
    if (registerWrite.pipelined ) {
		registerWrite.pipelined = false;
		writeIO( registerWrite.addr, registerWrite.value );
	}
    
	if (lpIrqPending) {
		lpIrqPending = false;
		updateIrq( Interrupt::LP );
	}
	
	// a written DEN bit in last cycle of 0x30 is recognized
    if ( !allowBadlines && (vCounter == 0x30) && den )
		allowBadlines = true;
	
	if(initVCounter) {
        vCounter = 0;
        initVCounter = false;
		lpLatched = false;	
		// retrigger happens in last pixel of second cycle for all Vic types	
		//lpTriggerDelay = !lpPin ? 1 : 0;
        triggerLightPen( lpPin, 3 );

        
		display.vcBase = display.vc = 0;
		refreshCounter = 0xff;
		allowBadlines = false;
    }    
		
	if (++cycle == lineCycles) {		
		cycle = 0;  
		
		// Note: line complete but vcounter is not incremented at this point
		if (vCounter == 0xf7)
			allowBadlines = false;		
		
		if (++vCounter == (ntsc ? 263 : 312) ) {
			// last line is not reseted this cycle but next
			vCounter = ntsc ? 262 : 311;
			initVCounter = true;			
		} else {
			// when vCounter increments to 0x30 we check for DEN
            // the above check in this function would miss the first cycle in line
			if ( !allowBadlines && (vCounter == 0x30) && den )
				allowBadlines = true;
		}
		
		if ( vCounter == vStart ) {
            updateBorderData();
            // we buffer all pixel data in non blanking area, of course a crt 
            // can not display the whole non blanking area
            // cropping is done later and not within Vic emulation
            visibleLine = true; // non v-blank
            if (lineCallback.finishVblank)
                vblankCallback();
			
        } else if ( lineVCounter == vHeight ) {
            visibleLine = false; // v-blank
            // push out the frame to host
            // we crop the h-blanking area before
            videoRefresh( frameBuffer + firstVisiblePixel, 
                hWidth, lineVCounter, VIC_MAX_LINE_LENGTH - hWidth
            );
			lineVCounter = 0;
		} else if (lineCallback.use && (lineVCounter == lineCallback.line))
            midScreenCallback();
	}    
	
	if (cycle == 1)
		setLineBuffer();  

	lastBusPhi2 = 0xff; // clear internal bus    
}

inline auto VicII::setLineBuffer() -> void {
    if (!visibleLine)
        return;
    
    linePtr = frameBuffer + lineVCounter * VIC_MAX_LINE_LENGTH;
    lineVCounter++;
    linePos = 0;
}

inline auto VicII::setLineInterrupt() -> void {
    
	if (vCounter == irqLine) {
		if (!lineIrqMatched) {
			updateIrq( Interrupt::Raster );
			lineIrqMatched = true;
		}
		return;
	}
	
	lineIrqMatched = false;		
}

inline auto VicII::clearCollisions() -> void {
	
	// is cleared one cycle after read, means collisions in second half
	// and following first half cycle will be ignored
	if (clearCollision == 0x1e)
		spriteSpriteCollided = 0;
	else if (clearCollision == 0x1f)
		spriteForegroundCollided = 0;
	
	clearCollision = 0;
	
	if (canSpriteSpriteCollisionIrq && spriteSpriteCollided)
		updateIrq( Interrupt::MMC );
	
	if (canSpriteForegroundCollisionIrq && spriteForegroundCollided)
		updateIrq( Interrupt::MBC );
}

// cycle: 16-2
auto VicII::spriteUpdateBase() -> void {

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
auto VicII::spriteDmaCheck() -> void {
    
    for( uint8_t i = 0; i < 8; i++ ) {
        Sprite* spr = &sprite[i];
        
        if (spr->enabled && !spr->dma && ( (vCounter & 0xff) == spr->y ) ) {
            spr->dma = true;
            updateSpriteBaState(i, true);
            spr->mcBase = 0;
            spr->expandYFlop = 1;
        }
    }
}
// cycle: 56-2
auto VicII::spriteFlip() -> void {

    for( uint8_t i = 0; i < 8; i++ ) {
		Sprite* spr = &sprite[i];

        if (spr->dma && spr->expandY)
            spr->expandYFlop ^= 1;
    }    
}
// cycle: 58-1
auto VicII::spriteDisplayCheck() -> void {
    spriteDisplayCycle = true;
            
    for( uint8_t i = 0; i < 8; i++ ) {
		Sprite* spr = &sprite[i];
        
        spr->mc = spr->mcBase;
        
        if (spr->dma) {
            if (spr->enabled && ( (vCounter & 0xff) == spr->y ) )  
                spriteDisplay |= 1 << i;
        } else 
            spriteDisplay &= ~(1 << i);
    }    
}

inline auto VicII::updateSpriteBaState( uint8_t sprNr, bool dmaActive ) -> void {
	// update BA line state for sprites
	// 5 cycles: 3 to finish possible cpu writes, 2 to use the bus in second phase of cycle
	// up to 3 cpu cycles are wasted because of the bad design
	// e.g. dma for sprite 0 and 2 stop cpu read during non dma sprite 1 too 
	// because of allowing a cpu read wouldn't give enough time to retake the bus for sprite 2
	// The three take over cycles are hard coded in vic design
	// e.g. dma for sprite 0 and 3 allow cpu one read access in between
	unsigned start = (ntsc ? 55 : 54) + (sprNr << 1);
	start %= lineCycles;
	
	for(auto i = 0; i < 5; i++) {
		unsigned pos = (start + i) % lineCycles;
		
		spriteBa[sprNr][ pos ] = dmaActive;		
		spriteBa[8][pos] = dmaActive;
		if (dmaActive)
			continue;
		//if any other sprite needs this position keep baLow state
		for( auto j = 0; j < 8; j++ ) {
			if (spriteBa[j][pos]) {
				spriteBa[8][pos] = true;
				break;
			}
		}
	}
}
// cycle: 14-2
auto VicII::updateVc() -> void {
	display.vc = display.vcBase;
	display.vmli = 0;
	if (badLine())
		display.rc = 0;
}
// cycle: 58-2
auto VicII::updateRc() -> void {
	if (display.rc == 7) {
		display.vcBase = display.vc;
		idleMode = true;            
	} 
	if (!idleMode || badLine()) {
		display.rc = (display.rc + 1) & 7;
		idleMode = false;            
	}		
}

inline auto VicII::updateBAState() -> void {
	
	bool _badLine = badLine();
	
	if (_badLine)
		idleMode = false;	
    
    if (cAccessArea) // 11 <= cycle <= 53
        baLow = _badLine; // for "c" accesses, no sprites pos
        
    else
		baLow = spriteBa[8][ cycle ]; // for "s" accesses    
		
	setRdy( baLow ); //update cpu rdy line
	
	if (baLow) {
		if(aecDelay)
			aecDelay--;		
	} else		
		aecDelay = 4;	
}

// a damn hack ... this is annoying
auto VicII::reuBaLow() -> bool {
    // of course the expansion port sees the same BA state like CPU RDY line.
    // there is a known case, when BA calculation takes more time within cycle.
    // for cpu it doesn't matter, because it checks later in cycle.
    // REU seems to check this sooner and can't recognize BA in this special cycle.
    // yeah i know this is a hack, because VIC is not aware of REU.
    // it's a limitation of half cycle accuracy.
    
    bool special = sprite[0].enabled && (cycle == 54) && (sprite[0].y == (vCounter & 0xff)) && !sprite[0].dma;
    
    return baLow && !special;
}

inline auto VicII::badLine() -> bool {
			
	return allowBadlines && (yScroll == (vCounter & 7));
}

inline auto VicII::borderControl() -> void {
    
    if (den && (vCounter == borderTop))        
        vFlipFlop = vFlipFlopShadow = false;           
    
    else if (vCounter == borderBottom)
        vFlipFlopShadow = true;   
    
    if (cycle == 0)
        vFlipFlop = vFlipFlopShadow;
}

auto VicII::borderLeft( bool c17 ) -> void {
	
	if ((cSel && c17) || (!cSel && !c17)) {
		if (vCounter == borderBottom) 
			vFlipFlopShadow = true;
		
		vFlipFlop = vFlipFlopShadow;
		if (!vFlipFlop) {
            hFlipFlop = 0;
        }			
	}
}

auto VicII::borderRight( bool c56 ) -> void {
	
	if ((!cSel && c56) || (cSel && !c56))
		hFlipFlop = 1;
}

auto VicII::idleCycle() -> void {
    lastReadPhi1 = read( 0x3fff );
}

auto VicII::refresh() -> void {
    lastReadPhi1 = read( (0x3f << 8) | refreshCounter-- );
}

auto VicII::fetchSpriteP( uint8_t pos ) -> void {
    
	spriteDmaCycle1 = 0x80 | pos;    
	
    Sprite* spr = &sprite[pos];
    
    spr->dataP = lastReadPhi1 = read( (vm << 10) | 0x3f8 | (pos & 7) );	
}

auto VicII::fetchSpriteS0( uint8_t pos ) -> void {    
    
    fetchSpriteSPhi2( pos, false );
}

auto VicII::fetchSpriteS2( uint8_t pos ) -> void {    
    
    fetchSpriteSPhi2( pos, true );
}

auto VicII::fetchSpriteSPhi2( uint8_t pos, bool last ) -> void {	
	
    Sprite* spr = &sprite[pos];
    
	uint8_t value = lastBusPhi2;
	
	if ( spr->dma ) {		
		if ( !aecDelay )
			value = read( (spr->dataP << 6) | spr->mc );
		
		spr->mc++;
		spr->mc &= 63;	
	}
    
	uint8_t shift = last ? 0 : 16;
	
	spr->dataS &= ~(0xff << shift);
	spr->dataS |= value << shift;
}

auto VicII::fetchSpriteS1( uint8_t pos ) -> void {    
    spriteDmaCycle2 = 0x80 | pos;
	
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

auto VicII::fetchC() -> void {
    if (!baLow)
        return;
        
	uint8_t color = !aecDelay ? readColor( display.vc ) : readCpu();
	
	uint8_t dataC = !aecDelay ? read( (vm << 10) | display.vc ) : 0xff;       
	
	display.cBuffer[ display.vmli ] = ((color & 0xf) << 8) | dataC;
}

auto VicII::addrG( uint8_t useMode ) -> uint16_t {
    
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

auto VicII::fetchG() -> void {        
    
    uint16_t addr;
    uint8_t useMode;
    
    if (rev65)
        useMode = modeEcmBmm;
    else
        useMode = modeEcmBmmDma; //is delayed one cycle for 85xx chips
    
    if ( idleMode ) {
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
}

}
