
template<uint8_t Size> auto defaultFlags(uint32_t data) -> void {
	ctx->v = ctx->c = 0;
	ctx->n = negative<Size>(data);
	ctx->z = zero<Size>(data);
}

template<uint8_t Size, uint8_t Mode> auto shift(uint32_t data, uint8_t shift) -> uint32_t {
	switch(Mode) {
		case Asl:	return asl<Size>( data, shift );
		case Asr:	return asr<Size>( data, shift );
		case Lsl:	return lsl<Size>( data, shift );
		case Lsr:	return lsr<Size>( data, shift );
		case Rol:	return rol<Size>( data, shift );
		case Ror:	return ror<Size>( data, shift );
		case Roxl:	return roxl<Size>( data, shift );
		case Roxr:	return roxr<Size>( data, shift );
		default: assert(false);
	}
}

template<uint8_t Size> auto asl(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = false;
	uint32_t overflow = 0;
	for(auto _ : range(shift)) {
		carry = data & msb<Size>();
		uint32_t before = data;
		data <<= 1;
		overflow |= before ^ data;
	}

	ctx->c = carry;
	ctx->v = negative<Size>(overflow);
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);
	if(shift) ctx->x = ctx->c;

	return clip<Size>(data);
}

template<uint8_t Size> auto asr(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = false;
	uint32_t overflow = 0;
	for(auto _ : range(shift)) {
		carry = data & 1;
		uint32_t before = data;
		data = sign<Size>(data) >> 1;
		overflow |= before ^ data;
	}

	ctx->c = carry;
	ctx->v = negative<Size>(overflow);
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);
	if(shift) ctx->x = ctx->c;

	return clip<Size>(data);
}

template<uint8_t Size> auto lsl(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = false;
	for(auto _ : range(shift)) {
		carry = data & msb<Size>();
		data <<= 1;
	}

	ctx->c = carry;
	ctx->v = 0;
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);
	if(shift) ctx->x = ctx->c;

	return clip<Size>(data);
}

template<uint8_t Size> auto lsr(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = false;
	for(auto _ : range(shift)) {
		carry = data & 1;
		data >>= 1;
	}

	ctx->c = carry;
	ctx->v = 0;
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);
	if(shift) ctx->x = ctx->c;

	return clip<Size>(data);
}

template<uint8_t Size> auto rol(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = false;
	for(auto _ : range(shift)) {
		carry = data & msb<Size>();
		data = data << 1 | carry;
	}

	ctx->c = carry;
	ctx->v = 0;
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);

	return clip<Size>(data);
}

template<uint8_t Size> auto ror(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = false;
	for(auto _ : range(shift)) {
		carry = data & 1;
		data >>= 1;
		if(carry) data |= msb<Size>();
	}

	ctx->c = carry;
	ctx->v = 0;
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);

	return clip<Size>(data);
}

template<uint8_t Size> auto roxl(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = ctx->x;
	for(auto _ : range(shift)) {
		bool extend = carry;
		carry = data & msb<Size>();
		data = data << 1 | extend;
	}

	ctx->c = carry;
	ctx->v = 0;
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);
	ctx->x = ctx->c;

	return clip<Size>(data);
}

template<uint8_t Size> auto roxr(uint32_t data, uint8_t shift) -> uint32_t {
	bool carry = ctx->x;
	for(auto _ : range(shift)) {
		bool extend = carry;
		carry = data & 1;
		data >>= 1;
		if(extend) data |= msb<Size>();
	}

	ctx->c = carry;
	ctx->v = 0;
	ctx->z = zero<Size>(data);
	ctx->n = negative<Size>(data);
	ctx->x = ctx->c;

	return clip<Size>(data);
}

template<uint8_t Mode> auto bit(uint32_t data, uint8_t bit) -> uint32_t {
	switch(Mode) {
		case Bchg: return bchg(data, bit);
		case Bset: return bset(data, bit);
		case Bclr: return bclr(data, bit);
		case Btst: return btst(data, bit);
		default: assert(false);
	}
}

auto bchg(uint32_t data, uint8_t bit) -> uint32_t {
	ctx->z = 1 ^ ((data >> bit) & 1);
	data ^= (1 << bit);
	return data;
}

auto bset(uint32_t data, uint8_t bit) -> uint32_t {	
	ctx->z = 1 ^ ((data >> bit) & 1);
	data |= (1 << bit);
	return data;
}

