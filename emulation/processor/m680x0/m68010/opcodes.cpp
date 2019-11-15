
auto M68010::opBkpt(uint8_t vector) -> void {
	ctx->sync(3);
	ctx->breakpoint(vector); //breakpoint acknowledge cycle
	ctx->sync(4);
	illegalException(4);
}

auto M68010::opMoveFromCcr(EffectiveAddress modify) -> void {
	modify.address = fetch<Word>( modify );
	noFetchTail<Word>(modify);	
	prefetch( modify.registerMode() );
	write<Word>(modify, getCCR(), true);
}

auto M68010::opMoveFromSr(EffectiveAddress modify) -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	modify.address = fetch<Word>( modify );
	noFetchTail<Word>(modify);
	prefetch( modify.registerMode() );
	write<Word>(modify, getSR(), true);
}

auto M68010::opRtd() -> void {
	state.data = true;
	ctx->pc = read<Long>( ctx->a[7] );
	ctx->a[7] += 4 + (int16_t)ctx->irc;
	fullPrefetch();
}

template<bool toControl> auto M68010::opMovec() -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}

	uint16_t data = ctx->irc;	
	uint8_t regPos = (data >> 12) & 7;	
	uint32_t* reg = data & 0x8000 ? &ctx->a[regPos] : &ctx->d[regPos];
	ctx->sync(2);
	state.data = false;
	
	switch(data & 0xfff) {
		case 0: //sfc
			if (toControl) ctx->sfc = *reg & 7;
			else *reg = ctx->sfc & 7;
			break;
		case 0x1: //dfc
			if (toControl) ctx->dfc = *reg & 7;
			else *reg = ctx->dfc & 7;			
			break;
		case 0x800: //usp
			if (toControl) ctx->usp = *reg;
			else *reg = ctx->usp;
			break;
		case 0x801: //vbr
			if (toControl) ctx->vbr = *reg;
			else *reg = ctx->vbr;
			break;
		default:
			ctx->sync(6);
			illegalException(4);
			return;
	}
	if (!toControl) ctx->sync(2);
	
	readExtensionWord();
	prefetch(true);
}

template<uint8_t Size> auto M68010::opMoves(EffectiveAddress ea) -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	
	uint16_t data = ctx->irc;
	readExtensionWord();
	uint8_t regPos = (data >> 12) & 7;
	
	AddressRegister areg{regPos};
	DataRegister dreg{regPos};
		
	bool toEa = (data >> 11) & 1;
	ea.address = fetch<Size>(ea);

	cyclesMoves(ea.mode);
	
	if (!toEa) {
		ea.calculated = true;
		state.overrideWithAlternateFC = true;
		state.alternateFC = ctx->sfc;
		uint32_t result = read<Size>(ea);
		if (data & 0x8000) {
			if (Size == Byte) Base::write(areg, (int8_t)result);
			if (Size == Word) Base::write(areg, (int16_t)result);
			if (Size == Long) Base::write(areg, result);
		} else {
			Base::write<Size>(dreg, result);
		}
	} else {		
		state.overrideWithAlternateFC = true;
		state.alternateFC = ctx->dfc;
		if (data & 0x8000) {
			write<Size>(ea, Base::read<Size>(areg));
		} else {
			write<Size>(ea, Base::read<Size>(dreg));
		}		
	}
	
	state.overrideWithAlternateFC = false;
	prefetch(true);
}

auto M68010::opRte() -> void {
	if (!ctx->s) {
		privilegeException();
		return;
	}
	state.data = true;
	uint16_t sp = ctx->a[7];
	
	uint16_t sr = read<Word>( sp );
	sp += 2;
	uint32_t pc = read<Long>( sp );
	sp += 4;
	uint16_t format = read<Word>( sp );
	sp += 2;
	
	format >>= 12;
	if (format != 0 && format != 8) {
		illegalException( 14 ); //format error
		return;
	}
	//not emulated, rte from bus/address errors
	if (format == 8) {
		/** todo read long stack frame and resume suspended instruction
		 * really complex, understanding of cpu internals needed for stack frame creation
		 * resuming opcodes needs rewriting of emulation code in a splitted way,
		 * so that handler can restart mid opcode
		 * is there any software relying on this feature ? */
		reset(); //to keep in a sane state
		return;
	}
	ctx->a[7] = sp; //updated this late, so exception before can't corrupt stack	
	ctx->pc = pc;	
	fullPrefetch();
	setSR( sr );
}

