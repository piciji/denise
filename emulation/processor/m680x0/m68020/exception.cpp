
auto M68020::group1exceptions() -> bool {
    state.instruction = false;
    
    if (ctx->trace) traceException();       

	if ( ctx->irqSamplingLevel > 0 ) {
		interruptException( ctx->irqSamplingLevel );
		return true;
	}
	return false;
}

auto M68020::interruptException( uint8_t level ) -> void {
    addSequencer(2);
    ctx->irqSamplingLevel = 0;
    ctx->stop = false;
    uint16_t SR = getSR();
    ctx->i = level;	
    switchToSupervisor();
    ctx->t = ctx->t0 = ctx->trace = 0;
    addSequencer(2);
    ctx->a[7] -= 8;
	state.data = true;
	addSequencer(4);
    auto vector = Base::getInterruptVector(level);

    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);    
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, vector << 2);	
	
	if (ctx->m) {
		catchUpSequencer();
		addSequencer(3);
		ctx->msp = ctx->a[7];
		ctx->a[7] = ctx->ssp;		
		ctx->a[7] -= 8;
		write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);		
		write<Word>(ctx->a[7] + 0, SR | (1 << 13) );
		write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
		write<Word>(ctx->a[7] + 6, 1 << 12 | vector << 2);
		ctx->m = 0;
	}	
    executeAt( vector );
}

auto M68020::traceException() -> void {
	addSequencer(4);
	ctx->stop = false;
    uint16_t SR = getSR();
    switchToSupervisor();
    ctx->t = ctx->t0 = ctx->trace = 0;
	addSequencer(3);      		
    ctx->a[7] -= 12;
	
	state.data = true;
	write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);    
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, (2 << 12) | (9 << 2));
	write<Long>(ctx->a[7] + 8, ctx->history.adrLastInstruction );
	
    executeAt(9);
}
//same for privilege exception
auto M68020::illegalException(uint8_t vector) -> void {
	state.instruction = false;
	addSequencer(2);
    uint16_t SR = getSR();
    switchToSupervisor();
    ctx->t = ctx->t0 = ctx->trace = 0;
	addSequencer(3);
    ctx->a[7] -= 8;
	state.data = true;
    write<Word>(ctx->a[7] + 4, ctx->pc & 0xffff);
    write<Word>(ctx->a[7] + 0, SR);
    write<Word>(ctx->a[7] + 2, (ctx->pc >> 16) & 0xffff);
	write<Word>(ctx->a[7] + 6, vector << 2);	
	
	executeAt( vector );
}

auto M68020::privilegeException() -> void {
	illegalException(8);
}

auto M68020::switchToSupervisor() -> void {
	if (ctx->s) return;
	
	ctx->usp = ctx->a[7];	
	if (ctx->m)
		ctx->a[7] = ctx->msp;
	else
		ctx->a[7] = ctx->ssp;
	
	ctx->s = true;	
}

auto M68020::executeAt(uint8_t vector) -> void {
	catchUpSequencer();
    ctx->pc = ctx->prefetchCounter = read<Long>(vector << 2);
	catchUpSequencer();
	addSequencer(1);
	prefetch<false>(true);
	prefetch<false>();	
	prefetch<false>();
	catchUpSequencer();
}

