
#include "w65816.h"

#ifdef W65816_REF
    #ifdef W65816_REF_INCLUDE
        #include W65816_REF_INCLUDE
    #endif
    #define REF_CALL ref.
#else
    #define REF_CALL
#endif

#define READ_BYTE           REF_CALL readByte
#define READ_VECTOR_BYTE    REF_CALL readVectorByte
#define WRITE_BYTE          REF_CALL writeByte
#define IDLE_CYCLE          REF_CALL idleCycle
#define OUTPUT_RDY_LOW      REF_CALL outputRDYLineLow
#define SET_MEMORY_LOCK     REF_CALL setMemoryLock
#define TRAP_HANDLER        REF_CALL trapHandler

#define CHECK_INTR     { if(lines & (NMI_TRANSITION | IRQ_LINE)) checkForInterrupt(); }

#define PAGE_CROSSED(a1, a2) (((a1) ^ (a2)) & 0xff00)

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
            return IDLE_CYCLE((pbr << 16) | pc);
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
            return IDLE_CYCLE((pbr << 16) | pc);
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
        readPCNoInc();
        readPCIdle();
    } else
        readPC();

    if(!modeE)
        push(pbr);

    push(pc >> 8);
    push(pc & 0xff);
    (hardware && modeE) ? push(p & ~0x10) : push(p);
    p.i = true;
    p.d = false;
    uint16_t newPC = read<VECTOR>(vector);
    newPC |= read<SAMPLE_INTR | VECTOR>(vector + 1) << 8;
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
    if (state) lines |= RDY_LINE;
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
        readPCIdle();
}

inline auto W65816::idle4(const uint16_t a1, const uint16_t a2) -> void {
    if(!p.x || PAGE_CROSSED(a1, a2))
        readBankIdle((a1 & 0xff00) | (a2 & 0xff));
}

inline auto W65816::idleIrq() -> void {
    if (control & (IRQ_PENDING | NMI_PENDING))
        readPCNoInc<SAMPLE_INTR>();
    else
        readPCIdle<SAMPLE_INTR>();
}

template<uint8_t actions> inline auto W65816::idle(uint32_t addr) -> void {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        IDLE_CYCLE(addr);
        if constexpr (actions & SET_FLAG_I)     p.i = true;
        if constexpr (actions & CLEAR_FLAG_I)   p.i = false;

        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
    }
#endif

    IDLE_CYCLE(addr);
}

template<uint8_t actions> inline auto W65816::read(uint32_t addr) -> uint8_t {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        IDLE_CYCLE(addr);
        if constexpr (actions & SET_FLAG_I)     p.i = true;
        if constexpr (actions & CLEAR_FLAG_I)   p.i = false;

        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
    }
#endif

#ifdef SEPARATE_VECTOR_READ
    if constexpr (!!(actions & VECTOR))
        return READ_VECTOR_BYTE((uint16_t)addr);
#endif
    return READ_BYTE(addr);
}

template<uint8_t actions> inline auto W65816::write(uint32_t addr, uint8_t value) -> void {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        IDLE_CYCLE(addr);
        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
    }
#endif

    WRITE_BYTE(addr, value);
}

template<uint8_t actions> inline auto W65816::readBank(uint32_t addr) -> uint8_t {
    return read<actions>( ((dbr << 16) + addr) & 0xffffff );
}

template<uint8_t actions> inline auto W65816::readBankIdle(uint32_t addr) -> void {
    idle<actions>( ((dbr << 16) + addr) & 0xffffff );
}

template<uint8_t actions> inline auto W65816::readPC() -> uint8_t {
    return read<actions>((pbr << 16) | pc++);
}

template<uint8_t actions> inline auto W65816::readPCNoInc() -> uint8_t {
    return read<actions>((pbr << 16) | pc);
}

template<uint8_t actions> inline auto W65816::readPCIdle() -> void {
    idle<actions>((pbr << 16) | pc);
}

template<uint8_t actions> inline auto W65816::readStack(uint32_t addr) -> uint8_t {
    return read<actions>((s + addr) & 0xffff );
}

template<uint8_t actions> inline auto W65816::writeBank(uint32_t addr, uint8_t data) -> void {
    write<actions>( ((dbr << 16) + addr) & 0xffffff, data );
}

template<uint8_t actions> inline auto W65816::writeStack(uint32_t addr, uint8_t data) -> void {
    write<actions>((s + addr) & 0xffff, data );
}

template<uint8_t actions> auto W65816::push(uint8_t data) -> void {
    write<actions>(s, data);
    if constexpr (!!(actions & NATIVE)) s--;
    else { modeE ? decByteL(s) : (void)s--; }
}

template<uint8_t actions> auto W65816::pull() -> uint8_t {
    if constexpr (!!(actions & NATIVE)) s++;
    else { modeE ? incByteL(s) : (void)s++; }
    return read<actions>(s);
}

inline auto W65816::directAdr(uint32_t addr) -> uint32_t {
    if(modeE && ((d & 0xff) == 0) )
        return (d & 0xff00) | (addr & 0xff);

    return (d + addr) & 0xffff;
}

auto W65816::getDirectAddressIndirect(uint32_t offset) -> uint16_t {
    uint8_t lsb = read( directAdr(offset) );

    if(!modeE || ((d & 0xff) == 0))
        return (read( directAdr(offset + 1) ) << 8) | lsb;

    uint16_t addr = directAdr(offset + 1);

    if((addr & 0xff) == 0) // if +1 wraps page -> undo
        return (read((uint16_t)(addr - 0x100)) << 8) | lsb;

    return (read(addr) << 8) | lsb;
}

