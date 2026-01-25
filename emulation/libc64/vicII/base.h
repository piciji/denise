
#pragma once

#include <cstdint>
#include "../../tools/macros.h"
#include "../../tools/serializer.h"
#include "../../interface.h"

#define VIC_MAX_LINE_LENGTH 65 * 8
#define VIC_MODE_MCM(_mode) (_mode & 4)
#define VIC_MODE_BMM(_mode) (_mode & 8)
#define VIC_MODE_ECM(_mode) (_mode & 0x10)
#define LEFT_LINE_ANOMALY 3500
#define LEFT_LINE_ANOMALY_ONE_PIX (LEFT_LINE_ANOMALY - 700)

namespace LIBC64 {  

struct System;
struct ExpansionPort;
struct M6510;
struct DebuggerSnapshot;

struct VicIIBase {
	VicIIBase(System* system);
	virtual ~VicIIBase();

	enum Interrupt {
		Raster = 0, MBC = 1, MMC = 2, LP = 3, Update = 4
	};

	enum Model {
		MOS6569R3 = 0, MOS8565 = 1, MOS6567R8 = 2, MOS8562 = 3,
		MOS6569R1 = 4, MOS6567R56A = 5, MOS6572 = 6, MOS6573 = 7,
		UnInitialized = 0xff
	} model = UnInitialized;

	enum CycleMode {
		C_PAL = 0, C_NTSC = 1, C_NTSC_OLD = 2
	};

	struct {
		// full posible border, considers rSel and cSel
		// doesn't consider sprites, misused as background in border area
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
        bool store = false;
        struct {
            uint8_t* data = nullptr;
            unsigned pos = 0;
            unsigned lastPos = 0;
        } spr[8];

        auto setEnable(bool state) -> void {
            store = state;
            reset();
        }
        auto reset() -> void { for (auto& s : spr) { s.lastPos = s.pos; s.pos = 0; } }
    } debugInfo;

    uint8_t reg2mhz;
    Emulator::Interface::DebuggerAction debuggerAction;
    
	M6510& cpu;
	ExpansionPort* expansionPort;
    System* system;
	auto setModel(Model model) -> void;
	auto getModel() -> Model { return model; }
	auto updateBorderData() -> void;
	auto setBorderData() -> void;
	auto getCrop(unsigned w, unsigned h) -> Emulator::Interface::Crop;
	
    virtual auto clock() -> void = 0;    
	virtual auto reuBaLow() -> bool = 0;
    virtual auto reuSprite0() -> bool = 0;
	virtual auto serialize(Emulator::Serializer& s) -> void = 0;
	virtual auto readReg( uint8_t addr ) -> uint8_t = 0;
    virtual auto peekReg( uint8_t addr ) -> uint8_t = 0;
    virtual auto writeReg( uint8_t addr, uint8_t value ) -> void = 0;	
    virtual auto power() -> void;
	virtual auto triggerLightPen( bool state ) -> void;
    virtual auto triggerLightPen( bool state, uint8_t subCycle ) -> void;
	virtual auto getCurrentLinePtr() -> uint8_t* = 0;
	virtual auto getCurrentFramePtr() -> uint8_t* = 0;
	virtual auto lastReadPhase1() -> uint8_t { return 0; }
	virtual auto isAecLow() -> bool { return false; }
	virtual auto isScanlineRenderer() -> bool = 0;

    auto isRevision65() -> bool { return rev65; }
    auto disableSequencer( bool state ) -> void;
    inline auto useSequencer() -> bool { return enableSequencer; }
    auto getHeight() -> unsigned { return vHeight; }
    auto getWidth() -> unsigned { return hWidth;  }
    auto getCyclesForNextLightTrigger( int x, int y, uint8_t& cyclePixel ) -> unsigned;
    
    auto isBaLow() -> bool { return baLow; }
    
    auto getVcounter() -> unsigned { return vCounter; }
	
	auto isNTSCGeometry() -> bool { return ntscGeometry; }
	auto isNTSCEncoding() -> bool { return ntscEncoding; }
	auto frequency() -> unsigned { return cyclesPerSec; }
	auto cyclesPerFrame() -> unsigned { return lineCycles * lines; }
	auto getCycle() -> uint8_t { return cycle; }

	auto getReg18() -> uint8_t;
	auto setUltimaxPhi1(bool state ) -> void { ultimaxPhi1 = state; };
    auto setUltimaxPhi2(bool state ) -> void { ultimaxPhi2 = state; };
    auto setUltimax(bool state ) -> void { ultimaxPhi1 = ultimaxPhi2 = state; };
    auto oldOne() -> bool { return oldIrqMode; }
    auto inVisibleArea() -> bool { return visibleLine; }
    virtual auto updateSnapshot(DebuggerSnapshot& snap) -> void;
    auto updatePositionSnapshot(DebuggerSnapshot& snap) -> void;
    auto setVicBank(uint16_t data) -> void { vicBank = data; }
    auto getVicBank() -> uint16_t { return vicBank; }

protected:     
    bool ultimaxPhi1;
    bool ultimaxPhi2;
	auto generateCycleTable(CycleMode _m) -> void;

	uint32_t cycleTab[65];
	uint8_t colorReg[0x2f];
	
	uint32_t flags;
	uint8_t color;	
	
	uint16_t vcBase;
	uint16_t vc;
	uint8_t rc;
	uint16_t cBuffer[40];

	bool rev65; //true: 65xx chips, false: 85xx chips
	unsigned lineCycles;
	unsigned lines;
	bool oldIrqMode;
	bool ntscGeometry; //true: 263, 262(OLD NTSC), false: 312
	bool ntscEncoding;
	bool ntscBorder; // true: NTSC, PAL-N, OLD-NTSC false: PAL
	unsigned cyclesPerSec;

	uint8_t cycle;
	unsigned vCounter;
	uint16_t xCounterLatch;
	uint16_t xCounterLatchBefore;
	unsigned vStart;
	unsigned vHeight;
	unsigned hWidth;
	unsigned firstVisiblePixel;

	bool baLow; //connected to 6510 rdy and expansion port	
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
	bool lpTrigger;
	uint8_t lpTriggerDelay;
	bool lpPhi1;

	bool rSel;
	bool cSel;

	uint8_t modeEcmBmm; // 4:ecm | 3:bmm "for easy oring with mcm"	(scanline renderer: 2:ecm | 1:bmm)
	uint8_t modeMcm; // 2:mcm  (scanline renderer: 0:mcm)
	
	uint8_t controlReg1;
	uint8_t controlReg2;

	unsigned linePos;
	unsigned lineVCounter;

	static uint8_t* frameBuffer;
	uint8_t lineBuffer[65 * 8];

	bool visibleLine;
	bool hFlipFlop;
	bool vFlipFlop;	
	bool idleMode;	
	bool initVCounter;

    uint16_t _addrG;
    uint16_t vicBank;

	struct Sprite {
		uint8_t position;
		bool enabled;

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
	} sprite[8], *sprite0, *sprite1, *sprite2, *sprite3, *sprite4, *sprite5, *sprite6, *sprite7;

	uint8_t spriteDma;
	uint8_t spriteActive;

	uint8_t spriteForegroundCollided;	
	uint8_t spriteSpriteCollided;
		
	bool canSpriteSpriteCollisionIrq;
	bool canSpriteForegroundCollisionIrq;
	
	bool enableSequencer = true;

    auto oneTimeDebuggerAction() -> void;
	auto updateIrq(Interrupt interrupt = Update) -> void;
	auto checkLightPen() -> void;
    auto storeSprite(Sprite& spr) -> void;
};

}
