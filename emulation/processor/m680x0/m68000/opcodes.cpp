
//asl, asr, lsl, lsr, rol, ror, roxl, roxr								-> shift/rotate instructions
//bchg, bclr, bset, btst												-> bit manipulation instructions
//clr, nbcd, neg, negx, not, scc, tas, tst								-> single operand instructions
//add, adda, and, cmp, cmpa, sub, suba, or, eor, mulu, muls, divu, divs -> standard instructions
//move, movea															-> move instructions
//addi, addq, andi, cmpi, eori, ori, moveq, subi, subq					-> immediate instructions
//bcc, bra, bsr, dbcc													-> specificational instructions
//jmp, jsr, lea, pea, movem												-> misc 1 instructions
//addx, cmpm, subx, abcd, sbcd											-> multiprecision instructions
//andiccr, andisr, chk, eorisr, eoriccr, orisr, oriccr, move from sr
//move to ccr, move to sr, exg, ext, link, moveusp, nop, reset
//rte, rtr, rts, stop, swap, trap, trapv, unlk							-> misc 2 instructions
//movep																	-> peripheral instruction


template<uint8_t Size, uint8_t Mode> auto M68000::opImmShift(uint8_t shift, DataRegister modify) -> void {
	prefetch();
	uint32_t result = this->shift<Size, Mode>( Base::read<Size>(modify), shift );	
	/* no need to sync each single shift because of external processes can not access cpu internals
	 shared memory only, so all bus operations needs carefully syncing in small steps */
	ctx->sync(2 + (Size == Long ? 2 : 0) + shift * 2);
	Base::write<Size>(modify, result);	
}

template<uint8_t Size, uint8_t Mode> auto M68000::opRegShift(DataRegister shift, DataRegister modify) -> void {	
	opImmShift<Size, Mode>( Base::read<Long>(shift) & 63, modify );	
}

template<uint8_t Mode> auto M68000::opEaShift(EffectiveAddress modify) -> void {
	uint32_t result = this->shift<Word, Mode>( read<Word>(modify), 1 );		
	prefetch(false);
	write<Word>(modify, result);
}

template<uint8_t Size, uint8_t Mode> auto M68000::opBit(EffectiveAddress modify, DataRegister bitreg) -> void {
	uint8_t bit = Size == Long ? ctx->d[ bitreg.pos ] & 31 : ctx->d[ bitreg.pos ] & 7;		

	uint32_t data = read<Size>(modify);
	data = this->bit<Mode>(data, bit);	
    prefetch(Mode == Btst || modify.mode == DataRegisterDirect);	

    if (modify.mode == DataRegisterDirect) cyclesBit<Mode>(bit);
    if (Mode != Btst) write<Size>(modify, data);
}

template<uint8_t Size, uint8_t Mode> auto M68000::opImmBit(EffectiveAddress modify) -> void {	
	uint8_t bit = Size == Long ? ctx->irc & 31 : ctx->irc & 7;
	readExtensionWord();

	uint32_t data = read<Size>(modify);
	data = this->bit<Mode>(data, bit);
    prefetch(Mode == Btst || modify.mode == DataRegisterDirect);	

    if (modify.mode == DataRegisterDirect) cyclesBit<Mode>(bit);
    if (Mode != Btst) write<Size>(modify, data);
}

template<uint8_t Size> auto M68000::opClr(EffectiveAddress modify) -> void {	
	read<Size>(modify); //dummy read
	ctx->z = 1;
	ctx->n = ctx->v = ctx->c = 0;
	
	prefetch( modify.registerMode() );
	if ( modify.registerMode() && Size == Long) ctx->sync( 2 );
	write<Size>(modify, 0);
}

auto M68000::opNbcd(EffectiveAddress modify) -> void {
	uint8_t data = read<Byte>(modify);
	prefetch( modify.registerMode() );
	data = this->sbcd(data, 0);
	if ( modify.registerMode() ) ctx->sync( 2 );
	write<Byte>(modify, data);
}

template<uint8_t Size, bool Extend> auto M68000::opNeg(EffectiveAddress modify) -> void {
	uint32_t data = read<Size>(modify);
	data = this->sub<Size, Extend>(data, 0);
	prefetch( modify.registerMode() );
	if ( modify.registerMode() && Size == Long ) ctx->sync( 2 );
	write<Size>(modify, data);
}

