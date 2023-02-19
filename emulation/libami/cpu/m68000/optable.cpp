
#include "m68000.h"

enum { S6 = 6, S8 = 8, S12 = 12, SO /* second operand */ = 1 << 4, O_16 = 2 << 4, O_256 = 4 << 4 };
enum { DR = 1, AR = 2, AI = 4, AIPI = 8, AIPD = 16, AID = 32, AII = 64, AS = 128, AL = 256, PCD = 512, PCI = 1024, IM = 2048,
        ADR_TYPICAL = AI | AIPI | AIPD | AID | AII | AS | AL, ADR_FULL = ADR_TYPICAL | PCD | PCI | IM };

#define _bindEA(id, F, I, M, S) { opTable[id] = &M68000::op##F<I, M, S>; }
#define _bind(id, F, I, S) { opTable[id] = &M68000::op##F<I, S>; }

#define _M_( op, F, I, M, S ) { \
    for (int j = 0; j < 8; j++) { \
        if (M & DR)     _bindEA((op) | 0 << 3 | j, F, I, DataRegisterDirect, S) \
        if (M & AR)     _bindEA((op) | 1 << 3 | j, F, I, AddressRegisterDirect, S) \
        if (M & AI)     _bindEA((op) | 2 << 3 | j, F, I, AddressRegisterIndirect, S) \
        if (M & AIPI)   _bindEA((op) | 3 << 3 | j, F, I, AddressRegisterIndirectWithPostIncrement, S) \
        if (M & AIPD)   _bindEA((op) | 4 << 3 | j, F, I, AddressRegisterIndirectWithPreDecrement, S) \
        if (M & AID)    _bindEA((op) | 5 << 3 | j, F, I, AddressRegisterIndirectWithDisplacement, S) \
        if (M & AII)    _bindEA((op) | 6 << 3 | j, F, I, AddressRegisterIndirectWithIndex, S) \
    } \
    if (M & AS)         _bindEA((op) | 7 << 3 | 0, F, I, AbsoluteShort, S) \
    if (M & AL)         _bindEA((op) | 7 << 3 | 1, F, I, AbsoluteLong, S) \
    if (M & PCD)        _bindEA((op) | 7 << 3 | 2, F, I, ProgramCounterIndirectWithDisplacement, S) \
    if (M & PCI)        _bindEA((op) | 7 << 3 | 3, F, I, ProgramCounterIndirectWithIndex, S) \
    if (M & IM)         _bindEA((op) | 7 << 3 | 4, F, I, Immediate, S) \
}

#define _EA_( op, flags, F, I, M, S ) { \
    int _b = 0, _w = 0, _l = 0;   \
    int _s = (flags) & 15;          \
    if (_s) _l = _s == 8 ? 1 : 2, _w = _s == 6 ? 1 : (_s == 8 ? 0 : 3), _b = _s == 6 ? 0 : 1;  \
    for (int i = 0; i < (((flags) & SO) ? 8 : 1); i++) {                                  \
        if ((S) & Byte) { _M_( op | i << 9 | _b << _s, F, I, (M), Byte ) } \
        if ((S) & Word) { _M_( op | i << 9 | _w << _s, F, I, (M), Word ) } \
        if ((S) & Long) { _M_( op | i << 9 | _l << _s, F, I, (M), Long ) } \
    } \
}

#define _B_( op, flags, F, I, S ) { \
    int os = ((flags) & O_256) ? 256 : (((flags) & O_16) ? 16 : 8); \
    int _b = 0, _w = 0, _l = 0;   \
    int _s = (flags) & 15;          \
    if (_s) _l = _s == 8 ? 1 : 2, _w = _s == 6 ? 1 : (_s == 8 ? 0 : 3), _b = _s == 6 ? 0 : 1;  \
    for (int i = 0; i < (((flags) & SO) ? 8 : 1); i++) {                                  \
        if ((S) & Byte) { for(uint16_t j = 0; j < os; j++) _bind( op | i << 9 | _b << _s | j, F, I, Byte ) } \
        if ((S) & Word) { for(uint16_t j = 0; j < os; j++) _bind( op | i << 9 | _w << _s | j, F, I, Word ) } \
        if ((S) & Long) { for(uint16_t j = 0; j < os; j++) _bind( op | i << 9 | _l << _s | j, F, I, Long ) } \
    } \
}