inline auto W65816::decByteL(uint16_t& reg) -> void {
    uint8_t byte = reg & 0xff;
    byte--;
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::incByteL(uint16_t& reg) -> void {
    uint8_t byte = reg & 0xff;
    byte++;
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::setByteL(uint16_t& reg, uint8_t byte) -> void {
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::setByteH(uint16_t& reg, const uint8_t& byte) -> void {
    reg = (reg & 0x00ff) | (byte << 8);
}

// s
#define A_SIG               case 0x00: case 0x02:
// d,s
#define A_STACK_RELATIVE    case 0x03:

// [d]
#define A_DIR_INDIRECT_LONG case 0x07:

#define A_IMPLIED                      case 0x08: case 0x0a: case 0x0b: case 0x12: case 0x18: case 0x1a: case 0x22: case 0x28: \
                            case 0x2a: case 0x32: case 0x38: case 0x3a: case 0x40: case 0x42: case 0x48: case 0x4a: case 0x52: \
                            case 0x58: case 0x5a: case 0x60: case 0x62: case 0x68: case 0x6a: case 0x72: case 0x78: case 0x7a: \
                            case 0x88: case 0x8a: case 0x92: case 0x98: case 0x9a: case 0xa8: case 0xaa: case 0xb2: case 0xb8: \
                            case 0xba: case 0xc8: case 0xca: case 0xd2: case 0xd8: case 0xda: case 0xe8: case 0xea: case 0xf2: \
                            case 0xf8: case 0xfa:
// (d,x)
#define A_INDEXED_INDIRECT  case 0x01: case 0x21: case 0x23: case 0x41: case 0x43: case 0x61: case 0x63: case 0x81: \
                            case 0x83: case 0xa1: case 0xa3: case 0xc1: case 0xc3: case 0xe1: case 0xe3:
// d
#define A_ZERO_PAGE         case 0x04: case 0x05: case 0x06: case 0x24: case 0x25: case 0x26: case 0x27: case 0x44: \
                            case 0x45: case 0x46: case 0x47: case 0x64: case 0x65: case 0x66: case 0x67: case 0x84: case 0x85: \
                            case 0x86: case 0x87: case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xc4: case 0xc5: case 0xc6: \
                            case 0xc7: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
// #
#define A_IMMEDIATE         case 0x09: case 0x29: case 0x2b: case 0x49: case 0x4b: case 0x69: case 0x6b: case 0x80: \
                            case 0x82: case 0x89: case 0x8b: case 0xa0: case 0xa2: case 0xa9: case 0xab: case 0xc0: case 0xc2: \
                            case 0xc9: case 0xcb: case 0xe0: case 0xe2: case 0xe9: case 0xeb:
// a
#define A_ABSOLUTE          case 0x0c: case 0x0d: case 0x0e: case 0x20: case 0x2c: case 0x2d: case 0x2e: case 0x2f: \
                            case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x6d: case 0x6e: case 0x6f: case 0x8c: case 0x8d: \
                            case 0x8e: case 0x8f: case 0xac: case 0xad: case 0xae: case 0xaf: case 0xcc: case 0xcd: case 0xce: \
                            case 0xcf: case 0xec: case 0xed: case 0xee: case 0xef:

// al
#define A_ABSOLUTE_LONG     case 0x0f:
// r
#define A_RELATIVE          case 0x10: case 0x30: case 0x50: case 0x70: case 0x90: case 0xb0: case 0xd0: case 0xf0:
#define A_INDIRECT_INDEXED  case 0x11: case 0x13: case 0x31: case 0x33: case 0x51: case 0x53: case 0x71: case 0x73: case 0x91: \
                            case 0x93: case 0xb1: case 0xb3: case 0xd1: case 0xd3: case 0xf1: case 0xf3:
#define A_ZERO_INDEXED_X    case 0x14: case 0x15: case 0x16: case 0x17: case 0x34: case 0x35: case 0x36: case 0x37: case 0x54: \
                            case 0x55: case 0x56: case 0x57: case 0x74: case 0x75: case 0x76: case 0x77: case 0x94: case 0x95: \
                            case 0xb4: case 0xb5: case 0xd4: case 0xd5: case 0xd6: case 0xd7: case 0xf4: case 0xf5: case 0xf6: \
                            case 0xf7:
#define A_ABS_INDEXED_Y     case 0x19: case 0x1b: case 0x39: case 0x3b: case 0x59: case 0x5b: case 0x79: case 0x7b: case 0x99: \
                            case 0x9b: case 0x9e: case 0x9f: case 0xb9: case 0xbb: case 0xbe: case 0xbf: case 0xd9: case 0xdb: \
                            case 0xf9: case 0xfb:
#define A_ABS_INDEXED_X     case 0x1c: case 0x1d: case 0x1e: case 0x1f: case 0x3c: case 0x3d: case 0x3e: case 0x3f: case 0x5c: \
                            case 0x5d: case 0x5e: case 0x5f: case 0x7c: case 0x7d: case 0x7e: case 0x7f: case 0x9c: case 0x9d: \
                            case 0xbc: case 0xbd: case 0xdc: case 0xdd: case 0xde: case 0xdf: case 0xfc: case 0xfd: case 0xfe: \
                            case 0xff:
#define A_INDIRECT          case 0x6c:
#define A_ZERO_INDEXED_Y    case 0x96: case 0x97: case 0xb6: case 0xb7:

}