template<uint8_t Size> auto M68000::opNot(EffectiveAddress modify) -> void {
	uint32_t data = read<Size>(modify);
	data = this->_not<Size>(data);
	prefetch( modify.registerMode() );
	if ( modify.registerMode() && Size == Long ) ctx->sync( 2 );
	write<Size>(modify, data);	
}

auto M68000::opScc(EffectiveAddress modify, uint8_t cc) -> void {
	read<Byte>(modify);
	uint8_t data = this->testCondition(cc) ? 0xff : 0;
	prefetch( modify.registerMode() );
	if ( data && modify.registerMode() ) ctx->sync( 2 );
	write<Byte>(modify, data);		
}

auto M68000::opTas(EffectiveAddress modify) -> void {
	state.rmw = true;
	uint8_t data = read<Byte>(modify);
	
	this->defaultFlags<Byte>(data);
	data |= 0x80;
	
	if ( !modify.registerMode() ) {
		ctx->tasWrite();
		ctx->sync( 2 );		
	}
	write<Byte>(modify, data, false);
	state.rmw = false;
	prefetch(true);
}

template<uint8_t Size> auto M68000::opTst(EffectiveAddress modify) -> void {
	uint32_t data = read<Size>(modify);
	this->defaultFlags<Size>(data);
	prefetch(true);
}

template<uint8_t Size, uint8_t Mode> auto M68000::opArithmetic(DataRegister modify, EffectiveAddress src) -> void {	
	uint32_t data = read<Size>(src);
	prefetch(true);
	
	if (Size == Long) {
		ctx->sync( 2 );
		if (!optimized && src.directMode()) ctx->sync( 2 );
	}
	
	Base::write<Size>(modify, this->arithmetic<Size, Mode>(data, Base::read<Size>(modify) ) );
}

template<uint8_t Size, uint8_t Mode> auto M68000::opArithmetic(EffectiveAddress modify, DataRegister src) -> void {	
	uint32_t data = read<Size>(modify);	
	prefetch(false);
	
	write<Size>(modify, this->arithmetic<Size, Mode>(Base::read<Size>(src), data ) );		
}

template<uint8_t Size> auto M68000::opAdda(AddressRegister modify, EffectiveAddress src) -> void {
	uint32_t data = read<Size>(src);
	if (Size == Word) data = (int16_t)data;
	prefetch(true);
	ctx->sync( 2 );
	if (Size == Word || ( !optimized && src.directMode()) ) ctx->sync( 2 );
	Base::write(modify, data + Base::read<Long>(modify));
}

template<uint8_t Size> auto M68000::opCmp(DataRegister dest, EffectiveAddress src) -> void {
	uint32_t data = read<Size>(src);
	prefetch(true);	
	if (Size == Long) ctx->sync( 2 );
	this->cmp<Size>(Base::read<Size>(dest), data);
}

template<uint8_t Size> auto M68000::opCmpa(AddressRegister dest, EffectiveAddress src) -> void {
	uint32_t data = read<Size>(src);
	if (Size == Word) data = (int16_t)data;
	this->cmp<Long>(Base::read<Size>(dest), data);
	prefetch(true);
	ctx->sync( 2 );
}

template<uint8_t Size> auto M68000::opSuba(AddressRegister modify, EffectiveAddress src) -> void {
	uint32_t data = read<Size>(src);
	if (Size == Word) data = (int16_t)data;
	prefetch(true);
	ctx->sync( 2 );
	if (Size == Word || src.registerMode() || src.immediateMode()) ctx->sync( 2 );
	if (Size == Word || ( !optimized && src.directMode()) ) ctx->sync( 2 );

	Base::write(modify, Base::read<Long>(modify) - data);	
}

template<uint8_t Size> auto M68000::opEor(EffectiveAddress modify, DataRegister src) -> void {
	uint32_t data = read<Size>(modify);	
	prefetch( modify.registerMode() );
	if (Size == Long && modify.registerMode()) ctx->sync( !optimized ? 4 : 2 );
	write<Size>(modify, this->_eor<Size>(Base::read<Size>(src), data ) );		
}

auto M68000::opMulu(DataRegister modify, EffectiveAddress src) -> void {
	uint16_t data = read<Word>(src);		
	prefetch(true);
	
	ctx->sync( !optimized ? 34 : 4 );
    while(data) {
        if (data & 1) ctx->sync(2);
        data >>= 1;
    }
	
	uint32_t result = data * Base::read<Word>(modify);
	Base::defaultFlags<Long>(result);
	Base::write<Long>(modify, result);
}

