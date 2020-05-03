
//pal: non blank height: 293
//h: 200 border: 93 top 43 bottom 50
//h: 192 border 101 top 47 bottom 54
//
//ntsc: non blank height: 253
//h: 200 border: 53 top 29 bottom 24
//h: 192 border 61 top 33 bottom 28
//
//
//pal: non blank width: 406
//w: 320 border 86 left 46 right 40
//w: 304 border 102 left 53 right 49
//
//ntsc: non blank width: 420
//w: 320 border 100 left 56 right 44
//w: 304 border 116 left 63 right 53

#pragma once

#include <functional>
#include <cstring>
#include "../../tools/serializer.h"

// pal (63 cycles): emulate 6569R3 and 8565, not the 6569R1
// ntsc (65 cycles): emulate 6567R8 and 8562, not the 6567R56A (64 cycles)

#define VIC_MAX_LINE_LENGTH 65 * 8
#define VIC_MODE_MCM(_mode) (_mode & 1)
#define VIC_MODE_BMM(_mode) (_mode & 2)
#define VIC_MODE_ECM(_mode) (_mode & 4)
#define LEFT_LINE_ANOMALY 3500
#define LEFT_LINE_ANOMALY_ONE_PIX (LEFT_LINE_ANOMALY - 700)

namespace LIBC64 {  
    
struct VicII {
	VicII();
	~VicII();
    
    std::function<uint8_t (uint16_t)> read;
	std::function<uint8_t (uint16_t)> readColor;
    std::function<void (bool)> setIrq;
    std::function<void (bool)> setRdy;
    std::function<void ( uint16_t*, unsigned, unsigned, unsigned)> videoRefresh;
	std::function<uint16_t ()> readCpu;
    std::function<bool (uint16_t)> isCharRomAccessed;
    std::function<void ()> midScreenCallback;
    std::function<void ()> vblankCallback;
    
    enum Interrupt { None = -1, Raster = 0, MBC = 1, MMC = 2, LP = 3 };		   
    
	// vic emulation code buffers visible data for whole non blanking area.
	// following struct contains pixel widths to crop away complete border or
	// typical overscan of a crt monitor
    struct {
		// full posible border, considers rSel and cSel
		// doesn't consider sprites in border area
        bool rSel;
        bool cSel;
		unsigned top;
		unsigned bottom;
		unsigned left;
		unsigned right;
		
		// typical overscan of a crt monitor
		unsigned topOverscan;
		unsigned bottomOverscan;
		unsigned leftOverscan;
		unsigned rightOverscan;
    } crop;

	struct {
		bool pipelined;
		uint8_t addr;
		uint8_t value;
	} registerWrite;
    
    struct {
        bool use;
        unsigned line;
        bool finishVblank;        
        bool silence;
        bool silenceNext;
    } lineCallback;    
	
	struct {
		uint8_t mode = 0;
		unsigned framePos = 1;
		bool permanent = false;
	} leftLineAnomaly;
	
	auto updateBorderData() -> void;
	auto setBorderData() -> void;
    
	auto power() -> void;    
    template<bool _useSequencer> auto phase1() -> void;
    template<bool _useSequencer> auto phase2() -> void;    
	
    auto writeIO(uint8_t addr, uint8_t value) -> void;
	auto writeIOPipelined(uint8_t addr, uint8_t value) -> void;
    auto readIO(uint8_t addr) -> uint8_t;
    auto setNtsc( bool state ) -> void;
	auto setRevision65( bool state ) -> void;
    auto isRevision65() -> bool;
    auto disableSequencer( bool state ) -> void;
    auto useSequencer() -> bool;
	auto triggerLightPen( bool state ) -> void;
    auto triggerLightPen( bool state, uint8_t subCycle ) -> void;
    auto getHeight() -> unsigned { return vHeight; }
    auto getWidth() -> unsigned { return hWidth;  }
    auto getCyclesForNextLightTrigger( int x, int y, uint8_t& cyclePixel ) -> unsigned;
    auto getCurrentLinePtr() -> uint16_t*;
    auto serialize(Emulator::Serializer& s) -> void;
    
    auto initColorWheel() -> void;
    auto getLuma(uint8_t index, bool newRevision) -> double;
    auto getChroma(uint8_t index) -> double; 
    auto reuBaLow() -> bool;
    
    auto lastReadPhase1() -> uint8_t { return lastReadPhi1; }
    auto isAecLow() -> bool { return aecDelay == 0; }
    auto isBaLow() -> bool { return baLow; }
    
    auto setVerticalLineAnomaly(uint8_t mode) -> void;
    auto getVerticalLineAnomaly() -> uint8_t;
    
    auto getVcounter() -> unsigned { return vCounter; }
   
protected:    
    bool rev65; //true: 65xx chips, false: 85xx chips
    
    double luma[2][16];
    double chroma[16]; // as angle on color wheel
    
	uint8_t lastReadPhi1;
	uint8_t lastBusPhi2;
    uint8_t render[4];
    uint8_t renderPipe[8];
    uint8_t colorReg[0x2f];
    uint8_t colorUse[0x2f];
    uint8_t lastColorReg;        
    
	unsigned cycle;
    unsigned vCounter;
    unsigned xCounter;
    unsigned xCounterLatch;
    unsigned xCounterSprites;
    unsigned vStart;
	unsigned vHeight;
    unsigned hWidth;
    unsigned lineCycles;
    unsigned firstVisiblePixel;
	
	bool baLow; //connected to 6510 rdy and expansion port
    uint8_t aecDelay;
    bool spriteBa[9][65];
	bool allowBadlines;
	
