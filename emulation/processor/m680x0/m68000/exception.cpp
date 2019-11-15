
auto M68000::group0exception(uint32_t addr, uint8_t type) -> void { //bus or address error	
    //interrupts opcodes or group 1/2 exception stacking
    if (ctx->halt) { //another group 0 exception during last one ... cpu halted
        useErrorContext();
        return;
    }
    ctx->halt = true;
	ctx->sync(2);
    uint16_t SR = getSR();
    uint16_t _state = (state.write ? 0 : 16) | (state.instruction ? 0 : 8);
    _state |= (SR & 0x2000 ? 4 : 0) | (state.data ? 1 : 2);
    ctx->t = ctx->trace = 0;
    switchToSupervisor();
    ctx->sync(2);
    ctx->a[7] -= 14;
    write<Word>(ctx->a[7] + 12, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 8, SR);
    write<Word>(ctx->a[7] + 10, (ctx->pc >> 16) & 0xffff);
    write<Word>(ctx->a[7] + 6, ctx->ird);
    write<Word>(ctx->a[7] + 4, addr & 0xffff);
    write<Word>(ctx->a[7] + 0, _state );
    write<Word>(ctx->a[7] + 2, (addr >> 16) & 0xffff);
	
    ctx->sync(2);
    executeAt(type == BusError ? 2 : 3);
    ctx->halt = false;
	useErrorContext(); //finish opcode within dummy context as an alternate to try/catch
}

auto M68000::group1exceptions() -> bool {
    state.instruction = false;
	
    if (ctx->trace) traceException();

	if ( ctx->level7SamplingTrigger || ( ctx->irqSamplingLevel > ctx->i ) ) {
		interruptException( ctx->irqSamplingLevel );
		return true;
	}
	return false;
}

auto M68000::interruptException( uint8_t level ) -> void {
	ctx->sync(2);
	ctx->irqSamplingLevel = 0;
	ctx->level7SamplingTrigger = false;
	ctx->stop = false;
	uint16_t SR = getSR();
	ctx->i = level;
	switchToSupervisor();
	ctx->t = ctx->trace = 0;
	ctx->sync(4);
	ctx->a[7] -= 6;
	write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
	ctx->sync(4);
	auto vector = Base::getInterruptVector(level);
	ctx->sync(4);
	write<Word>(ctx->a[7] + 0, SR);
	write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	executeAt( vector );
}

auto M68000::traceException() -> void {
	ctx->sync(2);
    uint16_t SR = getSR();
    switchToSupervisor();
    ctx->t = ctx->trace = 0;
    ctx->stop = false;
	ctx->sync(2);
    ctx->a[7] -= 6;
    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
    executeAt(9);
}

auto M68000::illegalException(uint8_t vector) -> void {
	state.instruction = false;
	ctx->sync(2);
    uint16_t SR = getSR();
    switchToSupervisor();
    ctx->t = ctx->trace = 0;
	ctx->sync(2);
    ctx->a[7] -= 6;
    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	executeAt( vector );
}

auto M68000::privilegeException() -> void {
	illegalException(8);
}

auto M68000::trapException(uint8_t vector) -> void { //group 2 exceptions will triggered within opcode
    uint16_t SR = getSR();
    ctx->t = 0;
    switchToSupervisor();
    ctx->a[7] -= 6;
    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
    executeAt(vector);
}

auto M68000::executeAt(uint8_t vector) -> void { //18 cycles
	state.data = true;
    ctx->pc = read<Long>(vector << 2);
	fullPrefetch<true>();
}
