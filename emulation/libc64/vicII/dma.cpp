
//  This code is based on VIC-II cycle engine in VICE
//  You can get a copy of the original here: https://sourceforge.net/projects/vice-emu/

/*
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
 *
 * Based on code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vicII.h"
#include "../system/system.h"

namespace LIBC64 {    

auto VicIICycle::clock() -> void {
	
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
	
	lastReadPhi1 = fetchPhi1( flags );
	
	if (isUpdateVc( flags ))
		updateVc();	
	else if (isUpdateRc( flags ))
		updateRc();
	
    // copy state of ECM / BMM directly before a possible write in order
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
		
		setLineBuffer();

		if ( vCounter == vStart ) {
            updateBorderData();
            // we buffer all pixel data in non blanking area, of course a CRT
            // can not display the whole non blanking area.
            // cropping is done later and not within Vic emulation
            visibleLine = true; // non v-blank

        } else if ( lineVCounter == vHeight ) {
            visibleLine = false; // v-blank

            if (leftLineAnomaly.mode)
                insertVerticalLineAnomaly( lineCallback.line, lineVCounter );

            // push out the frame to host
            // we crop the h-blanking area before
            system->videoRefresh( frameBuffer + firstVisiblePixel,
                hWidth, lineVCounter, VIC_MAX_LINE_LENGTH - hWidth
            );
			lineVCounter = 0;
		} else if (lineCallback.use && (lineVCounter == lineCallback.line)) {
            if (leftLineAnomaly.mode)
                insertVerticalLineAnomaly( 0, lineVCounter );

            system->VicMidScreenCallback();
        }
	}

    sprite0DmaLateBA = false;
}

inline auto VicIICycle::updateCollisions() -> void {

    spriteSpriteCollidedRead = spriteSpriteCollided;
    spriteForegroundCollidedRead = spriteForegroundCollided;
	
	if (canSpriteSpriteCollisionIrq && spriteSpriteCollided)
		updateIrq( Interrupt::MMC );
	
	if (canSpriteForegroundCollisionIrq && spriteForegroundCollided)
		updateIrq( Interrupt::MBC );
}

inline auto VicIICycle::clearCollisions() -> void {
	if (clearCollision == 0x1e)
		spriteSpriteCollided = 0;
	else /* if (clearCollision == 0x1f) */
		spriteForegroundCollided = 0;

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
	//static bool _baLow;
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

}
