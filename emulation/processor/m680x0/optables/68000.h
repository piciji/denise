
#define __ ,
#define bind( id, name, ... ) { \
	assert( !opTable[id] ); \
	opTable[id] = [=] { return op##name(__VA_ARGS__); }; \
}

#define _parse(s) (uint16_t)Base::parse(s)

//shift/rotate immediate
for ( uint8_t count : range(8) )
for ( uint8_t dreg : range(8) ) {		
	auto shift = count ? count : 8;
	DataRegister modify{dreg};

	opcode = _parse("1110 ---1 ss00 0---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Asl>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Asl>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Asl>, shift, modify);

	opcode = _parse("1110 ---0 ss00 0---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Asr>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Asr>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Asr>, shift, modify);

	opcode = _parse("1110 ---1 ss00 1---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Lsl>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Lsl>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Lsl>, shift, modify);

	opcode = _parse("1110 ---0 ss00 1---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Lsr>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Lsr>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Lsr>, shift, modify);

	opcode = _parse("1110 ---1 ss01 1---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Rol>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Rol>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Rol>, shift, modify);

	opcode = _parse("1110 ---0 ss01 1---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Ror>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Ror>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Ror>, shift, modify);

	opcode = _parse("1110 ---1 ss01 0---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Roxl>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Roxl>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Roxl>, shift, modify);

	opcode = _parse("1110 ---0 ss01 0---") | count << 9 | dreg;
	bind(opcode | 0 << 6, ImmShift<Byte __ Roxr>, shift, modify);
	bind(opcode | 1 << 6, ImmShift<Word __ Roxr>, shift, modify);
	bind(opcode | 2 << 6, ImmShift<Long __ Roxr>, shift, modify);
}

//shift/rotate register
for (uint8_t pos : range(8))
for (uint8_t dreg : range(8)) {		
	DataRegister shift{pos};
	DataRegister modify{dreg};

	opcode = _parse("1110 ---1 ss10 0---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Asl>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Asl>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Asl>, shift, modify);

	opcode = _parse("1110 ---0 ss10 0---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Asr>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Asr>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Asr>, shift, modify);

	opcode = _parse("1110 ---1 ss10 1---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Lsl>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Lsl>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Lsl>, shift, modify);

	opcode = _parse("1110 ---0 ss10 1---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Lsr>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Lsr>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Lsr>, shift, modify);

	opcode = _parse("1110 ---1 ss11 1---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Rol>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Rol>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Rol>, shift, modify);

	opcode = _parse("1110 ---0 ss11 1---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Ror>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Ror>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Ror>, shift, modify);

	opcode = _parse("1110 ---1 ss11 0---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Roxl>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Roxl>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Roxl>, shift, modify);

	opcode = _parse("1110 ---0 ss11 0---") | pos << 9 | dreg;
	bind(opcode | 0 << 6, RegShift<Byte __ Roxr>, shift, modify);
	bind(opcode | 1 << 6, RegShift<Word __ Roxr>, shift, modify);
	bind(opcode | 2 << 6, RegShift<Long __ Roxr>, shift, modify);
}

//shift/rotate effective address
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode <= 1 || (mode == 7 && reg >= 2)) continue;
	EffectiveAddress modify{mode, reg};

	opcode = _parse("1110 0001 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Asl>, modify);

	opcode = _parse("1110 0000 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Asr>, modify);

	opcode = _parse("1110 0011 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Lsl>, modify);

	opcode = _parse("1110 0010 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Lsr>, modify);

	opcode = _parse("1110 0111 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Rol>, modify);

	opcode = _parse("1110 0110 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Ror>, modify);

	opcode = _parse("1110 0101 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Roxl>, modify);

	opcode = _parse("1110 0100 11-- ----") | mode << 3 | reg;
	bind(opcode, EaShift<Roxr>, modify);
}

// bit manipulation
for (uint8_t dreg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode == 1) continue;

	EffectiveAddress modify{mode, reg};
	DataRegister bitreg{dreg};

	if (mode != 7 || reg < 2) {
		opcode = _parse("0000 ---1 01-- ----") | dreg << 9 | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, Bit<Long __ Bchg>, modify, bitreg);
		} else {
			bind(opcode, Bit<Byte __ Bchg>, modify, bitreg);
		}
		opcode = _parse("0000 ---1 11-- ----") | dreg << 9 | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, Bit<Long __ Bset>, modify, bitreg);
		} else {
			bind(opcode, Bit<Byte __ Bset>, modify, bitreg);
		}
		opcode = _parse("0000 ---1 10-- ----") | dreg << 9 | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, Bit<Long __ Bclr>, modify, bitreg);
		} else {
			bind(opcode, Bit<Byte __ Bclr>, modify, bitreg);
		}
	}
	if (mode != 7 || reg <= 4) {
		opcode = _parse("0000 ---1 00-- ----") | dreg << 9 | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, Bit<Long __ Btst>, modify, bitreg);
		} else {
			bind(opcode, Bit<Byte __ Btst>, modify, bitreg);
		}
	}
}

// bit manipulation Immediate
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode == 1) continue;
	EffectiveAddress modify{mode, reg};

	if (mode != 7 || reg < 2) {
		opcode = _parse("0000 1000 01-- ----") | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, ImmBit<Long __ Bchg>, modify);
		} else {
			bind(opcode, ImmBit<Byte __ Bchg>, modify);
		}
		opcode = _parse("0000 1000 11-- ----") | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, ImmBit<Long __ Bset>, modify);
		} else {
			bind(opcode, ImmBit<Byte __ Bset>, modify);
		}
		opcode = _parse("0000 1000 10-- ----") | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, ImmBit<Long __ Bclr>, modify);
		} else {
			bind(opcode, ImmBit<Byte __ Bclr>, modify);
		}
	}
	if (mode != 7 || reg < 4) {
		opcode = _parse("0000 1000 00-- ----") | mode << 3 | reg;
		if (mode == DataRegisterDirect) {
			bind(opcode, ImmBit<Long __ Btst>, modify);
		} else {
			bind(opcode, ImmBit<Byte __ Btst>, modify);
		}
	}
}

// clr
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};
	opcode = _parse("0100 0010 ss-- ----") | mode << 3 | reg;
	
	bind(opcode | 0 << 6, Clr<Byte>, modify);
	bind(opcode | 1 << 6, Clr<Word>, modify);
	bind(opcode | 2 << 6, Clr<Long>, modify);
}

