
/**
 * register change timings are the findings of the VICE team, thank you
 */

#include "vicII.h"

namespace LIBC64 {
    
inline auto VicII::sequencer(  ) -> void {            	    

    xCounter = xLookupPtrPhi1[cycle];
    checkLightPen<true>();
    
    sequencerPix0<true>(  );
    
    sequencerPix1<true>(  );
    
    sequencerPix2<true>(  );
    
    sequencerPix3<true>(  );
    
    borderArea<true>(  );
    
    draw<true>(  );

    if (onHalfCycle) {
        onHalfCycle();
        onHalfCycle = nullptr;
    }
    
    borderControl();
    clearCollisions();
    updateBAState();
    
    xCounter = xLookupPtrPhi2[cycle];
    checkLightPen<false>();

    if (lastColorReg != 0xff)
        colorUse[ lastColorReg ] = colorReg[ lastColorReg ];
    
    sequencerPix0<false>();

    sequencerPix1<false>();

    sequencerPix2<false>();

    sequencerPix3<false>();

    borderArea<false>();

    draw<false>();
}

inline auto VicII::sequencerSilent(  ) -> void {            	    

    xCounter = xLookupPtrPhi1[cycle];
    checkLightPen<true>();    

    if (onHalfCycle) {
        onHalfCycle();
        onHalfCycle = nullptr;
    }
    
    borderControl();
    clearCollisions();
    updateBAState();
    
    xCounter = xLookupPtrPhi2[cycle];
    checkLightPen<false>();

    if (lastColorReg != 0xff)
        colorUse[ lastColorReg ] = colorReg[ lastColorReg ];
}

template<bool phi1> inline auto VicII::sequencerPix0(  ) -> void {   
	
    if (phi1) {			
		if (rev65)
            modeEcmBmmSequencer |= modeEcmBmm; // 0 -> 1 transitions: update ECM + BMM directly after write half cycle       
        
        if ( spriteDmaCycle2 & 0x40 )
            sprite[spriteDmaCycle2 & 7].dataShiftReg = sprite[spriteDmaCycle2 & 7].dataS;

    } else {
       
        // like graphic processing, sprite processing is delayed 8 pixel too.
        // the x-coordinate of a sprite is compared with the x counter, which
        // is 8 pixel in the past to keep alignment with the background.
        // it seems the xCounter is latched each cycle
        // or each xCounter increment is latched and compared a cycle later with
        // sprite x positions.
        // we take the latch between the half cycles in order to keep aligned with
        // xCounter "wrap around" value of 0x1f8 or 0x200 for ntsc.
        // so we dont't need to wrap around the sprite-x compare positions too.
        xCounterSprites = xCounterLatch;   
        xCounterLatch = xCounter;

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
            if ( (xCounterSprites & 0x1f8) == (sprite[i].useX & 0x1f8) )
                spriteTrigger |= 1 << i;           
        }
        
		canSpriteSpriteCollisionIrq = spriteSpriteCollided == 0;
		canSpriteForegroundCollisionIrq = spriteForegroundCollided == 0;
		pipeGraphic();			
	}
					
	graphicSequencer( phi1 ? 4 : 0 );
    triggerSprites( xCounterSprites );
	spriteSequencer(  );
    render[0] = display.color;
}

template<bool phi1> inline auto VicII::sequencerPix1(  ) -> void {

    if (phi1) {                            
        if (rev65 && disableEcmBmmTogether)
            modeEcmBmmSequencer &= modeEcmBmm;            
    }
    
    graphicSequencer( phi1 ? 5 : 1 );
    triggerSprites( xCounterSprites + 1 );
	spriteSequencer(  );
    render[1] = display.color;
}

template<bool phi1> inline auto VicII::sequencerPix2( ) -> void {
	
    if (phi1) {
        if (rev65)
            modeEcmBmmSequencer &= modeEcmBmm; // 1 -> 0  transitions: update ECM + BMM 2 pixel later	
        
		else if (updateMc)
			updateMc8565();
		
        if (updatePrioExpand) {
            updatePrioExpand = 0;
            
            for(unsigned i=0; i < 8; i++) {
                sprite[i].usePrioMD = sprite[i].prioMD;
                sprite[i].useExpandX = sprite[i].expandX;
            }    
        }
        
	} else {
		
		if ( spriteDmaCycle2 & 0x80 ) { // when sprite dma completes this cycle
            sprite[spriteDmaCycle2 & 7].active = false;                                     
            spriteDmaCycle2 |= 0x40;
        }
	}
	
	graphicSequencer( phi1 ? 6 : 2 );
    triggerSprites( xCounterSprites + 2 );
	spriteSequencer( );
    render[2] = display.color;
}

