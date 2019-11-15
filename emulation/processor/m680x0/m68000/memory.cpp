
template<> auto M68000::read<Byte>(uint32_t addr, bool lastBusCycle) -> uint32_t {
	state.write = false;
	state.wordTransfer = false;
    ctx->sync(2); //put addr on bus
    if (lastBusCycle) sampleIrq();
    state.inputBuffer = ctx->read( addr & 0xffffff );
    ctx->sync(2);
	return state.inputBuffer;
}

template<> auto M68000::read<Word>(uint32_t addr, bool lastBusCycle) -> uint32_t {
	state.write = false;
	state.wordTransfer = true;
    ctx->sync(2); //put addr on bus
    if ((addr & 1) == 1) {
        group0exception( addr, AddressError );
    }
    if (lastBusCycle) sampleIrq();
    state.inputBuffer = ctx->readWord( addr & 0xffffff );
    ctx->sync(2);
	return state.inputBuffer;
}

template<> auto M68000::read<Long>(uint32_t addr, bool lastBusCycle) -> uint32_t {
	uint32_t data = read<Word>(addr) << 16;
	return data | read<Word>(addr + 2, lastBusCycle);
}

auto M68000::readExtensionWord() -> void {
	state.data = false;
	ctx->pc = ctx->prefetchCounterLast;
	ctx->prefetchCounterLast = ctx->prefetchCounter;
	ctx->irc = read<Word>( ctx->prefetchCounter );		
	ctx->prefetchCounter += 2;
}

auto M68000::prefetch(bool lastBusCycle) -> void {
	state.data = false;
	ctx->pc = ctx->prefetchCounterLast;
	ctx->prefetchCounterLast = ctx->prefetchCounter;
	//decode next opcode, concurrent to put read address on bus (2 cycles)
	ctx->ir = ctx->irc;
    ctx->irc = read<Word>(ctx->prefetchCounter, lastBusCycle);
	ctx->ird = ctx->ir;
	//increment prefetch counter, concurrent to read from bus (2 cycles)	
	ctx->prefetchCounter += 2;
}

template<bool exception> auto M68000::fullPrefetch() -> void {
	ctx->prefetchCounterLast = ctx->prefetchCounter = ctx->pc;
	readExtensionWord();
	if (exception) ctx->sync(2);
	prefetch(true);	
}

//write
template<> auto M68000::write<Byte>(uint32_t addr, uint32_t value, bool lastBusCycle) -> void {
	state.write = true;
	state.data = true;
	state.wordTransfer = false;
    ctx->sync(2);
    if (lastBusCycle) sampleIrq();
	state.outputBuffer = state.hbTransfer ? (value & 0xff) << 8 : value & 0xff;
    ctx->write(addr & 0xffffff, value & 0xff);
    ctx->sync(2);
}

template<> auto M68000::write<Word>(uint32_t addr, uint32_t value, bool lastBusCycle) -> void {
	state.write = true;
	state.data = true;
	state.wordTransfer = true;
    ctx->sync(2);
	if ((addr & 1) == 1) {
        group0exception( addr, AddressError );
	}
    if (lastBusCycle) sampleIrq();
	state.outputBuffer = value;
    ctx->writeWord(addr & 0xffffff, value & 0xffff);
    ctx->sync(2);
}

template<> auto M68000::write<Long>(uint32_t addr, uint32_t value, bool lastBusCycle) -> void {
    write<Word>(addr, (value >> 16) & 0xffff);
    write<Word>(addr+2, value & 0xffff, lastBusCycle);
}

auto M68000::writeStack(uint32_t value, bool lastBusCycle) -> void {
	ctx->a[7] -= 4;
    write<Long>(ctx->a[7], value, lastBusCycle);
}