// nbcd
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};
	opcode = _parse("0100 1000 00-- ----") | mode << 3 | reg;
	
	bind(opcode, Nbcd, modify);
}

// neg negx
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};
	opcode = _parse("0100 0100 ss-- ----") | mode << 3 | reg;
	
	bind(opcode | 0 << 6, Neg<Byte __ false>, modify);
	bind(opcode | 1 << 6, Neg<Word __ false>, modify);
	bind(opcode | 2 << 6, Neg<Long __ false>, modify);
	//negx
	opcode = _parse("0100 0000 ss-- ----") | mode << 3 | reg;
	
	bind(opcode | 0 << 6, Neg<Byte __ true>, modify);
	bind(opcode | 1 << 6, Neg<Word __ true>, modify);
	bind(opcode | 2 << 6, Neg<Long __ true>, modify);
}

// not
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};	
	opcode = _parse("0100 0110 ss-- ----") | mode << 3 | reg;
	
	bind(opcode | 0 << 6, Not<Byte>, modify);
	bind(opcode | 1 << 6, Not<Word>, modify);
	bind(opcode | 2 << 6, Not<Long>, modify);	
}

// scc
for (uint8_t mode : range(8))
for (uint8_t reg : range(8))		
for (uint8_t cc : range(16)) {
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0101 ---- 11-- ----") | cc << 8 | mode << 3 | reg;	
	bind(opcode, Scc, modify, cc);
}

// tas
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {		
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};	
	opcode = _parse("0100 1010 11-- ----") | mode << 3 | reg;
	
	bind(opcode, Tas, modify);
}

