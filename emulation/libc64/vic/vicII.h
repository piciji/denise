
#pragma once

#include "base.h"

namespace LIBC64 {  
    
struct VicIICycle : VicIIBase {

    auto clock() -> void;
    auto clockSilence() -> void;  
    auto power() -> void;
    auto reuBaLow() -> bool;
    auto serialize(Emulator::Serializer& s) -> void;
	auto readReg( uint8_t addr ) -> uint8_t;
    auto writeReg( uint8_t addr, uint8_t value ) -> void;

protected: 
    
    struct {
        uint8_t color;
        bool mcFlop;
        uint16_t dataC;
        uint16_t vcBase;
        uint16_t vc;
        uint8_t rc;
        uint8_t vmli;
        uint16_t cBuffer[40];
        uint16_t cBufferPipe1;
        uint16_t cBufferPipe2;
        uint8_t xScroll;
        uint8_t gBuffer;
        bool gBufferUse;
        uint8_t gBufferPipe1;
        uint8_t gBufferPipe2;
		bool enable;
		uint8_t dmli;
		uint8_t gBufferShift;
		uint8_t gBits; // 2 bit (0-3) or 1 bit: 10 or 00
		
		auto isForeground() -> bool { return gBits & 2; }
    } display;
    
    //dma
    auto setLineInterrupt() -> void;
    auto setLineBuffer() -> void;
	auto advanceCycle() -> void;	
	auto clearCollisions() -> void;	
	auto spriteUpdateBase() -> void;
	auto spriteDmaCheck() -> void;
	auto spriteFlip() -> void;
	auto spriteDisplayCheck() -> void;
	auto updateVc() -> void;
	auto updateRc() -> void;
	auto updateBAState() -> void;	      
    auto updateBadLine() -> void;
	auto borderControl() -> void;
	template<bool first> auto borderLeft( ) -> void;
	template<bool first> auto borderRight( ) -> void;
	auto idleCycle() -> void;
	auto refresh() -> void;
	template<uint8_t pos> auto fetchSpriteP(  ) -> void;
    auto fetchSpriteS0( uint8_t pos ) -> void;
    template<uint8_t pos> auto fetchSpriteS1(  ) -> void;
    auto fetchSpriteS2( uint8_t pos ) -> void;    
    auto fetchSpriteSPhi2( uint8_t pos ) -> void;
	auto fetchC() -> void;
    auto addrG( uint8_t useMode ) -> uint16_t;
    auto fetchG() -> void;    
    template<bool doubleStep = false> auto dummySpriteDma(Sprite* spr) -> void;
    
    //sequencer
	auto sequencer( ) -> void;
	template<bool phi1> auto sequencerPix0(  ) -> void;
	template<bool phi1> auto sequencerPix1(  ) -> void;
	template<bool phi1> auto sequencerPix2(  ) -> void;
	template<bool phi1> auto sequencerPix3(  ) -> void;
	auto pipeGraphic() -> void;
	auto graphicSequencer( uint8_t x ) -> void;
	auto triggerSprites( uint16_t xPos ) -> void;
    template<uint8_t sprPos> auto triggerSprites( uint16_t xPos ) -> void;
	auto updateMc6569() -> void;
	auto updateMc8565() -> void;	
	auto spriteSequencer(  ) -> void;
    template<uint8_t sprPos> auto spriteSequencer( Sprite* spr, Sprite*& sprUse, uint8_t& collision ) -> void;
	template<bool phi1> auto borderArea(  ) -> void;
	template<bool phi1> auto draw65( uint8_t x, uint8_t x1 ) -> void;
	template<bool phi1> auto draw85( uint8_t x ) -> void;
	template<bool phi1> auto draw() -> void;            
    std::function<void ()> onHalfCycle = nullptr;   
};

extern VicIICycle* vicIICycle;
}
