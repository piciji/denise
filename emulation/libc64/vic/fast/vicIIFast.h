
#pragma once

#include <atomic>
#include <thread>
#include <condition_variable>
#include "../base.h"

namespace LIBC64 {  
    
struct VicIIFast : VicIIBase {         
    
    VicIIFast();
    auto clock() -> void;
    auto power() -> void;
    auto powerOff() -> void;
    auto setThreading( bool state) -> void;
    auto isThreading() -> bool { return useThread; }
    auto serialize(Emulator::Serializer& s) -> void;
    auto clockSilence() -> void;    
	   
    auto readReg( uint8_t addr ) -> uint8_t;
    auto writeReg( uint8_t addr, uint8_t value ) -> void;
    auto getCurrentLinePtr() -> uint16_t*;
   
protected:
    uint8_t borderLeft;
    uint8_t borderRight;
    
    uint8_t color;
    uint8_t dataC;
    uint8_t dataG;
    uint8_t ecmBmmMcm;

    uint16_t vcBase;
    uint16_t vc;
    uint8_t rc;
    uint16_t cBuffer[40];
    uint32_t vicBank;
    Sprite* drawSprites[VIC_MAX_LINE_LENGTH << 1];
    std::atomic<bool> ready;
    std::atomic<bool> idle;
    std::condition_variable cv;
    bool useThread = false;
    uint8_t* patternBadline;
    uint8_t* patternLine;
    uint8_t* patternBadlineNtsc;
    uint8_t* patternLineNtsc;
    unsigned dmaDelay = 0;
    
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
    auto checkLightPenNew() -> void;
    auto initMetaPattern() -> void;
    auto applyMeta() -> void;
       
};

extern VicIIFast* vicIIFast;
}
