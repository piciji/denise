
#include "vicII.h"
#include "dma.cpp"    
#include "sequencer.cpp"
#include "serialization.cpp"
#include "silence.cpp"

namespace LIBC64 {
	
VicIICycle* vicIICycle = nullptr;
      
auto VicIICycle::power() -> void {

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
    display.gBufferUse = false;
    display.enable = false;
    display.dmli = 0;
    display.gBufferShift = 0;
    display.gBits = 0;
    
	setXLookUp();
	
    VicIIBase::power();
    
    onHalfCycle = nullptr;
}

inline auto VicIICycle::setLineInterrupt() -> void {
    
	if (vCounter == irqLine) {
		if (!lineIrqMatched) {
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
    
    linePtr = frameBuffer + lineVCounter * VIC_MAX_LINE_LENGTH;
    lineVCounter++;
    linePos = 0;
}

}

