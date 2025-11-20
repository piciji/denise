#include "m68000.h"

namespace M68FAMILY {

auto M68000::disassemble(uint32_t addr, unsigned& bytes, uint16_t* memSnap) -> std::string {
    DasmHandler d;
    uint32_t _pc = addr;
    uint16_t opcode = memSnap ? *memSnap++ : peek(_pc);
    d.memSnap = memSnap;

    (this->*dasmTable[opcode])(opcode, _pc, d);

    bytes = _pc - addr + 2;
    if (d.comment)
        d.addComment();

    return d.str;
}

auto M68000::disassembleData(uint32_t addr, unsigned bytes) -> std::string {
    DasmHandler d;
    d.hex24( addr );
    d.str.append( "|" );

    for(unsigned i = 0; i < (bytes / 2) ; ++i) {
        if (i)
            d.str.append( " " );

        d.hex16( peek( addr ) );
        addr += 2;
    }
    return d.str;
}

auto M68000::disassembleTrace(unsigned i, uint16_t& flags) -> std::string {
    DasmHandler d;
    unsigned bytes;
    HistoryEntry* historyEntry = historyHandler.get(i);
    if (!historyEntry)
        return "";
    d.hex24( historyEntry->addr );
    d.str.append( "|" );
    d.str.append( disassemble( historyEntry->addr, bytes, &historyEntry->mem[0]) );
    flags = historyEntry->flags;
    return d.str;
}

auto M68000::dasmIllegal(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    d.str.append( "dc.w " );
    d.tab().hex(opcode);
    d.str.append( "; ILLEGAL" );
}

auto M68000::dasmLineA(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    d.str.append( "dc.w " );
    d.tab().hex(opcode);
    d.str.append( "; Line A" );
}

auto M68000::dasmLineF(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    d.str.append( "dc.w " );
    d.tab().hex(opcode);
    d.str.append( "; Line F" );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmImmShift( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint8_t shift = (opcode >> 9) & 7;
    d.Ins( Inst ).si( Size ).tab().immD( shift ? shift : 8 ).sep().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmRegShift( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( (opcode >> 9) & 7 ).sep().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmShift( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmBit( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().dn( (opcode >> 9) & 7 ).sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmImmBit( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t bits = clip<Size>( peekInc<Word>( adr, d ) );
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().immU( bits ).sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmClr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmNbcd( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmNeg( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmScc( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmTas( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmTst( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmArithmetic( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmCmp( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmArithmeticEA( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().dn( (opcode >> 9) & 7 ).sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmArithmeticA( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().an( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmCmpa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().an( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMul( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmDiv( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMove( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Move ).si( Size ).tab().ea().sep();

    prepareEaForDasm<Inst, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMoveA( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep();

    prepareEaForDasm<AddressRegisterDirect, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmArithmeticI( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab();
    uint32_t ext = clip<Size>( peekInc<Size>( adr, d ) );
    d.immS( sign<Size>( ext ) ).sep();
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmArithmeticQ( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t operand = (opcode >> 9) & 7;
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().immU( operand ? operand : 8 ).sep().ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmMoveQ( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().immS( sign<Byte>( opcode & 0xff ) ).sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmBcc( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t _adr = adr + 2;
    _adr += (Size == Word ? (int16_t) peekInc<Word>( adr, d ) : (int8_t) (opcode & 0xff));

    d.Ins( Inst ).sis( Size ).tab().hex( _adr );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmBsr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    dasmBcc<Inst, Size>( opcode, adr, d );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmDbcc( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t _adr = adr + 2;
    _adr += (int16_t) peekInc<Word>( adr, d );
    d.Ins( Inst ).tab().dn( opcode & 7 ).sep().hex( _adr );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmJmp( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmJsr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmLea( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea().sep().an( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmPea( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMovemToReg( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().regList( ext );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMovemToEa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );

    if (Mode == AddressRegisterIndirectWithPreDecrement) {
        uint16_t _ext = 0;
        for (int _i = 15; _i >= 0; _i--)
            _ext |= ((ext >> _i) & 1) << (15 - _i);
        ext = _ext;
    }

    d.Ins( Inst ).si( Size ).tab().regList( ext ).sep().ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmArithmeticX( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( opcode & 7 ).sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmArithmeticBCD( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( opcode & 7 ).sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmArithmeticXEa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<AddressRegisterIndirectWithPreDecrement, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep();
    prepareEaForDasm<AddressRegisterIndirectWithPreDecrement, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmCmpm( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<AddressRegisterIndirectWithPostIncrement, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep();
    prepareEaForDasm<AddressRegisterIndirectWithPostIncrement, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmCcr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = clip<Size>( peekInc<Size>( adr, d ) );

    d.Ins( Inst ).si( Size ).tab().immS( sign<Size>( ext ) ).sep().ccr();
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmSr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = clip<Size>( peekInc<Size>( adr, d ) );

    d.Ins( Inst ).si( Size ).tab().immS( sign<Size>( ext ) ).sep().sr();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmChk( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMoveFromSr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().sr().sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMoveToSr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().sr();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68000::dasmMoveToCcr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Byte>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().ccr();
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmExgDxDy( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().dn( (opcode >> 9) & 7 ).sep().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmExgAxAy( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().an( (opcode >> 9) & 7 ).sep().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmExgAxDy( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().dn( (opcode >> 9) & 7 ).sep().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmExt( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmMoveUspAn( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().usp().sep().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmMoveAnUsp( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().an( opcode & 7 ).sep().usp();
}

auto M68000::dasmNop( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Nop );
}

auto M68000::dasmReset( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Reset );
}

auto M68000::dasmRte( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Rte );
}

auto M68000::dasmRtr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Rtr );
}

auto M68000::dasmRts( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Rts );
}

auto M68000::dasmStop( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = peekInc<Word>( adr, d );
    d.Ins( _Stop ).tab().immS( sign<Word>( ext ) );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmSwap( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmTrap( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().immU( opcode & 15 );
}

auto M68000::dasmTrapv( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Trapv );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmLink( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = peekInc<Size>( adr, d );
    d.Ins( Inst ).tab().an( opcode & 7 ).sep().immS( sign<Size>( ext ) );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmUnlink( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmMovepReg( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab();
    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, opcode & 7, d );
    d.ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68000::dasmMovepEa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab();
    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, opcode & 7, d );
    d.dn( (opcode >> 9) & 7 ).sep().ea();
}

}
