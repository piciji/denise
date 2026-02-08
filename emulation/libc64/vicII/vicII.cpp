
#include "vicII.h"

#include "fetch.cpp"
#include "dma.cpp"
#include "sequencer.cpp"
#include "register.cpp"

#include "silence.cpp"
#include "serialization.cpp"

namespace LIBC64 {
	
VicIICycle::VicIICycle(System* system) : VicIIBase(system) {
	greyDotBugDisabled = false;
    debugger.dmaFrame = new uint8_t[VIC_MAX_LINE_LENGTH * 320];
}

auto VicIICycle::disableGreyDotBug(bool state) -> void {
	greyDotBugDisabled = state;
}

inline auto VicIICycle::setLineInterrupt() -> void {
    
	if (vCounter == irqLine) {
		if (unlikely(!lineIrqMatched)) {
			updateIrq( Interrupt::Raster );
			lineIrqMatched = true;
		}
		return;
	}
	
	lineIrqMatched = false;		
}

inline auto VicIICycle::setLineBuffer() -> void {
    if (!visibleLine)
        return;
	
	std::memcpy( frameBuffer + lineVCounter * VIC_MAX_LINE_LENGTH, lineBuffer, lineCycles << 3 );
	
    lineVCounter++;
    linePos = 0;
}

auto VicIICycle::getCurrentFramePtr() -> uint8_t* {

    return frameBuffer + lineVCounter * VIC_MAX_LINE_LENGTH + linePos;
}

auto VicIICycle::getCurrentLinePtr() -> uint8_t* {

	return &lineBuffer[linePos];
}

auto VicIICycle::power() -> void {
	cBufferPipe1 = cBufferPipe2 = 0;
	xScrollPipe = 0;
	gBuffer = gBufferPipe1 = gBufferPipe2 = 0;
	gBufferUse = false;
	dmli = 0;
	gBufferShift = 0;
	gBits = 0;
	mcFlop = 0;
	vmli = 0;
	
	lastReadPhi1 = 0;
	lastBusPhi2 = 0xff;
	
	dataC = 0;
	aecDelay = 0;
	vFlipFlopShadow = true;
	idleModeTemp = true;
	refreshCounter = 0xff;
	clearCollision = 0;
	sprite0DmaLateBA = false;
	
	std::memset(renderPipe, 0, 8);
	std::memset(colorUse, 0, sizeof (colorUse));
	// direct colors
    for (unsigned i = 0; i <= 0xf; i++)
        colorUse[i] = i;
	
	lastColorReg = 0xff;
	
	modeEcmBmmDma = 0;
	modeEcmBmmSequencer = modeMcmSequencer = 0;
	disableEcmBmmTogether = false;

	spriteHalt = 0;
	spriteTrigger = 0;
	spritePending = 0;
	
	updateMc = 0;
    updatePrioExpand = 0;
	
	spriteForegroundCollidedRead = 0;
	spriteSpriteCollidedRead = 0;
	
	VicIIBase::power();
	
	firstVisiblePixel += 8;	// display is delayed 8 pixel
}

auto VicIICycle::updateVideoSnapshot(DebuggerSnapshot& snap) -> void {
    auto& s = snap.vicII;
    s.vmli = vmli;

    VicIIBase::updateVideoSnapshot(snap);
}

}

