
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
#define SYNC            REF_CALL sync
#define OUTPUT_RDY_LOW  REF_CALL outputRDYLineLow
#define SET_MEMORY_LOCK REF_CALL setMemoryLock

#define CHECK_INTR     { if(lines & (NMI_TRANSITION | IRQ_LINE)) checkForInterrupt(); }

#include "memory.cpp"
#include "instructions.cpp"
#include "alu.cpp"

namespace WDCFAMILY {

auto W65816::process()->void {
    if (control) {
        if (control & WAI) {
            // WAI sets RDY (bidirectional) low and repeats the same cycle. It's same behavior like external RDY change.
            // Since "WAI" can last a very long time, it is covered here to keep the emulation responsive.
            // Otherwise, the UI may not be refreshed in time. Furthermore, no "RDY" check is required in each cycle.
            // This requires additional power and can be switched off if no external "RDY" change is planned.
            // IRQ/NMI set RDY hi again and resume processing but only if RDY is not forced low from external.
            CHECK_INTR
            return SYNC();
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
            return idle();
        }

        if (control & RESET) {
            control &= ~RESET;
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
        idle();
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
    newPC |= read<SAMPLE_INTR>(vector + 1) << 8;
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
    lines = 0;
    control = RESET;
}

auto W65816::setNmiLineLow(bool state) -> void {
    if (state) {
        if ((lines & NMI_LINE) == 0)
            lines |= NMI_TRANSITION;
        lines |= NMI_LINE;
    } else
        lines &= ~NMI_LINE;
}

auto W65816::setIrqLineLow(bool state) -> void {
    if (state)
        lines |= IRQ_LINE;
    else
        lines &= ~IRQ_LINE;
}

auto W65816::setRdyLineLow(bool state) -> void {
    if (state)  lines |= RDY_LINE;
    else {
        lines &= ~RDY_LINE;
        control &= ~WAI;
    }
}

auto W65816::checkForInterrupt() -> void {
    if (lines & NMI_TRANSITION) {
        lines &= ~NMI_TRANSITION;
        control &= ~WAI;
        control |= NMI_PENDING;
    }

    if (lines & IRQ_LINE) {
        // will re-trigger if an external device doesn't change line before the next interrupt check
        if (!p.i)
            control |= IRQ_PENDING;
        control &= ~WAI;
    }
}

inline auto W65816::idle2() -> void {
    if(d & 0xff)
        idle();
}

#define PAGE_CROSSED(a1, a2) (((a1) ^ (a2)) & 0xff00)

inline auto W65816::idle4(const uint16_t a1, const uint16_t a2) -> void {
    if(!p.x || PAGE_CROSSED(a1, a2))
        idle();
}

inline auto W65816::idle6(uint16_t address) -> void {
    if(modeE && PAGE_CROSSED(pc, address))
        idle();
}

inline auto W65816::idleIrq() -> void {
    CHECK_INTR
    if (control & (IRQ_PENDING | NMI_PENDING))
        read<SAMPLE_INTR>((pbr << 16) | pc);
    else
        idle<SAMPLE_INTR>();
}

}
