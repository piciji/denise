
template<uint8_t Size, uint8_t Mode, bool dynamic> auto M68020::opImmShift(uint8_t shift, DataRegister modify) -> void {
	prefetch();
	uint32_t result = this->shift<Size, Mode>( Base::read<Size>(modify), shift );
	
	uint8_t cycles = 0;
	switch(Mode) {
		case Asl:	cycles = 8; break;
		case Asr:	cycles = 6; break;
		case Lsl:	
		case Lsr:	cycles = 4 + (dynamic ? 2 : 0); break;
		case Rol:	
		case Ror:	cycles = 8; break;
		case Roxl:	
		case Roxr:	cycles = 12; break;
	}
	
	addSequencer(cycles, true);	
	Base::write<Size>(modify, result);
}

template<uint8_t Size, uint8_t Mode> auto M68020::opRegShift(DataRegister shift, DataRegister modify) -> void {	
	opImmShift<Size, Mode, true>( Base::read<Long>(shift) & 63, modify );	
}

template<uint8_t Mode> auto M68020::opEaShift(EffectiveAddress modify) -> void {
	uint32_t result = this->shift<Word, Mode>( read<Word>(modify), 1 );		
	prefetch();
	
	uint8_t cycles = 0;
	switch(Mode) {
		case Asl:	cycles = 3; break;
		case Asr:	cycles = 2; break;
		case Lsl:	
		case Lsr:	cycles = 2; break;
		case Rol:	
		case Ror:	cycles = 4; break;
		case Roxl:	
		case Roxr:	cycles = 2; break;
	}	
	addSequencer(cycles);		
	write<Word>(modify, result);
	syncToBus();
}

template<uint8_t Size, uint8_t Mode> auto M68020::opBit(EffectiveAddress modify, DataRegister bitreg) -> void {
	uint8_t bit = Size == Long ? ctx->d[ bitreg.pos ] & 31 : ctx->d[ bitreg.pos ] & 7;		

	uint32_t data = read<Size>(modify);
	data = this->bit<Mode>(data, bit);	
    prefetch();	

    if (modify.mode == DataRegisterDirect) addSequencer(4);
	else addSequencer(1);
    if (Mode != Btst) write<Size>(modify, data);
}

template<uint8_t Size, uint8_t Mode> auto M68020::opImmBit(EffectiveAddress modify) -> void {	
	uint8_t bit = Size == Long ? ctx->irc & 31 : ctx->irc & 7;
	readExtensionWord();

	uint32_t data = read<Size>(modify);
	data = this->bit<Mode>(data, bit);
    prefetch();	

	if (modify.mode == DataRegisterDirect) addSequencer(4);
	else addSequencer(1);
    if (Mode != Btst) write<Size>(modify, data);
}

template<uint8_t Size> auto M68020::opClr(EffectiveAddress modify) -> void {	
	
}

auto M68020::opNbcd(EffectiveAddress modify) -> void {
	
}

template<uint8_t Size, bool Extend> auto M68020::opNeg(EffectiveAddress modify) -> void {
	
}

template<uint8_t size> auto M68020::opNot(EffectiveAddress modify) -> void {
	
}

auto M68020::opScc(EffectiveAddress modify, uint8_t cc) -> void {
	
}

auto M68020::opTas(EffectiveAddress modify) -> void {
	
}

template<uint8_t Size> auto M68020::opTst(EffectiveAddress modify) -> void {
	
}

template<uint8_t Size, uint8_t Mode> auto M68020::opArithmetic(DataRegister modify, EffectiveAddress src) -> void {
	
}

template<uint8_t Size, uint8_t Mode> auto M68020::opArithmetic(EffectiveAddress modify, DataRegister src) -> void {	
	
}

template<uint8_t Size> auto M68020::opAdda(AddressRegister modify, EffectiveAddress src) -> void {
	
}

template<uint8_t Size> auto M68020::opCmp(DataRegister dest, EffectiveAddress src) -> void {
	
}

template<uint8_t Size> auto M68020::opCmpa(AddressRegister dest, EffectiveAddress src) -> void {
	
}

template<uint8_t Size> auto M68020::opSuba(AddressRegister modify, EffectiveAddress src) -> void {
	
}

template<uint8_t Size> auto M68020::opEor(EffectiveAddress modify, DataRegister src) -> void {
	
}

auto M68020::opMulu(DataRegister modify, EffectiveAddress src) -> void {
	
}

auto M68020::opMuls(DataRegister modify, EffectiveAddress src) -> void {
	
}

auto M68020::opDivu(DataRegister modify, EffectiveAddress src) -> void {
	
}

auto M68020::opDivs(DataRegister modify, EffectiveAddress src) -> void {
	
}

template<uint8_t Size, uint8_t specialCase> auto M68020::opMove(EffectiveAddress src, EffectiveAddress dest) -> void {
	
}

template<uint8_t Size> auto M68020::opMovePreDec(EffectiveAddress src, EffectiveAddress dest) -> void {
	
}

template<uint8_t Size> auto M68020::opMovea(EffectiveAddress src, AddressRegister modify) -> void {
	
}

template<uint8_t Size, uint8_t Mode> auto M68020::opArithmeticI(EffectiveAddress modify) -> void {
	
}