// tst
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};	
	opcode = _parse("0100 1010 ss-- ----") | mode << 3 | reg;
	
	bind(opcode | 0 << 6, Tst<Byte>, modify);
	bind(opcode | 1 << 6, Tst<Word>, modify);
	bind(opcode | 2 << 6, Tst<Long>, modify);	
}

//add sub and or cmp register
for (uint8_t dreg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 7 && reg >= 5) continue;
	EffectiveAddress src{mode, reg};
	DataRegister dest{dreg};
	
	opcode = _parse("1101 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;	
	if (mode != 1)
		bind(opcode | 0 << 6, Arithmetic<Byte __ Add>, dest, src);
	
	bind(opcode | 1 << 6, Arithmetic<Word __ Add>, dest, src);
	bind(opcode | 2 << 6, Arithmetic<Long __ Add>, dest, src);
	
	opcode = _parse("1001 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;	
	if (mode != 1)
		bind(opcode | 0 << 6, Arithmetic<Byte __ Sub>, dest, src);
	
	bind(opcode | 1 << 6, Arithmetic<Word __ Sub>, dest, src);
	bind(opcode | 2 << 6, Arithmetic<Long __ Sub>, dest, src);
	
	opcode = _parse("1011 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;
	if (mode != 1)
		bind(opcode | 0 << 6, Cmp<Byte>, dest, src);
	
	bind(opcode | 1 << 6, Cmp<Word>, dest, src);
	bind(opcode | 2 << 6, Cmp<Long>, dest, src);	
	
	if (mode != 1) {
		opcode = _parse("1100 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;	
		bind(opcode | 0 << 6, Arithmetic<Byte __ And>, dest, src);
		bind(opcode | 1 << 6, Arithmetic<Word __ And>, dest, src);
		bind(opcode | 2 << 6, Arithmetic<Long __ And>, dest, src);

		opcode = _parse("1000 ---0 ss-- ----") | dreg << 9 | mode << 3 | reg;
		bind(opcode | 0 << 6, Arithmetic<Byte __ Or>, dest, src);
		bind(opcode | 1 << 6, Arithmetic<Word __ Or>, dest, src);
		bind(opcode | 2 << 6, Arithmetic<Long __ Or>, dest, src);			
	}
}

//add sub and or ea
for (uint8_t dreg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 0 || mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};
	DataRegister src{dreg};
	
	opcode = _parse("1101 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;	
	bind(opcode | 0 << 6, Arithmetic<Byte __ Add>, modify, src);	
	bind(opcode | 1 << 6, Arithmetic<Word __ Add>, modify, src);
	bind(opcode | 2 << 6, Arithmetic<Long __ Add>, modify, src);
	
	opcode = _parse("1001 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;
	bind(opcode | 0 << 6, Arithmetic<Byte __ Sub>, modify, src);	
	bind(opcode | 1 << 6, Arithmetic<Word __ Sub>, modify, src);
	bind(opcode | 2 << 6, Arithmetic<Long __ Sub>, modify, src);	
	
	opcode = _parse("1100 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;	
	bind(opcode | 0 << 6, Arithmetic<Byte __ And>, modify, src);	
	bind(opcode | 1 << 6, Arithmetic<Word __ And>, modify, src);
	bind(opcode | 2 << 6, Arithmetic<Long __ And>, modify, src);
	
	opcode = _parse("1000 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;	
	bind(opcode | 0 << 6, Arithmetic<Byte __ Or>, modify, src);	
	bind(opcode | 1 << 6, Arithmetic<Word __ Or>, modify, src);
	bind(opcode | 2 << 6, Arithmetic<Long __ Or>, modify, src);	
}

//adda suba cmpa
for (uint8_t areg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 7 && reg >= 5) continue;
	EffectiveAddress src{mode, reg};
	AddressRegister modify{areg};
	
	opcode = _parse("1101 ---s 11-- ----") | areg << 9 | mode << 3 | reg;	
	bind(opcode | 0 << 8, Adda<Word>, modify, src);
	bind(opcode | 1 << 8, Adda<Long>, modify, src);	
	
	opcode = _parse("1001 ---s 11-- ----") | areg << 9 | mode << 3 | reg;	
	bind(opcode | 0 << 8, Suba<Word>, modify, src);
	bind(opcode | 1 << 8, Suba<Long>, modify, src);	
	
	opcode = _parse("1011 ---s 11-- ----") | areg << 9 | mode << 3 | reg;
	bind(opcode | 0 << 8, Cmpa<Word>, modify, src);
	bind(opcode | 1 << 8, Cmpa<Long>, modify, src);
}

//eor
for (uint8_t dreg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};
	DataRegister src{dreg};
	opcode = _parse("1011 ---1 ss-- ----") | dreg << 9 | mode << 3 | reg;
	
	bind(opcode | 0 << 6, Eor<Byte>, modify, src);	
	bind(opcode | 1 << 6, Eor<Word>, modify, src);
	bind(opcode | 2 << 6, Eor<Long>, modify, src);	
}

//mulu, muls, divu, divs
for (uint8_t dreg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg >= 5) ) continue;
	EffectiveAddress src{mode, reg};
	DataRegister modify{dreg};
	
	opcode = _parse("1100 ---0 11-- ----") | dreg << 9 | mode << 3 | reg;	
	bind(opcode, Mulu, modify, src ); //word
	
	opcode = _parse("1100 ---1 11-- ----") | dreg << 9 | mode << 3 | reg;	
	bind(opcode, Muls, modify, src ); //word
	
	opcode = _parse("1000 ---0 11-- ----") | dreg << 9 | mode << 3 | reg;	
	bind(opcode, Divu, modify, src ); //word

	opcode = _parse("1000 ---1 11-- ----") | dreg << 9 | mode << 3 | reg;	
	bind(opcode, Divs, modify, src ); //word
}

//move
for (uint8_t mode : range(8))
for (uint8_t reg : range(8))
for (uint8_t destmode : range(8))
for (uint8_t destreg : range(8)) {
	if (mode == 7 && reg >= 5) continue;
	if (destmode == 1 || (destmode == 7 && destreg >= 2) ) continue;
	
	EffectiveAddress src{mode, reg};
	EffectiveAddress dest{destmode, destreg};
	
	opcode = _parse("00ss ---- ---- ----")	| destreg << 9 | destmode << 6
											| mode << 3 | reg;	

	if (dest.mode == AddressRegisterIndirectWithPreDecrement) {
		bind(opcode | 1 << 12, MovePreDec<Byte>, src, dest);
		bind(opcode | 3 << 12, MovePreDec<Word>, src, dest);
		bind(opcode | 2 << 12, MovePreDec<Long>, src, dest);
		continue;
	}
	
	if(dest.mode == AbsoluteLong && src.memoryMode() ) {
		bind(opcode | 1 << 12, Move<Byte __ NoLastPrefetch>, src, dest);
		bind(opcode | 3 << 12, Move<Word __ NoLastPrefetch>, src, dest);
		bind(opcode | 2 << 12, Move<Long __ NoLastPrefetch>, src, dest);
		continue;
	}
	
	if (mode != 1)
		bind(opcode | 1 << 12, Move<Byte __ None>, src, dest);
	
	bind(opcode | 3 << 12, Move<Word __ None>, src, dest);
	bind(opcode | 2 << 12, Move<Long __ None>, src, dest);
}

//movea
for (uint8_t areg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 7 && reg >= 5) continue;
	
	EffectiveAddress src{mode, reg};
	AddressRegister modify{areg};
	
	opcode = _parse("00ss ---0 01-- ----")	| areg << 9 | mode << 3 | reg;	

	bind(opcode | 3 << 12, Movea<Word>, src, modify);
	bind(opcode | 2 << 12, Movea<Long>, src, modify);

}

//addi andi cmpi eori ori subi
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;
	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0000 0110 ss-- ----")	| mode << 3 | reg;	
	bind(opcode | 0 << 6, ArithmeticI<Byte __ Add>, modify);
	bind(opcode | 1 << 6, ArithmeticI<Word __ Add>, modify);
	bind(opcode | 2 << 6, ArithmeticI<Long __ Add>, modify);
	
	opcode = _parse("0000 0010 ss-- ----")	| mode << 3 | reg;
	bind(opcode | 0 << 6, ArithmeticI<Byte __ And>, modify);	
	bind(opcode | 1 << 6, ArithmeticI<Word __ And>, modify);
	bind(opcode | 2 << 6, ArithmeticI<Long __ And>, modify);
	
	opcode = _parse("0000 1100 ss-- ----")	| mode << 3 | reg;
	bind(opcode | 0 << 6, Cmpi<Byte>, modify);	
	bind(opcode | 1 << 6, Cmpi<Word>, modify);
	bind(opcode | 2 << 6, Cmpi<Long>, modify);
	
	opcode = _parse("0000 1010 ss-- ----")	| mode << 3 | reg;
	bind(opcode | 0 << 6, ArithmeticI<Byte __ Eor>, modify);	
	bind(opcode | 1 << 6, ArithmeticI<Word __ Eor>, modify);
	bind(opcode | 2 << 6, ArithmeticI<Long __ Eor>, modify);
	
	opcode = _parse("0000 0000 ss-- ----")	| mode << 3 | reg;
	bind(opcode | 0 << 6, ArithmeticI<Byte __ Or>, modify);	
	bind(opcode | 1 << 6, ArithmeticI<Word __ Or>, modify);
	bind(opcode | 2 << 6, ArithmeticI<Long __ Or>, modify);
	
	opcode = _parse("0000 0100 ss-- ----")	| mode << 3 | reg;
	bind(opcode | 0 << 6, ArithmeticI<Byte __ Sub>, modify);	
	bind(opcode | 1 << 6, ArithmeticI<Word __ Sub>, modify);
	bind(opcode | 2 << 6, ArithmeticI<Long __ Sub>, modify);
}

