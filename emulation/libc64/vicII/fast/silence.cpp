
#include "vicIIFast.h"
#include "../../system/system.h"

namespace LIBC64 {    

auto VicIIFast::clockSilence() -> void {
    if (irqLatchPending) {
        irqLatch |= irqLatchPending & 0x7f;
        updateIrq();
        irqLatchPending = 0;
    }

    if (initVCounter) {
        if (debugger.storeSprites)
            debugger.resetSpriteStore();
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
            visibleLine = true;

        } else if (lineVCounter == vHeight) {
            visibleLine = false;

            system->videoRefresh(nullptr, 0, 0, 0);

            lineVCounter = 0;
        }

        if (visibleLine)
            lineVCounter++;        

		flags = cycleTab[0];

		setRdy(spriteDma & getSpriteBA(flags));   

    } else {
				
		flags = cycleTab[cycle];
		
		if (isStartPhi2(flags)) {

			setRdy( badLine );			

		} else if (isScanlineRender( flags )) {

			dmaSpritesOff();

		} else if (isScanlineRenderFin( flags )) {
			
			dmaSprites();

			setRdy( spriteDma & getSpriteBA( flags ) );

			dmaDelay = 0;

			if ( canSpriteSpriteCollisionIrq && spriteSpriteCollided) {
				canSpriteSpriteCollisionIrq = false;
				updateIrq( Interrupt::MMC );
			}

			if ( canSpriteForegroundCollisionIrq && spriteForegroundCollided) {
				canSpriteForegroundCollisionIrq = false;
				updateIrq( Interrupt::MBC );
			}

		} else if ( !isBgBA( flags ) ) {

			setRdy( spriteDma & getSpriteBA( flags ) );
		}
	}
	
    setLineInterrupt();

    if ( unlikely( lpTrigger ) ) {
		xCounterLatch = getXpos( flags );	
        checkLightPen();
	}
}

}
