
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
    
struct VicIIBase {
    
    VicIIBase();
    ~VicIIBase();

    std::function<uint8_t (uint16_t)> read;
	std::function<uint8_t (uint16_t)> readColor;
    std::function<void (bool)> setIrq;
    std::function<void (bool)> setRdy;
    std::function<void ( uint16_t*, unsigned, unsigned, unsigned)> videoRefresh;
	std::function<uint16_t ()> readCpu;
    std::function<bool (uint16_t)> isCharRomAccessed;
    std::function<void ()> midScreenCallback;
    std::function<void ()> vblankCallback;
    
    enum Interrupt { Raster = 0, MBC = 1, MMC = 2, LP = 3, Update = 4 };		   
    
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
        bool use;
        unsigned line;
        bool finishVblank;        
    } lineCallback;    
	
	struct {
		uint8_t mode = 0;
		unsigned framePos = 1;
		bool permanent = false;
	} leftLineAnomaly;
	
	auto updateBorderData() -> void;
	auto setBorderData() -> void;
    
	virtual auto power() -> void;    
    virtual auto powerOff() -> void {}
    virtual auto clock() -> void {}
    virtual auto clockSilence() -> void {}
    virtual auto serialize(Emulator::Serializer& s) -> void {}
    virtual auto reuBaLow() -> bool { return baLow; }
	
    virtual auto readReg(uint8_t addr) -> uint8_t { return 0; }
    virtual auto writeReg(uint8_t addr, uint8_t value) -> void {}
    auto setNtsc( bool state ) -> void;
	auto setRevision65( bool state ) -> void;
    auto setMeta( bool state ) -> void;
    auto isRevision65() -> bool;
    auto disableSequencer( bool state ) -> void;
    auto useSequencer() -> bool;
	auto triggerLightPen( bool state ) -> void;
    auto triggerLightPen( bool state, uint8_t subCycle ) -> void;
    auto getHeight() -> unsigned { return vHeight; }
    auto getWidth() -> unsigned { return hWidth;  }
    auto getCyclesForNextLightTrigger( int x, int y, uint8_t& cyclePixel ) -> unsigned;
    virtual auto getCurrentLinePtr() -> uint16_t*;
    
    auto initColorWheel() -> void;
    auto getLuma(uint8_t index, bool newRevision) -> double;
    auto getChroma(uint8_t index) -> double;     
    
    auto lastReadPhase1() -> uint8_t { return lastReadPhi1; }
    auto isAecLow() -> bool { return aecDelay == 0; }
    auto isBaLow() -> bool { return baLow; }
    
    auto setVerticalLineAnomaly(uint8_t mode) -> void;
    auto getVerticalLineAnomaly() -> uint8_t;
    
    auto getVcounter() -> unsigned { return vCounter; }
   
protected:    
    bool rev65; //true: 65xx chips, false: 85xx chips
    bool addMeta; // add aec and ba state to output 
    
    double luma[2][16];
    double chroma[16]; // as angle on color wheel
    
	uint8_t lastReadPhi1;
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
    bool badLine;
	
    uint16_t irqLine;
	bool lineIrqMatched;
    uint8_t irqLatchPending;
    
    bool den;
    unsigned borderTop;
    unsigned borderBottom;
    uint8_t yScroll;
    uint8_t xScroll;
    uint8_t lpx;
    uint8_t lpy;
    uint8_t lpxBefore;
    uint8_t lpyBefore;
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
	bool idleModeTemp;
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

    struct Sprite {     
        uint8_t position;
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
		unsigned xPos; // for scanline renderer
		unsigned mask;
    } sprite[8], *sprite0, *sprite1, *sprite2, *sprite3, *sprite4, *sprite5, *sprite6, *sprite7, *spriteOpenBus;
    
    uint8_t lastSpriteShift;	
    uint8_t spriteTrigger;
    uint8_t spritePending;
    
	uint8_t spriteForegroundCollided;
    uint8_t spriteForegroundCollidedRead;
	uint8_t spriteSpriteCollided;
    uint8_t spriteSpriteCollidedRead;    
    uint8_t spriteDmaCycle1;
    uint8_t spriteDmaCycle2;
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
            		
    auto updateIrq( Interrupt interrupt = Update ) -> void;
	template<bool phi1> auto checkLightPen( ) -> void;	
	
    auto insertVerticalLineAnomaly(unsigned start, unsigned end) -> void;
	auto initVerticalLineAnomaly() -> void;
    void buildXCounterLookupTable();    
	auto updateSpriteWithBusValue(uint8_t value) -> void;
    auto updateSpriteBaState( uint8_t pos, bool dmaActive ) -> void;
    template<bool permanent> auto insertVerticalLineAnomaly(unsigned start, unsigned end) -> void;         
};
 
extern VicIIBase* vicII;
}