//addq
for (uint8_t data : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 7 && reg >= 2) continue;
	
	opcode = _parse("0101 ---0 ss-- ----")	| data << 9 | mode << 3 | reg;
	data = data == 0 ? 8 : data;
	
	if (mode == 1) {
		AddressRegister areg{reg};
		bind(opcode | 1 << 6, ArithmeticQ<Add>, areg, data);
		bind(opcode | 2 << 6, ArithmeticQ<Add>, areg, data);
		continue;
	}
	
	EffectiveAddress modify{mode, reg};
	bind(opcode | 0 << 6, ArithmeticQ<Byte __ Add>, modify, data);	
	bind(opcode | 1 << 6, ArithmeticQ<Word __ Add>, modify, data);
	bind(opcode | 2 << 6, ArithmeticQ<Long __ Add>, modify, data);
}

//subq
for (uint8_t data : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 7 && reg >= 2) continue;
	
	opcode = _parse("0101 ---1 ss-- ----")	| data << 9 | mode << 3 | reg;
	data = data == 0 ? 8 : data;
	
	if (mode == 1) {
		AddressRegister areg{reg};
		bind(opcode | 1 << 6, ArithmeticQ<Sub>, areg, data);
		bind(opcode | 2 << 6, ArithmeticQ<Sub>, areg, data);
		continue;
	}
	
	EffectiveAddress modify{mode, reg};
	bind(opcode | 0 << 6, ArithmeticQ<Byte __ Sub>, modify, data);	
	bind(opcode | 1 << 6, ArithmeticQ<Word __ Sub>, modify, data);
	bind(opcode | 2 << 6, ArithmeticQ<Long __ Sub>, modify, data);
}