template<bool phi1> inline auto VicII::sequencerPix3(  ) -> void {
    
    if (phi1) {
		if ( modeMcm && !modeMcmSequencer )
			display.mcFlop = 0;
		
        modeMcmSequencer = modeMcm;
        
		if ( spriteDmaCycle2 & 0x40 ) {
            sprite[spriteDmaCycle2 & 7].halt = false;
            spriteDmaCycle2 = 0;
        }
		
		if (updateMc && rev65)
			updateMc6569();
		
	} else {
		
		if ( spriteDmaCycle1 & 0x80 )  {// first sprite dma cycle
            sprite[spriteDmaCycle1 & 7].halt = true;	
            spriteDmaCycle1 = 0;
        }
    }
			
	graphicSequencer( phi1 ? 7 : 3 );
    triggerSprites( xCounterSprites + 3 );
	spriteSequencer(  );
    render[3] = display.color;
    
    if (phi1) {    
        if (!rev65)
            modeEcmBmmSequencer = modeEcmBmm; // update ECM & BMM
               
            for( unsigned i = 0; i < 8; i++ )
                // changed sprite x register have 4 pixel delay
                sprite[i].useX = sprite[i].x;
		        
    } else {
		xCounterSprites += 4;
    }            
}

inline auto VicII::pipeGraphic() -> void {
	// 8 pixel delay before processing starts
	display.cBufferPipe2 = display.cBufferPipe1;
	display.gBufferPipe2 = display.gBufferPipe1;
	
	if (display.enable && !vFlipFlop) {
		display.gBufferPipe1 = display.gBuffer;
        display.gBufferUse = true;
        
		display.xScroll = controlReg2 & 7;

		if (!idleMode) 
			display.cBufferPipe1 = display.cBuffer[display.dmli];
		else
			display.cBufferPipe1 = 0;

	} else {
		display.gBufferPipe1 = 0;
        display.gBufferUse = false;
    }

	if (display.enable)
		display.dmli++;
	else
		display.dmli = 0;
}

inline auto VicII::graphicSequencer( uint8_t x ) -> void {
		
	if ( x == display.xScroll ) {
		display.gBufferShift = display.gBufferPipe2;
		display.dataC = display.cBufferPipe2;
		display.mcFlop = 1;
	}
	
	if ( modeMcmSequencer ) {
		
		if ( (modeEcmBmmSequencer & 2) || ( (display.dataC >> 11) & 1 ) ) {
			
			if (display.mcFlop)
				// hold for next pixel too
				display.gBits = display.gBufferShift >> 6;
		} else
			display.gBits = display.gBufferShift & 0x80 ? 2 : 0;			
		
	} else
		display.gBits = display.gBufferShift & 0x80 ? 2 : 0;	

	display.gBufferShift <<= 1;
	display.mcFlop ^= 1;
	display.color = 0x21;

	switch( modeEcmBmmSequencer | modeMcm ) {
		case 0:
			if (display.gBits & 2)
				display.color = (display.dataC >> 8) & 15;
			break;
		case 1:
			if ((display.dataC >> 11) & 1 ) {
				if ( display.gBits == 3 )
					display.color = (display.dataC >> 8) & 7;
				else
					display.color += display.gBits;
			} else {
				if (display.gBits & 2)
					display.color = (display.dataC >> 8) & 7;
			}
			break;
		case 2:
			display.color = (display.gBits & 2) ? ((display.dataC >> 4) & 15) : (display.dataC & 15);
			break;
		case 3:			
			if (display.gBits == 1)			display.color = (display.dataC >> 4) & 15;
			else if (display.gBits == 2)	display.color = display.dataC & 15;
			else if (display.gBits == 3)	display.color = (display.dataC >> 8) & 15;
			break;
		case 4:
			if (display.gBits & 2)
				display.color = (display.dataC >> 8) & 15;
			else
				display.color += ((display.dataC >> 6) & 3);
			break;
		case 5:
		case 6:
		case 7:
			display.color = 0;
			break;			
	}
}

