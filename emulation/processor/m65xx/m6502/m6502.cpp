
#include "m6502.h"

#include "memory.cpp"
#include "logic.cpp"
#include "address.cpp"
#include "externalOverflow.h"
#include "opcodes.cpp"
#include "service.cpp"
#include "undocumented.cpp"

namespace MOS65FAMILY {

    auto M65Model::create6502() -> M65Model* {
        return new M6502;
    }

    auto M6502::setContext(M65Context* context) -> void {
        this->workCtx = context;
        
        if (context->useDummy)
            setDummyContext();
        else        
            restoreContext();
    }
    
    inline auto M6502::setDummyContext() -> void {

        workCtx->useDummy = true;
        
        ctx = workCtx->dummyCtx;                
    }
        
    inline auto M6502::restoreContext() -> void {

        workCtx->useDummy = false;
        
        ctx = workCtx;                
    }
    
    inline auto M6502::busRead(uint16_t addr) -> uint8_t {

        ctx->dataBus = ctx->read(addr);
        
        return ctx->dataBus;
    }

    inline auto M6502::busWrite(uint16_t addr, uint8_t data) -> void {

        ctx->dataBus = data;
        
        ctx->write(addr, data);
    }

    inline auto M6502::busWatch() -> uint8_t {

        return ctx->readSelect();
    }

    auto M6502::hintUnblockedExecution() -> void {

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
        ctx->rdyLine = false;
        setFlags(0x02);

        reset();
    }

    auto M6502::reset() -> void {
        // a reset in 'rdy halted state' fires as soon as rdy goes hi again
        restoreContext();

        ctx->resetCompleted = false;
        ctx->irqLine = ctx->nmiLine =
        ctx->irqPending = ctx->nmiPending =
        ctx->nmiDetect = ctx->interruptSampled = false;
        ctx->soLine = ctx->soDetect = ctx->soSampled = false;
        ctx->rdyLastCycle = false;
        ctx->writeCycle = false;

        ctx->killed = false;
        ctx->xaa = false;
        ctx->cli = false;
        ctx->sei = false;
        ctx->storeFlags = false;
        ctx->soBlock = 0;
        dontBlockExecution = false;
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
        workCtx->rdyLine = state;
    }

    auto M6502::dataBus() -> uint8_t {
        // last used value on bus
        return ctx->dataBus;
    }

    auto M6502::addressBus() -> uint16_t {
        // last puted address on bus
        return ctx->addrBus;
    }
    
    auto M6502::isWriteCycle() -> bool {
        return ctx->writeCycle;
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

    auto M6502::setPCL(uint8_t data) -> void {

        PC = (PC & ~0xff) | data;
    }

    auto M6502::setPCH(uint8_t data) -> void {

        PC = (PC & 0xff) | (data << 8);
    }

    auto M6502::process() -> void {

        dontBlockExecution = false;

        if (!workCtx->resetCompleted) {
            // resume interrupted reset by rdy
            return resetRoutine();
        }

        /**
         * can recover from reset only, interrupt detection isn't working anymore
         */
        if (workCtx->killed) {
            read<1>(0xffff);
            return;
        }

        if (workCtx->interruptSampled)
            if (!workCtx->useDummy || (workCtx->resumeCycle & 0x80) )
                return interrupt();

        /**
         * basically next instruction fetch is part of currently processed opcode
         * hence some opcodes process final logic during instruction fetch
         * for reduced code complexity it's emulated here without loss of accuracy
         */
        ctx->IR = std::move( readPCInc<0>() );
        
        /**
         * next cpu half cycle is decoding
         */

        decode(workCtx->IR);
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