auto bclr(uint32_t data, uint8_t bit) -> uint32_t {	
	ctx->z = 1 ^ ((data >> bit) & 1);
	data &= ~(1 << bit);
	return data;
}

auto btst(uint32_t data, uint8_t bit) -> uint32_t {	
	ctx->z = 1 ^ ((data >> bit) & 1);
	return data;
}

template<uint8_t Size> auto _not(uint32_t data) -> uint32_t {
	data = ~data;
	defaultFlags<Size>(data);
	return data;
}

template<uint8_t Size, uint8_t Mode> auto arithmetic(uint32_t src, uint32_t dest) -> uint32_t {
	switch(Mode) {
		case Add: return add<Size, false>(src, dest);
		case Sub: return sub<Size, false>(src, dest);
		case And: return _and<Size>(src, dest);
		case Or: return _or<Size>(src, dest);
		case Eor: return _eor<Size>(src, dest);
		default: assert(false);
	}
}

template<uint8_t Size, uint8_t Mode> auto arithmeticX(uint32_t src, uint32_t dest) -> uint32_t {
	switch(Mode) {
		case Add: return add<Size, true>(src, dest);
		case Sub: return sub<Size, true>(src, dest);
		case Abcd: return abcd((uint8_t)src, (uint8_t)dest);
		case Sbcd: return sbcd((uint8_t)src, (uint8_t)dest);
		default: assert(false);
	}
}

template<uint8_t Size, bool Extend> auto sub(uint32_t src, uint32_t dest) -> uint32_t {
	uint64_t result = dest - src;
	if (Extend) result -= ctx->x;
	
	ctx->c = negative<Size>(result >> 1);
	ctx->v = negative<Size>((dest ^ src) & (dest ^ result));
	if (!Extend) ctx->z = zero<Size>(result);
	if (Extend) if (clip<Size>(result)) ctx->z = 0;
	ctx->n = negative<Size>(result);
	ctx->x = ctx->c;
	
	return result;
}

template<uint8_t Size, bool Extend> auto add(uint32_t src, uint32_t dest) -> uint32_t {
	uint64_t result = src + dest;
	if (Extend) result += ctx->x;
	
	ctx->c = negative<Size>(result >> 1);
	ctx->v = negative<Size>((src ^ result) & (dest ^ result));
	if (!Extend) ctx->z = zero<Size>(result);
	if (Extend) if (clip<Size>(result)) ctx->z = 0;
	ctx->n = negative<Size>(result);
	ctx->x = ctx->c;
	
	return result;
}

template<uint8_t Size> auto cmp(uint32_t src, uint32_t dest) -> void {
	uint64_t result = dest - src;
	
	ctx->c = negative<Size>(result >> 1);
	ctx->v = negative<Size>((dest ^ src) & (dest ^ result));
	ctx->z = zero<Size>(result);
	ctx->n = negative<Size>(result);
}

template<uint8_t Size> auto _and(uint32_t src, uint32_t dest) -> uint32_t {
	uint32_t result = src & dest;
	defaultFlags<Size>(result);
	return result;
}

template<uint8_t Size> auto _or(uint32_t src, uint32_t dest) -> uint32_t {
	uint32_t result = src | dest;
	defaultFlags<Size>(result);
	return result;
}

template<uint8_t Size> auto _eor(uint32_t src, uint32_t dest) -> uint32_t {
	uint32_t result = src ^ dest;
	defaultFlags<Size>(result);
	return result;
}

template<uint8_t Mode> auto ccr(uint8_t data) -> void {
	switch(Mode) {
		case And:	setCCR( getCCR() & data ); break;
		case Or:	setCCR( getCCR() | data ); break;
		case Eor:	setCCR( getCCR() ^ data ); break;
		default: assert(false);
	}
}

template<uint8_t Mode> auto sr(uint16_t data) -> void {
	switch(Mode) {
		case And:	setSR( getSR() & data ); break;
		case Or:	setSR( getSR() | data ); break;
		case Eor:	setSR( getSR() ^ data ); break;
		default: assert(false);
	}
}

template<uint8_t Size> auto updateRegA(EffectiveAddress& ea) -> void {
	switch (ea.mode) {
		case AddressRegisterIndirectWithPostIncrement:
			ea.address += bytes<Size>();
			if (Size == Byte && ea.reg == 7) ea.address += 1;
		case AddressRegisterIndirectWithPreDecrement:
			write(AddressRegister{ea.reg}, ea.address);
			break;
	}
}