auto M68000::opMuls(DataRegister modify, EffectiveAddress src) -> void {
	uint16_t data = read<Word>(src);	
	prefetch(true);
	uint32_t result = (int16_t)data * (int16_t)Base::read<Word>(modify);
	
	ctx->sync( !optimized ? 34 : 6 );
	data = ( (data << 1) ^ data ) & 0xffff;
    while(data) {
        if (data & 1) ctx->sync(2);
        data >>= 1;
    }	
	
	Base::defaultFlags<Long>(result);
	Base::write<Long>(modify, result);
}

auto M68000::opDivu(DataRegister modify, EffectiveAddress src) -> void {
	uint16_t data = read<Word>(src);
	ctx->n = ctx->v = ctx->z = ctx->c = 0;
	if (data == 0) {
		ctx->sync(8);
		return trapException( 5 );
	}
	
	uint32_t result = Base::read<Long>(modify) / data;
	uint16_t remainder = Base::read<Long>(modify) % data;
	
	ctx->sync( cyclesDivu( Base::read<Long>(modify), data ) - 4 ); //prefetch happens simultaneous
	prefetch(true);
	
	if (result > 0xffff) {
        ctx->v = ctx->n = 1;
        return;
    }
	Base::defaultFlags<Word>(result);
	Base::write<Long>(modify, (result & 0xffff) | (remainder << 16) );
}

auto M68000::opDivs(DataRegister modify, EffectiveAddress src) -> void {
	//todo check undefined flags for trap and overflow
	uint16_t data = read<Word>(src);
	ctx->n = ctx->v = ctx->z = ctx->c = 0;
	
	if (data == 0) {
		ctx->sync(8);
		return trapException( 5 );
	}
	uint32_t dividend = Base::read<Long>(modify);
	
	ctx->sync( cyclesDivs( (int32_t)dividend, (int16_t)data ) - 4 ); //prefetch happens simultaneous
	prefetch(true);
	
	if (dividend == 0x80000000 && data == -1) {
		/** some compilers produce binaries which crash for this special case,
		 * result doesn't fit in int32, better we prevent division */
		ctx->v = ctx->n = 1;
		return;
	}
	
	int32_t result = (int32_t)dividend / (int16_t)data;
	uint16_t remainder = (int32_t)dividend % (int16_t)data;
	/** following line shouldn't be needed, host cpu calculates signed remainder
	 * don't know if this is true for all architectures */
	if ( ((int16_t)remainder < 0) != ((int32_t)dividend < 0) ) remainder = -remainder;
	
	if ((result & 0xffff8000) != 0 && (result & 0xffff8000) != 0xffff8000) {
        ctx->v = ctx->n = 1;
        return;
    }
	Base::defaultFlags<Word>(result);
	Base::write<Long>(modify, (result & 0xffff) | (remainder << 16) );
}

template<uint8_t Size, uint8_t specialCase> auto M68000::opMove(EffectiveAddress src, EffectiveAddress dest) -> void {		
	uint32_t data = read<Size>(src);
		
	dest.address = fetch<Size, specialCase>(dest);
	
	Base::defaultFlags<Size>(data);
	
	write<Size>(dest, data, false);
	
	if (specialCase == NoLastPrefetch) readExtensionWord();
	
	prefetch(true);
}

template<uint8_t Size> auto M68000::opMovePreDec(EffectiveAddress src, EffectiveAddress dest) -> void {
	uint32_t data = read<Size>(src);
		
	dest.address = fetch<Size, NoCyclesPreDec>(dest);
	
	Base::defaultFlags<Size>(data);

	prefetch(false);
	
	write<Size>(dest, data);
}

template<uint8_t Size> auto M68000::opMovea(EffectiveAddress src, AddressRegister modify) -> void {
	uint32_t data = read<Size>(src);
	
	Base::write(modify, Size == Word ? (int16_t)data : data);
	
	prefetch(true);
}

template<uint8_t Size, uint8_t Mode> auto M68000::opArithmeticI(EffectiveAddress modify) -> void {
	EffectiveAddress src{7, 4};
	uint32_t immediate = read<Size>(src);
	uint32_t data = read<Size>(modify);	

	prefetch(modify.registerMode());
	
	if (Size == Long && modify.registerMode()) ctx->sync( !optimized ? 4 : 2 );
	write<Size>(modify, this->arithmetic<Size, Mode>(immediate, data) );
}

