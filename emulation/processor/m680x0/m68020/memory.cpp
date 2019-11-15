
template<> auto M68020::read<Byte>(uint32_t addr) -> uint32_t {
	ctx->lastBusAccessCycles = 0;
	state.write = false;
    addBus(2);
    uint8_t byte = ctx->read( addr );
    addBus(1);
    return byte;
}

template<> auto M68020::read<Word>(uint32_t addr) -> uint32_t {
	state.write = false;
	ctx->lastBusAccessCycles = 0;
    uint16_t word;
    uint8_t portSize = ctx->portSize( addr );
    
    if ( (portSize == Byte) || ( (portSize == Word) && (addr & 1) ) || ( (portSize == Long) && ((addr & 3) == 3) ) ) {
        addBus(2);
        word = ctx->read( addr ) << 8;
        addBus(3);        
        word |= ctx->read( addr + 1 );
        addBus(1);
    } else {
        addBus(2);
        word = ctx->readWord( addr );
        addBus(1);
    }
    return word;
}

template<> auto M68020::read<Long>(uint32_t addr) -> uint32_t {
	state.write = false;
	ctx->lastBusAccessCycles = 0;
    uint32_t lword = 0;
    uint8_t portSize = ctx->portSize( addr );
    
	std::vector<uint8_t> map;	
    if (portSize == Byte) map = LWordMap[Byte][0];
    else if (portSize == Word) map = LWordMap[Word][addr & 1];
    else map = LWordMap[Long][addr & 3];
	
    auto bits = 32;
    auto offset = 0;
    
    for(auto c : range(map.size())) {
		addBus(2);
        
        auto bytes = map[c];
        bits -= bytes << 3;
        
        if (bytes == 1) lword |= ctx->read( addr + offset ) << bits;
        else if (bytes == 2) lword |= ctx->readWord( addr + offset ) << bits;
        else if (bytes == 3) {
            lword |= ctx->readWord( addr + offset ) << (bits + 8);
            lword |= ctx->read( addr + offset + 2 ) << bits;
        } else {
            lword |= ctx->readWord( addr ) << 16;
            lword |= ctx->readWord( addr + 2 );
        }
        
        offset += bytes;
        addBus(1);
    }
    return lword;
}

template<> auto M68020::write<Byte>(uint32_t addr, uint32_t value) -> void {
    state.write = true;
	ctx->lastBusAccessCycles = 0;
    addBus(2);
    ctx->write( addr, value & 0xff );
    addBus(1);
}

template<> auto M68020::write<Word>(uint32_t addr, uint32_t value) -> void {
    state.write = true;
	ctx->lastBusAccessCycles = 0;
    uint8_t portSize = ctx->portSize( addr );
    
    if ( (portSize == Byte) || ( (portSize == Word) && (addr & 1) ) || ( (portSize == Long) && ((addr & 3) == 3) ) ) {
        addBus(2);
        ctx->write( addr, (value >> 8) & 0xff );
        addBus(3);
        ctx->write( addr + 1, value & 0xff );
        addBus(1);
    } else {
        addBus(2); //put addr on bus
        ctx->writeWord( addr, value & 0xffff );
        addBus(1);
    }
}

template<> auto M68020::write<Long>(uint32_t addr, uint32_t value) -> void {
    state.write = true;
	ctx->lastBusAccessCycles = 0;
    uint8_t portSize = ctx->portSize( addr );
    
	std::vector<uint8_t> map;	
    if (portSize == Byte) map = LWordMap[Byte][0];
    else if (portSize == Word) map = LWordMap[Word][addr & 1];
    else map = LWordMap[Long][addr & 3];
	
    auto bits = 32;
    auto offset = 0;
    
    for(auto c : range(map.size())) {
		addBus(2);
        
        auto bytes = map[c];
        bits -= bytes << 3;
        
        if (bytes == 1) ctx->write( addr + offset, (value >> bits) & 0xff );
        else if (bytes == 2) ctx->writeWord( addr + offset, (value >> bits) & 0xffff );
        else if (bytes == 3) {
            ctx->writeWord( addr + offset, (value >> (bits + 8)) & 0xffff );
            ctx->write( addr + offset + 2, (value >> bits) & 0xff );
        } else {
            ctx->writeWord( addr, (value >> 16) & 0xffff );
            ctx->writeWord( addr + 2, value & 0xffff );
        }
        
        offset += bytes;
        addBus(1);
    }
}

template<bool incrementPC> auto M68020::readExtensionWord() -> void {
	prefetch<incrementPC, true>(false);
}

template<bool incrementPC, bool extensionWord> auto M68020::prefetch(bool initial) -> void {
    state.data = false;
	state.write = false;
	if(incrementPC) ctx->pc += 2;

    if ((ctx->prefetchCounter & 1) == 1) {
//        group0exception( ctx->prefetchCounter, ADDRESS_ERROR );
    }
    
	if (!extensionWord) {		
		ctx->stageD = ctx->stageC;
		ctx->history.adrLastInstruction = ctx->history.adrStageD;
		ctx->history.adrStageD = ctx->history.adrStageC;
	}		
    ctx->stageC = ctx->stageB;
	ctx->history.adrStageC = ctx->history.adrStageB;
	ctx->history.adrStageB = ctx->prefetchCounter;
    
    if ( !initial && (ctx->prefetchCounter & 3) == 2) {
        ctx->stageB = ctx->cacheHolding & 0xffff;
    } else {
        uint8_t index = (ctx->prefetchCounter >> 2) & 0x3f;
        
        if (ctx->cache.valid[index] && (ctx->cache.FC2[index] == ctx->s) &&
            ( (ctx->prefetchCounter & ~0xff) == ctx->cache.tag[index] )
        ) { //cache hit
            ctx->cacheHolding = ctx->cache.data[index];                        
        } else {
            ctx->cacheHolding = read<Long>(ctx->prefetchCounter & ~3);
            ctx->cache.data[index] = ctx->cacheHolding;
            ctx->cache.valid[index] = !ctx->ca_f;
            ctx->cache.tag[index] = ctx->prefetchCounter & ~0xff;
            ctx->cache.FC2[index] = ctx->s;
        }		
		ctx->stageB = (ctx->prefetchCounter >> 1) & 1 ? ctx->cacheHolding & 0xffff : ctx->cacheHolding >> 16;
    }
    ctx->prefetchCounter += 2;
}

auto M68020::getSR() -> uint16_t {	
	return Base::getSR() | ctx->t0 << 14 | ctx->m << 12;
}

auto M68020::setSR(uint16_t data) -> void {
	Base::setSR( data );
	
	ctx->t0 = (data >> 14) & 1;
	ctx->m = (data >> 12) & 1;	
}