inline auto VicII::triggerSprites( uint16_t xPos ) -> void {
    
    if ( !spriteTrigger || !spritePending )
        return;
    
    // removed the loop for speed up reasons.
    // inline templating creates a single loop free function.
    // same effect like a define macro but better readable.
    // that gives 4 extra fps in my case.
    triggerSprites<0>( xPos );
    triggerSprites<1>( xPos );
    triggerSprites<2>( xPos );
    triggerSprites<3>( xPos );
    triggerSprites<4>( xPos );
    triggerSprites<5>( xPos );
    triggerSprites<6>( xPos );
    triggerSprites<7>( xPos ); 
}

template<uint8_t sprPos> inline auto VicII::triggerSprites( uint16_t xPos ) -> void {
    
    Sprite* spr = &sprite[sprPos];

    // in halt state a few x coordinates between first and second dma cannot trigger
    if ( !spr->active && !spr->halt && (spriteTrigger & (1 << sprPos)) && (spritePending & (1 << sprPos)) )
        if ( xPos == spr->useX ) {                
            // can not retrigger until sprite is shifted out completly or second sprite dma finishes                
            spr->active = true;
            // init the flops
            spr->expandXFlop = true;
            spr->mcFlop = true;
        }
}

inline auto VicII::updateMc6569() -> void {
    
    for(unsigned i=0; i < 8; i++) {
        Sprite& spr = sprite[i];
        // is bit changed: 0 > 1 or 1 > 0
        bool toggled = spr.multiColor ^ spr.useMultiColor;
        // when register changed, reset the mc flop, unchanged otherwise
        spr.mcFlop &= ~toggled;
		// now use register value in calculation
        spr.useMultiColor = spr.multiColor;
    }
    
    updateMc = false;
}

inline auto VicII::updateMc8565() -> void {
    
    for(unsigned i=0; i < 8; i++) {
        Sprite& spr = sprite[i];
        
        bool toggled = spr.multiColor ^ spr.useMultiColor;        
		// mc flop is toggled, when register is changed and sprite is not x expanded
		// unchanged oterwise
        spr.mcFlop ^= toggled & ~spr.expandXFlop;
        
        spr.useMultiColor = spr.multiColor;
    }
    
    updateMc = false;
}

inline auto VicII::spriteSequencer(  ) -> void {
    Sprite* sprUse = nullptr;
	uint8_t collision = 0;
    
    if(sprite7->active) spriteSequencer<7>( sprite7, sprUse, collision );
    if(sprite6->active) spriteSequencer<6>( sprite6, sprUse, collision );
    if(sprite5->active) spriteSequencer<5>( sprite5, sprUse, collision );
    if(sprite4->active) spriteSequencer<4>( sprite4, sprUse, collision );
    if(sprite3->active) spriteSequencer<3>( sprite3, sprUse, collision );
    if(sprite2->active) spriteSequencer<2>( sprite2, sprUse, collision );
    if(sprite1->active) spriteSequencer<1>( sprite1, sprUse, collision );
    if(sprite0->active) spriteSequencer<0>( sprite0, sprUse, collision );

    if (sprUse) {
        // at least one non transparent sprite pixel is found 
        // ok we have to find out if background pixel or sprite pixel is higher priorized

        // if display color is background, sprite prio doesn't matter
        if (!display.isForeground() || !sprUse->usePrioMD) {
            // ok sprite wins
            // we set the register positions only, not the contained values
            // the values will be fetched later, so we don't miss late register changes
            if (sprUse->shiftOut == 1) display.color = 0x25; // mc color register 1
            else if (sprUse->shiftOut == 3) display.color = 0x26; // mc color register 2
            else if (sprUse->shiftOut == 2) display.color = sprUse->colorCode; // sprite color
            // shiftOut == 0 can not happen at this point
        }

        if (display.isForeground())
            // if pixel is foreground all non transparent sprite pixel will be seted if not already
            spriteForegroundCollided |= collision;

        if (collision & (collision - 1))
            // at least two sprites have to be non transparent
            // otherwise no bit is set
            spriteSpriteCollided |= collision;
    }    
}