template<uint8_t Size, uint8_t Mode> auto M68000::opArithmeticQ(EffectiveAddress modify, uint8_t immediate) -> void {
	uint32_t data = read<Size>(modify);
	
	prefetch(modify.registerMode());
	
	if (Size == Long && modify.registerMode()) ctx->sync( 4 );
	
	write<Size>(modify, this->arithmetic<Size, Mode>(immediate, data) );
}

template<uint8_t Mode> auto M68000::opArithmeticQ(AddressRegister areg, uint8_t immediate) -> void {	
	//todo recheck if word same like long
	prefetch(true);

	ctx->sync( 4 );
	
	if (Mode == Add)
		Base::write(areg, Base::read<Long>(areg) + immediate );	
	else if (Mode == Sub)
		Base::write(areg, Base::read<Long>(areg) - immediate );
}

template<uint8_t Size> auto M68000::opCmpi(EffectiveAddress dest) -> void {
	EffectiveAddress src{7, 4};
	uint32_t immediate = read<Size>(src);
	uint32_t data = read<Size>(dest);	
	
	prefetch(true);
	if (!optimized && Size == Long && dest.registerMode()) ctx->sync( 2 );
	this->cmp<Size>(immediate, data);
}

auto M68000::opMoveq(DataRegister modify, uint8_t data) -> void {
	prefetch(true);
	Base::defaultFlags<Byte>( data );
	Base::write<Long>(modify, (int32_t)(int8_t)data);	
}

template<uint8_t Size> auto M68000::opBcc(uint8_t cond, uint8_t displacement) -> void {
	//todo check branch to next instruction
	ctx->sync(2);
		
	if (testCondition(cond)) {
		ctx->pc = ctx->prefetchCounterLast
				+ (Size == Word ? (int16_t)ctx->irc : (int8_t)displacement);

		fullPrefetch();
	} else {
		if (!optimized) ctx->sync(2);
		if (Size == Word) readExtensionWord();
		prefetch(true);
	}
}

template<uint8_t Size> auto M68000::opBra(uint8_t displacement) -> void {
	ctx->sync(2);
	ctx->pc = ctx->prefetchCounterLast
			+ (Size == Word ? (int16_t)ctx->irc : (int8_t)displacement);

	fullPrefetch();
}

template<uint8_t Size> auto M68000::opBsr(uint8_t displacement) -> void {
	//todo check pc on adr error
	ctx->sync(2);
	writeStack( Size == Word ? ctx->prefetchCounter : ctx->prefetchCounterLast );
	
	ctx->pc = ctx->prefetchCounterLast
			+ (Size == Word ? (int16_t)ctx->irc : (int8_t)displacement);

	fullPrefetch();
}

auto M68000::opDbcc(DataRegister modify, uint8_t cond ) -> void {
	/* takes only 2 cycles till prefetching from branch
	 * cpu seems to assume branch is taken, means it needs additional cycles
	 * if branch is not taken
	 */
	ctx->sync(2);		
	uint32_t memPC = ctx->prefetchCounter;
	ctx->prefetchCounter = ctx->prefetchCounterLast + (int16_t)ctx->irc;

	if (!testCondition(cond)) {		
		Base::write<Word>( modify, Base::read<Word>(modify) - 1 );				
		//we begin the prefetch while decrementing and checking if branch is taken
		readExtensionWord();
		//so check should be finish now
		if((int16_t)Base::read<Word>(modify) != -1 ) {
			prefetch(true);	
			return;
		}		
		//damn the prefetch of this branch was useless, we need next opcode		
	} else {
		ctx->sync(2);
		//damn we need 2 cycles to revert
	}
	ctx->prefetchCounter = memPC;
	
	readExtensionWord();
	prefetch(true);	
}

auto M68000::opJmp(EffectiveAddress src) -> void {
	uint32_t adr = fetch<Long, NoLastPrefetch>(src);
	cyclesJmp(src.mode);	
	ctx->pc = adr;
	fullPrefetch();
}

auto M68000::opJsr(EffectiveAddress src) -> void {
	uint32_t adr = fetch<Long, NoLastPrefetch>(src);
	cyclesJmp(src.mode);
	uint32_t memPC = ctx->prefetchCounterLast;
	ctx->prefetchCounterLast = ctx->prefetchCounter = ctx->pc = adr;
	readExtensionWord();
	writeStack( memPC );
	prefetch( true );		
}

