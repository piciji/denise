
#include "vicII.h"
#include "register.cpp"
#include "dma.cpp"    
#include "sequencer.cpp"
#include "serialization.cpp"
#include "colorWheel.cpp"

namespace LIBC64 {
	
VicII* vicII = nullptr;
        
VicII::VicII() {
    frameBuffer = new uint16_t[VIC_MAX_LINE_LENGTH * 293];	
        
    ntsc = false;
    rev65 = true;
    
    sprite0 = &sprite[0];
    sprite1 = &sprite[1];
    sprite2 = &sprite[2];
    sprite3 = &sprite[3];
    sprite4 = &sprite[4];
    sprite5 = &sprite[5];
    sprite6 = &sprite[6];
    sprite7 = &sprite[7];
    
    initColorWheel();
    
	lineCallback.use = false;
	lineCallback.finishVblank = false;
}

VicII::~VicII() {
	delete frameBuffer;
}
// pal / ntsc
auto VicII::setNtsc( bool state ) -> void {
    ntsc = state;
}
// revision 65xx / 85xx
auto VicII::setRevision65( bool state ) -> void {
	rev65 = state;
}

auto VicII::isRevision65() -> bool {
    return rev65;
}

auto VicII::getLastReadedValue() -> uint8_t {
    return lastReadPhi1;
}

auto VicII::updateIrq( Interrupt interrupt ) -> void {       
    
    if ( interrupt != Interrupt::None ) {
        irqLatch |= 1 << interrupt;
    }
    
    if (irqLatch & irqEnable) {
        irqLatch |= 0x80;
        setIrq( true );        
		
    } else {        
        irqLatch &= 0x7f;        
        setIrq( false );                
    }
}

auto VicII::getCurrentLinePtr() -> uint16_t* {
    
    return linePtr + linePos;
}

auto VicII::triggerLightPen( bool state ) -> void {
	// trigger by writing to cia
    lpPin = state;
	
	if (lpPin)
		return;	    
    
	lpTriggerDelay = rev65 ? 2 : 1;
    lpPhi1 = false;
}

auto VicII::triggerLightPen( bool state, uint8_t subCycle ) -> void {
    lpPin = state;
	
	if (lpPin)
		return;	    
    
    lpPhi1 = true;
    lpTriggerDelay = subCycle & 1;
    
    if (subCycle & 2)
        // happens in second half cycle
        lpPhi1 = false;            
}

template<bool phi1> auto VicII::checkLightPen( ) -> void {
        
    //if(lpLatched || !lpTrigger)
    if(lpPin || lpLatched || (phi1 != lpPhi1) )
        return;
	
	lpLatched = true;
	
	if ( (vCounter == (ntsc ? 261 : 311)) && (cycle > 0) )
		return;
        
    // 2 adjacent pixel [4,5] [6,7] give the same value for lpx, because of
	// the last bit is shifted out. it's a division by 2.
	// this code fire between the half cycles in pixel 4.
	// for the 8565 the latch happens by pixel 7, btw. pixel 6 would give the same.
	// for the 6569 the latch happens one pixel later, but it's already the next
	// 2 pixel block. 
    lpx = xCounter >> 1;
	// add one block for 85xx and 2 blocks for 65xx
	lpx += lpTriggerDelay;
	//lpTrigger = 0;
	
    lpy = vCounter & 0xff;		

    if (phi1)
        updateIrq( Interrupt::LP );
    else        
        // cpu mustn't recognize it this cycle, but next
        lpIrqPending = true;        
        
}

auto VicII::getCyclesForNextLightTrigger( int x, int y, uint8_t& cyclePixel ) -> unsigned {
    
    x += firstVisiblePixel - ((cycle + 1) << 3);
    
    if (x < 104)
        return 0;
    
    if (ntsc)        
        y += vStart - vCounter;
    
    else 
        y += 312 - vCounter + vStart;           
    
    // which pixel
    cyclePixel = x & 7;
    
    return (x / 8) + ( y * lineCycles );    
}

auto VicII::updateBorderData() -> void {
    // is used for border cropping only, not a VicII feature
    if (visibleLine ) {
        
        if (!crop.rSel)
            crop.rSel = rSel;
        
        if (!crop.cSel)
            crop.cSel = cSel;
        
    } else {        
        crop.rSel = rSel;
        crop.cSel = cSel;                        
    }        
}

auto VicII::setBorderData() -> void {
    // is used for border cropping only, not a VicII feature
	if ( crop.cSel ) {
		crop.left = ntsc ? 56 : 46;
		crop.right = ntsc ? 44 : 40;

	} else {
		crop.left = ntsc ? 63 : 53;
		crop.right = ntsc ? 53 : 49;
	}	
	
	if (crop.rSel) {
		crop.top = ntsc ? 28 : 42;
		crop.bottom = ntsc ? 25 : 51;

	} else {
		crop.top = ntsc ? 32 : 46;
		crop.bottom = ntsc ? 29 : 55;
	}	
}

auto VicII::power() -> void {
    registerWrite.pipelined = false;
	crop.leftOverscan = ntsc ? (56 - 32) : (46 - 32);
	crop.rightOverscan = ntsc ? (44 - 32) : (40 - 32);
	crop.topOverscan = ntsc ? 5 : 7;
	crop.bottomOverscan = ntsc ? 1 : 14;
	
    lastReadPhi1 = 0;
    lastBusPhi2 = 0xff;
    std::memset(renderPipe, 0, 8);
    memset(colorReg, 0, sizeof(colorReg));
    memset(colorUse, 0, sizeof(colorUse));
    // direct colors
    for( unsigned i = 0; i <= 0xf; i++ )
        colorUse[i] = i;
        
    lastColorReg = 0xff;
    cycle = 0;
    vCounter = 0;
    xCounter = ntsc ? 412 : 404;    
    xCounterLatch = xCounterSprites = xCounter;
    xCounter += 8; // to compensate first advance cycle
    vStart = ntsc ? 23 : 9;
	vHeight = ntsc ? 253 : 293; // max possible display height  
    hWidth =  ntsc ? 420 : 406; // max possible display width  
    firstVisiblePixel = ntsc ? 76 : 86;
    lineCycles = ntsc ? 65 : 63;
	xWrapAround = ntsc ? 0x200 : 0x1f8;
	baLow = false;
    aecDelay = 0;
    std::memset(spriteBa, 0, sizeof spriteBa);
	allowBadlines = false;
    irqLine = 0;
    lineIrqMatched = false;
    lpIrqPending = false;
    lpx = 0;
    lpy = 0;
    vm = 0;
    cb = 0;
    irqLatch = 0;
    irqEnable = 0;
    lpLatched = false;  
    lpPin = true;
	lpTriggerDelay = 0;
    controlReg1 = 0;
    controlReg2 = 0;
    linePos = 0;
    lineVCounter = 0;
    linePtr = frameBuffer;
    visibleLine = false;
    hFlipFlop = true;
    vFlipFlop = vFlipFlopShadow = true;
    idleMode = true;    
    initVCounter = false;
    refreshCounter = 0xff;     

    modeEcmBmm = modeMcm = 0;
    modeEcmBmmDma = modeMcmDma = 0;
    modeEcmBmmSequencer = modeMcmSequencer = 0;
        
    display.color = 0;
    display.mcFlop = 0;
    display.dataC = 0;
    display.vcBase = 0;
    display.vc = 0;
    display.rc = 0;
    display.vmli = 0;
    std::memset(display.cBuffer, 0, sizeof display.cBuffer);
    display.cBufferPipe1 = display.cBufferPipe2 = 0;
    display.xScroll = 0;
    display.gBuffer = display.gBufferPipe1 = display.gBufferPipe2 = 0;
    display.enable = false;
    display.dmli = 0;
    display.gBufferShift = 0;
    display.gBits = 0;
    
    for ( unsigned i = 0; i < 8; i++ ) {
        sprite[i].enabled = false;
        sprite[i].dma = false;
        sprite[i].halt = false;
        sprite[i].active = false;
        
        sprite[i].dataP = 0;
        sprite[i].dataS = 0;
        sprite[i].dataShiftReg = 0;
        sprite[i].shiftOut = 0;

        sprite[i].mcBase = 0;
        sprite[i].mc = 0;
        
        sprite[i].x = 0;
        sprite[i].y = 0;
        sprite[i].useX = 0;
        sprite[i].prioMD = false;
        sprite[i].usePrioMD = false;
        sprite[i].expandX = false;
        sprite[i].expandY = false;
        sprite[i].useExpandX = false;
        sprite[i].multiColor = false;
        sprite[i].useMultiColor = false;
        sprite[i].mcFlop = false;
        sprite[i].expandYFlop = false;
        sprite[i].expandXFlop = false;
        sprite[i].colorCode = 0x27 + i;
    }
    
    spriteForegroundCollided = 0;
    spriteSpriteCollided = 0;
    spriteDmaCycle1 = 0;
    spriteDmaCycle2 = 0;
    spriteDisplayCycle = 0;
    canSpriteSpriteCollisionIrq = false;
    canSpriteForegroundCollisionIrq = false;
    
    writeIO( 0x11, controlReg1 );
    writeIO( 0x16, controlReg2 );
    
    spriteTrigger = 0;
    spriteDisplay = 0;
    spritePending = 0;
    updateMc = 0;
    updatePrioExpand = 0;
    cAccessArea = 0;
}

}