    uint16_t irqLine;
	bool lineIrqMatched;
	bool lpIrqPending;
    
    bool den;
    unsigned borderTop;
    unsigned borderBottom;
    uint8_t yScroll;
    uint8_t lpx;
    uint8_t lpy;
    uint8_t vm;
    uint8_t cb;
    uint8_t irqLatch;
    uint8_t irqEnable;
    bool lpLatched;    
    bool lpPin;
	uint8_t lpTriggerDelay;
    bool lpPhi1;
    
    bool rSel;
    bool cSel;

    uint8_t controlReg1;
    uint8_t controlReg2;
    
    uint16_t* frameBuffer;
    uint16_t* linePtr;
    unsigned linePos;
    unsigned lineVCounter;

    bool visibleLine;
    bool hFlipFlop;
    bool vFlipFlop;
	bool vFlipFlopShadow;
    bool idleMode;
    bool initVCounter;    
    bool lpTrigger;
    
	uint8_t refreshCounter;    
    // ntsc: 6567R8 (65 cycles), pal: 6569 (63 cycles)
    bool ntsc;
    
    // actual register values
    uint8_t modeEcmBmm;  // 2:ecm | 1:bmm | 0:0  "for easy oring with mcm"
    uint8_t modeMcm;  // 0:mcm
    
    // there are delays of a few pixels till changed register values will be internally used
    // following variables gets the register values after some time of execution
    // in the context of DMA
    uint8_t modeEcmBmmDma;
    uint8_t modeMcmDma;
    // in the context of sequencer
    uint8_t modeEcmBmmSequencer;
    uint8_t modeMcmSequencer;        
    
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
        uint8_t gBufferPipe1;
        uint8_t gBufferPipe2;
		bool enable;
		uint8_t dmli;
		uint8_t gBufferShift;
		uint8_t gBits; // 2 bit (0-3) or 1 bit: 10 or 00
		
		auto isForeground() -> bool { return gBits & 2; }
    } display;

    struct Sprite {       
		bool enabled;
        bool dma;
        bool halt;
        bool active;

        uint8_t dataP;
        uint32_t dataS; 
        uint32_t dataShiftReg;
		uint8_t shiftOut;
                
        uint8_t mcBase;
        uint8_t mc;
        		
		uint8_t y;		
        uint16_t x;
        uint16_t useX;
		bool prioMD;
		bool usePrioMD;
		bool expandY;
		bool expandX;
		bool useExpandX;
		bool multiColor;
        bool useMultiColor;
        bool mcFlop;
		bool expandYFlop;
        bool expandXFlop;
        uint8_t colorCode;
    } sprite[8], *sprite0, *sprite1, *sprite2, *sprite3, *sprite4, *sprite5, *sprite6, *sprite7;
	
    uint8_t spriteTrigger;
    uint8_t spriteDisplay;
    uint8_t spritePending;
    
	uint8_t spriteForegroundCollided;
	uint8_t spriteSpriteCollided;    
    uint8_t spriteDmaCycle1;
    uint8_t spriteDmaCycle2;
    bool spriteDisplayCycle;
	uint8_t clearCollision;
	bool canSpriteSpriteCollisionIrq;
	bool canSpriteForegroundCollisionIrq;
    bool updateMc;
    bool updatePrioExpand;
    bool cAccessArea;
    bool sprite0DmaLateBA;
    bool disableEcmBmmTogether;
    uint16_t xLookUpPalPhi1[63];
    uint16_t xLookUpPalPhi2[63];
    uint16_t xLookUpNtscPhi1[65];
    uint16_t xLookUpNtscPhi2[65];
    uint16_t* xLookupPtrPhi1;
    uint16_t* xLookupPtrPhi2;
    bool enableSequencer = true;
            		
    auto updateIrq( Interrupt interrupt = None ) -> void;
	template<bool phi1> auto checkLightPen( ) -> void;	
	
	//dma
	auto advanceCycle() -> void;
	auto setLineInterrupt() -> void;
	auto clearCollisions() -> void;
	auto setLineBuffer() -> void;
	auto spriteUpdateBase() -> void;
	auto spriteDmaCheck() -> void;
	auto spriteFlip() -> void;
	auto spriteDisplayCheck() -> void;
	auto updateSpriteBaState( uint8_t pos, bool dmaActive ) -> void;
	auto updateVc() -> void;
	auto updateRc() -> void;
	auto updateBAState() -> void;	      
    auto badLine() -> bool;
	auto borderControl() -> void;
	auto borderLeft( bool c17 ) -> void;
	auto borderRight( bool c56 ) -> void;
	auto idleCycle() -> void;
	auto refresh() -> void;
	auto fetchSpriteP( uint8_t pos ) -> void;
    auto fetchSpriteS0( uint8_t pos ) -> void;
    auto fetchSpriteS1( uint8_t pos ) -> void;
    auto fetchSpriteS2( uint8_t pos ) -> void;    
    auto fetchSpriteSPhi2( uint8_t pos, bool last ) -> void;
	auto fetchC() -> void;
    auto addrG( uint8_t useMode ) -> uint16_t;
    auto fetchG() -> void;    
    template<bool permanent> auto insertVerticalLineAnomaly(unsigned start, unsigned end) -> void;
	auto insertVerticalLineAnomaly(unsigned start, unsigned end) -> void;
	auto initVerticalLineAnomaly() -> void;
	
	//sequencer
	template<bool phi1> auto sequencer(  ) -> void;
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
    void buildXCounterLookupTable();
};

extern VicII* vicII;
}
