
#include "m68000.h"

enum { S6 = 6, S8 = 8, S12 = 12, SO /* second operand */ = 1 << 4, O_16 = 2 << 4, O_256 = 4 << 4 };
enum { DR = 1, AR = 2, AI = 4, AIPI = 8, AIPD = 16, AID = 32, AII = 64, AS = 128, AL = 256, PCD = 512, PCI = 1024, IM = 2048,
        ADR_TYPICAL = AI | AIPI | AIPD | AID | AII | AS | AL, ADR_FULL = ADR_TYPICAL | PCD | PCI | IM };

#define _bindEA(id, F, I, M, S) { opTable[id] = &M68000::op##F<I, M, S>; dasmTable[id] = &M68000::dasm##F<I, M, S>; mnemonics[id] = DasmHandler::mnemonic( I ); }
#define _bind(id, F, I, S) { opTable[id] = &M68000::op##F<I, S>; dasmTable[id] = &M68000::dasm##F<I, S>; mnemonics[id] = DasmHandler::mnemonic( I ); }

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
    dasmTable = new OpDasm[0x10000];

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
    _EA_( o, S6, Clr, Clr, ADR_TYPICAL | DR, BWL )

    // nbcd
    o = parse("0100 1000 00-- ----");
    _EA_( o, 0, Nbcd, Nbcd, ADR_TYPICAL | DR, Byte )

    // neg
    o = parse("0100 0100 ss-- ----");
    _EA_( o, S6, Neg, Neg, ADR_TYPICAL | DR, BWL )

    // negx
    o = parse("0100 0000 ss-- ----");
    _EA_( o, S6, Neg, Negx, ADR_TYPICAL | DR, BWL )

    // not
    o = parse("0100 0110 ss-- ----");
    _EA_( o, S6, Neg, Not, ADR_TYPICAL | DR, BWL )

    // scc
    o = parse("0101 ---- 11-- ----");
    _EA_( o | (0 << 8), 0, Scc, St, ADR_TYPICAL | DR, Byte )
    _EA_( o | (1 << 8), 0, Scc, Sf, ADR_TYPICAL | DR, Byte )
    _EA_( o | (2 << 8), 0, Scc, Shi, ADR_TYPICAL | DR, Byte )
    _EA_( o | (3 << 8), 0, Scc, Sls, ADR_TYPICAL | DR, Byte )
    _EA_( o | (4 << 8), 0, Scc, Scc, ADR_TYPICAL | DR, Byte )
    _EA_( o | (5 << 8), 0, Scc, Scs, ADR_TYPICAL | DR, Byte )
    _EA_( o | (6 << 8), 0, Scc, Sne, ADR_TYPICAL | DR, Byte )
    _EA_( o | (7 << 8), 0, Scc, Seq, ADR_TYPICAL | DR, Byte )
    _EA_( o | (8 << 8), 0, Scc, Svc, ADR_TYPICAL | DR, Byte )
    _EA_( o | (9 << 8), 0, Scc, Svs, ADR_TYPICAL | DR, Byte )
    _EA_( o | (10 << 8), 0, Scc, Spl, ADR_TYPICAL | DR, Byte )
    _EA_( o | (11 << 8), 0, Scc, Smi, ADR_TYPICAL | DR, Byte )
    _EA_( o | (12 << 8), 0, Scc, Sge, ADR_TYPICAL | DR, Byte )
    _EA_( o | (13 << 8), 0, Scc, Slt, ADR_TYPICAL | DR, Byte )
    _EA_( o | (14 << 8), 0, Scc, Sgt, ADR_TYPICAL | DR, Byte )
    _EA_( o | (15 << 8), 0, Scc, Sle, ADR_TYPICAL | DR, Byte )

    // tas
    o = parse("0100 1010 11-- ----");
    _EA_( o, 0, Tas, Tas, ADR_TYPICAL | DR, Byte )

    // tst
    o = parse("0100 1010 ss-- ----");
    _EA_( o, S6, Tst, Tst, ADR_TYPICAL | DR, BWL )

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
    _EA_( o, S8 | SO, Cmpa, Cmpa, ADR_FULL | DR | AR, Word | Long )

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
    _EA_( o, S12 | SO, MoveDataRegisterDirect, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, MoveDataRegisterDirect, Move, ADR_FULL | AR | DR, WL )

    o = parse("00ss ---0 10-- ----");
    _EA_( o, S12 | SO, MoveAddressRegisterIndirect, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, MoveAddressRegisterIndirect, Move, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss ---0 11-- ----");
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithPostIncrement, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithPostIncrement, Move, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss ---1 00-- ----");
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithPreDecrement, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithPreDecrement, Move, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss ---1 01-- ----");
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithDisplacement, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithDisplacement, Move, ADR_FULL | AR | DR, WL )

    o = parse("00ss ---1 10-- ----");
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithIndex, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12 | SO, MoveAddressRegisterIndirectWithIndex, Move, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss 0001 11-- ----");
    _EA_( o, S12, MoveAbsoluteShort, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12, MoveAbsoluteShort, Move, ADR_FULL | AR | DR, WL )
    
    o = parse("00ss 0011 11-- ----");
    _EA_( o, S12, MoveAbsoluteLong, Move, ADR_FULL | DR, Byte )
    _EA_( o, S12, MoveAbsoluteLong, Move, ADR_FULL | AR | DR, WL )
    
    // movea
    o = parse("00ss ---0 01-- ----");
    _EA_( o, S12 | SO, MoveA, Movea, ADR_FULL | AR | DR, WL )
    
    // addi
    o = parse("0000 0110 ss-- ----");
    _EA_( o, S6, ArithmeticI, Addi, ADR_TYPICAL | DR, BWL )

    // andi
    o = parse("0000 0010 ss-- ----");
    _EA_( o, S6, ArithmeticI, Andi, ADR_TYPICAL | DR, BWL )
    
    // cmpi
    o = parse("0000 1100 ss-- ----");
    _EA_( o, S6, ArithmeticI, Cmpi, ADR_TYPICAL | DR, BWL )
    
    // eori
    o = parse("0000 1010 ss-- ----");
    _EA_( o, S6, ArithmeticI, Eori, ADR_TYPICAL | DR, BWL )
    
    // ori
    o = parse("0000 0000 ss-- ----");
    _EA_( o, S6, ArithmeticI, Ori, ADR_TYPICAL | DR, BWL )
    
    // subi
    o = parse("0000 0100 ss-- ----");
    _EA_( o, S6, ArithmeticI, Subi, ADR_TYPICAL | DR, BWL )

    // addq
    o = parse("0101 ---0 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticQ, Addq, ADR_TYPICAL | DR , BWL )
    _EA_( o, S6 | SO, ArithmeticQ, Addq, AR, WL )

    // subq
    o = parse("0101 ---1 ss-- ----");
    _EA_( o, S6 | SO, ArithmeticQ, Subq, ADR_TYPICAL | DR , BWL )
    _EA_( o, S6 | SO, ArithmeticQ, Subq, AR, WL )

    // moveq
    o = parse("0111 ---0 ---- ----");
    _B_( o, SO | O_256, MoveQ, Moveq, Long )

    // bcc, bra
    o = parse("0110 ---- ---- ----");
    _B_( o | 0x000, O_256, Bcc, Bra, Byte )        // bra
    _B_( o | 0x200, O_256, Bcc, Bhi, Byte )
    _B_( o | 0x300, O_256, Bcc, Bls, Byte )
    _B_( o | 0x400, O_256, Bcc, Bcc, Byte )
    _B_( o | 0x500, O_256, Bcc, Bcs, Byte )
    _B_( o | 0x600, O_256, Bcc, Bne, Byte )
    _B_( o | 0x700, O_256, Bcc, Beq, Byte )
    _B_( o | 0x800, O_256, Bcc, Bvc, Byte )
    _B_( o | 0x900, O_256, Bcc, Bvs, Byte )
    _B_( o | 0xa00, O_256, Bcc, Bpl, Byte )
    _B_( o | 0xb00, O_256, Bcc, Bmi, Byte )
    _B_( o | 0xc00, O_256, Bcc, Bge, Byte )
    _B_( o | 0xd00, O_256, Bcc, Blt, Byte )
    _B_( o | 0xe00, O_256, Bcc, Bgt, Byte )
    _B_( o | 0xf00, O_256, Bcc, Ble, Byte )

    // override a displacement of zero with "Word"
    _bind(o | 0x000, Bcc, Bra, Word)               // bra
    _bind(o | 0x200, Bcc, Bhi, Word)
    _bind(o | 0x300, Bcc, Bls, Word)
    _bind(o | 0x400, Bcc, Bcc, Word)
    _bind(o | 0x500, Bcc, Bcs, Word)
    _bind(o | 0x600, Bcc, Bne, Word)
    _bind(o | 0x700, Bcc, Beq, Word)
    _bind(o | 0x800, Bcc, Bvc, Word)
    _bind(o | 0x900, Bcc, Bvs, Word)
    _bind(o | 0xa00, Bcc, Bpl, Word)
    _bind(o | 0xb00, Bcc, Bmi, Word)
    _bind(o | 0xc00, Bcc, Bge, Word)
    _bind(o | 0xd00, Bcc, Blt, Word)
    _bind(o | 0xe00, Bcc, Bgt, Word)
    _bind(o | 0xf00, Bcc, Ble, Word)

    // bsr
    o = parse("0110 0001 ---- ----");
    _B_( o, O_256, Bsr, Bsr, Byte )
    // override a displacement of zero with "Word"
    _bind(o, Bsr, Bsr, Word)
    
    // dbcc
    o = parse("0101 ---- 1100 1---");
    _B_(o | 0x000, 0, Dbcc, Dbt, Word)
    _B_(o | 0x100, 0, Dbcc, Dbf, Word)
    _B_(o | 0x200, 0, Dbcc, Dbhi, Word)
    _B_(o | 0x300, 0, Dbcc, Dbls, Word)
    _B_(o | 0x400, 0, Dbcc, Dbcc, Word)
    _B_(o | 0x500, 0, Dbcc, Dbcs, Word)
    _B_(o | 0x600, 0, Dbcc, Dbne, Word)
    _B_(o | 0x700, 0, Dbcc, Dbeq, Word)
    _B_(o | 0x800, 0, Dbcc, Dbvc, Word)
    _B_(o | 0x900, 0, Dbcc, Dbvs, Word)
    _B_(o | 0xa00, 0, Dbcc, Dbpl, Word)
    _B_(o | 0xb00, 0, Dbcc, Dbmi, Word)
    _B_(o | 0xc00, 0, Dbcc, Dbge, Word)
    _B_(o | 0xd00, 0, Dbcc, Dblt, Word)
    _B_(o | 0xe00, 0, Dbcc, Dbgt, Word)
    _B_(o | 0xf00, 0, Dbcc, Dble, Word)
    
    // jmp
    o = parse("0100 1110 11-- ----");
    _EA_( o, 0, Jmp, Jmp, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // jsr
    o = parse("0100 1110 10-- ----");
    _EA_( o, 0, Jsr, Jsr, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // lea
    o = parse("0100 ---1 11-- ----");
    _EA_( o, SO, Lea, Lea, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // pea
    o = parse("0100 1000 01-- ----");
    _EA_( o, 0, Pea, Pea, AI | AID | AII | AS | AL | PCD | PCI, Long )

    // movem -> reg
    o = parse("0100 1100 1s-- ----");
    _EA_( o | (0 << 6), 0, MovemToReg, Movem, AI | AIPI | AID | AII | AS | AL | PCD | PCI, Word )
    _EA_( o | (1 << 6), 0, MovemToReg, Movem, AI | AIPI | AID | AII | AS | AL | PCD | PCI, Long )

    // movem -> ea
    o = parse("0100 1000 1s-- ----");
    _EA_( o | (0 << 6), 0, MovemToEa, Movem, AI | AIPD | AID | AII | AS | AL, Word )
    _EA_( o | (1 << 6), 0, MovemToEa, Movem, AI | AIPD | AID | AII | AS | AL, Long )

    // addx to reg
    o = parse("1101 ---1 ss00 0---");
    _B_( o, SO | S6, ArithmeticX, Addx, BWL )

    // subx to reg
    o = parse("1001 ---1 ss00 0---");
    _B_( o, SO | S6, ArithmeticX, Subx, BWL )

    // abcd to reg
    o = parse("1100 ---1 0000 0---");
    _B_( o, SO, ArithmeticBCD, Abcd, Byte )

    // sbcd to reg
    o = parse("1000 ---1 0000 0---");
    _B_( o, SO, ArithmeticBCD, Sbcd, Byte )

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
    _B_( o, SO | S6, Cmpm, Cmpm, BWL )

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
    _EA_( o, SO, Chk, Chk, ADR_FULL | DR, Word )

    // move from sr
    o = parse("0100 0000 11-- ----");
    _EA_( o, 0, MoveFromSr, Move, ADR_TYPICAL | DR, Word )

    // move to sr
    o = parse("0100 0110 11-- ----");
    _EA_( o, 0, MoveToSr, Move, ADR_FULL | DR, Word )

    // move to ccr
    o = parse("0100 0100 11-- ----");
    _EA_( o, 0, MoveToCcr, Move, ADR_FULL | DR, Word )

    // exg Dx,Dy
    o = parse("1100 ---1 0100 0---");
    _B_( o, SO, ExgDxDy, Exg, Long )

    // exg Ax,Ay
    o = parse("1100 ---1 0100 1---");
    _B_( o, SO, ExgAxAy, Exg, Long )

    // exg Ax,Dy
    o = parse("1100 ---1 1000 1---");
    _B_( o, SO, ExgAxDy, Exg, Long )

    // ext
    o = parse("0100 1000 ss00 0---");
    _B_( o | (2 << 6), 0, Ext, Ext, Word )
    _B_( o | (3 << 6), 0, Ext, Ext, Long )

    // link
    o = parse("0100 1110 0101 0---");
    _B_( o, 0, Link, Link, Word )

    // move to usp
    o = parse("0100 1110 0110 ----");
    _B_( o | (1 << 3), 0, MoveUspAn, Move, Long )
    _B_( o | (0 << 3), 0, MoveAnUsp, Move, Long )

    // nop
    o = parse("0100 1110 0111 0001");
    opTable[o] = &M68000::opNop;
    dasmTable[o] = &M68000::dasmNop;
    mnemonics[o] = DasmHandler::mnemonic( Nop );

    // reset
    o = parse("0100 1110 0111 0000");
    opTable[o] = &M68000::opReset;
    dasmTable[o] = &M68000::dasmReset;
    mnemonics[o] = DasmHandler::mnemonic( Reset );

    // rte
    o = parse("0100 1110 0111 0011");
    opTable[o] = &M68000::opRte;
    dasmTable[o] = &M68000::dasmRte;
    mnemonics[o] = DasmHandler::mnemonic( Rte );

    // rtr
    o = parse("0100 1110 0111 0111");
    opTable[o] = &M68000::opRtr;
    dasmTable[o] = &M68000::dasmRtr;
    mnemonics[o] = DasmHandler::mnemonic( Rtr );

    // rts
    o = parse("0100 1110 0111 0101");
    opTable[o] = &M68000::opRts;
    dasmTable[o] = &M68000::dasmRts;
    mnemonics[o] = DasmHandler::mnemonic( Rts );

    // stop
    o = parse("0100 1110 0111 0010");
    opTable[o] = &M68000::opStop;
    dasmTable[o] = &M68000::dasmStop;
    mnemonics[o] = DasmHandler::mnemonic( _Stop );

    // swap
    o = parse("0100 1000 0100 0---");
    _B_( o, 0, Swap, Swap, Word )

    // trap
    o = parse("0100 1110 0100 ----");
    _B_( o, O_16, Trap, Trap, Word )

    // trapv
    o = parse("0100 1110 0111 0110");
    opTable[o] = &M68000::opTrapv;
    mnemonics[o] = DasmHandler::mnemonic( Trapv );

    // unlink
    o = parse("0100 1110 0101 1---");
    _B_( o, 0, Unlink, Unlk, Long )

    // movep to reg
    o = parse("0000 ---1 0s00 1---");
    _B_( o | (0 << 6), SO, MovepReg, Movep, Word )
    _B_( o | (1 << 6), SO, MovepReg, Movep, Long )

    // movep to mem
    o = parse("0000 ---1 1s00 1---");
    _B_( o | (0 << 6), SO, MovepEa, Movep, Word )
    _B_( o | (1 << 6), SO, MovepEa, Movep, Long )

    for (int i = 0; i < 0x1000; i++) {
        opTable[(0xa << 12) | i] = &M68000::lineA;
        opTable[(0xf << 12) | i] = &M68000::lineF;
        dasmTable[(0xa << 12) | i] = &M68000::dasmLineA;
        dasmTable[(0xf << 12) | i] = &M68000::dasmLineF;
        mnemonics[(0xa << 12) | i] = DasmHandler::mnemonic( LineA );
        mnemonics[(0xf << 12) | i] = DasmHandler::mnemonic( LineF );
    }

    mulCycleLookup = new uint8_t[0x10000];
    mulCycleLookup[0] = 34;

    for( int i = 0; i < 0x10000; i ++ ) {
        mulCycleLookup[i] = ((i & 1) << 1) + mulCycleLookup[i / 2];

        if (!opTable[i]) {
            // note: initialized with zero in header file
            opTable[i] = &M68000::illegal;
            dasmTable[i] = &M68000::dasmIllegal;
            mnemonics[i] = DasmHandler::mnemonic( Illegal );
        }
    }

    stepOuts.reserve(64);
}

}

#undef _bind
#undef _bindEA
#undef _M_
#undef _EA_
#undef _B_
