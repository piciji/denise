
#pragma once

#include <functional>

namespace MOS65FAMILY {       
    
struct M65Context {

    /** read byte from address */
	std::function<uint8_t (uint16_t)> read;
    
    /** write byte to address */
	std::function<void (uint16_t, uint8_t)> write;
    
    /** get last palced value from bus in case of cpu don't own bus */
    std::function<uint8_t ()> watch;
    
    /** informs about cpu has proceeded one half cycle
     *  alternates always between lo and hi cycle, lo + hi = 1 cpu cycle
     *  bus is always accessed each second half cycle ( there are no pure internal cpu cycles )
     *  this results in some dummy accesses
     *  hi cycle is internal operation which put address on bus
     *  lo cycle stalls bus by reading or writing
     *  lo and hi cycles take same absolute time in relation to cpu frequency
     *  NOTE: this cycle exact emulation requires that you sync up all other bus participants immediately
     *  otherwise interupt recognition is not working properly
     */
    std::function<void ()> syncHi;
    std::function<void ()> syncLo;    

	/** informs about cpu has updated port lines (6510 only)	 */
	std::function<void (uint8_t, uint8_t)> updatePort;
    
    bool c;
    bool z;
    bool i;
    bool d;
    bool v;
    bool n;    
	
    uint8_t IR;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint16_t pc;
    uint8_t db; //last readed or written value on bus
    uint16_t addrBus; //last puted address on bus
    
    /** represents cpu pin not the internal state, because it's not detected immediately */
    bool irqLine = false;
    bool nmiLine = false;
    
    /** edge detected when transition (0->1) only */
    bool nmiDetect = false;    
    
    /** pending state is recognized during level or edge detection */
    bool irqPending = false;
    bool nmiPending = false;

    /** polled state */
    bool interruptSampled = false;
	
    bool rdyLine = false;
    
    bool killed = false;
	
	uint8_t magicAne = 0xee;
    
    // SO Handling
    bool soLine = false;
    
    bool soDetect = false;
    
    bool soSampled = false;        
    
	// 6510 cpu port
	
    // 6510 data direction register
	uint8_t ddr;
	/** 6510 peripheral output register 
	 *  isn't overwritten for incomming data
	 *  contains always data you wrote to $01, doesn't matter if bit is input or output
	 */
	uint8_t por;
	
	/**
	 * lines in output mode show state of por register
	 * lines in input mode show state provided from external devices
	 */
	uint8_t ioLines;
	
	/**
	 * seted lines in input mode will be pulled up (1)
	 * lines not pulled up or down don't change by switching from output to input
	 */
	uint8_t pullup = 0;
	/**
	 * seted lines in input mode will be pulled down (0)
	 * lines not pulled up or down don't change by switching from output to input
	 */
	uint8_t pulldown = 0;
	
	/**
	 * for bit 6 and bit 7 there is no io line
	 * in output mode it reads back what you stored (like the other bits)
	 * in input mode a written 1 drops back to 0 after a few cycles
	 * NOTE1: happens only if you switch direction from output to input
	 * NOTE2: doesn't mean a 1 in the por reg is overwritten too
	 */
	struct {
		unsigned cycles;
		uint8_t charge;
	} bit6, bit7;
    
    // need to memory a few values mostly for unstable undocumented opcodes
    struct {
		uint16_t absolute;
        uint16_t absIndexed;
        uint8_t zeroPage;
		bool boundaryCrossing;
		bool rdyLastCycle;
		bool xaa; // or ane
        bool cli;
        bool sei;
        bool storeFlags;
        uint8_t soBlock;
	} mem;
        
    // unlike VIC the c64 expansion port dma line is able to halt(rdy) cpu for a long time, e.g. reu, super cpu.
    // but we need to jump out at least one time each frame to synchronize UI events.
    // remember cycle position within opcode, save context, execute opcode in a dummy context, jump out ... do external stuff
    // jump in, repeat execution of last opcode in dummy context till interrupted cycle, swap real context and finally go on.
    struct {
        uint8_t cycle = 0;
        bool active = false;
        M65Context* ctx = nullptr;
    } jumpOut;
	
    M65Context() {
        
        read = [](uint16_t) { return 0; };
        write = [](uint16_t, uint8_t) { };
        watch = []() { return 0; };
        syncHi = []() {};
        syncLo = []() {};
		updatePort = [](uint8_t, uint8_t) {};
    }
};

}