auto M68000::opLea(EffectiveAddress src, AddressRegister modify) -> void {
	Base::write(modify, fetch<Long>(src) );
	if ( src.indexMode() ) ctx->sync(2);	
	prefetch(true);
}

auto M68000::opPea(EffectiveAddress src) -> void {
	uint32_t adr = fetch<Long>(src);
	if ( src.indexMode() ) ctx->sync(2);
	
	if (src.absMode()) {
		writeStack( adr, false );
		prefetch(true);
	} else {
		prefetch(false);
		writeStack( adr, true );
	}	
}

template<uint8_t Size, bool memToReg> auto M68000::opMovem(EffectiveAddress src) -> void {
	uint8_t byteCount = Size == Word ? 2 : 4;
	
	if (!memToReg && Size == Long && src.indexMode()) ctx->sync(2);
	
	uint32_t adr = fetch<Size, NoCyclesD8>(src);
	
	if ( (memToReg || Size == Word) && src.indexMode()) ctx->sync(2);
	
	uint16_t mask = ctx->irc;
	
	readExtensionWord();
	
	state.data = !src.pcMode();
	if (memToReg && Size == Long) read<Word>(adr);
	
	for(auto i : range(16)) {
		if (!mask) break;
		
		bool isBitSet = mask & 1;
        mask >>= 1;
		
		if (!isBitSet) continue;
		
		uint32_t* reg = i > 7 ? &ctx->a[i-8] : &ctx->d[i];
		
		if (memToReg) *reg = Size == Word ? (int16_t)read<Word>(adr) : read<Long>(adr);
		else write<Size>(adr, *reg);

		adr += byteCount;
	}
	
	if (memToReg && Size == Word) read<Word>(adr); //dummy read, but can cause a bus error
	prefetch(true);
}

template<uint8_t Size> auto M68000::opMovemPostInc(AddressRegister modify) -> void {
	uint8_t byteCount = Size == Word ? 2 : 4;
	
	uint32_t adr = Base::read( modify );
	
	uint16_t mask = ctx->irc;
	
	readExtensionWord();
	
	state.data = true;
	if (Size == Long) read<Word>(adr);
	
	for(auto i : range(16)) {
		if (!mask) break;
		
		bool isBitSet = mask & 1;
        mask >>= 1;
		
		if (!isBitSet) continue;
		
		uint32_t* reg = i > 7 ? &ctx->a[i-8] : &ctx->d[i];
		
		*reg = Size == Word ? (int16_t)read<Word>(adr) : read<Long>(adr);

		adr += byteCount;
		
		if (!mask) Base::write(modify, adr);
	}
	
	if (Size == Word) read<Word>(adr); //dummy read, but can cause a bus error
	prefetch(true);
}

template<uint8_t Size> auto M68000::opMovemPreDec(AddressRegister modify) -> void {
	uint8_t byteCount = Size == Word ? 2 : 4;
	
	uint32_t adr = Base::read( modify );
	adr -= byteCount;
	
	uint16_t mask = ctx->irc;
	
	readExtensionWord();
	
	state.data = true;
	
	for(auto i : range(16)) {
		if (!mask) break;
		
		bool isBitSet = mask & 1;
        mask >>= 1;
		
		if (!isBitSet) continue;
		
		uint32_t* reg = i > 7 ? &ctx->d[ ~(i-8) & 7 ] : &ctx->a[ ~i & 7 ];		
		
		write<Size>(adr, *reg);		
		
		if (!mask) Base::write(modify, adr);
		
		adr -= byteCount;
	}
	
	prefetch(true);
}

template<uint8_t Size, uint8_t Mode> auto M68000::opArithmeticX(DataRegister src, DataRegister dest) -> void {		
	prefetch(true);	
	if (Size == Long) ctx->sync(!optimized ? 4 : 2);
	if (Mode == Abcd || Mode == Sbcd) ctx->sync(2);
	
	Base::write<Size>(dest, this->arithmeticX<Size, Mode>( Base::read<Size>(src), Base::read<Size>(dest) ) );
}

