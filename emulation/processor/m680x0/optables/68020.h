
#define __ ,
#define bind( id, name, ... ) { \
	assert( !opTable[id] ); \
	opTable[id] = [=] { return op##name(__VA_ARGS__); }; \
}

#define unbind( id ) { \
	opTable[id] = nullptr; \
}

#define _parse(s) (uint16_t)Base::parse(s)

//bcc long overmap
for (uint8_t cond : range(2,16)) {
	opcode = _parse("0110 ---- 1111 1111")	| cond << 8;
	unbind( opcode );	
	bind(opcode, Bcc<Long>, cond, 0xff);
}

//bra long overmap
opcode = _parse("0110 0000 1111 1111");
unbind( opcode );	
bind(opcode, Bra<Long>, 0xff);

//bsr long overmap
opcode = _parse("0110 0001 1111 1111");
unbind( opcode );	
bind(opcode, Bsr<Long>, 0xff);

//callm
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 0 || mode == 1 || mode == 3 || mode == 4 || (mode == 7 && reg >= 4 ) ) continue;	
	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0000 0110 11-- ----") | mode << 3 | reg;
	bind(opcode, Callm, modify);
}

//bfchg bfclr bfexts bfextu bfffo bfins bfset bftst
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || mode == 3 || mode == 4 ) continue;	
	EffectiveAddress modify{mode, reg};
	
	if (mode != 7 || reg < 2) {
		opcode = _parse("1110 1010 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bfchg>, modify);
	
		opcode = _parse("1110 1100 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bfclr>, modify);
		
		opcode = _parse("1110 1111 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bfins>, modify);
		
		opcode = _parse("1110 1110 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bfset>, modify);
	}
	
	if (mode != 7 || reg <= 3) {
		opcode = _parse("1110 1011 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bfexts>, modify);
		
		opcode = _parse("1110 1001 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bfextu>, modify);
		
		opcode = _parse("1110 1101 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bfffo>, modify);

		opcode = _parse("1110 1000 11-- ----") | mode << 3 | reg;
		bind(opcode, BitField<Bftst>, modify);
	}
}

//cas
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 0 || mode == 1 || (mode == 7 && reg >= 2 ) ) continue;	
	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0000 1ss0 11-- ----") | mode << 3 | reg;
	bind(opcode | 1 << 9, Cas<Byte>, modify);
	bind(opcode | 2 << 9, Cas<Word>, modify);
	bind(opcode | 3 << 9, Cas<Long>, modify);
}

//cas2
opcode = _parse("0000 1ss0 1111 1100");
bind(opcode | 2 << 9, Cas2<Word>);
bind(opcode | 3 << 9, Cas2<Long>);

//chk long
for (uint8_t dreg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg > 4) ) continue;
	
	EffectiveAddress src{mode, reg};
	DataRegister srcD{dreg};
	
	opcode = _parse("0100 ---1 00-- ----") | dreg << 9 | mode << 3 | reg;
	
	bind(opcode, Chk<Long>, src, srcD);
}

//chk2 cmp2 (distinction is in extension word)
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 0 || mode == 1 || mode == 3 || mode == 4 || (mode == 7 && reg >= 4 ) ) continue;
	
	EffectiveAddress modify{mode, reg};
	opcode = _parse("0000 0ss0 11-- ----") | mode << 3 | reg;
	
	bind(opcode | 0 << 9, Chk2<Byte>, modify);
	bind(opcode | 1 << 9, Chk2<Word>, modify);
	bind(opcode | 2 << 9, Chk2<Long>, modify);
}

//divsl divul (distinction is in extension word)
//mulsl mulul (distinction is in extension word)
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg >= 5) ) continue;
	EffectiveAddress src{mode, reg};
	
	opcode = _parse("0100 1100 01-- ----") | mode << 3 | reg;	
	bind(opcode, Divl, src );
	
	opcode = _parse("0100 1100 00-- ----") | mode << 3 | reg;	
	bind(opcode, Mull, src );
}

//extb
for (uint8_t dreg : range(8)) {
	DataRegister modify{dreg};
	
	opcode = _parse("0100 1001 1100 0---") | dreg;
	bind(opcode, Extb, modify);
}

//link long
for (uint8_t areg : range(8)) {
	AddressRegister modify{areg};
	
	opcode = _parse("0100 1000 0000 1---") | areg;
	
	bind(opcode, Link<Long>, modify);
}

//pack unpk data
for (uint8_t dregx : range(8))
for (uint8_t dregy : range(8)) {
	DataRegister dx{dregx};
	DataRegister dy{dregy};
	
	opcode = _parse("1000 ---1 0100 0---") | dregy << 9 | dregx;
	bind(opcode, Pack, dx, dy);
	
	opcode = _parse("1000 ---1 1000 0---") | dregy << 9 | dregx;
	bind(opcode, Unpack, dx, dy);
}
//pack unpk ea
for (uint8_t aregx : range(8))
for (uint8_t aregy : range(8)) {
	EffectiveAddress eax{AddressRegisterIndirectWithPreDecrement, aregx};
	EffectiveAddress eay{AddressRegisterIndirectWithPreDecrement, aregy};
		
	opcode = _parse("1000 ---1 0100 1---") | aregy << 9 | aregx;
	bind(opcode, Pack, eax, eay);
	
	opcode = _parse("1000 ---1 1000 1---") | aregy << 9 | aregx;
	bind(opcode, Unpack, eax, eay);
}

//rtm data
for (uint8_t dreg : range(8)) {
	DataRegister src{dreg};
	
	opcode = _parse("0000 0110 1100 0---") | dreg;
	bind(opcode, Rtm, src);
}

//rtm address
for (uint8_t areg : range(8)) {
	AddressRegister src{areg};
	
	opcode = _parse("0000 0110 1100 1---") | areg;
	bind(opcode, Rtm, src);
}

//trapcc
for (uint8_t cond : range(16)) {
	opcode = _parse("0101 ---- 1111 1---") | cond << 8;
	bind(opcode | 2, Trapcc<2>, cond);
	bind(opcode | 3, Trapcc<3>, cond);
	bind(opcode | 4, Trapcc<4>, cond);
}

//tst
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && (reg == 2 || reg == 3 || reg == 4) ) ) {		
		EffectiveAddress modify{mode, reg};	
		opcode = _parse("0100 1010 ss-- ----") | mode << 3 | reg;

		if (mode != 1) {
			bind(opcode | 0 << 6, Tst<Byte>, modify);	
		}	
		bind(opcode | 1 << 6, Tst<Word>, modify);
		bind(opcode | 2 << 6, Tst<Long>, modify);	
	}	
}

#undef __
#undef bind
#undef unbind
#undef _parse