//moveq
for (uint8_t dreg : range(8))
for (uint8_t data : range(256)) {
	DataRegister modify{dreg};
	
	opcode = _parse("0111 ---0 ---- ----")	| dreg << 9 | data;
	
	bind(opcode, Moveq, modify, data);
}

//bcc
for (uint8_t displacement : range(256))
for (uint8_t cond : range(2,16)) {
	opcode = _parse("0110 ---- ---- ----")	| cond << 8 | displacement;

	if (displacement == 0) {
		bind(opcode, Bcc<Word>, cond, displacement);
	} else {		
		bind(opcode, Bcc<Byte>, cond, displacement);
	}
}

//bra
for (uint8_t displacement : range(256)) {
	opcode = _parse("0110 0000 ---- ----")	| displacement;

	if (displacement == 0) {
		bind(opcode, Bra<Word>, displacement);
	} else {		
		bind(opcode, Bra<Byte>, displacement);
	}
}

//bsr
for (uint8_t displacement : range(256)) {
	opcode = _parse("0110 0001 ---- ----")	| displacement;

	if (displacement == 0) {
		bind(opcode, Bsr<Word>, displacement);
	} else {		
		bind(opcode, Bsr<Byte>, displacement);
	}
}

//dbcc
for (uint8_t dreg : range(8))
for (uint8_t cond : range(16)) {
	DataRegister modify{dreg};
	
	opcode = _parse("0101 ---- 1100 1---")	| cond << 8 | dreg;

	bind(opcode, Dbcc, modify, cond);
}