template<uint8_t Size, uint8_t Mode> auto M68000::opArithmeticX(EffectiveAddress src, EffectiveAddress dest) -> void {		
	uint32_t dataSrc = read<Size>(src);
	uint32_t data = read<Size, NoCyclesPreDec>(dest);
	
	data = this->arithmeticX<Size, Mode>(dataSrc, data);
	
	prefetch(false);	
	write<Size>(dest, data);
}

template<uint8_t Size> auto M68000::opCmpm(EffectiveAddress src, EffectiveAddress dest) -> void {		
	uint32_t dataSrc = read<Size>(src);
	uint32_t data = read<Size>(dest);
	
	this->cmp<Size>(dataSrc, data);
		
	prefetch(true);	
}

template<uint8_t Mode> auto M68000::opCcr() -> void {
	uint8_t data = ctx->irc;
	readExtensionWord();
	ctx->sync(8);
	this->ccr<Mode>( data );
	
	if (!optimized) {		
		ctx->pc = ctx->prefetchCounterLast;
		fullPrefetch();
	} else {
		prefetch(true);
	}
}

template<uint8_t Mode> auto M68000::opSr() -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}	
	uint16_t data = ctx->irc;
	readExtensionWord();
	ctx->sync(8);
	this->sr<Mode>( data );
	
	if (!optimized) {		
		ctx->pc = ctx->prefetchCounterLast;
		fullPrefetch();
	} else {
		prefetch(true);
	}	
}

template<uint8_t Size> auto M68000::opChk(EffectiveAddress src, DataRegister srcD) -> void {
	int16_t data = (int16_t)read<Word>(src);
	int16_t dataD = (int16_t)Base::read<Word>(srcD);
		
	prefetch(true);
	ctx->sync( !optimized ? 4 : 2 );
	
	ctx->z = this->zero<Word>( dataD );
	ctx->v = 0;
	ctx->c = 0;
	ctx->n = 0;
	
	if ( dataD > data) {
		ctx->sync(4);
		trapException( 6 );
		return;
	}
	ctx->sync(2);
	
	if (dataD < 0) {
		ctx->sync(4);
		ctx->n = 1;
		trapException( 6 );
	}
}

auto M68000::opMoveFromSr(EffectiveAddress modify) -> void {
	read<Word>( modify );	
	prefetch( modify.registerMode() );
	if ( modify.registerMode() ) ctx->sync(2);
	write<Word>(modify, getSR(), true);
}

auto M68000::opMoveToCcr(EffectiveAddress modify) -> void {
	uint16_t data = read<Word>( modify );	
	ctx->sync(4);
	Base::setCCR( data & 0xff );
	ctx->pc = ctx->prefetchCounterLast;
	fullPrefetch();
}

auto M68000::opMoveToSr(EffectiveAddress modify) -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	uint16_t data = read<Word>( modify );	
	ctx->sync(4);
	setSR( data );
	ctx->pc = ctx->prefetchCounterLast;
	fullPrefetch();
}

auto M68000::opExg(DataRegister d1, DataRegister d2) -> void {
	prefetch(true);
	ctx->sync(2);
	uint32_t temp = ctx->d[d1.pos];
	ctx->d[d1.pos] = ctx->d[d2.pos];
	ctx->d[d2.pos] = temp;
}

auto M68000::opExg(AddressRegister a1, AddressRegister a2) -> void {
	prefetch(true);
	ctx->sync(2);
	uint32_t temp = ctx->a[a1.pos];
	ctx->a[a1.pos] = ctx->a[a2.pos];
	ctx->a[a2.pos] = temp;
}

auto M68000::opExg(DataRegister d, AddressRegister a) -> void {
	prefetch(true);
	ctx->sync(2);
	uint32_t temp = ctx->a[a.pos];
	ctx->a[a.pos] = ctx->d[d.pos];
	ctx->d[d.pos] = temp;
}

template<uint8_t Size> auto M68000::opExt(DataRegister modify) -> void {
	prefetch(true);
	uint32_t data;
	
	if (Size == Long) {
		data = Base::read<Word>(modify);
		data |= data & 0x8000 ? 0xffff << 16 : 0;
		Base::write<Long>(modify, data );		
	} else {
		data = Base::read<Byte>(modify);
		data |= data & 0x80 ? 0xff << 8 : 0;
		Base::write<Word>(modify, data );		
	}
	this->defaultFlags<Size>(data);
}

