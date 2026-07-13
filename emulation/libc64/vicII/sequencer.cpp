
//  This code is based on VIC-II cycle engine in VICE
//  You can get a copy of the original here: https://sourceforge.net/projects/vice-emu/

/*
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

/**
 * register change timings are mostly the findings of the VICE team, thank you.
 * in VICE a cycle processes the last 4 pixel of previous cycle and first 4 pixel of current cycle
 * in Denise a cycle processes the 8 pixel of current cycle.
 * a lot of important changes happen between the half cycles, so doing it the VICE way you can use more local variables
 * and generate faster code this way.
 * I have decided against it for readability, so for me it's easier to imagine the overall process.
 *
 * todo videomode-z
 * videomode-z test: disabling (not enabling) MCM (both: LUT and gbits ) happens a pixel later
 * BUT other videomode tests fail. todo: find the right condition
 */

#include "vicII.h"

namespace LIBC64 {

#define VIC_D021     0x21
#define VIC_D022     0x22	
#define VIC_D023     0x23	
	
#define VIC_VBUF_L	 0x11
#define VIC_VBUF_H   0x12
#define VIC_CBUF     0x13

#define VIC_CBUF_MC  0x14
#define VIC_D02X_EXT 0x15

const uint8_t VicIICycle::colorLUT[] = {
    VIC_D021, VIC_D021, VIC_CBUF, VIC_CBUF,        
    VIC_D021, VIC_D022, VIC_D023, VIC_CBUF_MC,     
    VIC_VBUF_L, VIC_VBUF_L, VIC_VBUF_H, VIC_VBUF_H,
    VIC_D021, VIC_VBUF_H, VIC_VBUF_L, VIC_CBUF,     
    VIC_D02X_EXT, VIC_D02X_EXT, VIC_CBUF, VIC_CBUF, 
	// mode 5, 6, 7
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};	
	
inline auto VicIICycle::sequencer( uint32_t flags ) -> void {
    
    sequencerPix0<true>(  );
    
    sequencerPix1<true>(  );
    
    sequencerPix2<true>(  );
    
	if (!isSprSecondCycle( flags ))
		spriteHalt = 0;
	
    sequencerPix3<true>(  );
        	
	uint8_t metaDataFirstHalf = ((!aecDelay) << 5) | (baLow << 4);
	
	bool hFlipFlopFirstHalf = hFlipFlop;
	
	if (isBrdLeftFirst(flags))
		borderLeft<true>();
	else if (isBrdLeftSecond(flags))
		borderLeft<false>();
	else if (isBrdRightFirst(flags))
		borderRight<true>();
	else if (isBrdRightSecond(flags))
		borderRight<false>();

	if (isSprDma(flags))
		spriteDmaCheck();		
	
    borderControl();
    updateBAState( flags );    
    
	// like graphic processing, sprite processing is delayed 8 pixel too.
	// the x-coordinate of a sprite is compared with the x counter, which
	// is 8 pixel in the past to keep alignment with the background.
	// it seems the xCounter is latched each cycle
	// or each xCounter increment is latched and compared a cycle later with
	// sprite x positions.
	// we take the latch between the half cycles in order to keep aligned with
	// xCounter "wrap around" value of 0x1f8 or 0x200 for NTSC.
	xCounterLatchBefore = xCounterLatch;   
	xCounterLatch = getXpos( flags );

	if (unlikely(lpTrigger))
		checkLightPen();
	
	spriteSpriteCollidedRead = spriteSpriteCollided;
	spriteForegroundCollidedRead = spriteForegroundCollided;

	// collisions in the second half trigger IRQ next cycle
	if (canSpriteSpriteCollisionIrq && spriteSpriteCollided) {
	    if (!disableSpriteCollisions)
		    updateIrq( Interrupt::MMC );
		canSpriteSpriteCollisionIrq = false;
	}

	if (canSpriteForegroundCollisionIrq && spriteForegroundCollided) {
	    if (!disableSpriteCollisions)
		    updateIrq( Interrupt::MBC );
		canSpriteForegroundCollisionIrq = false;
	}

	pipeGraphic( flags );
	
    sequencerPix0<false>();

    sequencerPix1<false>();

	if (isSprSecondCycle( flags )) {
		spriteActive &= ~(1 << getSpr( flags ) );
	}
	
    sequencerPix2<false>();

	if (isSprFirstCycle( flags )) {
		spriteHalt = 1 << getSpr( flags );
	}
	
    sequencerPix3<false>();

	if(visibleLine) {
		borderArea( hFlipFlopFirstHalf );
		draw( metaDataFirstHalf );
	} else if (lastColorReg != 0xff) {		
		colorUse[ lastColorReg ] = colorReg[ lastColorReg ];
		lastColorReg = 0xff;
	}
	
	if (unlikely(clearCollision))
		clearCollisions();
}

template<bool phi1> inline auto VicIICycle::sequencerPix0( ) -> void {   
	
    if (phi1) {			
		if (rev65)
            modeEcmBmmSequencer |= modeEcmBmm; // 0 -> 1 transitions: update ECM + BMM directly after write half cycle       

    } else {
       
        // this is for speed up reasons only.
        // for a sprite display line we check the trigger position each cycle.
        // a cycle processes 8 pixel. so we check 8 * 8 = 64 times per cycle.
        // the x trigger position of a sprite is latched after every first half cycle.
        // so it can not change for the next 8 comparisons.
        // the sprite x compare counter is aligned with raster line 'wrap around'.
        // by simply reseting the last 3 bits we can find out if the sprite will trigger
        // within the following 8 pixel. that reduces the 64 comparisons to 8 in case of
        // the sprite will not trigger. Of course it increases to 72 comparisons in trigger case.
        // but its more likely that it won't trigger. in sum the amount of comparisons will be reduced.
        spriteTrigger = 0;
        for (uint8_t i = 0; i < 8; i++) {
            if ( (xCounterLatchBefore & 0x1f8) == (sprite[i].useX & 0x1f8) )
                spriteTrigger |= 1 << i;           
        }        					
	}
					
	graphicSequencer( phi1 ? 4 : 0 );
    if (spriteTrigger & spritePending ) triggerSprites( xCounterLatchBefore );
	if (spriteActive) spriteSequencer(  );
    render[!phi1 ? 4 : 0] = color;
}

template<bool phi1> inline auto VicIICycle::sequencerPix1(  ) -> void {

    if (phi1) {                            
        if (rev65 && disableEcmBmmTogether)
            modeEcmBmmSequencer &= modeEcmBmm;            
    }
    
    graphicSequencer( phi1 ? 5 : 1 );
    if (spriteTrigger & spritePending ) triggerSprites( xCounterLatchBefore + 1 );
	if (spriteActive) spriteSequencer(  );
    render[!phi1 ? 5 : 1] = color;
}

template<bool phi1> inline auto VicIICycle::sequencerPix2( ) -> void {
	
    if (phi1) {
        if (rev65)
            modeEcmBmmSequencer &= modeEcmBmm; // 1 -> 0  transitions: update ECM + BMM 2 pixel later	
        
		else if (updateMc)
			updateMc8565();
		
        if (updatePrioExpand) {
            updatePrioExpand = false;
            
            for(unsigned i=0; i < 8; i++) {
                sprite[i].usePrioMD = sprite[i].prioMD;
                sprite[i].useExpandX = sprite[i].expandX;
            }    
        }        
	}
	
	graphicSequencer( phi1 ? 6 : 2 );
    if (spriteTrigger & spritePending ) triggerSprites( xCounterLatchBefore + 2 );
	if (spriteActive) spriteSequencer( );
    render[!phi1 ? 6 : 2] = color;
}

template<bool phi1> inline auto VicIICycle::sequencerPix3(  ) -> void {
    
    if (phi1) {
		if ( modeMcm && !modeMcmSequencer )
			mcFlop = 0;
		
        modeMcmSequencer = modeMcm;
        		
		if (updateMc && rev65)
			updateMc6569();		
	}
					
	graphicSequencer( phi1 ? 7 : 3 );
    if (spriteTrigger & spritePending ) triggerSprites( xCounterLatchBefore + 3 );
	if (spriteActive) spriteSequencer(  );
    render[!phi1 ? 7 : 3] = color;
    
    if (phi1) {    
        if (!rev65)
            modeEcmBmmSequencer = modeEcmBmm; // update ECM & BMM
               
		for( unsigned i = 0; i < 8; i++ )
			// changed sprite x register have 4 pixel delay
			sprite[i].useX = sprite[i].x;
		        
    } else {
		xCounterLatchBefore += 4;
    }            
}

inline auto VicIICycle::pipeGraphic( uint32_t flags ) -> void {
	// 8 pixel delay before processing starts
	cBufferPipe2 = cBufferPipe1;
	gBufferPipe2 = gBufferPipe1;
	bool enable = isVisible( flags );
	
	if (enable && !vFlipFlop) {
		gBufferPipe1 = gBuffer;
        gBufferUse = true;
        
		xScrollPipe = xScroll;

		if (!idleModeTemp) 
			cBufferPipe1 = cBuffer[dmli++];
		else
			cBufferPipe1 = 0;

	} else {
		gBufferPipe1 = 0;
        gBufferUse = false;
    }

	if (!enable)
		dmli = 0;
}

inline auto VicIICycle::graphicSequencer(uint8_t x) -> void {

    if (x == xScrollPipe) {
		gBufferShift = gBufferPipe2;
		dataC = cBufferPipe2;
		mcFlop = 1;
    }

    if (modeMcmSequencer) {
        if ( (modeEcmBmmSequencer & 8) || ( dataC & 0x800 ) ) {            
			if (mcFlop)
				// hold for next pixel too
				gBits = gBufferShift >> 6;
        } else {
            gBits = (gBufferShift & 0x80) ? 3 : 0;
        }
    } else {
		if (gBufferShift & 0x80) {
			if ( (modeEcmBmmSequencer & 8) || ( dataC & 0x800 ) )
				gBits = 2;
			else
				gBits = 3;
		} else
			gBits = 0;    
    }

    gBufferShift <<= 1;
    mcFlop ^= 1;
	
    color = colorLUT[ modeEcmBmmSequencer | modeMcm | gBits ];

    switch (color) {
        case VIC_VBUF_L:
            color = dataC & 0x0f;
            break;
        case VIC_VBUF_H:
            color = (dataC >> 4) & 0xf;
            break;
        case VIC_CBUF:
            color = (dataC >> 8) & 15;
            break;
        case VIC_CBUF_MC:
            color = (dataC >> 8) & 0x07;
            break;
        case VIC_D02X_EXT:
            color = VIC_D021 + ((dataC >> 6) & 3);
            break;
        default:
            break;
    }
}

inline auto VicIICycle::triggerSprites( uint16_t xPos ) -> void {
    
    triggerSprites<0>( xPos );
    triggerSprites<1>( xPos );
    triggerSprites<2>( xPos );
    triggerSprites<3>( xPos );
    triggerSprites<4>( xPos );
    triggerSprites<5>( xPos );
    triggerSprites<6>( xPos );
    triggerSprites<7>( xPos ); 
}

template<uint8_t sprPos> inline auto VicIICycle::triggerSprites( uint16_t xPos ) -> void {
    
    Sprite* spr = &sprite[sprPos];

	uint8_t mask = (1 << sprPos);
	
    // in halt state a few x coordinates between first and second dma cannot trigger
    if ( !(spriteActive & mask) && !(spriteHalt & mask) && (spriteTrigger & mask) && (spritePending & mask) )
        if ( xPos == spr->useX ) {                
            // can not retrigger until sprite is shifted out completely or second sprite dma finishes
			spriteActive |= mask;
            // init the flops
            spr->expandXFlop = true;
            spr->mcFlop = true;
        }
}

inline auto VicIICycle::updateMc6569() -> void {
    
    for(unsigned i=0; i < 8; i++) {
        Sprite& spr = sprite[i];
        // is bit changed: 0 > 1 or 1 > 0
        bool toggled = spr.multiColor ^ spr.useMultiColor;
        // when register changed, reset the mc flop, unchanged otherwise
        spr.mcFlop &= !toggled;
		// now use register value in calculation
        spr.useMultiColor = spr.multiColor;
    }
    
    updateMc = false;
}

inline auto VicIICycle::updateMc8565() -> void {
    
    for(unsigned i=0; i < 8; i++) {
        Sprite& spr = sprite[i];
        
        bool toggled = spr.multiColor ^ spr.useMultiColor;        
		// mc flop is toggled, when register is changed and sprite is not x expanded
		// unchanged otherwise
    	if (toggled & !spr.expandXFlop) {
    		if (spr.multiColor)
    			spr.mcFlop ^= 1;
    		else
    			spr.mcFlop = true;
    	}
        
        spr.useMultiColor = spr.multiColor;
    }
    
    updateMc = false;
}

inline auto VicIICycle::spriteSequencer(  ) -> void {
    Sprite* sprUse = nullptr;
	uint8_t collision = 0;
    
	if(spriteActive & 0x80) spriteSequencer<7>( sprite7, sprUse, collision );
	if(spriteActive & 0x40) spriteSequencer<6>( sprite6, sprUse, collision );
	if(spriteActive & 0x20) spriteSequencer<5>( sprite5, sprUse, collision );
	if(spriteActive & 0x10) spriteSequencer<4>( sprite4, sprUse, collision );
	if(spriteActive & 0x08) spriteSequencer<3>( sprite3, sprUse, collision );
	if(spriteActive & 0x04) spriteSequencer<2>( sprite2, sprUse, collision );
	if(spriteActive & 0x02) spriteSequencer<1>( sprite1, sprUse, collision );
	if(spriteActive & 0x01) spriteSequencer<0>( sprite0, sprUse, collision );
	
    if (sprUse) {
        // at least one non-transparent sprite pixel is found
        // ok we have to find out if background pixel or sprite pixel is higher prioritized

        // if display color is background, sprite prio doesn't matter
        if (!isForeground() || !sprUse->usePrioMD) {
            // ok sprite wins
            // we set the register positions only, not the contained values
            // the values will be fetched later, so we don't miss late register changes
            if (sprUse->shiftOut == 1) color = 0x25; // mc color register 1
            else if (sprUse->shiftOut == 3) color = 0x26; // mc color register 2
            else /* else if (sprUse->shiftOut == 2) */ color = sprUse->colorCode; // sprite color
            // shiftOut == 0 can not happen at this point
        }

        if (isForeground())
            // if pixel is foreground all non-transparent sprite pixel will be set if not already
            spriteForegroundCollided |= collision;

        if (collision & (collision - 1))
            // at least two sprites have to be non-transparent
            // otherwise no bit is set
            spriteSpriteCollided |= collision;
    }    
}

template<uint8_t sprPos> inline auto VicIICycle::spriteSequencer( Sprite* spr, Sprite*& sprUse, uint8_t& collision ) -> void {
			
	uint8_t mask = (1 << sprPos);
	
    // active, but no data left or last "shift Out" contains no data anymore (transparent)
    if ( !(spr->dataShiftReg || spr->shiftOut) ) {
        spriteActive &= ~mask;
        return;
    }
    // in halted state, last "shift out" is used always and no flip-flops will be toggled
    if (!(spriteHalt & mask )) {		
        if (spr->expandXFlop) {				
            if (spr->useMultiColor) {
                // 2 bits per pixel (repeated)
                if (spr->mcFlop)
                    spr->shiftOut = (spr->dataShiftReg >> 22) & 3;

                spr->mcFlop ^= 1; //repeat last shift out for second pixel
            } else if (spr->mcFlop || rev65)
                spr->shiftOut = ((spr->dataShiftReg >> 23) & 1) << 1; // 2: sprite color, 0: transparent
        	else
        		spr->mcFlop = true;
        }

        if (spr->expandXFlop)
            // pixel is repeated, so "shift out" every second time
            spr->dataShiftReg <<= 1;			

        if (spr->useExpandX)
            spr->expandXFlop ^= 1;
        else
            spr->expandXFlop = true;

        // if pixel is x-expanded and uses multicolor, the 2 bit "shift out" is
        // repeated for three more pixel, see below
        // pixel | x-flop | mc-flop | shift out | shift count
        // --------------------------------------------------
        // 1	 | 1	  | 1		| 2	bits	| 1 bit
        // 2	 | 0	  | 0		| unchanged	| -
        // 3	 | 1	  | 0		| unchanged	| 2 bits ( both bits for this pixel )
        // 4	 | 0	  | 1		| unchanged	| -
        // 5	 | 1	  | 1		| 2 bits	| 3 bits			
    }

    if (spr->shiftOut) {
        // the lowest non-transparent sprite has prio at this pixel position
        sprUse = spr;
        // remember all non-transparent sprites for this pixel position
        collision |= mask;
    } else {
		spr->dataShiftReg &= 0xffffff;
	}
}

inline auto VicIICycle::borderArea( bool hFlipFirstHalf ) -> void {
		
	if (hFlipFirstHalf == hFlipFlop) {
		if (hFlipFlop)
			std::memset( render, 0x20, 8 );
		
	} else {
		
		if (!hFlipFlop)
			std::memset( render, 0x20, cSel ? 4 : 3 );
		else {
			if (cSel)
				std::memset( &render[4], 0x20, 4 );
			else
				std::memset( &render[3], 0x20, 5 );
		}			
	}	
}

inline auto VicIICycle::draw65( unsigned offset, uint8_t x, uint8_t x1, uint8_t metaData ) -> void {
    // color regs will be evaluated one pixel sooner than 85xx chips
    // a register change, which can happen between the half cycles, will
    // show the old value one pixel more
	
    renderPipe[x1] = colorUse[ renderPipe[x1] ];
    
	// aec darkens luma in second half cycle always
	lineBuffer[offset + x] = metaData | renderPipe[x];
    
    renderPipe[x] = render[ x ];
}

inline auto VicIICycle::draw85( unsigned offset, uint8_t x, uint8_t metaData, bool greyDot ) -> void {
        
    // if same color register was written a cycle before and is accessed on fifth pixel the grey dot bug will happen
    //if ( x == 4 && (lastColorReg == renderPipe[4]) )
	if (greyDot)
        renderPipe[x] = 0x0f; // grey dot for newer chips
    else
		// colors within 0xf are direct colors
		// for all other color codes we fetch the proper
		// register values ( 2 cycles later than dma fetch ends )
        renderPipe[x] = colorUse[ renderPipe[x] ];    

	// aec darkens luma in second half cycle always
	lineBuffer[offset + x] = metaData | renderPipe[x];
    // now this slot is free for next pixel
	// waiting another cycle before applying
	// color registers
    renderPipe[x] = render[ x ];
}

inline auto VicIICycle::draw( uint8_t metaDataFirstHalf ) -> void {

	unsigned offset = linePos;
	uint8_t metaDataSecondHalf = (1 << 5) | (baLow << 4);

	if (rev65) {
		draw65( offset, 0, 1, metaDataFirstHalf );
		draw65( offset, 1, 2, metaDataFirstHalf );
		draw65( offset, 2, 3, metaDataFirstHalf );
		draw65( offset, 3, 4, metaDataFirstHalf );
		
		if (lastColorReg != 0xff) {
			colorUse[ lastColorReg ] = colorReg[ lastColorReg ];
			lastColorReg = 0xff;
		}
		
		draw65( offset, 4, 5, metaDataSecondHalf );
		draw65( offset, 5, 6, metaDataSecondHalf );
		draw65( offset, 6, 7, metaDataSecondHalf );
		draw65( offset, 7, 0, metaDataSecondHalf );
	} else {
		draw85( offset, 0, metaDataFirstHalf );
		draw85( offset, 1, metaDataFirstHalf );
		draw85( offset, 2, metaDataFirstHalf );
		draw85( offset, 3, metaDataFirstHalf );

		if (lastColorReg != 0xff) {
			colorUse[ lastColorReg ] = colorReg[ lastColorReg ];						
			draw85( offset, 4, metaDataSecondHalf, greyDotBugDisabled ? false : (lastColorReg == renderPipe[4]) );
			lastColorReg = 0xff;
		} else
			draw85( offset, 4, metaDataSecondHalf );
		
		draw85( offset, 5, metaDataSecondHalf );
		draw85( offset, 6, metaDataSecondHalf );
		draw85( offset, 7, metaDataSecondHalf );
	}
	
	linePos += 8;
}

}
