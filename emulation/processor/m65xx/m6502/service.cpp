
#include "m6502.h"

namespace MOS65FAMILY {
    

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
        
        if (workCtx->useDummy && !software)
            // remember if dummy was swapped in by nmi/irq and not an opcode
            workCtx->resumeCycle |= 0x80;
    }

    
}