template<uint8_t Size> auto M68000::opLink(AddressRegister modify) -> void {
	int16_t displacement = (int16_t)ctx->irc;
	readExtensionWord();
	writeStack( Base::read<Long>(modify) - (modify.pos == 7 ? 4 : 0), false );
	prefetch(true);
	Base::write(modify, ctx->a[7]);
	ctx->a[7] += (int32_t)displacement;
}

template<bool toUsp> auto M68000::opMoveUsp(AddressRegister modify) -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	prefetch(true);
	
	if (toUsp)
		Base::write(modify, ctx->usp);
	else
		ctx->usp = Base::read<Long>(modify);
}

auto M68000::opNop() -> void {
	prefetch(true);
}

auto M68000::opReset() -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	ctx->sync(128);
	ctx->resetInstruction();
	prefetch(true);
}

auto M68000::opRte() -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	state.data = true;
	uint16_t sr = read<Word>( ctx->a[7] );
	ctx->a[7] += 2;
	ctx->pc = read<Long>( ctx->a[7] );
	ctx->a[7] += 4;
	fullPrefetch();
	setSR( sr );
}

auto M68000::opRtr() -> void {
	state.data = true;
	setCCR( read<Word>( ctx->a[7] ) & 0xff );
	ctx->a[7] += 2;
	ctx->pc = read<Long>( ctx->a[7] );
	ctx->a[7] += 4;
	fullPrefetch();
}

auto M68000::opRts() -> void {
	state.data = true;
	ctx->pc = read<Long>( ctx->a[7] );
	ctx->a[7] += 4;
	fullPrefetch();
}

auto M68000::opStop() -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	ctx->sync(2);
	setSR( ctx->irc | (1 << 13) );
	ctx->stop = true;
	ctx->pc = ctx->prefetchCounter;
	sampleIrq();
	ctx->sync(2);
}

auto M68000::opSwap(DataRegister modify) -> void {
	prefetch(true);
	uint32_t data = Base::read<Long>(modify);
	data = (data >> 16) | (data & 0xffff) << 16;	
	Base::write<Long>( modify, data );
	this->defaultFlags<Long>(data);
}

auto M68000::opTrap(uint8_t vector) -> void {
	ctx->sync(4);
	trapException( (vector & 0xf) + 32 );
}

auto M68000::opTrapv() -> void {
	prefetch(true);
	if (ctx->v) {
		trapException(7);
	}
}

auto M68000::opUnlink(AddressRegister modify) -> void {
	EffectiveAddress src{AddressRegisterIndirect, 7};
	ctx->a[7] = ctx->a[modify.pos];
	ctx->a[modify.pos] = read<Long>( src );
	ctx->a[7] += 4;
	prefetch(true);
}

template<uint8_t Mode> auto M68000::opMovep(EffectiveAddress ea, DataRegister dataReg) -> void {
	ea.address = fetch<Long>(ea);
	uint32_t data;
	state.data = true;
	
	switch(Mode) {
		case 4: //word: mem -> reg
			state.hbTransfer = true;
			data = read<Byte>(ea.address) << 8; ea.address += 2;
			state.hbTransfer = false;
			data |= read<Byte>(ea.address);
			Base::write<Word>(dataReg, data);
			break;
		case 5: //long: mem -> reg
			state.hbTransfer = true;
			data = read<Byte>(ea.address) << 24; ea.address += 2;
			state.hbTransfer = false;
			data |= read<Byte>(ea.address) << 16; ea.address += 2;
			state.hbTransfer = true;
			data |= read<Byte>(ea.address) << 8; ea.address += 2;
			state.hbTransfer = false;
			data |= read<Byte>(ea.address);
			Base::write<Long>(dataReg, data);
			break;
		case 6: //word: reg -> mem
			data = Base::read<Word>(dataReg);
			state.hbTransfer = true;
			write<Byte>(ea.address, data >> 8); ea.address += 2;
			state.hbTransfer = false;
			write<Byte>(ea.address, data & 0xff);
			break;
		case 7: //long: reg -> mem
			data = Base::read<Long>(dataReg);
			state.hbTransfer = true;
			write<Byte>(ea.address, data >> 24); ea.address += 2;
			state.hbTransfer = false;
			write<Byte>(ea.address, data >> 16); ea.address += 2;
			state.hbTransfer = true;
			write<Byte>(ea.address, data >> 8); ea.address += 2;
			state.hbTransfer = false;
			write<Byte>(ea.address, data & 0xff);
			break;
	}
	prefetch(true);
}
