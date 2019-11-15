
#define bind( id, name, ... ) { \
	assert( !opTable[id] ); \
	opTable[id] = [=] { return op##name(__VA_ARGS__); }; \
}

#define _parse(s) (uint16_t)Base::parse(s)

//bkpt
for (uint8_t vector : range(8)) {
	opcode = _parse("0100 1000 0100 1---") | vector;
	bind(opcode, Bkpt, vector );
}

//cmpi pc relative addressing
for (uint8_t reg : range(2, 4)) {

	opcode = _parse("0000 1100 ss11 1---")	| reg;
	
	EffectiveAddress dest{7, reg};
	bind(opcode | 0 << 6, Cmpi<Byte>, dest);	
	bind(opcode | 1 << 6, Cmpi<Word>, dest);
	bind(opcode | 2 << 6, Cmpi<Long>, dest);
}

//move from ccr
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;

	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0100 0010 11-- ----") | mode << 3 | reg;
	
	bind(opcode, MoveFromCcr, modify);
}

//rtd
bind( _parse("0100 1110 0111 0100"), Rtd );

//movec
opcode = _parse("0100 1110 0111 101-");
bind(opcode | 0, Movec<false>);
bind(opcode | 1, Movec<true>);

//moves
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 0 || mode == 1 || (mode == 7 && reg >= 2) ) continue;

	EffectiveAddress ea{mode, reg};
	
	opcode = _parse("0000 1110 ss-- ----") | mode << 3 | reg;
	bind(opcode | 0 << 6, Moves<Byte>, ea);
	bind(opcode | 1 << 6, Moves<Word>, ea);
	bind(opcode | 2 << 6, Moves<Long>, ea);
}

#undef bind
#undef _parse