template<uint8_t sprPos> inline auto VicII::spriteSequencer( Sprite* spr, Sprite*& sprUse, uint8_t& collision ) -> void {
			
    // active, but no data left or last "shift Out" contains no data anymore (transparent)
    if ( !(spr->dataShiftReg || spr->shiftOut) ) {
        spr->active = false;
        return;
    }
    // in halted state, last "shift out" is used always and no flip flops will be toggled
    if (!spr->halt) {			
        if (spr->expandXFlop) {				
            if (spr->useMultiColor) {
                // 2 bits per pixel (repeated)
                if (spr->mcFlop)
                    spr->shiftOut = (spr->dataShiftReg >> 22) & 3;

                spr->mcFlop ^= 1; //repeat last shift out for second pixel
            } else
                spr->shiftOut = ((spr->dataShiftReg >> 23) & 1) << 1; // 2: sprite color, 0: transparent
        }

        if (spr->expandXFlop)
            // pixel is repeated, so "shift out" every second time
            spr->dataShiftReg <<= 1;			

        if (spr->useExpandX)
            spr->expandXFlop ^= 1;
        else
            spr->expandXFlop = 1;

        // if pixel is x-expanded and uses multi color, the 2 bit "shift out" is
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
        // the lowest non transparent sprite has prio at this pixel position
        sprUse = spr;
        // remember all non transparent sprites for this pixel position
        collision |= 1 << sprPos;
    }
}

template<bool phi1> inline auto VicII::borderArea(  ) -> void {
    
    if ( !hFlipFlop )
        // no border area
        // sprites are only visible in border area if hFlipFlop is off 
        return;
    
	// so if hflip keeps seted, the first we have todo in the following
    // sequencer step is to overwrite the pixel before with border state.   
    // this pixel is already in render pipe so we update it there.
    // this hack is needed because of only half cycle accuracy between
    // dma and sequencer.
    if ( !phi1 && !cSel )
        renderPipe[3] = 0x20;
	
    if ( !phi1 || cSel ) {
        // set border for all pixel of this half cycle
        std::memset( render, 0x20, 4 );
        return;
    }
    
    // set border for the first three pixel of second half cycle only.
    // thats a bit annoying. the sequencer, which process 4 pixel, runs before
    // general cycle logic in this emulation, because it's more comfortable.
    // when hflip changes in first half cycle, the switch happens after 3 pixel
    // in sequencer processing for cSel == 0
    std::memset( render, 0x20, 3 );   
}

template<bool phi1> inline auto VicII::draw65( uint8_t x, uint8_t x1 ) -> void {
    // color regs will be evaluated one pixel sooner than 85xx chips
    // a register change, which can happen between the half cycles, will
    // show the old value one pixel more
    // uint8_t x1 = (x + 1) & 7;
    
    renderPipe[x1] = colorUse[ renderPipe[x1] ];
    
    if(visibleLine)
        // aec darkens luma in second half cycle always
        *( linePtr + linePos++ ) = ((!phi1 | !aecDelay) << 9) | (baLow << 8) | renderPipe[x];
    
    renderPipe[x] = render[ x & 3 ];
}

template<bool phi1> inline auto VicII::draw85( uint8_t x ) -> void {
        
    // if same color register was written a cycle before and is accessed on fifth pixel the grey dot bug will happen
    if ( x == 4 && (lastColorReg == renderPipe[4]) )
        renderPipe[x] = 0x0f; // grey dot for newer chips
    else
		// colors within 0xf are direct colors
		// for all other color codes we fetch the proper
		// register values ( 2 cycles later than dma fetch ends )
        renderPipe[x] = colorUse[ renderPipe[x] ];
    // write back final color to frame buffer
    if(visibleLine)
        // aec darkens luma in second half cycle always
        *( linePtr + linePos++ ) = ((!phi1 | !aecDelay) << 9) | (baLow << 8) | renderPipe[x];
    // now this slot is free for next pixel
	// waiting another cycle before applying
	// color registers
    renderPipe[x] = render[ x & 3 ];
}

template<bool phi1> inline auto VicII::draw(  ) -> void {
        
    if (rev65) {
        draw65<phi1>( phi1 ? 0 : 4, phi1 ? 1 : 5 );
        draw65<phi1>( phi1 ? 1 : 5, phi1 ? 2 : 6 );
        draw65<phi1>( phi1 ? 2 : 6, phi1 ? 3 : 7 );
        draw65<phi1>( phi1 ? 3 : 7, phi1 ? 4 : 0 );
    } else {
        draw85<phi1>( phi1 ? 0 : 4 );
        draw85<phi1>( phi1 ? 1 : 5 );
        draw85<phi1>( phi1 ? 2 : 6 );
        draw85<phi1>( phi1 ? 3 : 7 );        
    }
    	                
}

}