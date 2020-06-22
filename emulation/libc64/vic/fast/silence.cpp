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

        if (++vCounter == (ntsc ? 263 : 312)) {
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

        setRdy(spriteBa[8][ cycle ]);

    } else if (cycle == 11) {

        setRdy(badLine);
        cAccessArea = true;
   
    } else if (cycle == 20) {

        dmaSpritesOff();

    } else if (cycle == 54) {
        cAccessArea = false;
        dmaSprites();
        setRdy( spriteBa[8][ cycle ] );

        dmaDelay = 0;
        
        if (spriteSpriteCollided)
            updateIrq(Interrupt::MMC);

        if (spriteForegroundCollided)
            updateIrq(Interrupt::MBC);
        
    } else if (!cAccessArea) {
        setRdy(spriteBa[8][ cycle ]);
    }

    setLineInterrupt();

    if (lpTrigger)
        checkLightPenNew();
}

}
