
#define __ ,

#define bindL( id, name, ... ) { \
	opLoop[id] = [=] { return opLoop##name(__VA_ARGS__); }; \
}

#define _parse(s) (uint16_t)Base::parse(s)

for (auto loop : opLoop) loop = nullptr;

//move
for (uint8_t mode : range(5))
for (uint8_t reg : range(8))
for (uint8_t destmode : range(2,5))
for (uint8_t destreg : range(8)) {
	EffectiveAddress src{mode, reg};
	EffectiveAddress dest{destmode, destreg};

	if (src.mode == AddressRegisterDirect || src.mode == DataRegisterDirect) {
		if (dest.mode == AddressRegisterIndirectWithPreDecrement) continue;
	}

	opcode = _parse("00ss ---- ---- ----")	| destreg << 9 | destmode << 6
											| mode << 3 | reg;

	if (mode != 1)
		bindL(opcode | 1 << 12, Move<Byte>, src, dest);

	bindL(opcode | 3 << 12, Move<Word>, src, dest);
	bindL(opcode | 2 << 12, Move<Long>, src, dest);
}

//add sub and or cmp reg
for (uint8_t mode : range(2,5))
for (uint8_t reg : range(8))
for (uint8_t dreg : range(8) ) {
	EffectiveAddress src{mode, reg};
	DataRegister dest{dreg};

	//add
	opcode = _parse("1101 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;			
	bindL(opcode | 0 << 6, Arithmetic<Byte __ Add>, dest, src);
	bindL(opcode | 1 << 6, Arithmetic<Word __ Add>, dest, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ Add>, dest, src);
	//sub
	opcode = _parse("1001 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;	
	bindL(opcode | 0 << 6, Arithmetic<Byte __ Sub>, dest, src);
	bindL(opcode | 1 << 6, Arithmetic<Word __ Sub>, dest, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ Sub>, dest, src);
	//cmp
	opcode = _parse("1011 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 6, Cmp<Byte>, dest, src);
	bindL(opcode | 1 << 6, Cmp<Word>, dest, src);
	bindL(opcode | 2 << 6, Cmp<Long>, dest, src);	
	//and 
	opcode = _parse("1100 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 6, Arithmetic<Byte __ And>, dest, src);
	bindL(opcode | 1 << 6, Arithmetic<Word __ And>, dest, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ And>, dest, src);
	//or
	opcode = _parse("1000 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 6, Arithmetic<Byte __ Or>, dest, src);
	bindL(opcode | 1 << 6, Arithmetic<Word __ Or>, dest, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ Or>, dest, src);		
}

//add sub and or ea
for (uint8_t mode : range(2,5))
for (uint8_t reg : range(8))
for (uint8_t dreg : range(8) ) {
	EffectiveAddress modify{mode, reg};
	DataRegister src{dreg};

	//add
	opcode = _parse("1101 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;		
	bindL(opcode | 0 << 6, Arithmetic<Byte __ Add>, modify, src);	
	bindL(opcode | 1 << 6, Arithmetic<Word __ Add>, modify, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ Add>, modify, src);

	//sub
	opcode = _parse("1001 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;		
	bindL(opcode | 0 << 6, Arithmetic<Byte __ Sub>, modify, src);	
	bindL(opcode | 1 << 6, Arithmetic<Word __ Sub>, modify, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ Sub>, modify, src);	

	//and
	opcode = _parse("1100 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 6, Arithmetic<Byte __ And>, modify, src);	
	bindL(opcode | 1 << 6, Arithmetic<Word __ And>, modify, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ And>, modify, src);

	//or
	opcode = _parse("1000 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 6, Arithmetic<Byte __ Or>, modify, src);	
	bindL(opcode | 1 << 6, Arithmetic<Word __ Or>, modify, src);
	bindL(opcode | 2 << 6, Arithmetic<Long __ Or>, modify, src);
}
//eor
for (uint8_t mode : range(2,5))
for (uint8_t reg : range(8))
for (uint8_t dreg : range(8) ) {
	EffectiveAddress modify{mode, reg};
	DataRegister src{dreg};

	opcode = _parse("1011 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 6, Eor<Byte>, modify, src);	
	bindL(opcode | 1 << 6, Eor<Word>, modify, src);
	bindL(opcode | 2 << 6, Eor<Long>, modify, src);	
}

//adda suba cmpa
for (uint8_t areg : range(8))
for (uint8_t mode : range(2,5))
for (uint8_t reg : range(8)) {
	EffectiveAddress src{mode, reg};
	AddressRegister modify{areg};

	opcode = _parse("1101 ---s 11-- ----") | areg << 9 | mode << 3 | reg;		
	bindL(opcode | 0 << 8, Adda<Word>, modify, src);
	bindL(opcode | 1 << 8, Adda<Long>, modify, src);	

	opcode = _parse("1001 ---s 11-- ----") | areg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 8, Suba<Word>, modify, src);
	bindL(opcode | 1 << 8, Suba<Long>, modify, src);	

	opcode = _parse("1011 ---s 11-- ----") | areg << 9 | mode << 3 | reg;
	bindL(opcode | 0 << 8, Cmpa<Word>, modify, src);
	bindL(opcode | 1 << 8, Cmpa<Long>, modify, src);
}
//clr neg negx not tst nbcd
for (uint8_t mode : range(2,5))
for (uint8_t reg : range(8)) {
	EffectiveAddress modify{mode, reg};
	//clr
	opcode = _parse("0100 0010 ss-- ----") | mode << 3 | reg;
	bindL(opcode | 0 << 6, Clr<Byte>, modify);
	bindL(opcode | 1 << 6, Clr<Word>, modify);
	bindL(opcode | 2 << 6, Clr<Long>, modify);
	//neg
	opcode = _parse("0100 0100 ss-- ----") | mode << 3 | reg;
	bindL(opcode | 0 << 6, Neg<Byte __ false>, modify);
	bindL(opcode | 1 << 6, Neg<Word __ false>, modify);
	bindL(opcode | 2 << 6, Neg<Long __ false>, modify);
	//negx
	opcode = _parse("0100 0000 ss-- ----") | mode << 3 | reg;
	bindL(opcode | 0 << 6, Neg<Byte __ true>, modify);
	bindL(opcode | 1 << 6, Neg<Word __ true>, modify);
	bindL(opcode | 2 << 6, Neg<Long __ true>, modify);
	//not
	opcode = _parse("0100 0110 ss-- ----") | mode << 3 | reg;
	bindL(opcode | 0 << 6, Not<Byte>, modify);
	bindL(opcode | 1 << 6, Not<Word>, modify);
	bindL(opcode | 2 << 6, Not<Long>, modify);	
	//tst
	opcode = _parse("0100 1010 ss-- ----") | mode << 3 | reg;
	bindL(opcode | 0 << 6, Tst<Byte>, modify);
	bindL(opcode | 1 << 6, Tst<Word>, modify);
	bindL(opcode | 2 << 6, Tst<Long>, modify);	
	//nbcd
	opcode = _parse("0100 1000 00-- ----") | mode << 3 | reg;
	bindL(opcode, Nbcd, modify);
}
// addx subx abcd sbcd
for (uint8_t srcReg : range(8))
for (uint8_t destReg : range(8)) {
	EffectiveAddress src{AddressRegisterIndirectWithPreDecrement, srcReg};	
	EffectiveAddress dest{AddressRegisterIndirectWithPreDecrement, destReg};

	//addx
	opcode = _parse("1101 ---1 ss00 1---")	| destReg << 9 | srcReg;
	bindL(opcode | 0 << 6, ArithmeticX<Byte __ Add>, src, dest);
	bindL(opcode | 1 << 6, ArithmeticX<Word __ Add>, src, dest);
	bindL(opcode | 2 << 6, ArithmeticX<Long __ Add>, src, dest);
	//subx
	opcode = _parse("1001 ---1 ss00 1---")	| destReg << 9 | srcReg;
	bindL(opcode | 0 << 6, ArithmeticX<Byte __ Sub>, src, dest);
	bindL(opcode | 1 << 6, ArithmeticX<Word __ Sub>, src, dest);
	bindL(opcode | 2 << 6, ArithmeticX<Long __ Sub>, src, dest);
	//abcd
	opcode = _parse("1100 ---1 0000 1---")	| destReg << 9 | srcReg;
	bindL(opcode, ArithmeticX<Byte __ Abcd>, src, dest);
	//sbcd
	opcode = _parse("1000 ---1 0000 1---")	| destReg << 9 | srcReg;
	bindL(opcode, ArithmeticX<Byte __ Sbcd>, src, dest);
}

//cmpm
for (uint8_t srcReg : range(8))
for (uint8_t destReg : range(8)) {
	EffectiveAddress src{AddressRegisterIndirectWithPostIncrement, srcReg};	
	EffectiveAddress dest{AddressRegisterIndirectWithPostIncrement, destReg};
	
	opcode = _parse("1011 ---1 ss00 1---")	| destReg << 9 | srcReg;
	
	bindL(opcode | 0 << 6, Cmpm<Byte>, src, dest);
	bindL(opcode | 1 << 6, Cmpm<Word>, src, dest);
	bindL(opcode | 2 << 6, Cmpm<Long>, src, dest);
}

//shift/rotate effective address
for (uint8_t mode : range(2,5))
for (uint8_t reg : range(8)) {	
	EffectiveAddress modify{mode, reg};	
	//asl
	opcode = _parse("1110 0001 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Asl>, modify);
	//asr
	opcode = _parse("1110 0000 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Asr>, modify);
	//lsl
	opcode = _parse("1110 0011 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Lsl>, modify);
	//lsr
	opcode = _parse("1110 0010 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Lsr>, modify);
	//rol
	opcode = _parse("1110 0111 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Rol>, modify);
	//ror
	opcode = _parse("1110 0110 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Ror>, modify);
	//roxl
	opcode = _parse("1110 0101 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Roxl>, modify);
	//roxr
	opcode = _parse("1110 0100 11-- ----") | mode << 3 | reg;
	bindL(opcode, EaShift<Roxr>, modify);
}

#undef __
#undef bindL
#undef _parse
