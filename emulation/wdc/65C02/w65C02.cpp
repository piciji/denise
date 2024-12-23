
#include "w65C02.h"

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

#ifdef SUPPORT_SOB
#define CHECK_SOB       { if (lines & (SOB_TRANSITION | SOB_BLOCK1 | SOB_BLOCK2)) checkForSOB(); }
#else
#define CHECK_SOB
#endif

#define ___(id, ident, ...)         case id: op##ident(__VA_ARGS__); break;
#define __M(id, ident, inst)        case id: op##ident<inst>(); break;
#define i_M(id, ident, inst, index) case id: op##ident<inst, index>(); break;

#include "instructions.cpp"
#include "alu.cpp"

namespace WDCFAMILY {

template<bool hardware> auto W65C02::interrupt(const uint16_t& vector) -> void {
    if constexpr(hardware) {
        idle();
        idle();
    } else
        readPC();

    push(pc >> 8);
    push(pc & 0xff);

    p.b = !hardware;
    push<WRITE_LATE_P_REG>( p | 0x20);
    p.i = true;
    p.d = false;
    uint16_t newPC = read(vector);
    newPC |= read<SAMPLE_INTR>(vector + 1) << 8;
    pc = newPC;
}

auto W65C02::power() -> void {
    pc = 0;
    a = 0;
    x = 0;
    y = 0;
    s = 0x00;
    p = 0x34;
    lines = 0;
    control = RESET;
}

auto W65C02::setNmiLineLow(bool state) -> void {
    if (state) {
        if ((lines & NMI_LINE) == 0)
            lines |= NMI_TRANSITION;
        lines |= NMI_LINE;
    } else
        lines &= ~NMI_LINE;
}

auto W65C02::setIrqLineLow(bool state) -> void {
    if (state)  lines |= IRQ_LINE;
    else        lines &= ~IRQ_LINE;
}

auto W65C02::setRdyLineLow(bool state) -> void {
    if (state)  lines |= RDY_LINE;
    else {
        lines &= ~RDY_LINE;
        control &= ~WAI;
    }
}

auto W65C02::setSobLineLow(bool state) -> void {
    if (state) {
        if ((lines & SOB_LINE) == 0)
            lines |= SOB_TRANSITION;

        lines |= SOB_LINE;
    } else
        lines &= ~SOB_LINE;
}

auto W65C02::checkForInterrupt() -> void {
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

auto W65C02::checkForSOB() -> void {
    if (lines & SOB_BLOCK2) {
        lines &= ~(SOB_BLOCK2 | SOB_TRANSITION); // prevent pending transition
        lines |= SOB_BLOCK1;
    } else if (lines & SOB_BLOCK1) {
        lines &= ~(SOB_BLOCK1 | SOB_TRANSITION); // prevent pending transition
    } else /* if (lines & SOB_TRANSITION) */ {
        lines &= ~SOB_TRANSITION;
        p.v = true;
    }
}

template<uint8_t actions> inline auto W65C02::read(uint16_t addr) -> uint8_t {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR
    CHECK_SOB
    SYNC();

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        if constexpr (actions & SET_FLAG_I)     p.i = true;
        if constexpr (actions & CLEAR_FLAG_I)   p.i = false;

        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
        CHECK_SOB
        SYNC(); // process other BUS participants and hope someone clears RDY
    }
#endif

    return READ_BYTE(addr);
}

template<uint8_t actions> inline auto W65C02::write(uint16_t addr, uint8_t value) -> void {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR
    CHECK_SOB
    SYNC();

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) { // note: NMOS 6502 ignores RDY during writes, CMOS does not
        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
        CHECK_SOB
        SYNC(); // process other BUS participants and hope someone clears RDY
    }

    if constexpr (actions & WRITE_LATE_P_REG)  WRITE_BYTE(addr, p | 0x10 | 0x20); // reflect a possible SOB transition while RDY
    else
#endif
        WRITE_BYTE(addr, value);
}

template<uint8_t actions> inline auto W65C02::idle() -> void {
    read<actions>(pc);
}

template<uint8_t actions> inline auto W65C02::readPC() -> uint8_t {
    return read<actions>( pc++);
}

template<uint8_t actions> auto W65C02::push(uint8_t data) -> void {
    write<actions>(0x100 | s--, data);
}

template<uint8_t actions> auto W65C02::pull() -> uint8_t {
    return read<actions>(0x100 | ++s);
}

