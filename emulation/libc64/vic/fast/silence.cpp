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
        }

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

    } else if (cycle == 10) {
        if (!allowBadlines && (vCounter == 0x30) && den)
            allowBadlines = true;

        badLine = allowBadlines && (yScroll == (vCounter & 7));

        setRdy(badLine);
        cAccessArea = true;
    } else if (cycle == 15) {

        dmaSpritesOff();

    } else if (cycle == 53) {
        setRdy(false);
        cAccessArea = false;
        dmaSprites();
    } else if (!cAccessArea) {
        setRdy(spriteBa[8][ cycle ]);
    }

    setLineInterrupt();

    if (lpTrigger)
        checkLightPenNew();
}

}
