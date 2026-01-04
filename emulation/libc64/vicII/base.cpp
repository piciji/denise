
#include "base.h"
#include "../system/system.h"
#include "cycleTable.cpp"
#include "../system/debuggerSnapshot.h"

namespace LIBC64 { 

uint8_t* VicIIBase::frameBuffer = new uint8_t[VIC_MAX_LINE_LENGTH * 294];

VicIIBase::VicIIBase(System* system) : system(system), cpu(system->cpu) {
	
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

    for(auto& s : debugInfo.spr)
        s.data = new uint8_t[16384];
}	

VicIIBase::~VicIIBase() {
	if (frameBuffer)
		delete[] frameBuffer;

    for(auto& s : debugInfo.spr)
        delete[] s.data;
		
	frameBuffer = nullptr;
}
	
auto VicIIBase::disableSequencer(bool state) -> void {
    enableSequencer = !state;
}

auto VicIIBase::getReg18() -> uint8_t {
    return (vm << 4) | ((cb & 7) << 1) | 1;
}

auto VicIIBase::updateIrq(Interrupt interrupt) -> void {

    if (interrupt != Interrupt::Update) {
        irqLatch |= 1 << interrupt;
    }

    if (irqLatch & irqEnable) {
        irqLatch |= 0x80;
        system->setVicIrq(true);

    } else {
        irqLatch &= 0x7f;
        system->setVicIrq(false);
    }
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

auto VicIIBase::getCrop(unsigned w, unsigned h) -> Emulator::Interface::Crop {
    Interface::Crop _c;

    if (w == 320) {
        _c.left = ntscBorder ? 56 : 46;
        _c.right = ntscBorder ? 44 : 40;
        _c.top = ntscBorder ? 28 : 42;
        _c.bottom = ntscBorder ? 25 : 51;
    } else if (w == 384) {
        _c.left = crop.leftOverscan;
        _c.right = crop.rightOverscan;
        _c.top = crop.topOverscan;
        _c.bottom = crop.bottomOverscan;
    }
    return _c;
}

auto VicIIBase::triggerLightPen(bool state) -> void {
    // trigger by writing to cia        
    lpPin = state;

    if (lpPin || lpTrigger)
        return;

    lpTrigger = !lpLatched;
    lpPhi1 = false;
    lpTriggerDelay = rev65 ? 2 : 1;

    checkLightPen();
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

auto VicIIBase::checkLightPen() -> void {

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
    
	if (lpPhi1)
		lpx = (xCounterLatchBefore >> 1) + 2;
	else
		lpx = xCounterLatch >> 1;
	
    lpx += lpTriggerDelay;

    // vCounter is incremented in second half cycle of last line cycle.
    // if this latch happens in last pixel (like the 85xx) vCounter is already
    // incremented. I don't know if a latch in second to last pixel recognizes 
    // incremented vCounter too. From a CIA point of view it happens only in last or
    // first pixel of next cycle. From a Light Gun(Pen) point of view it could happen
    // an any cycle pixel, but vCounter increments in non visible area... means no problem

    if (!lpPhi1 && (cycle == (lineCycles - 1)))
        lpy = (vCounter + 1) & 0xff;
    else
        lpy = vCounter & 0xff;

	if (oldIrqMode)
		return;
	
    if (lpPhi1)
        updateIrq(Interrupt::LP);
    else
        // cpu mustn't recognize it this cycle, but next
        irqLatchPending |= 0x80 | (1 << Interrupt::LP);
}

auto VicIIBase::getCyclesForNextLightTrigger(int x, int y, uint8_t& cyclePixel) -> unsigned {

	unsigned _firstVisiblePixel;
	
	if (isScanlineRenderer())
		_firstVisiblePixel = firstVisiblePixel;
	else
		// cycle renderer has 8 pixel calculation delay
		_firstVisiblePixel = firstVisiblePixel - 8;
		
    x += _firstVisiblePixel - ((cycle + 1) << 3);

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

auto VicIIBase::storeSprite(Sprite& spr) -> void {
    auto& _spr = debugInfo.spr[spr.position];

    bool mcFlop = true;
    bool expandXFlop = true;
    bool useExpandX = spr.expandX;
    bool useMultiColor = spr.useMultiColor;
    uint32_t dataShiftReg = spr.dataShiftReg & 0xffffff;
    uint8_t shiftOut = 0;
    uint8_t colorCode = spr.colorCode;
    unsigned _pos = _spr.pos;

    std::memset(_spr.data + _pos, 0x80, 48);

    while (dataShiftReg || shiftOut) {
        if (expandXFlop) {
            if (useMultiColor) {
                if (mcFlop)
                    shiftOut = (dataShiftReg >> 22) & 3;

                mcFlop ^= 1;
            } else
                shiftOut = ((dataShiftReg >> 23) & 1) << 1;
        }

        if (expandXFlop) {
            dataShiftReg <<= 1;
            dataShiftReg &= 0xffffff;
        }

        if (useExpandX)
            expandXFlop ^= 1;
        else
            expandXFlop = true;

        if (shiftOut == 1)      _spr.data[_pos] = colorReg[0x25];
        else if (shiftOut == 2) _spr.data[_pos] = colorReg[colorCode];
        else if (shiftOut == 3) _spr.data[_pos] = colorReg[0x25];

        _pos++;
    }
    _spr.pos += 48;
    _spr.pos &= 0x3fff;
}

auto VicIIBase::power() -> void {

	if (model == UnInitialized)
		setModel( MOS6569R3 );
	
    crop.leftOverscan = ntscGeometry ? (56 - 32) : (46 - 32);
    crop.rightOverscan = ntscGeometry ? (44 - 32) : (40 - 32);
    crop.topOverscan = ntscGeometry ? 5 : 7;
    crop.bottomOverscan = ntscGeometry ? 1 : 14;
	
	color = 0;    
    vcBase = 0;
    vc = 0;
    rc = 0;

    _addrG = 0;
    vicBank = 0;
    
    std::memset(cBuffer, 0, sizeof cBuffer); 	            
    std::memset(colorReg, 0, sizeof (colorReg));
        
    cycle = 10;
    vCounter = 0;

    xCounterLatch = xCounterLatchBefore = ntscBorder ? 412 : 404;
	
    vStart = ntscGeometry ? 23 : 9;
    vHeight = ntscGeometry ? 253 : 293; // max possible display height 	
	
    hWidth = ntscBorder ? 420 : 406; // max possible display width  
    firstVisiblePixel = ntscBorder ? 76 : 86;	
    baLow = false;
    
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
	lpTrigger = false;
    lpTriggerDelay = 0;    
    controlReg1 = 0;
    controlReg2 = 0;
    linePos = 0;
    lineVCounter = 0;

    visibleLine = false;
    hFlipFlop = true;
    vFlipFlop = true;
    idleMode = true;    
    initVCounter = false;

    debugInfo.reset();

    for (unsigned i = 0; i < 8; i++) {
        sprite[i].enabled = false;

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

	spriteDma = 0;
	spriteActive = 0;
	
    spriteForegroundCollided = 0;    
    spriteSpriteCollided = 0;
    	
    canSpriteSpriteCollisionIrq = true;
    canSpriteForegroundCollisionIrq = true;

	modeEcmBmm = modeMcm = 0;
    writeReg(0x11, controlReg1);
    writeReg(0x16, controlReg2);
    
    reg2mhz = 0;
}

auto VicIIBase::oneTimeDebuggerAction() -> void {
    system->leaveEmulation = true;
    system->debugger.action = debuggerAction;
    system->debugger.addr = 0;
    debuggerAction = Emulator::Interface::DebuggerAction::None;
}

#define _fullAdr( __addr ) (((__addr) & 0x3fff) | vicBank)
auto VicIIBase::updateSnapshot(DebuggerSnapshot& snap) -> void {
    auto& s = snap.vicII;

    for (unsigned i = 0; i < 8; i++) {
        auto& _sprD = debugInfo.spr[i];
        auto& _sprT = s.spr[i];
        auto& _spr = sprite[i];
        unsigned pos = _sprD.pos;
        if (pos)
            std::memcpy(_sprT.data, _sprD.data, pos);

        _sprT.pos = pos;
        _sprT.expandX = _spr.expandX;
        _sprT.expandY = _spr.expandY;
        _sprT.prioMD = _spr.prioMD;
        _sprT.multiColor = _spr.multiColor;
        _sprT.x = _spr.x;
        _sprT.y = _spr.y;
        _sprT.addr = _fullAdr((_spr.dataP << 6) | _spr.mc);
        _sprT.mcBase = _spr.mcBase;
    }

    s.spriteForegroundCollided = spriteForegroundCollided;
    s.spriteSpriteCollided = spriteSpriteCollided;
    s.xPos = xCounterLatch;
    s.vcBase = vcBase;
    s.vc = vc;
    s.rc = rc;
    s.den = den;
    s.badLine = badLine;
    s.visibleLine = visibleLine;
    s.hFlipFlip = hFlipFlop;
    s.vFlipFlip = vFlipFlop;
    s.idleMode = idleMode;
    s.mode = (modeEcmBmm | modeMcm) >> 2;
    s.irqLatch = irqLatch;
    s.irqEnable = irqEnable;
    s.irqLine = irqLine;
    s.xScroll = xScroll;
    s.yScroll = yScroll;
    s.vicBank = vicBank;
    s.screenMemory = _fullAdr(_addrG);
    s.charMemory = _fullAdr((vm << 10) | vc);
    s.lpx = lpx;
    s.lpy = lpy;
    s.lpPin = lpPin;
    s.lpLatched = lpLatched;
    s.controlReg1 = controlReg1;
    s.controlReg2 = controlReg2;
}
#undef _fullAdr

auto VicIIBase::updatePositionSnapshot(DebuggerSnapshot& snap) -> void {
    snap.vPos = vCounter;
    snap.hPos = cycle;
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
			cyclesPerSec = 985248;
			generateCycleTable( C_PAL );
			break;
			
		case MOS8565: // PAL-B
			lineCycles = 63;
			lines = 312;
			rev65 = false;
			oldIrqMode = false;
			ntscGeometry = false;
			ntscEncoding = false;
			ntscBorder = false;
			cyclesPerSec = 985248;
			generateCycleTable( C_PAL );
			break;
				
		case MOS6567R8: // NTSC-M
			lineCycles = 65;
			lines = 263;
			rev65 = true;
			oldIrqMode = false;
			ntscGeometry = true;
			ntscEncoding = true;
			ntscBorder = true;
			cyclesPerSec = 1022730;
			generateCycleTable( C_NTSC );
			break;
			
		case MOS8562: // NTSC-M
			lineCycles = 65;
			lines = 263;
			rev65 = false;
			oldIrqMode = false;
			ntscGeometry = true;
			ntscEncoding = true;
			ntscBorder = true;
			cyclesPerSec = 1022730;
			generateCycleTable( C_NTSC );
			break;
						
		case MOS6569R1: // PAL-B
			lineCycles = 63;
			lines = 312;
			rev65 = true;
			oldIrqMode = true;
			ntscGeometry = false;
			ntscEncoding = false;
			ntscBorder = false;
			cyclesPerSec = 985248;
			generateCycleTable( C_PAL );
			break;
			
		case MOS6567R56A: // OLD NTSC
			lineCycles = 64;
			lines = 262;
			rev65 = true;
			oldIrqMode = true;
			ntscGeometry = true;
			ntscEncoding = true;
			ntscBorder = true;
			cyclesPerSec = 1022730;
			generateCycleTable( C_NTSC_OLD );
			break;
			
		case MOS6572: // PAL-N (DREAN)
			lineCycles = 65;
			lines = 312;
			rev65 = true;
			oldIrqMode = false;
			ntscGeometry = false;
			ntscEncoding = false;
			ntscBorder = true; // no typo
			cyclesPerSec = 1023440;
			generateCycleTable( C_NTSC );
			break;
			
		case MOS6573: // PAL-M
			lineCycles = 65;
			lines = 263;
			rev65 = true;
			oldIrqMode = false;
			ntscGeometry = true;
			ntscEncoding = false;
			ntscBorder = true;
			cyclesPerSec = 1022730;
			generateCycleTable( C_NTSC );
			break;		
	}
}

}