//jmp, jsr
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if ( (mode == 0) || (mode == 1) || (mode == 3) || (mode == 4) || (mode == 7 && reg >= 4)) continue;
	EffectiveAddress src{mode, reg};
	
	opcode = _parse("0100 1110 11-- ----")	| mode << 3 | reg;			
	bind(opcode, Jmp, src);
	
	opcode = _parse("0100 1110 10-- ----")	| mode << 3 | reg;			
	bind(opcode, Jsr, src);
}

//lea
for (uint8_t areg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if ( (mode == 0) || (mode == 1) || (mode == 3) || (mode == 4) || (mode == 7 && reg >= 4)) continue;
	EffectiveAddress src{mode, reg};
	AddressRegister modify{areg};
	
	opcode = _parse("0100 ---1 11-- ----") | areg << 9	| mode << 3 | reg;
	bind(opcode, Lea, src, modify);	
}

//pea
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if ( (mode == 0) || (mode == 1) || (mode == 3) || (mode == 4) || (mode == 7 && reg >= 4)) continue;
	EffectiveAddress src{mode, reg};
	
	opcode = _parse("0100 1000 01-- ----")	| mode << 3 | reg;			
	bind(opcode, Pea, src);	
}

//movem
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 0 || mode == 1 || (mode == 7 && reg >= 4) ) continue;
	
	EffectiveAddress src{mode, reg};	
	
	opcode = _parse("0100 1-00 1s-- ----")	| mode << 3 | reg;
	
	if (mode == 4) {
		AddressRegister modify{reg};
		bind(opcode | 0 << 10 | 0 << 6, MovemPreDec<Word>, modify);
		bind(opcode | 0 << 10 | 1 << 6, MovemPreDec<Long>, modify);
		
	} else if (mode == 3) {		
		AddressRegister modify{reg};
		bind(opcode | 1 << 10 | 0 << 6, MovemPostInc<Word>, modify);
		bind(opcode | 1 << 10 | 1 << 6, MovemPostInc<Long>, modify);
		
	} else {
		if (mode != 7 || reg < 2) {
			bind(opcode | 0 << 10 | 0 << 6, Movem<Word __ false>, src);
			bind(opcode | 0 << 10 | 1 << 6, Movem<Long __ false>, src);				
		}
		
		bind(opcode | 1 << 10 | 0 << 6, Movem<Word __ true>, src);
		bind(opcode | 1 << 10 | 1 << 6, Movem<Long __ true>, src);		
	}
}