namespace M68FAMILY {

auto M68000::parse(const char* s, uint16_t sum) -> uint16_t {

    return  *s == '1' ? parse(s + 1, (sum << 1) + 1) :
            *s == ' ' ? parse(s + 1, sum) :
            *s ? parse(s + 1, sum << 1) : sum;
}

auto M68000::build() -> void {
    uint16_t o;

    //shift/rotate immediate
    o = parse("1110 ---1 ss00 0---");
    _B_( o, S6 | SO, ImmShift, Asl, BWL )

    o = parse("1110 ---0 ss00 0---");
    _B_( o, S6 | SO, ImmShift, Asr, BWL )

    o = parse("1110 ---1 ss00 1---");
    _B_( o, S6 | SO, ImmShift, Lsl, BWL )

    o = parse("1110 ---0 ss00 1---");
    _B_( o, S6 | SO, ImmShift, Lsr, BWL )

    o = parse("1110 ---1 ss01 1---");
    _B_( o, S6 | SO, ImmShift, Rol, BWL )

    o = parse("1110 ---0 ss01 1---");
    _B_( o, S6 | SO, ImmShift, Ror, BWL )

    o = parse("1110 ---1 ss01 0---");
    _B_( o, S6 | SO, ImmShift, Roxl, BWL )

    o = parse("1110 ---0 ss01 0---");
    _B_( o, S6 | SO, ImmShift, Roxr, BWL )

    //shift/rotate register
    o = parse("1110 ---1 ss10 0---");
    _B_( o, S6 | SO, RegShift, Asl, BWL )

    o = parse("1110 ---0 ss10 0---");
    _B_( o, S6 | SO, RegShift, Asr, BWL )

    o = parse("1110 ---1 ss10 1---");
    _B_( o, S6 | SO, RegShift, Lsl, BWL )

    o = parse("1110 ---0 ss10 1---");
    _B_( o, S6 | SO, RegShift, Lsr, BWL )

    o = parse("1110 ---1 ss11 1---");
    _B_( o, S6 | SO, RegShift, Rol, BWL )

    o = parse("1110 ---0 ss11 1---");
    _B_( o, S6 | SO, RegShift, Ror, BWL )

    o = parse("1110 ---1 ss11 0---");
    _B_( o, S6 | SO, RegShift, Roxl, BWL )

    o = parse("1110 ---0 ss11 0---");
    _B_( o, S6 | SO, RegShift, Roxr, BWL )

    //shift/rotate effective address
    o = parse("1110 0001 11-- ----");
    _EA_( o, 0, Shift, Asl, ADR_TYPICAL, Word )

    o = parse("1110 0000 11-- ----");
    _EA_( o, 0, Shift, Asr, ADR_TYPICAL, Word )

    o = parse("1110 0011 11-- ----");
    _EA_( o, 0, Shift, Lsl, ADR_TYPICAL, Word )

    o = parse("1110 0010 11-- ----");
    _EA_( o, 0, Shift, Lsr, ADR_TYPICAL, Word )

    o = parse("1110 0111 11-- ----");
    _EA_( o, 0, Shift, Rol, ADR_TYPICAL, Word )

    o = parse("1110 0110 11-- ----");
    _EA_( o, 0, Shift, Ror, ADR_TYPICAL, Word )

    o = parse("1110 0101 11-- ----");
    _EA_( o, 0, Shift, Roxl, ADR_TYPICAL, Word )

    o = parse("1110 0100 11-- ----");
    _EA_( o, 0, Shift, Roxr, ADR_TYPICAL, Word )

    // bit manipulation
    o = parse("0000 ---1 01-- ----");
    _EA_( o, SO, Bit, Bchg, ADR_TYPICAL, Byte )
    _EA_( o, SO, Bit, Bchg, DR, Long )

    o = parse("0000 ---1 11-- ----");
    _EA_( o, SO, Bit, Bset, ADR_TYPICAL, Byte )
    _EA_( o, SO, Bit, Bset, DR, Long )

    o = parse("0000 ---1 10-- ----");
    _EA_( o, SO, Bit, Bclr, ADR_TYPICAL, Byte )
    _EA_( o, SO, Bit, Bclr, DR, Long )

    o = parse("0000 ---1 00-- ----");
    _EA_( o, SO, Bit, Btst, ADR_FULL, Byte )
    _EA_( o, SO, Bit, Btst, DR, Long )

    // bit manipulation Immediate
    o = parse("0000 1000 01-- ----");
    _EA_( o, 0, ImmBit, Bchg, ADR_TYPICAL, Byte )
    _EA_( o, 0, ImmBit, Bchg, DR, Long )

    o = parse("0000 1000 11-- ----");
    _EA_( o, 0, ImmBit, Bset, ADR_TYPICAL, Byte )
    _EA_( o, 0, ImmBit, Bset, DR, Long )

    o = parse("0000 1000 10-- ----");
    _EA_( o, 0, ImmBit, Bclr, ADR_TYPICAL, Byte )
    _EA_( o, 0, ImmBit, Bclr, DR, Long )

    o = parse("0000 1000 00-- ----");
    _EA_( o, 0, ImmBit, Btst, ADR_TYPICAL | PCD | PCI, Byte )
    _EA_( o, 0, ImmBit, Btst, DR, Long )

    // clr
    o = parse("0100 0010 ss-- ----");
    _EA_( o, S6, Clr, 0, ADR_TYPICAL | DR, BWL )

    // nbcd
    o = parse("0100 1000 00-- ----");
    _EA_( o, 0, Nbcd, 0, ADR_TYPICAL | DR, Byte )

    // neg
    o = parse("0100 0100 ss-- ----");
    _EA_( o, S6, Neg, Sub, ADR_TYPICAL | DR, BWL )

    // negx
    o = parse("0100 0000 ss-- ----");
    _EA_( o, S6, Neg, Subx, ADR_TYPICAL | DR, BWL )

    // not
    o = parse("0100 0110 ss-- ----");
    _EA_( o, S6, Neg, Not, ADR_TYPICAL | DR, BWL )

    // scc
    o = parse("0101 ---- 11-- ----");
    _EA_( o | (0 << 8), 0, Scc, 0, ADR_TYPICAL | DR, Byte )
    _EA_( o | (1 << 8), 0, Scc, 1, ADR_TYPICAL | DR, Byte )
    _EA_( o | (2 << 8), 0, Scc, 2, ADR_TYPICAL | DR, Byte )
    _EA_( o | (3 << 8), 0, Scc, 3, ADR_TYPICAL | DR, Byte )
    _EA_( o | (4 << 8), 0, Scc, 4, ADR_TYPICAL | DR, Byte )
    _EA_( o | (5 << 8), 0, Scc, 5, ADR_TYPICAL | DR, Byte )
    _EA_( o | (6 << 8), 0, Scc, 6, ADR_TYPICAL | DR, Byte )
    _EA_( o | (7 << 8), 0, Scc, 7, ADR_TYPICAL | DR, Byte )
    _EA_( o | (8 << 8), 0, Scc, 8, ADR_TYPICAL | DR, Byte )
    _EA_( o | (9 << 8), 0, Scc, 9, ADR_TYPICAL | DR, Byte )
    _EA_( o | (10 << 8), 0, Scc, 10, ADR_TYPICAL | DR, Byte )
    _EA_( o | (11 << 8), 0, Scc, 11, ADR_TYPICAL | DR, Byte )
    _EA_( o | (12 << 8), 0, Scc, 12, ADR_TYPICAL | DR, Byte )
    _EA_( o | (13 << 8), 0, Scc, 13, ADR_TYPICAL | DR, Byte )
    _EA_( o | (14 << 8), 0, Scc, 14, ADR_TYPICAL | DR, Byte )
    _EA_( o | (15 << 8), 0, Scc, 15, ADR_TYPICAL | DR, Byte )

    // tas
    o = parse("0100 1010 11-- ----");
    _EA_( o, 0, Tas, 0, ADR_TYPICAL | DR, Byte )

    // tst
    o = parse("0100 1010 ss-- ----");
    _EA_( o, S6, Tst, 0, ADR_TYPICAL | DR, BWL )

    // add
    o = parse("1101 ---0 ss-- ----");
    _EA_( o, S6 | SO, Arithmetic, Add, ADR_FULL | DR, Byte )
    _EA_( o, S6 | SO, Arithmetic, Add, ADR_FULL | DR | AR, WL )

    // sub
    o = parse("1001 ---0 ss-- ----");
    _EA_( o, S6 | SO, Arithmetic, Sub, ADR_FULL | DR, Byte )
    _EA_( o, S6 | SO, Arithmetic, Sub, ADR_FULL | DR | AR, WL )

    // cmp
    o = parse("1011 ---0 ss-- ----");
    _EA_( o, S6 | SO, Cmp, Cmp, ADR_FULL | DR, Byte )
    _EA_( o, S6 | SO, Cmp, Cmp, ADR_FULL | DR | AR, WL )

    // and
    o = parse("1100 ---0 ss-- ----");
    _EA_( o, S6 | SO, Arithmetic, And, ADR_FULL | DR, BWL )

    // or
    o = parse("1000 ---0 ss-- ----");
    _EA_( o, S6 | SO, Arithmetic, Or, ADR_FULL | DR, BWL )

    // add
    o = parse("1101 ---1 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticEA, Add, ADR_TYPICAL, BWL )

    // sub
    o = parse("1001 ---1 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticEA, Sub, ADR_TYPICAL, BWL )

    // and
    o = parse("1100 ---1 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticEA, And, ADR_TYPICAL, BWL )

    // or
    o = parse("1000 ---1 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticEA, Or, ADR_TYPICAL, BWL )

    // adda
    o = parse("1101 ---s 11-- ----");
    _EA_( o, S8 | SO, ArithmeticA, Adda, ADR_FULL | DR | AR, Word | Long )

    // suba
    o = parse("1001 ---s 11-- ----");
    _EA_( o, S8 | SO, ArithmeticA, Suba, ADR_FULL | DR | AR, Word | Long )

    // cmpa
    o = parse("1011 ---s 11-- ----");
    _EA_( o, S8 | SO, Cmpa, Cmp, ADR_FULL | DR | AR, Word | Long )

    // eor
    o = parse("1011 ---1 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticEA, Eor, ADR_TYPICAL | DR, BWL )

    // mulu
    o = parse("1100 ---0 11-- ----");
    _EA_( o, SO, Mul, Mulu, ADR_FULL | DR, Word )

    // muls
    o = parse("1100 ---1 11-- ----");
    _EA_( o, SO, Mul, Muls, ADR_FULL | DR, Word )

    // divu
    o = parse("1000 ---0 11-- ----");
    _EA_( o, SO, Div, Divu, ADR_FULL | DR, Word )

    // divs
    o = parse("1000 ---1 11-- ----");
    _EA_( o, SO, Div, Divs, ADR_FULL | DR, Word )
    
    // move
    o = parse("00ss ---0 00-- ----");
    _EA_( o, S12 | SO, Move, DataRegisterDirect, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, Move, DataRegisterDirect, ADR_FULL | AR | DR, WL )

    o = parse("00ss ---0 10-- ----");
    _EA_( o, S12 | SO, Move, AddressRegisterIndirect, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, Move, AddressRegisterIndirect, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss ---0 11-- ----");
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithPostIncrement, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithPostIncrement, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss ---1 00-- ----");
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithPreDecrement, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithPreDecrement, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss ---1 01-- ----");
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithDisplacement, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithDisplacement, ADR_FULL | AR | DR, WL )

    o = parse("00ss ---1 10-- ----");
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithIndex, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, Move, AddressRegisterIndirectWithIndex, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss 0001 11-- ----");
    _EA_( o, S12, Move, AbsoluteShort, ADR_FULL | DR, Byte )
    _EA_( o, S12, Move, AbsoluteShort, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss 0011 11-- ----");
    _EA_( o, S12, Move, AbsoluteLong, ADR_FULL | DR, Byte )
    _EA_( o, S12, Move, AbsoluteLong, ADR_FULL | AR | DR, WL )
    
    // movea
    o = parse("00ss ---0 01-- ----");
    _EA_( o, S12 | SO, MoveA, 0, ADR_FULL | AR | DR, WL )
    
    // addi
    o = parse("0000 0110 ss-- ----");
    _EA_( o, S6, ArithmeticI, Add, ADR_TYPICAL | DR, BWL )

    // andi
    o = parse("0000 0010 ss-- ----");
    _EA_( o, S6, ArithmeticI, And, ADR_TYPICAL | DR, BWL )
    
    // cmpi
    o = parse("0000 1100 ss-- ----");
    _EA_( o, S6, ArithmeticI, Cmp, ADR_TYPICAL | DR, BWL )
    
    // eori
    o = parse("0000 1010 ss-- ----");
    _EA_( o, S6, ArithmeticI, Eor, ADR_TYPICAL | DR, BWL )
    
    // ori
    o = parse("0000 0000 ss-- ----");
    _EA_( o, S6, ArithmeticI, Or, ADR_TYPICAL | DR, BWL )
    
    // subi
    o = parse("0000 0100 ss-- ----");
    _EA_( o, S6, ArithmeticI, Sub, ADR_TYPICAL | DR, BWL )

    // addq
    o = parse("0101 ---0 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticQ, Add, ADR_TYPICAL | DR , BWL )
    _EA_( o, S6 | SO, ArithmeticQ, Add, AR, WL )

    // subq
    o = parse("0101 ---1 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticQ, Sub, ADR_TYPICAL | DR , BWL )
    _EA_( o, S6 | SO, ArithmeticQ, Sub, AR, WL )

    // moveq
    o = parse("0111 ---0 ---- ----");
    _B_( o, SO | O_256, MoveQ, 0, Long )

    // bcc, bra
    o = parse("0110 ---- ---- ----");
    _B_( o | 0x000, O_256, Bcc, 0, Byte )        // bra
    _B_( o | 0x200, O_256, Bcc, 2, Byte )
    _B_( o | 0x300, O_256, Bcc, 3, Byte )
    _B_( o | 0x400, O_256, Bcc, 4, Byte )
    _B_( o | 0x500, O_256, Bcc, 5, Byte )
    _B_( o | 0x600, O_256, Bcc, 6, Byte )
    _B_( o | 0x700, O_256, Bcc, 7, Byte )
    _B_( o | 0x800, O_256, Bcc, 8, Byte )
    _B_( o | 0x900, O_256, Bcc, 9, Byte )
    _B_( o | 0xa00, O_256, Bcc, 0xa, Byte )
    _B_( o | 0xb00, O_256, Bcc, 0xb, Byte )
    _B_( o | 0xc00, O_256, Bcc, 0xc, Byte )
    _B_( o | 0xd00, O_256, Bcc, 0xd, Byte )
    _B_( o | 0xe00, O_256, Bcc, 0xe, Byte )
    _B_( o | 0xf00, O_256, Bcc, 0xf, Byte )

    // override a displacement of zero with "Word"
    _bind(o | 0x000, Bcc, 0, Word)               // bra
    _bind(o | 0x200, Bcc, 2, Word)
    _bind(o | 0x300, Bcc, 3, Word)
    _bind(o | 0x400, Bcc, 4, Word)
    _bind(o | 0x500, Bcc, 5, Word)
    _bind(o | 0x600, Bcc, 6, Word)
    _bind(o | 0x700, Bcc, 7, Word)
    _bind(o | 0x800, Bcc, 8, Word)
    _bind(o | 0x900, Bcc, 9, Word)
    _bind(o | 0xa00, Bcc, 0xa, Word)
    _bind(o | 0xb00, Bcc, 0xb, Word)
    _bind(o | 0xc00, Bcc, 0xc, Word)
    _bind(o | 0xd00, Bcc, 0xd, Word)
    _bind(o | 0xe00, Bcc, 0xe, Word)
    _bind(o | 0xf00, Bcc, 0xf, Word)

    // bsr
    o = parse("0110 0001 ---- ----");
    _B_( o, O_256, Bsr, 0, Byte )
    // override a displacement of zero with "Word"
    _bind(o, Bsr, 0, Word)
    
    // dbcc
    o = parse("0101 ---- 1100 1---");
    _B_(o | 0x000, 0, Dbcc, 0, Word)
    _B_(o | 0x100, 0, Dbcc, 1, Word)
    _B_(o | 0x200, 0, Dbcc, 2, Word)
    _B_(o | 0x300, 0, Dbcc, 3, Word)
    _B_(o | 0x400, 0, Dbcc, 4, Word)
    _B_(o | 0x500, 0, Dbcc, 5, Word)
    _B_(o | 0x600, 0, Dbcc, 6, Word)
    _B_(o | 0x700, 0, Dbcc, 7, Word)
    _B_(o | 0x800, 0, Dbcc, 8, Word)
    _B_(o | 0x900, 0, Dbcc, 9, Word)
    _B_(o | 0xa00, 0, Dbcc, 0xa, Word)
    _B_(o | 0xb00, 0, Dbcc, 0xb, Word)
    _B_(o | 0xc00, 0, Dbcc, 0xc, Word)
    _B_(o | 0xd00, 0, Dbcc, 0xd, Word)
    _B_(o | 0xe00, 0, Dbcc, 0xe, Word)
    _B_(o | 0xf00, 0, Dbcc, 0xf, Word)
    
    // jmp
    o = parse("0100 1110 11-- ----");
    _EA_( o, 0, Jmp, 0, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // jsr
    o = parse("0100 1110 10-- ----");
    _EA_( o, 0, Jsr, 0, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // lea
    o = parse("0100 ---1 11-- ----");
    _EA_( o, SO, Lea, 0, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // pea
    o = parse("0100 1000 01-- ----");
    _EA_( o, 0, Pea, 0, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // movem -> reg
    o = parse("0100 1100 1s-- ----");
    _EA_( o | (0 << 6), 0, MovemToReg, 0, AI | AIPI | AID | AII | AS | AL | PCD | PCI, Word )
    _EA_( o | (1 << 6), 0, MovemToReg, 0, AI | AIPI | AID | AII | AS | AL | PCD | PCI, Long )

    // movem -> ea
    o = parse("0100 1000 1s-- ----");
    _EA_( o | (0 << 6), 0, MovemToEa, 0, AI | AIPD | AID | AII | AS | AL, Word )
    _EA_( o | (1 << 6), 0, MovemToEa, 0, AI | AIPD | AID | AII | AS | AL, Long )

    // addx to reg
    o = parse("1101 ---1 ss00 0---");
    _B_( o, SO | S6, ArithmeticX, Addx, BWL )

    // subx to reg
    o = parse("1001 ---1 ss00 0---");
    _B_( o, SO | S6, ArithmeticX, Subx, BWL )

    // abcd to reg
    o = parse("1100 ---1 0000 0---");
    _B_( o, SO, ArithmeticX, Abcd, Byte )

    // sbcd to reg
    o = parse("1000 ---1 0000 0---");
    _B_( o, SO, ArithmeticX, Sbcd, Byte )

    // addx to mem
    o = parse("1101 ---1 ss00 1---");
    _B_( o, SO | S6, ArithmeticXEa, Addx, BWL )

    // subx to mem
    o = parse("1001 ---1 ss00 1---");
    _B_( o, SO | S6, ArithmeticXEa, Subx, BWL )

    // abcd to mem
    o = parse("1100 ---1 0000 1---");
    _B_( o, SO, ArithmeticXEa, Abcd, Byte )

    // sbcd to mem
    o = parse("1000 ---1 0000 1---");
    _B_( o, SO, ArithmeticXEa, Sbcd, Byte )

    // cmpm
    o = parse("1011 ---1 ss00 1---");
    _B_( o, SO | S6, Cmpm, Cmp, BWL )

    // andi ccr
    o = parse("0000 0010 0011 1100");
    _bind(o, Ccr, And, Byte)

    // ori ccr
    o = parse("0000 0000 0011 1100");
    _bind(o, Ccr, Or, Byte)

    // eori ccr
    o = parse("0000 1010 0011 1100");
    _bind(o, Ccr, Eor, Byte)

    // andi sr
    o = parse("0000 0010 0111 1100");
    _bind(o, Sr, And, Word)

    // ori sr
    o = parse("0000 0000 0111 1100");
    _bind(o, Sr, Or, Word)

    // eori sr
    o = parse("0000 1010 0111 1100");
    _bind(o, Sr, Eor, Word)

    // chk
    o = parse("0100 ---1 10-- ----");
    _EA_( o, SO, Chk, 0, ADR_FULL | DR, Word )

    // move from sr
    o = parse("0100 0000 11-- ----");
    _EA_( o, 0, MoveFromSr, 0, ADR_TYPICAL | DR, Word )

    // move to sr
    o = parse("0100 0110 11-- ----");
    _EA_( o, 0, MoveToSr, 0, ADR_FULL | DR, Word )

    // move to ccr
    o = parse("0100 0100 11-- ----");
    _EA_( o, 0, MoveToCcr, 0, ADR_FULL | DR, Word )

    // exg Dx,Dy
    o = parse("1100 ---1 0100 0---");
    _B_( o, SO, ExgDxDy, 0, Long )

    // exg Ax,Ay
    o = parse("1100 ---1 0100 1---");
    _B_( o, SO, ExgAxAy, 0, Long )

    // exg Ax,Dy
    o = parse("1100 ---1 1000 1---");
    _B_( o, SO, ExgAxDy, 0, Long )

    // ext
    o = parse("0100 1000 ss00 0---");
    _B_( o | (2 << 6), 0, Ext, 0, Word )
    _B_( o | (3 << 6), 0, Ext, 0, Long )

    // link
    o = parse("0100 1110 0101 0---");
    _B_( o, 0, Link, 0, Long )

    // move to usp
    o = parse("0100 1110 0110 ----");
    _B_( o | (1 << 3), 0, MoveUsp, UspAn, Long )
    _B_( o | (0 << 3), 0, MoveUsp, AnUsp, Long )

    // nop
    o = parse("0100 1110 0111 0001");
    opTable[o] = &M68000::opNop;

    // reset
    o = parse("0100 1110 0111 0000");
    opTable[o] = &M68000::opReset;

    // rte
    o = parse("0100 1110 0111 0011");
    opTable[o] = &M68000::opRte;

    // rtr
    o = parse("0100 1110 0111 0111");
    opTable[o] = &M68000::opRtr;

    // rts
    o = parse("0100 1110 0111 0101");
    opTable[o] = &M68000::opRts;

    // stop
    o = parse("0100 1110 0111 0010");
    opTable[o] = &M68000::opStop;

    // swap
    o = parse("0100 1000 0100 0---");
    _B_( o, 0, Swap, 0, Word )

    // trap
    o = parse("0100 1110 0100 ----");
    _B_( o, O_16, Trap, 0, Word )

    // trapv
    o = parse("0100 1110 0111 0110");
    opTable[o] = &M68000::opTrapv;

    // unlink
    o = parse("0100 1110 0101 1---");
    _B_( o, 0, Unlink, 0, Long )

    // movep to reg
    o = parse("0000 ---1 0s00 1---");
    _B_( o | (0 << 6), SO, Movep, ToReg, Word )
    _B_( o | (1 << 6), SO, Movep, ToReg, Long )

    // movep to mem
    o = parse("0000 ---1 1s00 1---");
    _B_( o | (0 << 6), SO, Movep, ToMem, Word )
    _B_( o | (1 << 6), SO, Movep, ToMem, Long )

    for (int i = 0; i < 0x1000; i++) {
        opTable[(0xa << 12) | i] = &M68000::lineA;
        opTable[(0xf << 12) | i] = &M68000::lineF;
    }

    mulCycleLookup = new uint8_t[0x10000];
    mulCycleLookup[0] = 34;

    for( int i = 0; i < 0x10000; i ++ ) {
        mulCycleLookup[i] = ((i & 1) << 1) + mulCycleLookup[i / 2];

        if (!opTable[i]) // note: initialized with zero in header file
            opTable[i] = &M68000::illegal;
    }
}

}

#undef _bind
#undef _bindEA
#undef _M_
#undef _EA_
#undef _B_