template<uint8_t Size, uint8_t Mode> auto M68020::opArithmeticQ(EffectiveAddress modify, uint8_t immediate) -> void {
}

template<uint8_t Mode> auto M68020::opArithmeticQ(AddressRegister areg, uint8_t immediate) -> void {	
}

template<uint8_t Size> auto M68020::opCmpi(EffectiveAddress dest) -> void {
	
}

auto M68020::opMoveq(DataRegister modify, uint8_t data) -> void {
	
}

template<uint8_t Size> auto M68020::opBcc(uint8_t cond, uint8_t displacement) -> void {
	
}

template<uint8_t Size> auto M68020::opBra(uint8_t displacement) -> void {
	
}

template<uint8_t Size> auto M68020::opBsr(uint8_t displacement) -> void {
	
}

auto M68020::opDbcc(DataRegister modify, uint8_t cond ) -> void {
	
}

auto M68020::opJmp(EffectiveAddress src) -> void {
	
}

auto M68020::opJsr(EffectiveAddress src) -> void {
	
}

auto M68020::opLea(EffectiveAddress src, AddressRegister modify) -> void {
	
}

auto M68020::opPea(EffectiveAddress src) -> void {
	
}

template<uint8_t Size, bool memToReg> auto M68020::opMovem(EffectiveAddress src) -> void {
	
}

template<uint8_t Size> auto M68020::opMovemPostInc(AddressRegister modify) -> void {
	
}

template<uint8_t Size> auto M68020::opMovemPreDec(AddressRegister modify) -> void {
	
}

template<uint8_t Size, uint8_t Mode> auto M68020::opArithmeticX(DataRegister src, DataRegister dest) -> void {		
	
}

template<uint8_t Size, uint8_t Mode> auto M68020::opArithmeticX(EffectiveAddress src, EffectiveAddress dest) -> void {		
	
}

template<uint8_t Size> auto M68020::opCmpm(EffectiveAddress src, EffectiveAddress dest) -> void {		
	
}

template<uint8_t Mode> auto M68020::opCcr() -> void {
	
}

template<uint8_t Mode> auto M68020::opSr() -> void {
	
}

template<uint8_t Size> auto M68020::opChk(EffectiveAddress src, DataRegister srcD) -> void {
	
}

auto M68020::opMoveFromSr(EffectiveAddress modify) -> void {
	
}

auto M68020::opMoveToCcr(EffectiveAddress modify) -> void {
	
}

auto M68020::opMoveToSr(EffectiveAddress modify) -> void {
	
}

auto M68020::opExg(DataRegister d1, DataRegister d2) -> void {
}

auto M68020::opExg(AddressRegister a1, AddressRegister a2) -> void {
}

auto M68020::opExg(DataRegister d, AddressRegister a) -> void {
}

template<uint8_t Size> auto M68020::opExt(DataRegister modify) -> void {
	
}

template<uint8_t Size> auto M68020::opLink(AddressRegister modify) -> void {
	
}

template<bool toUsp> auto M68020::opMoveUsp(AddressRegister modify) -> void {
	
}

auto M68020::opNop() -> void {
	
}

auto M68020::opReset() -> void {
	
}

auto M68020::opRte() -> void {
}

auto M68020::opRtr() -> void {
}

auto M68020::opRts() -> void {
}

auto M68020::opStop() -> void {
}

auto M68020::opSwap(DataRegister modify) -> void {
}

auto M68020::opTrap(uint8_t vector) -> void {
}

auto M68020::opTrapv() -> void {
}

auto M68020::opUnlink(AddressRegister modify) -> void {
}

template<uint8_t Mode> auto M68020::opMovep(EffectiveAddress ea, DataRegister dataReg) -> void {
}

template<uint8_t Mode> auto M68020::opBitField(EffectiveAddress modify) -> void {
	
}

auto M68020::opCallm(EffectiveAddress modify) -> void {
	
}

template<uint8_t Size> auto M68020::opCas(EffectiveAddress modify) -> void {
	
}
template<uint8_t Size> auto M68020::opCas2() -> void {
	
}

template<uint8_t Size> auto M68020::opChk2(EffectiveAddress modify) -> void {
	
}

auto M68020::opBkpt(uint8_t vector) -> void {
	
}

auto M68020::opDivl(EffectiveAddress src) -> void {
	
}

auto M68020::opMull(EffectiveAddress src) -> void {
	
}


auto M68020::opExtb(DataRegister modify) -> void {
	
}

auto M68020::opMoveFromCcr(EffectiveAddress modify) -> void {
	
}

auto M68020::opPack(DataRegister src, DataRegister dest) -> void {
	
}

auto M68020::opPack(EffectiveAddress src, EffectiveAddress dest) -> void {
	
}

auto M68020::opUnpack(DataRegister src, DataRegister dest) -> void {
	
}

auto M68020::opUnpack(EffectiveAddress src, EffectiveAddress dest) -> void {
	
}

auto M68020::opRtd() -> void {
	
}

auto M68020::opRtm(DataRegister src) -> void {
	
}

auto M68020::opRtm(AddressRegister src) -> void {
	
}

template<uint8_t Mode> auto M68020::opTrapcc(uint8_t cond) -> void {
	
}

template<bool toControl> auto M68020::opMovec() -> void {
	
}

template<uint8_t Size> auto M68020::opMoves(EffectiveAddress ea) -> void {
	
}
