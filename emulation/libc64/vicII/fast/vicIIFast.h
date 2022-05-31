
#pragma once

#include "../base.h"

namespace LIBC64 {  
    
struct VicIIFast : VicIIBase {         
    
    VicIIFast();
    auto clock() -> void;
    auto power() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto clockSilence() -> void;    
	   
    auto readReg( uint8_t addr ) -> uint8_t;
    auto writeReg( uint8_t addr, uint8_t value ) -> void;
    auto getCurrentLinePtr() -> uint8_t*;
	auto getCurrentFramePtr() -> uint8_t*;
	
	auto triggerLightPen( bool state ) -> void;
    auto triggerLightPen( bool state, uint8_t subCycle ) -> void;
	auto isScanlineRenderer() -> bool { return true; }
	auto reuBaLow() -> bool { return baLow; }
    auto reuSprite0() -> bool { return false; }
	auto setMeta( bool state ) -> void;
   
protected:
	#include "../flags.h"

    uint8_t borderLeft;
    uint8_t borderRight;
    
    uint8_t dataC;
    uint8_t dataG;
    uint8_t ecmBmmMcm;
	
	uint8_t* linePtr;
	bool addMeta; // add aec and ba state to output 

    uint16_t vicBank;
    Sprite* drawSprites[VIC_MAX_LINE_LENGTH << 1];
    uint8_t* patternBadline;
    uint8_t* patternLine;
    uint8_t* patternBadlineNtsc;
    uint8_t* patternLineNtsc;
    unsigned dmaDelay = 0;
    bool sprCrunching = false;

    auto setLineInterrupt() -> void;
	
    auto scanline() -> void;
    auto applyBorder() -> void;
    auto setBorderDim() -> void;
    auto fetch(unsigned i) -> void;
    auto mode0() -> void;
    auto mode1() -> void;
    auto mode2() -> void;
    auto mode3() -> void;
    auto mode4() -> void;
    auto mode5() -> void;
    auto mode6() -> void;
    auto mode7() -> void;
    auto dmaSprites() -> void;
    auto dmaSpritesOff() -> void;
    auto applySprites() -> void;
    template<bool phi1> inline auto readPhi(uint16_t addr) -> uint8_t;
    
    auto initMetaPattern() -> void;
    auto applyMeta() -> void;
	auto calcSpriteX(Sprite* spr) -> void;
	auto calcSpriteMask(Sprite* spr) -> void;
	inline auto setRdy(bool _baLow) -> void;
};

extern VicIIFast* vicIIFast;
}
