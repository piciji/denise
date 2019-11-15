
auto M68010::group0exception(uint32_t addr, uint8_t type) -> void { //bus or address error	
    //interrupts opcodes or group 1/2 exception stacking
	state.overrideWithAlternateFC = false; //if happens during moves
	ctx->loopMode = false;
    if (ctx->halt) { //another group 0 exception during last one ... cpu halted
        useErrorContext();
        return;
    }
    ctx->halt = true;
	ctx->sync(2);
	auto vector = type == BusError ? 2 : 3;
    uint16_t SR = getSR();
    uint16_t ssw = !state.write ? 1 << 8 : 0;	
    ssw |= !state.data ? 1 << 13 : 1 << 12;
	ssw |= !state.wordTransfer ? 1 << 9 : 0;
	ssw |= state.rmw ? 1 << 11 : 0;
	ssw |= state.hbTransfer ? 1 << 10 : 0;
	ssw |= Base::getFunctionCodes();
	//todo improve ssw (if, df bits)
	state.rmw = false;
	state.hbTransfer = false;
    ctx->t = ctx->trace = 0;
    switchToSupervisor();
    ctx->sync(2);
    ctx->a[7] -= 58;
	
	write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, 8 << 12 | vector << 2);
	//stacked data needed for software rerun of failed cycle
	//developer could update ssw, so RTE skip failed cycle instead of rerun
	write<Word>(ctx->a[7] + 8, ssw );
	write<Word>(ctx->a[7] + 10, (addr >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 12, addr & 0xffff);
	write<Word>(ctx->a[7] + 16, state.outputBuffer);
	write<Word>(ctx->a[7] + 20, state.inputBuffer);
	write<Word>(ctx->a[7] + 24, ctx->irc);
	
	for(auto i : range(16)) {
		/** todo: cpu stacks internal data to resume instruction later on */
		write<Word>(ctx->a[7] + 26 + (i << 1), 0);
	}
    ctx->sync(2);
    executeAt( vector );
    ctx->halt = false;
	useErrorContext(); //finish opcode within dummy context as an alternate to try/catch
}

auto M68010::interruptException( uint8_t level ) -> void {
	ctx->sync(2);
	ctx->irqSamplingLevel = 0;
	ctx->level7SamplingTrigger = false;
	ctx->loopMode = false;
	ctx->stop = false;
	uint16_t SR = getSR();
	ctx->i = level;
	switchToSupervisor();
	ctx->t = ctx->trace = 0;
	ctx->sync(4);
	ctx->a[7] -= 8;
	write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
	ctx->sync(4);
	auto vector = Base::getInterruptVector(level);
	ctx->sync(2);
	write<Word>(ctx->a[7] + 0, SR);
	write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, vector << 2);
	executeAt( vector );
}

auto M68010::traceException() -> void {
	ctx->sync(2);
	ctx->loopMode = false;
    uint16_t SR = getSR();
    switchToSupervisor();
    ctx->t = ctx->trace = 0;
    ctx->stop = false;
	ctx->sync(2);
    ctx->a[7] -= 8;
    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, 9 << 2);
    executeAt(9);
}

auto M68010::illegalException(uint8_t vector) -> void {
	state.instruction = false;
	ctx->sync(2);
    uint16_t SR = getSR();
    switchToSupervisor();
    ctx->t = ctx->trace = 0;
	ctx->loopMode = false;
	ctx->sync(2);
    ctx->a[7] -= 8;
    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, vector << 2);
	executeAt( vector );
}

auto M68010::trapException(uint8_t vector) -> void { //group 2 exceptions will triggered within opcode
    uint16_t SR = getSR();
    ctx->t = 0;
	ctx->loopMode = false;
    switchToSupervisor();
    ctx->a[7] -= 8;
    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, vector << 2);
    executeAt( vector );
}

auto M68010::executeAt(uint8_t vector) -> void {
	state.data = true;
    ctx->pc = read<Long>( ctx->vbr + (vector << 2) );
	fullPrefetch<true>();
}