template<uint8_t Size> auto M68010::opClr(EffectiveAddress modify) -> void {	
	modify.address = fetch<Size>(modify); //useless dummy read removed for 68010 cpu
	if (modify.indexMode()) ctx->sync( 2 );
	ctx->z = 1;
	ctx->n = ctx->v = ctx->c = 0;
	
	prefetch( modify.registerMode() );
	if ( modify.registerMode() && Size == Long) ctx->sync( 2 );
	write<Size>(modify, 0);
}

auto M68010::opScc(EffectiveAddress modify, uint8_t cc) -> void {
	modify.address = fetch<Byte>(modify); //useless dummy read removed for 68010 cpu
	noFetchTail<Byte>(modify);
	uint8_t data = this->testCondition(cc) ? 0xff : 0;
	prefetch( modify.registerMode() );
	write<Byte>(modify, data);		
}

auto M68010::opDbcc(DataRegister modify, uint8_t cond ) -> void {
	if (ctx->loopMode) {
		opLoopDbcc(modify, cond);
		return;
	}
	
	ctx->sync(2);		
	uint32_t memPC = ctx->prefetchCounter;
	uint16_t displacement = ctx->irc;
	ctx->prefetchCounter = ctx->prefetchCounterLast + (int16_t)displacement;

	if (!testCondition(cond)) {		
		Base::write<Word>( modify, Base::read<Word>(modify) - 1 );				
		readExtensionWord();
		if((int16_t)Base::read<Word>(modify) != -1 ) {
			prefetch(true);
			
			if (opLoop[ctx->ird] && (int16_t)displacement == -4) {
				ctx->loopMode = true;
				ctx->ir = ctx->ird;
				ctx->ird = ctx->irc;
				ctx->irc = displacement;
				ctx->prefetchCounter = memPC;
			}			
			return;
		} else {
			ctx->sync(2);
		}	
	} 	
	ctx->pc = memPC;
	fullPrefetch();
}
// loop mode
auto M68010::opLoopDbcc(DataRegister modify, uint8_t cond ) -> void {	
	uint8_t custom = opLoop[ctx->ir]();
	//no interrupt recognition here, looped opcode is nested in
	if(custom != Loop_Custom2) ctx->sync(2);
	uint16_t decremented = Base::read<Word>(modify) - 1;

	if((int16_t)decremented != -1 ) {
		ctx->sync(custom == Loop_Custom2 ? 4 : 2);
		
		if (!testCondition(cond)) {		
			ctx->sync(2);												
			Base::write<Word>( modify, decremented );				
			return;
		}		
	} else {
		Base::write<Word>( modify, decremented );		
	}	
	if(custom == Loop_Custom1) ctx->sync(2);
	
	ctx->loopMode = false;
	ctx->pc = ctx->prefetchCounter;
	fullPrefetch();
}

template<uint8_t Size> auto M68010::opLoopMove(EffectiveAddress src, EffectiveAddress dest) -> uint8_t {		
	uint32_t data = read<Size>(src);
		
	dest.address = fetch<Size>(dest);
	
	Base::defaultFlags<Size>(data);
	
	write<Size>(dest, data, false);

	if (Size != Long) {
		if (src.registerMode()) return Loop_Custom1;
	} else {
		if (!src.registerMode()) return Loop_Custom2;
	}
	return Loop_Normal;
}

template<uint8_t Size, uint8_t Mode> auto M68010::opLoopArithmetic(DataRegister modify, EffectiveAddress src) -> uint8_t {
	uint32_t data = read<Size>(src);		
	ctx->sync( Size == Long ? 8 : 6 );			
	Base::write<Size>(modify, this->arithmetic<Size, Mode>(data, Base::read<Size>(modify) ) );	
	return Loop_Normal;
}

template<uint8_t Size> auto M68010::opLoopCmp(DataRegister dest, EffectiveAddress src) -> uint8_t {
	uint32_t data = read<Size>(src);
	ctx->sync( Size == Long ? 4 : 2 );
	this->cmp<Size>(Base::read<Size>(dest), data);	
	return Size == Long ? Loop_Custom2 : Loop_Normal;
}

template<uint8_t Size, uint8_t Mode> auto M68010::opLoopArithmetic(EffectiveAddress modify, DataRegister src) -> uint8_t {	
	uint32_t data = read<Size>(modify);	
	ctx->sync( 2 );	
	write<Size>(modify, this->arithmetic<Size, Mode>(Base::read<Size>(src), data ), false );		
	return Loop_Normal;
}