//addx subx abcd sbcd -> reg
for (uint8_t srcReg : range(8))
for (uint8_t destReg : range(8)) {
	DataRegister src{srcReg};
	DataRegister dest{destReg};
	
	opcode = _parse("1101 ---1 ss00 0---")	| destReg << 9 | srcReg;
	
	bind(opcode | 0 << 6, ArithmeticX<Byte __ Add>, src, dest);
	bind(opcode | 1 << 6, ArithmeticX<Word __ Add>, src, dest);
	bind(opcode | 2 << 6, ArithmeticX<Long __ Add>, src, dest);
	
	opcode = _parse("1001 ---1 ss00 0---")	| destReg << 9 | srcReg;
	
	bind(opcode | 0 << 6, ArithmeticX<Byte __ Sub>, src, dest);
	bind(opcode | 1 << 6, ArithmeticX<Word __ Sub>, src, dest);
	bind(opcode | 2 << 6, ArithmeticX<Long __ Sub>, src, dest);
	
	opcode = _parse("1100 ---1 0000 0---")	| destReg << 9 | srcReg;
	
	bind(opcode, ArithmeticX<Byte __ Abcd>, src, dest);

	opcode = _parse("1000 ---1 0000 0---")	| destReg << 9 | srcReg;
	
	bind(opcode, ArithmeticX<Byte __ Sbcd>, src, dest);

}

//addx subx abcd sbcd -> mem
for (uint8_t srcReg : range(8))
for (uint8_t destReg : range(8)) {
	EffectiveAddress src{AddressRegisterIndirectWithPreDecrement, srcReg};	
	EffectiveAddress dest{AddressRegisterIndirectWithPreDecrement, destReg};
	
	opcode = _parse("1101 ---1 ss00 1---")	| destReg << 9 | srcReg;
	
	bind(opcode | 0 << 6, ArithmeticX<Byte __ Add>, src, dest);
	bind(opcode | 1 << 6, ArithmeticX<Word __ Add>, src, dest);
	bind(opcode | 2 << 6, ArithmeticX<Long __ Add>, src, dest);
	
	opcode = _parse("1001 ---1 ss00 1---")	| destReg << 9 | srcReg;
	
	bind(opcode | 0 << 6, ArithmeticX<Byte __ Sub>, src, dest);
	bind(opcode | 1 << 6, ArithmeticX<Word __ Sub>, src, dest);
	bind(opcode | 2 << 6, ArithmeticX<Long __ Sub>, src, dest);

	opcode = _parse("1100 ---1 0000 1---")	| destReg << 9 | srcReg;
	
	bind(opcode, ArithmeticX<Byte __ Abcd>, src, dest);
	
	opcode = _parse("1000 ---1 0000 1---")	| destReg << 9 | srcReg;
	
	bind(opcode, ArithmeticX<Byte __ Sbcd>, src, dest);
}

//cmpm
for (uint8_t srcReg : range(8))
for (uint8_t destReg : range(8)) {
	EffectiveAddress src{AddressRegisterIndirectWithPostIncrement, srcReg};	
	EffectiveAddress dest{AddressRegisterIndirectWithPostIncrement, destReg};
	
	opcode = _parse("1011 ---1 ss00 1---")	| destReg << 9 | srcReg;
	
	bind(opcode | 0 << 6, Cmpm<Byte>, src, dest);
	bind(opcode | 1 << 6, Cmpm<Word>, src, dest);
	bind(opcode | 2 << 6, Cmpm<Long>, src, dest);
}

//andi ccr
bind( _parse("0000 0010 0011 1100"), Ccr<And> );
//ori ccr
bind( _parse("0000 0000 0011 1100"), Ccr<Or> );
//eori ccr
bind( _parse("0000 1010 0011 1100"), Ccr<Eor> );
//andi sr
bind( _parse("0000 0010 0111 1100"), Sr<And> );
//ori sr
bind( _parse("0000 0000 0111 1100"), Sr<Or> );
//eori sr
bind( _parse("0000 1010 0111 1100"), Sr<Eor> );

//chk
for (uint8_t dreg : range(8))
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg > 4) ) continue;
	
	EffectiveAddress src{mode, reg};
	DataRegister srcD{dreg};
	
	opcode = _parse("0100 ---1 10-- ----") | dreg << 9 | mode << 3 | reg;
	
	bind(opcode, Chk<Word>, src, srcD);
}

//move from sr (privileged for 68010+)
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg >= 2) ) continue;

	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0100 0000 11-- ----") | mode << 3 | reg;
	
	bind(opcode, MoveFromSr, modify);
}

//move to ccr
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg > 4) ) continue;

	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0100 0100 11-- ----") | mode << 3 | reg;
	
	bind(opcode, MoveToCcr, modify);
}

