    
#include "m6502.h"

namespace MOS65FAMILY {

inline auto M6502::handleSo() -> void {
    // a sampled external SO is executed in first half cycle. when cpu accesses v flag
    // internally in second half cycle, the external change in first half cycle is wasted.
    // instructions like adc, sbc don't calculate in last opcode cycle. there is 
    // simply no time because the data fetch for calculation happens in last half cycle.
    // the calculation is done in first cycle of next instruction during the opcode fetch.
    // for simplicity in emulation the calculation is done in context of last instruction cycle.
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
            ctx->data2 |= 1 << 6;
    }

    // is detected in any first half cycle at ~400 ns    
    ctx->soSampled = !ctx->soDetect && ctx->soLine;

    ctx->soDetect = ctx->soLine;
}

}
