
#include "w65816.h"

#ifdef REF
    #ifdef REF_INCLUDE
        #include REF_INCLUDE
    #endif
    #define REF_CALL ref.
#else
    #define REF_CALL
#endif

#define READ_BYTE       REF_CALL readByte
#define WRITE_BYTE      REF_CALL writeByte
#define IDLE            REF_CALL sync

#define SAMPLE_INTR     { if(intrLine & (NMI_TRANSITION | IRQ_LINE)) checkForInterrupt(); }

#include "memory.cpp"
#include "instructions.cpp"
#include "alu.cpp"

namespace WDCFAMILY {

auto W65816::process()->void {
    if (control) {
        if (control & WAI) {
            SAMPLE_INTR
            if ((control & WAI) == 0) IDLE();
            return IDLE();
        }

        if (control & NMI_PENDING) {
            control &= ~NMI_PENDING;
            return interrupt( modeE ? 0xfffa : 0xffea );
        }

        if (control & IRQ_PENDING) {
            control &= ~IRQ_PENDING;
            return interrupt( modeE ? 0xfffe : 0xffee );
        }

        // check STP and RESET last for performance reasons
        if (control & STP) {
            return IDLE();
        }

        if (control & RESET) {
            control &= ~RESET;
            IDLE(132);
            return interrupt( 0xfffc );
        }
    }

    switch(readPC()) {
        #include "optable.h"
    }
}

template<bool hardware> auto W65816::interrupt(const uint16_t& vector) -> void {
    if constexpr(hardware) {
        read( (pbr << 8) | pc );
        IDLE();
    } else
        readPC();

    if(!modeE)
        push(pbr);

    push(pc >> 8);
    push(pc & 0xff);
    (hardware && modeE) ? push(p & ~0x10) : push(p);
    p.i = true;
    p.d = false;
    uint16_t newPC = read(vector);
    SAMPLE_INTR
    newPC |= read(vector + 1) << 8;
    pc = newPC;
    pbr = 0;
}

auto W65816::power() -> void {
    modeE = true;
    pc = 0;
    pbr = 0;
    dbr = 0;
    a = 0;
    x = 0;
    y = 0;
    s = 0x01ff;
    d = 0;
    p = 0x34;
    intrLine = 0;
    control = RESET;
}

auto W65816::setNmiLineLow(bool state) -> void {
    if (state) {
        if ((intrLine & NMI_LINE) == 0)
            intrLine |= NMI_TRANSITION;
        intrLine |= NMI_LINE;
    } else
        intrLine &= ~NMI_LINE;
}

auto W65816::setIrqLineLow(bool state) -> void {
    if (state)
        intrLine |= IRQ_LINE;
    else
        intrLine &= ~IRQ_LINE;
}

auto W65816::checkForInterrupt() -> void {
    if (intrLine & NMI_TRANSITION) {
        intrLine &= ~NMI_TRANSITION;
        control &= ~WAI;
        control |= NMI_PENDING;
    }

    if (intrLine & IRQ_LINE) {
        // will re-trigger if an external device doesn't change line before the next interrupt check
        if (!p.i)
            control |= IRQ_PENDING;
        control &= ~WAI;
    }
}

inline auto W65816::idle2() -> void {
    if(d & 0xff)
        IDLE();
}

#define PAGE_CROSSED(a1, a2) (((a1) ^ (a2)) & 0xff00)

inline auto W65816::idle4(const uint16_t a1, const uint16_t a2) -> void {
    if(!p.x || PAGE_CROSSED(a1, a2))
        IDLE();
}

inline auto W65816::idle6(uint16_t address) -> void {
    if(modeE && PAGE_CROSSED(pc, address))
        IDLE();
}

inline auto W65816::idleIrq() -> void {
    if (control & (IRQ_PENDING | NMI_PENDING))
        read((pbr << 16) | pc);
    else
        IDLE();
}

}