//move to sr
for (uint8_t mode : range(8))
for (uint8_t reg : range(8)) {
	if (mode == 1 || (mode == 7 && reg > 4) ) continue;

	EffectiveAddress modify{mode, reg};
	
	opcode = _parse("0100 0110 11-- ----") | mode << 3 | reg;
	
	bind(opcode, MoveToSr, modify);
}

//exg data
for (uint8_t dreg1 : range(8))
for (uint8_t dreg2 : range(8)) {
	DataRegister d1{dreg1};
	DataRegister d2{dreg2};
	
	opcode = _parse("1100 ---1 0100 0---") | dreg1 << 9 | dreg2;
	
	bind(opcode, Exg, d1, d2);
}

//exg address
for (uint8_t areg1 : range(8))
for (uint8_t areg2 : range(8)) {
	AddressRegister a1{areg1};
	AddressRegister a2{areg2};
	
	opcode = _parse("1100 ---1 0100 1---") | areg1 << 9 | areg2;
	
	bind(opcode, Exg, a1, a2);
}

//exg data <> address
for (uint8_t areg : range(8))
for (uint8_t dreg : range(8)) {
	AddressRegister a{areg};
	DataRegister d{dreg};
	
	opcode = _parse("1100 ---1 1000 1---") | dreg << 9 | areg;
	
	bind(opcode, Exg, d, a);
}

//ext
for (uint8_t dreg : range(8)) {
	DataRegister modify{dreg};
	
	opcode = _parse("0100 1000 ss00 0---") | dreg;
	
	bind(opcode | 2 << 6, Ext<Word>, modify);
	bind(opcode | 3 << 6, Ext<Long>, modify);
}

//link
for (uint8_t areg : range(8)) {
	AddressRegister modify{areg};
	
	opcode = _parse("0100 1110 0101 0---") | areg;
	
	bind(opcode, Link<Word>, modify);
}

//move to usp
for (uint8_t areg : range(8)) {
	AddressRegister modify{areg};
	opcode = _parse("0100 1110 0110 0---") | areg;
	bind(opcode | 0 << 3, MoveUsp<false>, modify);
	bind(opcode | 1 << 3, MoveUsp<true>, modify);
}

//nop
bind(_parse("0100 1110 0111 0001"), Nop);

//reset
bind(_parse("0100 1110 0111 0000"), Reset);

//rte (behaves different for 68010+)
bind(_parse("0100 1110 0111 0011"), Rte);

//rtr
bind(_parse("0100 1110 0111 0111"), Rtr);

//rts
bind(_parse("0100 1110 0111 0101"), Rts);

//stop
bind(_parse("0100 1110 0111 0010"), Stop);

//swap
for (uint8_t dreg : range(8)) {
	DataRegister modify{dreg};	
	opcode = _parse("0100 1000 0100 0---") | dreg;
	bind(opcode, Swap, modify);
}	

//trap
for (uint8_t vector : range(16)) {
	opcode = _parse("0100 1110 0100 ----") | vector;
	bind(opcode, Trap, vector);	
}

//trapv
bind(_parse("0100 1110 0111 0110"), Trapv);	

//unlk
for (uint8_t areg : range(8)) {
	AddressRegister modify{areg};
	opcode = _parse("0100 1110 0101 1---") | areg;
	bind(opcode, Unlink, modify);
}

//movep
for (uint8_t areg : range(8))
for (uint8_t dreg : range(8)) {
	DataRegister dataReg{dreg};
	EffectiveAddress src{AddressRegisterIndirectWithDisplacement, areg};
	opcode = _parse("0000 ---s ss00 1---") | dreg << 9 | areg;
	
	bind(opcode | 4 << 6, Movep<4>, src, dataReg);
	bind(opcode | 5 << 6, Movep<5>, src, dataReg);
	bind(opcode | 6 << 6, Movep<6>, src, dataReg);
	bind(opcode | 7 << 6, Movep<7>, src, dataReg);
}

#undef __
#undef bind
#undef _parse
