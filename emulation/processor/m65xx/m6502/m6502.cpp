
#include "m6502.h"

#include "memory.cpp"
#include "logic.cpp"
#include "address.cpp"
#include "opcodes.cpp"
//#include "optable.cpp"
#include "undocumented.cpp"

namespace MOS65FAMILY {

    auto M65Model::create6502() -> M65Model* {
        return new M6502;
    }

    M6502::M6502() {
        dummyCtx = new M65Context;
        dummyCtx->isDummy = true;
        dummyCtx->rdyLine = false;
    }

    auto M6502::setContext(M65Context* context) -> void {
        this->ctx = this->workCtx = context;
    }

    inline auto M6502::busRead(uint16_t addr) -> uint8_t {

        return ctx->read(addr);
    }

    inline auto M6502::busWrite(uint16_t addr, uint8_t data) -> void {

        ctx->write(addr, data);
    }

    inline auto M6502::busWatch() -> uint8_t {

        return ctx->watch();
    }

    auto M6502::leaveRdyHaltedOpcode() -> void {

        if (!ctx->rdyLine)
            return;

        dontBlockExecution = true;
    }

    auto M6502::power() -> void {

        if (workCtx == nullptr)
            workCtx = new M65Context; //forget something :-)

        restoreContext();

        S = 0x00;
        //some of these values could be random on first power on
        X = 0x00;
        Y = 0x00;
        A = 0xaa;
        PC = 0x00ff;
        ctx->db = 0;
        ctx->addrBus = 0;
        ctx->rdyLine = false;
        setFlags(0x02);

        reset();
    }

    auto M6502::reset() -> void {
        // a reset in (rdy) halted state fires as soon as rdy goes hi again, interrupted opcde will to resumed first.    
        restoreContext();

        ctx->resetCompleted = false;
        ctx->irqLine = ctx->nmiLine =
            ctx->irqPending = ctx->nmiPending =
            ctx->nmiDetect = ctx->interruptSampled = false;
        ctx->soLine = ctx->soDetect = ctx->soSampled = false;

        ctx->killed = false;
        ctx->xaa = false;
        ctx->cli = false;
        ctx->sei = false;
        ctx->storeFlags = false;
        ctx->soBlock = 0;
        dontBlockExecution = false;
    }

    auto M6502::resetRoutine() -> void {
        // cold boot:   8 cycles
        // reset:       8 cycles + 1 cycle for transition detection

        // for power on (cold boot), this cycle does the initialization in power function above.  
        readPC<1>();

        // the abillity of 6502 to reset in between an opcode is not emulated. 
        // the emulated reset happens only at opcode edge.
        // a real 6502 can reset within each cycle when a transition of 0 -> 1 is detected at reset pin.
        // if reset transition happens in a write cycle, it seems write cycles in a row will complete before
        // reset routine starts.
        // from an emulation point of view an external reset by user can only happen while UI events are processed.
        // mostly one time per frame. the emulated reset will happen in last cycle (transition cycle) of an opcode.
        // assume you trigger it by accident always in last opcode cycle. :-)

        // emulated: rdy halts the processor in any read cycle. that is true for all reset cycles too.

        readPC<2>();
        readPC<3>();

        read<4>(0x100 | ctx->s--); //yes post decrementing for pull
        // not quite right but without consequences, only the final decrementation of stack is saved in register
        read<5>(0x100 | ctx->s--);
        read<6>(0x100 | ctx->s--);

        setPCL(read<7>(0xfffc));
        ctx->i = true; //disable interrupts   
        setPCH(read<8>(0xfffd));

        ctx->resetCompleted = true;
    }

    inline auto M6502::restoreContext() -> void {

        ctx = workCtx;
    }

    auto M6502::setMagicForAne(uint8_t magicAne) -> void {
        ctx->magicAne = magicAne;
    }

    auto M6502::getMagicForAne() -> uint8_t {
        return ctx->magicAne;
    }

    auto M6502::setIrq(bool state) -> void {
        // level sensitive
        ctx->irqLine = state;
    }

    auto M6502::setNmi(bool state) -> void {
        // edge sensitive ( triggers only: 0 -> 1)
        // technical its a negative going edge ( 1 -> 0 )
        // we invert it for emulation, because it's better readable
        ctx->nmiLine = state;
    }

    auto M6502::setSo(bool state) -> void {
        // edge sensitive too ( triggers only: 0 -> 1)
        ctx->soLine = state;
    }

    auto M6502::setRdy(bool state) -> void {
        //halts the cpu in next read
        ctx->rdyLine = state;
    }

    auto M6502::dataBus() -> uint8_t {
        // last used value on bus
        return ctx->db;
    }

    auto M6502::addressBus() -> uint16_t {
        // last puted address on bus
        return ctx->addrBus;
    }

    auto M6502::getFlags() -> uint8_t {
        return C | Z << 1 | I << 2 | D << 3 | V << 6 | N << 7;
    }

