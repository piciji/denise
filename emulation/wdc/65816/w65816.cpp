
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

// #
#define A_IMMEDIATE                         case 0x09: case 0x29: case 0x49: case 0x69:
// A
#define A_ACCUMULATOR                       case 0x0a: case 0x1a: case 0x2a: case 0x3a: case 0x4a: case 0x6a:
// r
#define A_PC_RELATIVE                       case 0x10: case 0x30: case 0x50: case 0x70:
// rl
#define A_PC_RELATIVE_LONG
// I
#define A_IMPLIED                           case 0x18: case 0x1b: case 0x38: case 0x3b: case 0x42: case 0x58: case 0x5b: case 0x78: \
                                            case 0x7b:
// s
#define A_STACK                             case 0x00: case 0x02: case 0x08: case 0x0b: case 0x28: case 0x2b: case 0x40: case 0x48: \
                                            case 0x4b: case 0x5a: case 0x60: case 0x62: case 0x68: case 0x6b: case 0x7a:
// d
#define A_DIRECT                            case 0x04: case 0x05: case 0x06: case 0x14: case 0x24: case 0x25: case 0x26: case 0x45: \
                                            case 0x46: case 0x64: case 0x65: case 0x66:
// d,x
#define A_DIRECT_INDEXED_WITH_X             case 0x15: case 0x16: case 0x34: case 0x35: case 0x36: case 0x55: case 0x56: case 0x74: \
                                            case 0x75: case 0x76:
// d,y
#define A_DIRECT_INDEXED_WITH_Y
// (d)
#define A_DIRECT_INDIRECT                   case 0x12: case 0x32: case 0x52: case 0x72:
// (d,x)
#define A_INDEXED_INDIRECT                  case 0x01: case 0x21: case 0x41: case 0x61:
// (d),y
#define A_INDIRECT_INDEXED                  case 0x11: case 0x31: case 0x51: case 0x71:
// [d]
#define A_DIRECT_INDIRECT_LONG              case 0x07: case 0x27: case 0x47: case 0x67:
// [d],y
#define A_DIRECT_INDIRECT_LONG_INDEXED      case 0x17: case 0x37: case 0x57: case 0x77:
// a
#define A_ABSOLUTE                          case 0x0c: case 0x0d: case 0x0e: case 0x1c: case 0x20: case 0x2c: case 0x2d: case 0x2e: \
                                            case 0x4c: case 0x4d: case 0x4e: case 0x6d: case 0x6e:
// a,x
#define A_ABSOLUTE_INDEXED_WITH_X           case 0x1d: case 0x1e: case 0x3c: case 0x3d: case 0x3e: case 0x5d: case 0x5e: case 0x7d: \
                                            case 0x7e:
// a,y
#define A_ABSOLUTE_INDEXED_WITH_Y           case 0x19: case 0x39: case 0x59: case 0x79:
// al
#define A_ABSOLUTE_LONG                     case 0x0f: case 0x22: case 0x2f: case 0x4f: case 0x5c: case 0x6f:
// al,x
#define A_ABSOLUTE_LONG_INDEXED             case 0x1f: case 0x3f: case 0x5f: case 0x7f:
// d,s
#define A_STACK_RELATIVE                    case 0x03: case 0x23: case 0x43: case 0x63:
// (d,s),y
#define A_STACK_RELATIVE_INDIRECT_INDEXED   case 0x13: case 0x33: case 0x53: case 0x73:
// (a)
#define A_ABSOLUTE_INDIRECT                 case 0x6c:
// (a,x)
#define A_ABSOLUTE_INDEXED_INDIRECT         case 0x7c:
// xyz
#define A_BLOCK_MOVE                        case 0x44: case 0x54:





}
