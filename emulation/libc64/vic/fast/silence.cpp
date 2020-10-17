#include "vicIIFast.h"

namespace LIBC64 {    

auto VicIIFast::clockSilence() -> void {
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
            visibleLine = true;

        } else if (lineVCounter == vHeight) {
            visibleLine = false;

            videoRefresh(nullptr, 0, 0, 0);

            lineVCounter = 0;
        }

        if (visibleLine) {
            linePtr = frameBuffer + lineVCounter * VIC_MAX_LINE_LENGTH;
            lineVCounter++;
        }

        setRdy( spriteBaTabPtr[cycle] );

    } else if (cycle == 11) {

        setRdy(badLine);
        cAccessArea = true;
   
    } else if (cycle == 20) {

        dmaSpritesOff();

    } else if (cycle == 54) {
        cAccessArea = false;
        dmaSprites();

        setRdy( spriteBaTabPtr[cycle] );

        dmaDelay = 0;
        
        if ( canSpriteSpriteCollisionIrq && spriteSpriteCollided) {
            canSpriteSpriteCollisionIrq = false;
            updateIrq( Interrupt::MMC );
        }
        
        if ( canSpriteForegroundCollisionIrq && spriteForegroundCollided) {
            canSpriteForegroundCollisionIrq = false;
            updateIrq( Interrupt::MBC );
        }
        
    } else if (!cAccessArea) {

        setRdy( spriteBaTabPtr[cycle] );
    }

    setLineInterrupt();

    if (lpTrigger)
        checkLightPenNew();
}

}
