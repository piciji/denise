
#include "base.h"
#include "register.cpp"
#include "colorWheel.cpp"
#include "verticalLineAnomaly.cpp"

namespace LIBC64 {  
    
VicIIBase* vicII = nullptr;    
    
VicIIBase::VicIIBase() {

    frameBuffer = new uint16_t[VIC_MAX_LINE_LENGTH * 294];
    std::fill_n(frameBuffer, VIC_MAX_LINE_LENGTH * 294, 0);

    addMeta = false;
	setModel( MOS6569R3 );

    sprite0 = &sprite[0];
    sprite1 = &sprite[1];
    sprite2 = &sprite[2];
    sprite3 = &sprite[3];
    sprite4 = &sprite[4];
    sprite5 = &sprite[5];
    sprite6 = &sprite[6];
    sprite7 = &sprite[7];
    
    sprite0->position = 0;
    sprite1->position = 1;
    sprite2->position = 2;
    sprite3->position = 3;
    sprite4->position = 4;
    sprite5->position = 5;
    sprite6->position = 6;
    sprite7->position = 7;

    initColorWheel();
    buildXCounterLookupTable();

    lineCallback.use = false;
    lineCallback.line = 0;
    lineCallback.finishVblank = false;    
}

VicIIBase::~VicIIBase() {
    delete frameBuffer;
}

auto VicIIBase::setMeta( bool state ) -> void {
    addMeta = state;
}

auto VicIIBase::disableSequencer(bool state) -> void {
    enableSequencer = !state;
}

auto VicIIBase::useSequencer() -> bool {
    return enableSequencer;
}

auto VicIIBase::isRevision65() -> bool {
    return rev65;
}

auto VicIIBase::setVerticalLineAnomaly(uint8_t mode) -> void {
    leftLineAnomaly.mode = mode;

    if (mode)
        initVerticalLineAnomaly();
}

auto VicIIBase::getVerticalLineAnomaly() -> uint8_t {
    return leftLineAnomaly.mode;
}

auto VicIIBase::updateIrq(Interrupt interrupt) -> void {

    if (interrupt != Interrupt::Update) {
        irqLatch |= 1 << interrupt;
    }

    if (irqLatch & irqEnable) {
        irqLatch |= 0x80;
        setIrq(true);

    } else {
        irqLatch &= 0x7f;
        setIrq(false);
    }
}


auto VicIIBase::getCurrentLinePtr() -> uint16_t* {

    return linePtr + linePos;
}

auto VicIIBase::triggerLightPen(bool state) -> void {
    // trigger by writing to cia        
    lpPin = state;

    if (lpPin || lpTrigger)
        return;

    lpTrigger = !lpLatched;
    lpPhi1 = false;
    lpTriggerDelay = rev65 ? 2 : 1;

    xCounter = xLookupPtrPhi2[cycle];
    checkLightPen<false>();
}

auto VicIIBase::triggerLightPen(bool state, uint8_t subCycle) -> void {
    lpPin = state;

    if (lpPin)
        return;
	
	if (oldIrqMode && (cycle == 0))
		updateIrq(Interrupt::LP);

    lpTrigger = !lpLatched;
    lpPhi1 = (subCycle & 2) ? false : true;
    lpTriggerDelay = subCycle & 1;
}

template<bool phi1> auto VicIIBase::checkLightPen() -> void {

    if (!lpTrigger || (phi1 != lpPhi1))
        return;

    lpTrigger = false;
    lpLatched = true;

    // last line doesn't latch lpx or lpy.
    if (vCounter == (lines - 1) )
        return;

    // 2 adjacent pixel [4,5] [6,7] give the same value for lpx, because of
    // the last bit is shifted out. it's a division by 2.
    // this code fires between the half cycles in pixel 4.
    // for the 8565 the latch happens by pixel 7, btw. pixel 6 would give the same.
    // for the 6569 the latch happens one pixel later, but it's already the next
    // two pixel block. 

    lpxBefore = lpx;
    lpyBefore = lpy;
    
    lpx = xCounter >> 1;
    lpx += lpTriggerDelay;

    // vCounter is incremented in second half cycle of last line cycle.
    // if this latch happens in last pixel (like the 85xx) vCounter is already
    // incremented. I don't know if a latch in second to last pixel recognizes 
    // incremented vCounter too. From a CIA point of view it happens only in last or
    // first pixel of next cycle. From a Light Gun(Pen) point of view it could happen
    // an any cycle pixel, but vCounter increments in non visible area... means no problem

    if (!phi1 && (cycle == (lineCycles - 1)))
        lpy = (vCounter + 1) & 0xff;
    else
        lpy = vCounter & 0xff;

	if (oldIrqMode)
		return;
	
    if (phi1)
        updateIrq(Interrupt::LP);
    else
        // cpu mustn't recognize it this cycle, but next
        irqLatchPending |= 0x80 | (1 << Interrupt::LP);
}

auto VicIIBase::getCyclesForNextLightTrigger(int x, int y, uint8_t& cyclePixel) -> unsigned {

    x += firstVisiblePixel - ((cycle + 1) << 3);

    if (x < 104)
        return 0;

    if (ntscGeometry)
        y += vStart - vCounter;

    else
        y += lines - vCounter + vStart;

    // which pixel
    cyclePixel = x & 7;

    return (x / 8) + (y * lineCycles);
}

auto VicIIBase::updateBorderData() -> void {
    // is used for border cropping only, not a VicII feature
    if (visibleLine) {

        if (!crop.rSel)
            crop.rSel = rSel;

        if (!crop.cSel)
            crop.cSel = cSel;

    } else {
        crop.rSel = rSel;
        crop.cSel = cSel;
    }
}

auto VicIIBase::setBorderData() -> void {
    // is used for border cropping only, not a VicII feature
    if (crop.cSel) {
        crop.left = ntscBorder ? 56 : 46;
        crop.right = ntscBorder ? 44 : 40;

    } else {
        crop.left = ntscBorder ? 63 : 53;
        crop.right = ntscBorder ? 53 : 49;
    }

    if (crop.rSel) {
        crop.top = ntscBorder ? 28 : 42;
        crop.bottom = ntscBorder ? 25 : 51;

    } else {
        crop.top = ntscBorder ? 32 : 46;
        crop.bottom = ntscBorder ? 29 : 55;
    }
}

auto VicIIBase::initVerticalLineAnomaly() -> void {

    leftLineAnomaly.framePos = LEFT_LINE_ANOMALY;
    leftLineAnomaly.permanent = false;
}

auto VicIIBase::buildXCounterLookupTable() -> void {

    uint16_t* ptr;
	bool xCounterLock;

    for (unsigned r = 0; r < 3; r++) {
		// pal:			r == 0
		// ntsc:		r == 1
		// ntsc old:	r == 2
		
        unsigned x = (r == 0) ? 404 : 412;

        unsigned xWrapAround = (r == 0) ? 0x1f8 : 0x200;

		unsigned _cycles = (r == 0) ? 63 : ((r == 1 ? 65 : 64));
		
        for (unsigned c = 0; c < _cycles; c++) {

            for (unsigned p = 0; p < 2; p++) {

                bool phi1 = p == 0;

                if (r == 0)
                    ptr = phi1 ? &xLookUpPalPhi1[c] : &xLookUpPalPhi2[c];
                else if (r == 1)
                    ptr = phi1 ? &xLookUpNtscPhi1[c] : &xLookUpNtscPhi2[c];
				else
					ptr = phi1 ? &xLookUpNtscOldPhi1[c] : &xLookUpNtscOldPhi2[c];
				
                *ptr = x;

				// ntsc only
                xCounterLock = (r == 1) && ((!phi1 && c == 60) || (phi1 && c == 61));

                if (!xCounterLock)
                    x += 4;

                if (x == xWrapAround)
                    x = 0; // happens always at the end of phase 1                        
            }
        }
    }

    xLookupPtrPhi1 = &xLookUpPalPhi1[0];
    xLookupPtrPhi2 = &xLookUpPalPhi2[0];
}

auto VicIIBase::updateSpriteBaState( uint8_t sprNr, bool dmaActive ) -> void {
	// update BA line state for sprites
	// 5 cycles: 3 to finish possible cpu writes, 2 to use the bus in second phase of cycle
	// up to 3 cpu cycles are wasted because of the bad design
	// e.g. dma for sprite 0 and 2 stop cpu read during non dma sprite 1 too 
	// because of allowing a cpu read wouldn't give enough time to retake the bus for sprite 2
	// The three take over cycles are hard coded in vic design
	// e.g. dma for sprite 0 and 3 allow cpu one read access in between
	unsigned start = (ntscBorder ? 55 : 54) + (sprNr << 1);
	start %= lineCycles;
	
	for(auto i = 0; i < 5; i++) {
		unsigned pos = (start + i) % lineCycles;
		
		spriteBa[sprNr][ pos ] = dmaActive;		
		spriteBa[8][pos] = dmaActive;
		if (dmaActive)
			continue;
		//if any other sprite needs this position keep baLow state
		for( auto j = 0; j < 8; j++ ) {
			if (spriteBa[j][pos]) {
				spriteBa[8][pos] = true;
				break;
			}
		}
	}
}

auto VicIIBase::power() -> void {
    crop.leftOverscan = ntscGeometry ? (56 - 32) : (46 - 32);
    crop.rightOverscan = ntscGeometry ? (44 - 32) : (40 - 32);
    crop.topOverscan = ntscGeometry ? 5 : 7;
    crop.bottomOverscan = ntscGeometry ? 1 : 14;

    lastReadPhi1 = 0;
    std::memset(renderPipe, 0, 8);
    memset(colorReg, 0, sizeof (colorReg));
    memset(colorUse, 0, sizeof (colorUse));
    // direct colors
    for (unsigned i = 0; i <= 0xf; i++)
        colorUse[i] = i;

    lastColorReg = 0xff;
    cycle = 0;
    vCounter = 0;
    xCounter = ntscBorder ? 412 : 404;
    xCounterLatch = xCounterSprites = xCounter;
    xCounter += 8; // to compensate first advance cycle	
	
    vStart = ntscGeometry ? 23 : 9;
    vHeight = ntscGeometry ? 253 : 293; // max possible display height 	
	
    hWidth = ntscBorder ? 420 : 406; // max possible display width  
    firstVisiblePixel = ntscBorder ? 76 : 86;
    baLow = false;
    aecDelay = 0;
    std::memset(spriteBa, 0, sizeof spriteBa);
    allowBadlines = false;
    badLine = false;
    irqLine = 0;
    lineIrqMatched = false;
    irqLatchPending = 0;
    lpx = 0;
    lpy = 0;
    lpxBefore = 0;
    lpyBefore = 0;
    vm = 0;
    cb = 0;
    irqLatch = 0;
    irqEnable = 0;
    lpLatched = false;
    lpPin = true;
    lpTriggerDelay = 0;
    lpTrigger = false;
    controlReg1 = 0;
    controlReg2 = 0;
    linePos = 0;
    lineVCounter = 0;
    linePtr = frameBuffer;
    visibleLine = false;
    hFlipFlop = true;
    vFlipFlop = vFlipFlopShadow = true;
    idleMode = true;
    idleModeTemp = true;
    initVCounter = false;
    refreshCounter = 0xff;
    sprite0DmaLateBA = false;

    modeEcmBmm = modeMcm = 0;
    modeEcmBmmDma = modeMcmDma = 0;
    modeEcmBmmSequencer = modeMcmSequencer = 0;

    for (unsigned i = 0; i < 8; i++) {
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
		
		sprite[i].xPos = 0;
		sprite[i].mask = ~0;
    }
    spriteOpenBus = nullptr;
    lastSpriteShift = 16;

    spriteForegroundCollided = 0;
    spriteForegroundCollidedRead = 0;
    spriteSpriteCollided = 0;
    spriteSpriteCollidedRead = 0;
    spriteDmaCycle1 = 0;
    spriteDmaCycle2 = 0;
    canSpriteSpriteCollisionIrq = false;
    canSpriteForegroundCollisionIrq = false;

    writeReg(0x11, controlReg1);
    writeReg(0x16, controlReg2);

    spriteTrigger = 0;
    spritePending = 0;
    updateMc = 0;
    updatePrioExpand = 0;
    cAccessArea = 0;
    disableEcmBmmTogether = false;
    
    initVerticalLineAnomaly();
}

auto VicIIBase::setXLookUp() -> void {
	
	if (ntscXLock && ntscBorder) {
		xLookupPtrPhi1 = &xLookUpNtscPhi1[0];
		xLookupPtrPhi2 = &xLookUpNtscPhi2[0];

	} else if (!ntscXLock && !ntscBorder) {
		xLookupPtrPhi1 = &xLookUpPalPhi1[0];
		xLookupPtrPhi2 = &xLookUpPalPhi2[0];

	} else {
		xLookupPtrPhi1 = &xLookUpNtscOldPhi1[0];
		xLookupPtrPhi2 = &xLookUpNtscOldPhi2[0];
	}
}


auto VicIIBase::setModel(Model model) -> void {
	
	this->model = model;
	
	switch(model) {
		default:
		case MOS6569R3: // PAL-B
			lineCycles = 63;
			lines = 312;
			rev65 = true;
			oldIrqMode = false;
			ntscGeometry = false;
			ntscEncoding = false;
			ntscBorder = false;
			ntscXLock = false;
			cyclesPerSec = 985248;
			break;
			
		case MOS8565: // PAL-B
			lineCycles = 63;
			lines = 312;
			rev65 = false;
			oldIrqMode = false;
			ntscGeometry = false;
			ntscEncoding = false;
			ntscBorder = false;
			ntscXLock = false;
			cyclesPerSec = 985248;
			break;
				
		case MOS6567R8: // NTSC-M
			lineCycles = 65;
			lines = 263;
			rev65 = true;
			oldIrqMode = false;
			ntscGeometry = true;
			ntscEncoding = true;
			ntscBorder = true;
			ntscXLock = true;
			cyclesPerSec = 1022730;
			break;
			
		case MOS8562: // NTSC-M
			lineCycles = 65;
			lines = 263;
			rev65 = false;
			oldIrqMode = false;
			ntscGeometry = true;
			ntscEncoding = true;
			ntscBorder = true;
			ntscXLock = true;
			cyclesPerSec = 1022730;
			break;
						
		case MOS6569R1: // PAL-B
			lineCycles = 63;
			lines = 312;
			rev65 = true;
			oldIrqMode = true;
			ntscGeometry = false;
			ntscEncoding = false;
			ntscBorder = false;
			ntscXLock = false;
			cyclesPerSec = 985248;
			break;
			
		case MOS6567R56A: // OLD NTSC
			lineCycles = 64;
			lines = 262;
			rev65 = true;
			oldIrqMode = true;
			ntscGeometry = true;
			ntscEncoding = true;
			ntscBorder = true;
			ntscXLock = false;
			cyclesPerSec = 1022730;
			break;
			
		case MOS6572: // PAL-N (DREAN)
			lineCycles = 65;
			lines = 312;
			rev65 = true;
			oldIrqMode = false;
			ntscGeometry = false;
			ntscEncoding = false;
			ntscBorder = true; // no typo
			ntscXLock = true;
			cyclesPerSec = 1023440;
			break;
			
		case MOS6573: // PAL-M
			lineCycles = 65;
			lines = 263;
			rev65 = true;
			oldIrqMode = false;
			ntscGeometry = true;
			ntscEncoding = false;
			ntscBorder = true;
			ntscXLock = true;
			cyclesPerSec = 1022730;
			break;		
	}
}

template auto VicIIBase::checkLightPen<true>() -> void;
template auto VicIIBase::checkLightPen<false>() -> void;

}

