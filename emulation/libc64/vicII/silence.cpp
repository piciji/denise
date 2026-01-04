#include "vicII.h"

namespace LIBC64 {    

// software can detect silencer only by checking collisions or by checking 'lastReadPhi1'.
// it's safe for games to use it in warp mode. only a few demos / tests fail.
// for example: ef_test.crt which relies on last bus value of first half cycle
	
auto VicIICycle::clockSilence() -> void {

	if (isSprFirstCycle(flags)) {
		uint8_t sprPos = getSpr(flags);

		if (sprHasDma(sprPos)) {
			sprite[sprPos].mc++;
			sprite[sprPos].mc &= 63;
		}

	} else if (isSprSecondCycle(flags)) {
		uint8_t sprPos = getSpr(flags);

		if (sprHasDma(sprPos)) {
			sprite[sprPos].mc++;
			sprite[sprPos].mc &= 63;
		}
	}
	
    if (irqLatchPending) {
        irqLatch |= irqLatchPending & 0x7f;
        updateIrq();
        irqLatchPending = 0;
    }

    if ( !allowBadlines && (vCounter == 0x30) && den )
		allowBadlines = true;
    
    if (initVCounter) {
        if (debugInfo.store)
            debugInfo.reset();
        vCounter = 0;
        initVCounter = false;
        lpLatched = false;
        // retrigger happens in last pixel of second cycle for all Vic types,
        // if lp line is held low in beginning of cycle
        if (!lpPin)
            triggerLightPen(false, 3);

        vcBase = vc = 0;
        refreshCounter = 0xff;
        allowBadlines = false;
    }

    if (++cycle == lineCycles) {
        cycle = 0;
        if (linePos)
            linePos = 0; // this matters when switching between silence and normal mode outside of vblank, could be happen while long REU transfer

        // Note: line complete but vcounter is not incremented at this point
        if (vCounter == 0xf7)
            allowBadlines = false;

        if (++vCounter == lines) {
            // last line is not reseted this cycle but next
            vCounter -= 1;
            initVCounter = true;
        } else {
            if (!allowBadlines && (vCounter == 0x30) && den)
                allowBadlines = true;
        }
		
		if (visibleLine)
			lineVCounter++;
		
        if (vCounter == vStart) {
            visibleLine = true; // non v-blank

        } else if (lineVCounter == vHeight) {
            visibleLine = false; // v-blank

            system->videoRefresh(nullptr, 0, 0, 0);
            
            lineVCounter = 0;
        }
    }
        
    sprite0DmaLateBA = false;
    
	flags = cycleTab[cycle];
    updateBadLine();
    setLineInterrupt();

	if (isSprDma(flags))
		spriteDmaCheck();

    borderControl();
	if (unlikely(clearCollision))
		clearCollisions();
    updateBAState( flags );

	xCounterLatch = getXpos( flags );
	if (unlikely(lpTrigger))
		checkLightPen();
	
    if (lastColorReg != 0xff) {
        colorUse[ lastColorReg ] = colorReg[ lastColorReg ];
		lastColorReg = 0xff;
	}

	if (isSprDisp(flags))
		spriteDisplayCheck();
	else if (isSprMcBase(flags))
		spriteUpdateBase();
	else if (isSprExp(flags))
		spriteExpand();
	
	if (isSprSecondCycle(flags)) {
		uint8_t sprPos = getSpr(flags);
		
		if (sprHasDma( sprPos ) ) {
			sprite[sprPos].mc++;
			sprite[sprPos].mc &= 63;
		}
	}	
	
	if (isUpdateVc(flags))
		updateVc();
	else if (isUpdateRc(flags))
		updateRc();   
    
    modeEcmBmmDma = modeEcmBmm;   
}

}
