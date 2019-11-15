
#pragma once

#include <functional>
#include <vector>

namespace M68FAMILY {
	
struct M68Context {		
	/** read byte from address */
	std::function<uint8_t (uint32_t)> read;
	/** read word from address */
	std::function<uint16_t (uint32_t)> readWord;
	/** write byte to address */
	std::function<void (uint32_t, uint8_t)> write;
	/** write word to address */
	std::function<void (uint32_t, uint16_t)> writeWord;
	/** consumed cycles in small steps, is called more times per instruction */
	std::function<void (unsigned)> sync;
	/** cpu informs about halted state, call reset to recover */
	std::function<void ()> cpuHalted;
	/** reset external devices (e.g. Amiga), unsupported for (e.g. Genesis) */
	std::function<void ()> resetInstruction;
	/** interrupt acknowledge cycle fetches vector from external device (e.g. Amiga),
	not needed for AUTO_VECTOR (e.g. Genesis) */
	std::function<unsigned (uint8_t)> getUserVector;
	/** for 68000, 68010: cpu samples state of ipl lines (interrupt level) during last bus cycle of instruction.
    for 68020+: interrupts will be sampled within two cycles and not at a fixed point.
	you have the chance to sync up interrupt generating devices before */
	std::function<void ()> sampleIrq;
	/** informs just before TAS places write on bus
	address strobe (AS pin) remains asserted after read cycle, so read and following write
	are indivisible, just like one single bus cycle. Assume two cpus performs TAS at the same time
	be carefull that second cpu doesn't read just after first cpu reads but before first cpu writes		
	it would result that both cpus assume that they could access bus

	it's unsupported for Genesis (I/O controller of Genesis 1 and 2 prevent write)
	it's unsupported for Amiga (danger: tas is not atomic if Amiga uses DMA, means conccurrent access) */
	std::function<void ()> tasWrite;	
	/** 68010+ bkpt instruction informs hardware just before illegal exception happens */
	std::function<void (uint8_t)> breakpoint;
    
	/** port size modes */
	enum : uint8_t { Byte = 0, Word = 1, Long = 2 };
    /** dynamic bus sizing for 68020+ cpus, return port size above */
    std::function<uint8_t (uint32_t)> portSize;

/** private, describes internal state of cpu
	set it for resuming purposes only 
 */	
	uint32_t d[8] = {0};
	uint32_t a[8] = {0}; // a7 is usp, a7' is ssp, a7'' is msp
	uint32_t usp;
	uint32_t ssp; // ssp = (isp) for 68020+
	uint32_t msp; // 68020+	
	uint32_t prefetchCounter; //assume 2 internal prefetch counters, pc + 4
	uint32_t prefetchCounterLast; //pc + 2
	/**
	 * assume pc is updated from prefetch counter and not calculated
	 * e.g. bcc needs only 2 cycles to increment pc and add displacement
	 */
	uint32_t pc;
	//68000 instruction regs
	uint16_t ir;
	uint16_t irc;
	uint16_t ird;
	//68020 instruction regs
	uint32_t cacheHolding;
	uint16_t stageB;
	uint16_t stageC;
	uint16_t stageD;

	bool c;		//carry
	bool v;		//overflow
	bool z;		//zero
	bool n;		//negative
	bool x;		//extend
	uint8_t i;	//interrupt mask
	bool s;		//supervisor mode
	bool t;		//trace mode (t1 for 68020+)
	//68020+
	bool t0;	//branch trace
	bool m;		//master interrupt mode

	//68020+ cache regs
	bool ca_c;
	bool ca_ce;
	bool ca_f;
	bool ca_e;
	uint32_t caar;

	//68010+ vector regs
	uint32_t vbr;
	uint32_t sfc;
	uint32_t dfc;
				
	uint8_t irqPendingLevel = 0; //reflect ipl lines
	uint8_t irqSamplingLevel = 0;
	bool level7PendingTrigger = false;
	bool level7SamplingTrigger = false;	

	bool stop = false;
	bool halt = false;
	bool trace = false;
	
	//68020+ for stacking address of previous instruction
	struct {
		uint32_t adrStageB;
		uint32_t adrStageC;
		uint32_t adrStageD;
		uint32_t adrLastInstruction;
	} history;
    
    //68020+ cache memory
	struct {
		bool valid[64];
		uint32_t data[64];
		uint32_t tag[64];
		bool FC2[64];		
	} cache;

	/** 68020+ instruction overlap and interrupt recognition */	
	int busCycles = 0; //relative, describes the difference between bus and sequencer
	unsigned absCycles = 0; //absolute consumed cycles, needed for detecting interrupts
	/** amount of cycles for last bus activity only, depends on port size and alignment */
	unsigned lastBusAccessCycles = 0;
	
	struct InterruptHistory {
		unsigned cycle;
		uint8_t level; //sampled level
		bool l7Trigger;
	};	
	std::vector<InterruptHistory> intrHis;
	
	//68010 loop Mode
	bool loopMode = false;
	
	M68Context() {
		read = [](uint32_t) { return 0; };
		readWord = [](uint32_t) { return 0; };
		write = [](uint32_t, uint8_t) { };
		writeWord = [](uint32_t, uint8_t) { };	
		sync = [](unsigned) {};
		cpuHalted = []() {};
		resetInstruction = []() {};
		getUserVector = [](uint8_t) { return 0; };
		sampleIrq = []() {};
		tasWrite = []() {};
        portSize = [](uint32_t) { return Word; };
		breakpoint = [](uint8_t) {};
	}
};

}
