#include "m68Core.h"

namespace M68FAMILY {

auto M68Core::disassemble(uint32_t addr, unsigned& bytes, uint16_t* memSnap) -> std::string {
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

auto M68Core::disassembleData(uint32_t addr, unsigned bytes) -> std::string {
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

auto M68Core::disassembleTrace(unsigned i, uint16_t& flags) -> std::string {
    DasmHandler d;
    unsigned bytes;
    Emulator::HistoryEntry<uint16_t>* historyEntry = historyHandler.get(i);
    if (!historyEntry)
        return "";
    d.hex24( historyEntry->addr );
    d.str.append( "|" );
    d.str.append( disassemble( historyEntry->addr, bytes, &historyEntry->mem[0]) );
    flags = historyEntry->flags;
    return d.str;
}

auto M68Core::dasmIllegal(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    d.str.append( "dc.w " );
    d.tab().hex(opcode);
    d.str.append( "; ILLEGAL" );
}

auto M68Core::dasmLineA(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    d.str.append( "dc.w " );
    d.tab().hex(opcode);
    d.str.append( "; Line A" );
}

auto M68Core::dasmLineF(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    d.str.append( "dc.w " );
    d.tab().hex(opcode);
    d.str.append( "; Line F" );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmImmShift( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint8_t shift = (opcode >> 9) & 7;
    d.Ins( Inst ).si( Size ).tab().immD( shift ? shift : 8 ).sep().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmRegShift( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( (opcode >> 9) & 7 ).sep().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmShift( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmBit( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().dn( (opcode >> 9) & 7 ).sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmImmBit( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t bits = clip<Size>( peekInc<Word>( adr, d ) );
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().immU( bits ).sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmBitField(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    d.Ins( Inst ).tab();
    uint16_t ext = peekInc<Word>( adr, d );
    unsigned of = (ext >> 6) & 0x1f;
    unsigned wi = ext & 0x1f;

    if (Inst == Bfins)
        d.dn( (ext >> 12) & 7 ).sep();

    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.ea();

    d.bitField(of, wi == 0 ? 32 : 0, !!(ext & 0x800), !!(ext & 0x20));

    if (Inst == Bfextu || Inst == Bfexts || Inst == Bfffo)
        d.sep().dn( (ext >> 12) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmBitFieldEA(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmBitField<Inst, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmClr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmNbcd( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmNeg( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmScc( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmTas( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmTst( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmArithmetic( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmCmp( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmArithmeticEA( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().dn( (opcode >> 9) & 7 ).sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmArithmeticA( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().an( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmCmpa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().an( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmMul( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmDiv( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveDataRegisterDirect(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<DataRegisterDirect, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveAddressRegisterIndirect(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<AddressRegisterIndirect, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveAddressRegisterIndirectWithPostIncrement(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<AddressRegisterIndirectWithPostIncrement, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveAddressRegisterIndirectWithPreDecrement(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<AddressRegisterIndirectWithPreDecrement, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveAddressRegisterIndirectWithDisplacement(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<AddressRegisterIndirectWithDisplacement, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveAddressRegisterIndirectWithIndex(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<AddressRegisterIndirectWithIndex, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveAbsoluteShort(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<AbsoluteShort, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMoveAbsoluteLong(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    dasmMove<AbsoluteLong, Mode, Size>(opcode, adr, d);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
inline auto M68Core::dasmMove( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Move ).si( Size ).tab().ea().sep();

    prepareEaForDasm<Inst, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmMoveA( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep();

    prepareEaForDasm<AddressRegisterDirect, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmArithmeticI( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab();
    uint32_t ext = clip<Size>( peekInc<Size>( adr, d ) );
    d.immS( sign<Size>( ext ) ).sep();
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmArithmeticQ( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t operand = (opcode >> 9) & 7;
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().immU( operand ? operand : 8 ).sep().ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmMoveQ( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().immS( sign<Byte>( opcode & 0xff ) ).sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmBcc( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t _adr = adr + 2;
    _adr += (Size == Word ? (int16_t) peekInc<Word>( adr, d ) : (int8_t) (opcode & 0xff));

    d.Ins( Inst ).sis( Size ).tab().hex( _adr );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmBsr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    dasmBcc<Inst, Size>( opcode, adr, d );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmDbcc( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t _adr = adr + 2;
    _adr += (int16_t) peekInc<Word>( adr, d );
    d.Ins( Inst ).tab().dn( opcode & 7 ).sep().hex( _adr );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmJmp( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmJsr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmLea( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea().sep().an( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmPea( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmMovemToReg( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().regList( ext );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmMovemToEa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
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
auto M68Core::dasmArithmeticX( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( opcode & 7 ).sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmArithmeticBCD( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( opcode & 7 ).sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmArithmeticXEa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<AddressRegisterIndirectWithPreDecrement, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep();
    prepareEaForDasm<AddressRegisterIndirectWithPreDecrement, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmCmpm( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<AddressRegisterIndirectWithPostIncrement, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep();
    prepareEaForDasm<AddressRegisterIndirectWithPostIncrement, Size>( adr, (opcode >> 9) & 7, d );
    d.ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmCcr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = clip<Size>( peekInc<Size>( adr, d ) );

    d.Ins( Inst ).si( Size ).tab().immS( sign<Size>( ext ) ).sep().ccr();
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmSr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = clip<Size>( peekInc<Size>( adr, d ) );

    d.Ins( Inst ).si( Size ).tab().immS( sign<Size>( ext ) ).sep().sr();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmChk( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmChk2( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );

    if (ext & 0x800) {
        d.Ins( Inst ).si( Size ).tab().ea().sep().rn( (opcode >> 12) & 0xf );
    } else {
        d.Ins( Cmp2 ).si( Size ).tab().ea().sep().rn( (opcode >> 12) & 0xf );
    }
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmMoveFromSr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().sr().sep().ea();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmMoveToSr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().sr();
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size>
auto M68Core::dasmMoveToCcr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    prepareEaForDasm<Mode, Byte>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().ea().sep().ccr();
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmExgDxDy( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().dn( (opcode >> 9) & 7 ).sep().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmExgAxAy( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().an( (opcode >> 9) & 7 ).sep().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmExgAxDy( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().dn( (opcode >> 9) & 7 ).sep().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmExt( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmExtb( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmMoveUspAn( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().usp().sep().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmMoveAnUsp( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().an( opcode & 7 ).sep().usp();
}

auto M68Core::dasmNop( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Nop );
}

auto M68Core::dasmReset( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Reset );
}

auto M68Core::dasmRtd( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = peekInc<Word>( adr, d );
    d.Ins( Rtd ).tab().immS( sign<Word>( ext ) );
}

auto M68Core::dasmRte( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Rte );
}

auto M68Core::dasmRtr( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Rtr );
}

auto M68Core::dasmRts( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Rts );
}

auto M68Core::dasmStop( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = peekInc<Word>( adr, d );
    d.Ins( _Stop ).tab().immS( sign<Word>( ext ) );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmSwap( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().dn( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmTrap( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().immU( opcode & 15 );
}

auto M68Core::dasmTrapv( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Trapv );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmLink( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    uint32_t ext = peekInc<Size>( adr, d );
    d.Ins( Inst ).tab().an( opcode & 7 ).sep().immS( sign<Size>( ext ) );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmUnlink( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().an( opcode & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmMovepReg( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab();
    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, opcode & 7, d );
    d.ea().sep().dn( (opcode >> 9) & 7 );
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmMovepEa( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).si( Size ).tab();
    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, opcode & 7, d );
    d.dn( (opcode >> 9) & 7 ).sep().ea();
}

template<uint8_t Inst, uint8_t Size>
auto M68Core::dasmBkpt( uint16_t opcode, uint32_t& adr, DasmHandler& d ) -> void {
    d.Ins( Inst ).tab().immU( opcode & 7 );
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmCas(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).si( Size ).tab().dn(ext & 7).sep().dn( (ext >> 6) & 7 ).sep().ea();
}

template<uint8_t Inst, uint8_t Size> auto M68Core::dasmCas2(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    int dc1 = ext & 7;
    int du1 = (ext >> 6) & 7;
    int rn1 = (ext >> 12) & 15;
    ext = peekInc<Word>( adr, d );
    int dc2 = ext & 7;
    int du2 = (ext >> 6) & 7;
    int rn2 = (ext >> 12) & 15;

    d.Ins( Inst ).si( Size ).tab().dn(dc1).col().dn(dc2).sep().dn(du1).col().dn(du2).sep();
    d.rnBrackets( rn1 ).col().rnBrackets( rn2 );
}

template<uint8_t Inst, uint8_t Size> auto M68Core::dasmPackDn(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    d.Ins( Inst ).tab().dn(opcode & 7).sep().dn((opcode >> 9) & 7).immU( ext );
}

template<uint8_t Inst, uint8_t Size> auto M68Core::dasmPackPd(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea().sep();

    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, (opcode >> 9) & 7, d );
    d.ea().immU( ext );
}

template<uint8_t Inst, uint8_t Size> auto M68Core::dasmUnpackDn(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    d.Ins( Inst ).tab().dn(opcode & 7).sep().dn((opcode >> 9) & 7).immU( ext );
}

template<uint8_t Inst, uint8_t Size> auto M68Core::dasmUnpackPd(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, opcode & 7, d );
    d.Ins( Inst ).tab().ea().sep();

    prepareEaForDasm<AddressRegisterIndirectWithDisplacement, Size>( adr, (opcode >> 9) & 7, d );
    d.ea().immU( ext );
}

template<uint8_t Inst, uint8_t Size> auto M68Core::dasmTrapcc(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext;

    switch (Size) {
        case Byte:
            d.Ins( Inst ).tab();
            break;
        case Word:
            ext = peekInc<Word>( adr, d );
            d.Ins( Inst ).tab().immU( ext );
            break;
        case Long:
            ext = peekInc<Long>( adr, d );
            d.Ins( Inst ).tab().immU( ext );
            break;
    }
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmMull(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );

    d.Ins( ext & 0x800 ? Muls : Mulu ).si(Size).tab().ea().sep();

    if (ext & 0x400)
        d.dn(ext & 7).col();

    d.dn((ext >> 12) & 7);
}

template<uint8_t Inst, uint8_t Mode, uint8_t Size> auto M68Core::dasmDivl(uint16_t opcode, uint32_t& adr, DasmHandler& d) -> void {
    uint16_t ext = peekInc<Word>( adr, d );
    int dr = irc & 7;
    int dq = (irc >> 12) & 7;
    prepareEaForDasm<Mode, Size>( adr, opcode & 7, d );

    d.Ins( ext & 0x800 ? Divs : Divu );

    if (ext & 0x400)
        d.si(Size).tab().ea().sep().dn(dr).col().dn(dq);
    else {
        if (dr == dq)
            d.si(Size).tab().ea().sep().dn(dr);
        else {
            d.str.append( "l" );
            d.si(Size).tab().ea().sep().dn(dr).col().dn(dq);
        }
    }
}

}