    auto M6502::setFlags(uint8_t data) -> void {
        C = data & 1;
        Z = (data >> 1) & 1;
        I = (data >> 2) & 1;
        D = (data >> 3) & 1;
        V = (data >> 6) & 1;
        N = (data >> 7) & 1;
    }

    auto M6502::interrupt(bool software) -> void {

        if (!software) {
            readPC<1>();
            readPC<2>();
        } else
            readPCInc<1>();

        pushStack((PC >> 8) & 0xff);
        pushStack(PC & 0xff);

        ctx->vector = 0xfffe;

        /**
         * NOTE: new pending nmi's recognized in the beginning of this service routine
         * (software break too) will be lost
         */
        if (ctx->nmiPending) {
            ctx->nmiPending = false;
            ctx->vector = 0xfffa; // a late nmi can hijack irq
        }

        ctx->storeFlags = true;
        pushStack(getFlags() | (software ? 0x30 : 0x20));
        ctx->storeFlags = false;

        ctx->dataW = read<3>(ctx->vector);

        I = 1;

        ctx->dataW |= read<4>(ctx->vector + 1) << 8;

        PC = ctx->dataW;
        /**	 
         * no interrupt polling at the end of this service routine (software break too)
         * so at least one opcode is following before could interrupted again by nmi
         */

        ctx->interruptSampled = false;
    }

    inline auto M6502::sampleInterrupt() -> void {

        // happens during last cpu cycle    
        ctx->interruptSampled |= ctx->nmiPending | (ctx->irqPending & ~ctx->i);
    }

    inline auto M6502::detectInterrupt() -> void {
        /**
         * happens during second half cycle of each cpu cycle
         * NOTE: a short transition in first half cycle and reverting back in the second
         * isn't recognized
         */
        ctx->irqPending = ctx->irqLine;

        if (!ctx->nmiDetect && ctx->nmiLine)
            ctx->nmiPending = true;

        ctx->nmiDetect = ctx->nmiLine;
    }

    inline auto M6502::handleSo() -> void {
        // a sampled external SO is executed in first half cycle. when cpu accesses v flag
        // internally in second half cycle too, the external change is wasted.
        // instructions like adc, sbc do the calculation not in last opcode cycle. there is 
        // simply no time because the data fetch for calculation happens in last half cycle.
        // the calculation is done in first cycle of next instruction during the opcode fetch.
        // for simplicity the calculation is done in context of last instruction cycle.
        // so we need to take care in case of external SO in emulation, because it would
        // execute in wrong order. thats why we use the following "Block" variable, seted
        // in the end of overflow accessing instructions.

        if (ctx->soBlock)
            // overflow is not delayed but not executed
            ctx->soBlock--;

        else if (ctx->soSampled) {
            // executes in first half, one cycle after detection
            V = 1;
            // some instructions store the status register on stack.
            // because of emulator design the status register is sampled
            // before this function call. so we have to inject the seted overflow
            // value just before second half cycle executes. 
            if (ctx->storeFlags) // php, interrupt, brk
                ctx->db |= 1 << 6;
        }

        // is detected in any first half cycle at ~400 ns    
        ctx->soSampled = !ctx->soDetect && ctx->soLine;

        ctx->soDetect = ctx->soLine;
    }

    auto M6502::setPCL(uint8_t data) -> void {

        PC = (PC & ~0xff) | data;
    }

    auto M6502::setPCH(uint8_t data) -> void {

        PC = (PC & 0xff) | (data << 8);
    }

    auto M6502::process() -> void {

        dontBlockExecution = false;

        if (!ctx->resetCompleted) {
            // resume interrupted reset by rdy
            return resetRoutine();
        }

        /**
         * can recover from reset only, interrupt detection isn't working anymore
         */
        if (ctx->killed) {
            read<1>(0xffff);
            return;
        }

        if (ctx->interruptSampled)
            return interrupt();

        /**
         * basically next instruction fetch is part of currently processed opcode
         * hence some opcodes process final logic during instruction fetch
         * for reduced code complexity it's emulated here without loss of accuracy
         */
        if (!ctx->isDummy)
            ctx->IR = readPCInc();
        /**
         * next cpu half cycle is decoding
         */

        decode(ctx->IR);
    }

    auto M6502::decode( uint8_t IR ) -> void {
        #define COMMA ,
        #define op(id, name, ...) case id: return name(__VA_ARGS__);
        #define fp(name) &M6502::_##name
        #define UO //undocumented opcode but always predictable
        /**
         * don't use these kind of opcodes
         * some results differs between visual6502 and real cpu
         * Visual6502 is a digital representation so it can not handle race conditions that good
         * real cpu could produce different results, depending on a lot of things like heat, cpu version, bus usage and so on
         */
        #define UUO //unstable undocumented opcode

        switch( IR ) {
            #include "optable.cpp"
        }
        
        #undef op
        #undef fp
        #undef UO
        #undef UUO
        #undef COMMA	
    }
}