template<uint8_t Size> auto M68010::opLoopEor(EffectiveAddress modify, DataRegister src) -> uint8_t {
	uint32_t data = read<Size>(modify);	
	ctx->sync( 2 );	
	write<Size>(modify, this->_eor<Size>(Base::read<Size>(src), data ), false );	
	return Loop_Normal;	
}

template<uint8_t Size> auto M68010::opLoopAdda(AddressRegister modify, EffectiveAddress src) -> uint8_t {
	uint32_t data = read<Size>(src);
	ctx->sync( 8 );
	if (Size == Word) data = (int16_t)data;
	Base::write(modify, data + Base::read<Long>(modify));
	return Loop_Normal;		
}

template<uint8_t Size> auto M68010::opLoopSuba(AddressRegister modify, EffectiveAddress src) -> uint8_t {
	uint32_t data = read<Size>(src);
	ctx->sync( 8 );
	if (Size == Word) data = (int16_t)data;
	Base::write(modify, Base::read<Long>(modify) - data);
	return Loop_Normal;
}

template<uint8_t Size> auto M68010::opLoopCmpa(AddressRegister dest, EffectiveAddress src) -> uint8_t {
	uint32_t data = read<Size>(src);
	if (Size == Word) data = (int16_t)data;
	ctx->sync( Size == Long ? 4 : 2 );
	this->cmp<Long>(Base::read<Size>(dest), data);	
	return Size == Long ? Loop_Custom2 : Loop_Normal;
}

template<uint8_t Size> auto M68010::opLoopClr(EffectiveAddress modify) -> uint8_t {	
	modify.address = fetch<Size>(modify);
	ctx->z = 1;
	ctx->n = ctx->v = ctx->c = 0;	
	write<Size>(modify, 0, false);
	return Loop_Custom1;
}

template<uint8_t Size, bool Extend> auto M68010::opLoopNeg(EffectiveAddress modify) -> uint8_t {
	uint32_t data = read<Size>(modify);
	ctx->sync(2);
	data = this->sub<Size, Extend>(data, 0);
	write<Size>(modify, data, false);
	return Loop_Normal;
}

template<uint8_t Size> auto M68010::opLoopNot(EffectiveAddress modify) -> uint8_t {
	uint32_t data = read<Size>(modify);
	ctx->sync(2);
	data = this->_not<Size>(data);
	write<Size>(modify, data, false);	
	return Loop_Normal;
}

template<uint8_t Size> auto M68010::opLoopTst(EffectiveAddress modify) -> uint8_t {
	uint32_t data = read<Size>(modify);
	ctx->sync( Size == Long ? 4 : 2);
	this->defaultFlags<Size>(data);
	return Size == Long ? Loop_Custom2 : Loop_Normal;
}

auto M68010::opLoopNbcd(EffectiveAddress modify) -> uint8_t {
	uint8_t data = read<Byte>(modify);
	ctx->sync( 4 );
	data = this->sbcd(data, 0);	
	write<Byte>(modify, data);
	return Loop_Normal;
}

template<uint8_t Size, uint8_t Mode> auto M68010::opLoopArithmeticX(EffectiveAddress src, EffectiveAddress dest) -> uint8_t {		
	uint32_t dataSrc = read<Size>(src);
	uint32_t data = read<Size, NoCyclesPreDec>(dest);
	
	data = this->arithmeticX<Size, Mode>(dataSrc, data);
	
	ctx->sync( 2 );
	if (Mode == Abcd || Mode == Sbcd) ctx->sync(2);
	
	write<Size>(dest, data, false);
	return Loop_Normal;
}

template<uint8_t Size> auto M68010::opLoopCmpm(EffectiveAddress src, EffectiveAddress dest) -> uint8_t {		
	uint32_t dataSrc = read<Size>(src);
	uint32_t data = read<Size>(dest);
	
	this->cmp<Size>(dataSrc, data);
	if (Size == Long) ctx->sync( 2 );
	return Size == Long ? Loop_Custom2 : Loop_Normal;
}

template<uint8_t Mode> auto M68010::opLoopEaShift(EffectiveAddress modify) -> uint8_t {
	uint32_t result = this->shift<Word, Mode>( read<Word>(modify), 1 );		
	ctx->sync(4);
	write<Word>(modify, result);
	return Loop_Normal;
}
