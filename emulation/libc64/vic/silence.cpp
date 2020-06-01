#include "vicII.h"

namespace LIBC64 {    

auto VicIICycle::clockSilence() -> void {
    if (irqLatchPending) {
        irqLatch |= irqLatchPending & 0x7f;
        updateIrq();
        irqLatchPending = 0;
    }

    if ( !allowBadlines && (vCounter == 0x30) && den )
		allowBadlines = true;
    
    if (initVCounter) {
        vCounter = 0;
        initVCounter = false;
        lpLatched = false;
        // retrigger happens in last pixel of second cycle for all Vic types,
        // if lp line is held low in beginning of cycle
        if (!lpPin)
            triggerLightPen(false, 3);

        display.vcBase = display.vc = 0;
        refreshCounter = 0xff;
        allowBadlines = false;
    }

    if (++cycle == lineCycles) {
        cycle = 0;

        // Note: line complete but vcounter is not incremented at this point
        if (vCounter == 0xf7)
            allowBadlines = false;

        if (++vCounter == (ntsc ? 263 : 312)) {
            // last line is not reseted this cycle but next
            vCounter = ntsc ? 262 : 311;
            initVCounter = true;
        } else {
            if (!allowBadlines && (vCounter == 0x30) && den)
                allowBadlines = true;
        }

        if (vCounter == vStart) {
            visibleLine = true; // non v-blank

        } else if (lineVCounter == vHeight) {
            visibleLine = false; // v-blank

            videoRefresh(nullptr, 0, 0, 0);
            
            lineVCounter = 0;
        }

    } else if (cycle == 1)
        setLineBuffer();

    spriteOpenBus = nullptr;
    sprite0DmaLateBA = false;
    
    updateBadLine();
    setLineInterrupt();

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
    
    
    switch (cycle) {
        case 0: if (ntsc)   { dummySpriteDma<true>(sprite3); } 
                else        { dummySpriteDma(sprite3); }
                break;
        case 1: if (ntsc)   { dummySpriteDma(sprite4); }
                else        { dummySpriteDma<true>(sprite3); }
                break;
        case 2: if (ntsc)   { dummySpriteDma<true>(sprite4); }
                else        { dummySpriteDma(sprite4); }
                break;
        case 3: if(ntsc)    { dummySpriteDma(sprite5); }
                else        { dummySpriteDma<true>(sprite4); }
                break;
        case 4: if(ntsc)    { dummySpriteDma<true>(sprite5); }
                else        { dummySpriteDma(sprite5); }
                break;
        case 5: if(ntsc)    { dummySpriteDma(sprite6); }
                else        { dummySpriteDma<true>(sprite5); }
                break;
        case 6: if(ntsc)    { dummySpriteDma<true>(sprite6); }
                else        { dummySpriteDma(sprite6); }
                break;
        case 7: if(ntsc)    { dummySpriteDma(sprite7); }
                else        { dummySpriteDma<true>(sprite6); }
                break;
        case 8: if(ntsc)    { dummySpriteDma<true>(sprite7); }
                else        { dummySpriteDma(sprite7); }
                break;
        case 9: if(ntsc)    {  }
                else        { dummySpriteDma<true>(sprite7); }
                break;
        case 10: 
            cAccessArea = true;
            break;

        case 15:            
            spriteUpdateBase();
            break;

        case 53:            
            cAccessArea = false;
            if (!ntsc)
                onHalfCycle = [this]() { spriteDmaCheck(); };
            break;
            
        case 54:    
            onHalfCycle = [this]() {  spriteDmaCheck(); };
            break;
            
        case 55:
            spriteFlip();
            onHalfCycle = [this]() { if(ntsc) spriteDmaCheck(); };
            break;

        case 57:    if (ntsc)   {  }
                    else        { spriteDisplayCheck(); dummySpriteDma(sprite0); }            
                    break;
        case 58:    if (ntsc)   { spriteDisplayCheck(); dummySpriteDma(sprite0); }
                    else        { dummySpriteDma<true>(sprite0); }
                    break;
        case 59:    if (ntsc)   { dummySpriteDma<true>(sprite0); }
                    else        { dummySpriteDma(sprite1); }
                    break;
        case 60:    if (ntsc)   { dummySpriteDma(sprite1); }
                    else        { dummySpriteDma<true>(sprite1); }
                    break;
        case 61:    if (ntsc)   { dummySpriteDma<true>(sprite1); }
                    else        { dummySpriteDma(sprite2); }
                    break;
        case 62:    if (ntsc)   { dummySpriteDma(sprite2); }
                    else        { dummySpriteDma<true>(sprite2); }
                    break;
        // ntsc only
        case 63:    dummySpriteDma<true>(sprite2);
                    break;
        case 64:    dummySpriteDma(sprite3);
                    break;
    }     
    
    modeEcmBmmDma = modeEcmBmm;
    lastColorReg = 0xff;
}

template<bool doubleStep> inline auto VicIICycle::dummySpriteDma(Sprite* spr) -> void {
    if ( spr->dma ) {
        spr->mc++;
        spr->mc &= 63;
        
        if (doubleStep) {
            spr->mc++;
            spr->mc &= 63;            
        }
    }
}

}