auto W65C02::process()->void {
    if (control) {
        if (control & WAI) {
            // WAI sets RDY (bidirectional) low and repeats the same cycle. It's same behavior like external RDY change.
            // Since "WAI" can last a very long time, it is covered here to keep the emulation responsive.
            // Otherwise, the UI may not be refreshed in time. Furthermore, no "RDY" check is required in each cycle.
            // This requires additional power and can be switched off if no external "RDY" change is planned.
            // IRQ/NMI set RDY hi again and resume processing but only if RDY is not forced low from external.
            CHECK_INTR
            CHECK_SOB
            return SYNC();
        }

        if (control & NMI_PENDING) {
            control &= ~NMI_PENDING;
            return interrupt( 0xfffa );
        }

        if (control & IRQ_PENDING) {
            control &= ~IRQ_PENDING;
            return interrupt( 0xfffe );
        }

        // check STP and RESET last for performance reasons
        if (control & STP) {
            return idle();
        }

        if (control & RESET) {
            control &= ~RESET;
            return interrupt(0xfffc);
        }
    }

    switch(readPC()) {
        ___(0x00, BRK)
        __M(0x01, ZeroPageIndexedIndirect, ORA)
        ___(0x02, Invalid)
        __M(0x04, ModifyZeroPage, TSB)
        __M(0x05, ZeroPage, ORA)
        __M(0x06, ModifyZeroPage, ASL)
        __M(0x07, ModifyZeroPage, RMB0)
        ___(0x08, PHP)
        __M(0x09, Immediate, ORA)
        __M(0x0a, Implied, ASL)
        __M(0x0c, ModifyAbsolute, TSB)
        __M(0x0d, Absolute, ORA)
        __M(0x0e, ModifyAbsolute, ASL)
        __M(0x0f, BB, BBR0)
        ___(0x10, Branch, !p.n)
        i_M(0x11, ZeroPageIndirect, ORA, INDEX_Y)
        __M(0x12, ZeroPageIndirect, ORA)
        __M(0x14, ModifyZeroPage, TRB)
        i_M(0x15, ZeroPage, ORA, INDEX_X)
        i_M(0x16, ModifyZeroPage, ASL, INDEX_X)
        __M(0x17, ModifyZeroPage, RMB1)
        ___(0x18, ClearC)
        i_M(0x19, Absolute, ORA, INDEX_Y)
        __M(0x1a, Implied, INC)
        __M(0x1c, ModifyAbsolute, TRB)
        i_M(0x1d, Absolute, ORA, INDEX_X)
        i_M(0x1e, ModifyAbsolute, ASL, INDEX_X)
        __M(0x1f, BB, BBR1)
        ___(0x20, JSR)
        __M(0x21, ZeroPageIndexedIndirect, AND)
        ___(0x22, Invalid)
        __M(0x24, ZeroPage, BIT)
        __M(0x25, ZeroPage, AND)
        __M(0x26, ModifyZeroPage, ROL)
        __M(0x27, ModifyZeroPage, RMB2)
        ___(0x28, PLP)
        __M(0x29, Immediate, AND)
        __M(0x2a, Implied, ROL)
        __M(0x2c, Absolute, BIT)
        __M(0x2d, Absolute, AND)
        __M(0x2e, ModifyAbsolute, ROL)
        __M(0x2f, BB, BBR2)
        ___(0x30, Branch, p.n)
        i_M(0x31, ZeroPageIndirect, AND, INDEX_Y)
        __M(0x32, ZeroPageIndirect, AND)
        i_M(0x34, ZeroPage, BIT, INDEX_X)
        i_M(0x35, ZeroPage, AND, INDEX_X)
        i_M(0x36, ModifyZeroPage, ROL, INDEX_X)
        __M(0x37, ModifyZeroPage, RMB3)
        ___(0x38, SetC)
        i_M(0x39, Absolute, AND, INDEX_Y)
        __M(0x3a, Implied, DEC)
        i_M(0x3c, Absolute, BIT, INDEX_X)
        i_M(0x3d, Absolute, AND, INDEX_X)
        i_M(0x3e, ModifyAbsolute, ROL, INDEX_X)
        __M(0x3f, BB, BBR3)
        ___(0x40, RTI)
        __M(0x41, ZeroPageIndexedIndirect, EOR)
        ___(0x42, Invalid)
        __M(0x44, ZeroPage, LDD)
        __M(0x45, ZeroPage, EOR)
        __M(0x46, ModifyZeroPage, LSR)
        __M(0x47, ModifyZeroPage, RMB4)
        __M(0x48, Push, STA)
        __M(0x49, Immediate, EOR)
        __M(0x4a, Implied, LSR)
        ___(0x4c, JmpAbsolute)
        __M(0x4d, Absolute, EOR)
        __M(0x4e, ModifyAbsolute, LSR)
        __M(0x4f, BB, BBR4)
        ___(0x50, Branch, !p.v)
        i_M(0x51, ZeroPageIndirect, EOR, INDEX_Y)
        __M(0x52, ZeroPageIndirect, EOR)
        i_M(0x54, ZeroPage, LDD, INDEX_X)
        i_M(0x55, ZeroPage, EOR, INDEX_X)
        i_M(0x56, ModifyZeroPage, LSR, INDEX_X)
        __M(0x57, ModifyZeroPage, RMB5)
        ___(0x58, UpdateI<false>)
        i_M(0x59, Absolute, EOR, INDEX_Y)
        __M(0x5a, Push, STY)
        ___(0x5c, NoOp5c)
        i_M(0x5d, Absolute, EOR, INDEX_X)
        i_M(0x5e, ModifyAbsolute, LSR, INDEX_X)
        __M(0x5f, BB, BBR5)
        ___(0x60, RTS)
        __M(0x61, ZeroPageIndexedIndirect, ADC)
        ___(0x62, Invalid)
        __M(0x64, ZeroPage, STZ)
        __M(0x65, ZeroPage, ADC)
        __M(0x66, ModifyZeroPage, ROR)
        __M(0x67, ModifyZeroPage, RMB6)
        __M(0x68, Pull, STA)
        __M(0x69, Immediate, ADC)
        __M(0x6a, Implied, ROR)
        ___(0x6c, JmpIndirect)
        __M(0x6d, Absolute, ADC)
        __M(0x6e, ModifyAbsolute, ROR)
        __M(0x6f, BB, BBR6)
        ___(0x70, Branch, p.v)
        i_M(0x71, ZeroPageIndirect, ADC, INDEX_Y)
        __M(0x72, ZeroPageIndirect, ADC)
        i_M(0x74, ZeroPage, STZ, INDEX_X)
        i_M(0x75, ZeroPage, ADC, INDEX_X)
        i_M(0x76, ModifyZeroPage, ROR, INDEX_X)
        __M(0x77, ModifyZeroPage, RMB7)
        ___(0x78, UpdateI<true>)
        i_M(0x79, Absolute, ADC, INDEX_Y)
        __M(0x7a, Pull, STY)
        ___(0x7c, JmpAbsIndexedIndirect)
        i_M(0x7d, Absolute, ADC, INDEX_X)
        i_M(0x7e, ModifyAbsolute, ROR, INDEX_X)
        __M(0x7f, BB, BBR7)
        ___(0x80, Branch, true)
        __M(0x81, ZeroPageIndexedIndirect, STA)
        ___(0x82, Invalid)
        __M(0x84, ZeroPage, STY)
        __M(0x85, ZeroPage, STA)
        __M(0x86, ZeroPage, STX)
        __M(0x87, ModifyZeroPage, SMB0)
        i_M(0x88, Implied, DEC, INDEX_Y)
        __M(0x89, Immediate, BIT_IM)
        ___(0x8a, TXA)
        __M(0x8c, Absolute, STY)
        __M(0x8d, Absolute, STA)
        __M(0x8e, Absolute, STX)
        __M(0x8f, BB, BBS0)
        ___(0x90, Branch, !p.c)
        i_M(0x91, ZeroPageIndirect, STA, INDEX_Y)
        __M(0x92, ZeroPageIndirect, STA)
        i_M(0x94, ZeroPage, STY, INDEX_X)
        i_M(0x95, ZeroPage, STA, INDEX_X)
        i_M(0x96, ZeroPage, STX, INDEX_Y)
        __M(0x97, ModifyZeroPage, SMB1)
        ___(0x98, TYA)
        i_M(0x99, Absolute, STA, INDEX_Y)
        ___(0x9a, TXS)
        __M(0x9c, Absolute, STZ)
        i_M(0x9d, Absolute, STA, INDEX_X)
        i_M(0x9e, Absolute, STZ, INDEX_X)
        __M(0x9f, BB, BBS1)
        __M(0xa0, Immediate, LDY)
        __M(0xa1, ZeroPageIndexedIndirect, LDA)
        __M(0xa2, Immediate, LDX)
        __M(0xa4, ZeroPage, LDY)
        __M(0xa5, ZeroPage, LDA)
        __M(0xa6, ZeroPage, LDX)
        __M(0xa7, ModifyZeroPage, SMB2)
        ___(0xa8, TAY)
        __M(0xa9, Immediate, LDA)
        ___(0xaa, TAX)
        __M(0xac, Absolute, LDY)
        __M(0xad, Absolute, LDA)
        __M(0xae, Absolute, LDX)
        __M(0xaf, BB, BBS2)
        ___(0xb0, Branch, p.c)
        i_M(0xb1, ZeroPageIndirect, LDA, INDEX_Y)
        __M(0xb2, ZeroPageIndirect, LDA)
        i_M(0xb4, ZeroPage, LDY, INDEX_X)
        i_M(0xb5, ZeroPage, LDA, INDEX_X)
        i_M(0xb6, ZeroPage, LDX, INDEX_Y)
        __M(0xb7, ModifyZeroPage, SMB3)
        ___(0xb8, ClearV)
        i_M(0xb9, Absolute, LDA, INDEX_Y)
        ___(0xba, TSX)
        i_M(0xbc, Absolute, LDY, INDEX_X)
        i_M(0xbd, Absolute, LDA, INDEX_X)
        i_M(0xbe, Absolute, LDX, INDEX_Y)
        __M(0xbf, BB, BBS3)
        __M(0xc0, Immediate, CPY)
        __M(0xc1, ZeroPageIndexedIndirect, CMP)
        ___(0xc2, Invalid)
        __M(0xc4, ZeroPage, CPY)
        __M(0xc5, ZeroPage, CMP)
        __M(0xc6, ModifyZeroPage, DEC)
        __M(0xc7, ModifyZeroPage, SMB4)
        i_M(0xc8, Implied, INC, INDEX_Y)
        __M(0xc9, Immediate, CMP)
        i_M(0xca, Implied, DEC, INDEX_X)
        ___(0xcb, Wait)
        __M(0xcc, Absolute, CPY)
        __M(0xcd, Absolute, CMP)
        __M(0xce, ModifyAbsolute, DEC)
        __M(0xcf, BB, BBS4)
        ___(0xd0, Branch, !p.z)
        i_M(0xd1, ZeroPageIndirect, CMP, INDEX_Y)
        __M(0xd2, ZeroPageIndirect, CMP)
        i_M(0xd4, ZeroPage, LDD, INDEX_X)
        i_M(0xd5, ZeroPage, CMP, INDEX_X)
        i_M(0xd6, ModifyZeroPage, DEC, INDEX_X)
        __M(0xd7, ModifyZeroPage, SMB5)
        ___(0xd8, ClearD)
        i_M(0xd9, Absolute, CMP, INDEX_X)
        __M(0xda, Push, STX)
        ___(0xdb, Stop)
        __M(0xdc, Absolute, LDD)
        i_M(0xdd, Absolute, CMP, INDEX_X)
        i_M(0xde, ModifyAbsolute, DEC, INDEX_X)
        __M(0xdf, BB, BBS5)
        __M(0xe0, Immediate, CPX)
        __M(0xe1, ZeroPageIndexedIndirect, SBC)
        ___(0xe2, Invalid)
        __M(0xe4, ZeroPage, CPX)
        __M(0xe5, ZeroPage, SBC)
        __M(0xe6, ModifyZeroPage, INC)
        __M(0xe7, ModifyZeroPage, SMB6)
        i_M(0xe8, Implied, INC, INDEX_X)
        __M(0xe9, Immediate, SBC)
        ___(0xea, NOP)
        __M(0xec, Absolute, CPX)
        __M(0xed, Absolute, SBC)
        __M(0xee, ModifyAbsolute, INC)
        __M(0xef, BB, BBS6)
        ___(0xf0, Branch, p.z)
        i_M(0xf1, ZeroPageIndirect, SBC, INDEX_Y)
        __M(0xf2, ZeroPageIndirect, SBC)
        i_M(0xf4, ZeroPage, LDD, INDEX_X)
        i_M(0xf5, ZeroPage, SBC, INDEX_X)
        i_M(0xf6, ModifyZeroPage, INC, INDEX_X)
        __M(0xf7, ModifyZeroPage, SMB7)
        ___(0xf8, SetD)
        i_M(0xf9, Absolute, SBC, INDEX_Y)
        __M(0xfa, Pull, STX)
        __M(0xfc, Absolute, LDD)
        i_M(0xfd, Absolute, SBC, INDEX_X)
        i_M(0xfe, ModifyAbsolute, INC, INDEX_X)
        __M(0xff, BB, BBS7)

        default:  // 30 opcodes: 0x*3   0x*b (except: 0xcb, 0xdb)
            CHECK_INTR
            break;
    }
}

}
