
#pragma once

#include "base.h"

namespace LIBC64 {  
    
struct VicIICycle : VicIIBase {
	VicIICycle(System* system);
	
	auto clock() -> void;
    auto clockLogged() -> void;
    auto clockMaybeLogged() -> void;
    template<bool logDma> auto clockCycle() -> void;
	auto clockSilence() -> void;
    // of course expansion port sees the same BA state like CPU RDY line.
    // but there is a known case, when BA calculation takes more time within cycle.
    // for CPU it doesn't matter, because it checks later in cycle.
    // REU seems to check this sooner and can't recognize BA in this special cycle.
	auto reuBaLow() -> bool { return baLow && !sprite0DmaLateBA; }
    auto reuSprite0() -> bool { return sprite0DmaLateBA; }
	auto serialize(Emulator::Serializer& s) -> void;
	auto readReg(uint8_t addr) -> uint8_t;
    auto peekReg(uint8_t addr) -> uint8_t;
	auto writeReg(uint8_t addr, uint8_t value) -> void;
	auto power() -> void;
	
	auto disableGreyDotBug(bool state) -> void;
	auto hasGreyDotBugDisbled() -> bool { return greyDotBugDisabled; }
	auto getCurrentLinePtr() -> uint8_t*;
	auto getCurrentFramePtr() -> uint8_t*;
	
	auto lastReadPhase1() -> uint8_t { return lastReadPhi1; }
	auto isAecLow() -> bool { return !aecDelay; }
	auto isScanlineRenderer() -> bool { return false; }
    auto updateVideoSnapshot(DebuggerSnapshot& snap) -> void;
	
protected:       
	#include "flags.h"
	
	uint16_t dataC;
	uint8_t aecDelay;
	bool vFlipFlopShadow;
	bool idleModeTemp;
	
	uint16_t cBufferPipe1;
	uint16_t cBufferPipe2;
	uint8_t xScrollPipe;
	uint8_t gBuffer;
	bool gBufferUse;
	uint8_t gBufferPipe1;
	uint8_t gBufferPipe2;
	uint8_t dmli;
	uint8_t gBufferShift;
	uint8_t gBits;	
	bool mcFlop;
	uint8_t vmli;
	
	bool greyDotBugDisabled = false;
	
	uint8_t lastReadPhi1;
	uint8_t lastBusPhi2;

	uint8_t renderPipe[8];
	uint8_t render[8];	
	uint8_t colorUse[0x2f];
	uint8_t lastColorReg;

	uint8_t modeEcmBmmDma;
	uint8_t modeEcmBmmSequencer;
	uint8_t modeMcmSequencer;
	bool disableEcmBmmTogether;

	uint8_t spriteHalt;
	uint8_t spriteTrigger;
	uint8_t spritePending;
	
	bool updateMc;
	bool updatePrioExpand;
	
	uint8_t spriteForegroundCollidedRead;
	uint8_t spriteSpriteCollidedRead;
	
	uint8_t refreshCounter;
	uint8_t clearCollision;
	bool sprite0DmaLateBA;
	
	static const uint8_t colorLUT[32];
	
	auto isForeground() -> bool { return gBits & 2; }

	inline auto setLineInterrupt() -> void;
	inline auto setLineBuffer() -> void;
     	
	//dma
	auto advanceCycle() -> void;	
	auto clearCollisions() -> void;
	auto spriteUpdateBase() -> void;
	auto spriteDmaCheck() -> void;
	auto spriteExpand() -> void;
	auto spriteDisplayCheck() -> void;
	auto updateVc() -> void;
	auto updateRc() -> void;
	auto updateBAState( uint32_t flags ) -> void;	      
    auto updateBadLine() -> void;
	auto borderControl() -> void;
	template<bool first> auto borderLeft( ) -> void;
	template<bool first> auto borderRight( ) -> void;
	auto idleCycle() -> void;
	auto refresh() -> void;

	// fetch
	template<bool logDma> inline auto fetchPhi1( uint32_t flags ) -> uint8_t;
	inline auto fetchSprPhi2( uint32_t flags ) -> void;
	inline auto sprHasDma(uint8_t pos) -> bool;
	inline auto addrG( uint8_t useMode ) -> uint16_t;
	template<bool logDma> auto fetchIdleG() -> uint8_t;
	template<bool logDma> auto fetchG() -> uint8_t;
	template<bool logDma> auto fetchC() -> void;
	template<bool logDma> inline auto fetchSpriteP( uint8_t pos ) -> uint8_t;
	template<bool logDma> auto fetchSpriteS1(uint8_t pos) -> uint8_t;
	inline auto fetchSpriteS0(uint8_t pos) -> void;
	inline auto fetchSpriteS2(uint8_t pos) -> void;
	auto isCharRomAccessed(uint16_t addr) -> bool;
	inline auto readCpu() -> uint8_t;
    template<bool phi1, bool logDma, bool peek = false> inline auto readPhi(uint16_t addr) -> uint8_t;
    
    //sequencer
	auto sequencer( uint32_t flags ) -> void;
	template<bool phi1> auto sequencerPix0(  ) -> void;
	template<bool phi1> auto sequencerPix1(  ) -> void;
	template<bool phi1> auto sequencerPix2(  ) -> void;
	template<bool phi1> auto sequencerPix3(  ) -> void;
	auto pipeGraphic( uint32_t flags ) -> void;
	auto graphicSequencer( uint8_t x ) -> void;
	auto triggerSprites( uint16_t xPos ) -> void;
    template<uint8_t sprPos> auto triggerSprites( uint16_t xPos ) -> void;
	auto updateMc6569() -> void;
	auto updateMc8565() -> void;	
	auto spriteSequencer(  ) -> void;
    template<uint8_t sprPos> auto spriteSequencer( Sprite* spr, Sprite*& sprUse, uint8_t& collision ) -> void;

	inline auto draw( uint8_t metaDataFirstHalf ) -> void;    
	inline auto draw65( unsigned offset, uint8_t x, uint8_t x1, uint8_t metaData ) -> void;    
	inline auto draw85( unsigned offset, uint8_t x, uint8_t metaData, bool greyDot = false ) -> void;    
	inline auto borderArea( bool hFlipFirstHalf  ) -> void;

    auto nextLineWithDmaView() -> void;
	
};

}
