
template<uint8_t Size, uint8_t specialCase> auto M68000::fetch(EffectiveAddress& ea) -> uint32_t {
	if (ea.calculated) return ea.address;
	uint32_t adr;
	
	switch(ea.mode) {
		case DataRegisterDirect:
			return Base::read<Size>( DataRegister{ea.reg} );
			
		case AddressRegisterDirect:
			return Base::read<Size>( AddressRegister{ea.reg} );
			
		case AddressRegisterIndirect:
			return Base::read( AddressRegister{ea.reg} );
			
		case AddressRegisterIndirectWithPostIncrement:
			return Base::read( AddressRegister{ea.reg} );
			
		case AddressRegisterIndirectWithPreDecrement:
			if(specialCase != NoCyclesPreDec) ctx->sync(2);
			adr = Base::read( AddressRegister{ea.reg} );
			adr -= bytes<Size>();
			if (Size == Byte && ea.reg == 7) adr -= 1;
			return adr;
			
		case AddressRegisterIndirectWithDisplacement:
			adr = Base::read( AddressRegister{ea.reg} ) + (int16_t)ctx->irc;
			if (specialCase != NoLastPrefetch) readExtensionWord();
			return adr;
			
		case AddressRegisterIndirectWithIndex: {			
			adr = Base::read( AddressRegister{ea.reg} );
		d8Xn:
			if(specialCase != NoCyclesD8) ctx->sync(2);
			uint8_t displacement = ctx->irc & 0xff;
			uint8_t reg = (ctx->irc & 0x7000) >> 12;
			uint32_t dispReg = ctx->irc & 0x8000 
					? Base::read( AddressRegister{reg} )
					: Base::read( DataRegister{reg} );
			if ( !(ctx->irc & 0x800) ) dispReg = (int16_t)dispReg;			
			adr += dispReg + (int8_t)displacement;
			
			if (specialCase != NoLastPrefetch) readExtensionWord();
			return adr;
		}
		case AbsoluteShort:
			adr = (int16_t)ctx->irc;
			if (specialCase != NoLastPrefetch) readExtensionWord();
			return adr;
			
		case AbsoluteLong:
			adr = ctx->irc << 16;
			readExtensionWord();
			adr |= ctx->irc;
			if (specialCase != NoLastPrefetch) readExtensionWord();
			return adr;
			
		case ProgramCounterIndirectWithDisplacement:
			adr = ctx->prefetchCounterLast + (int16_t)ctx->irc;			
			if (specialCase != NoLastPrefetch) readExtensionWord();
			return adr;
			
		case ProgramCounterIndirectWithIndex:
			adr = ctx->prefetchCounterLast;
			goto d8Xn;
			
		case Immediate:
			if (Size == Byte) {
				adr = ctx->irc & 0xff;
			} else if (Size == Word) {
				adr = ctx->irc;
			} else {
				adr = ctx->irc << 16;
				readExtensionWord();
				adr |= ctx->irc;				
			}
			readExtensionWord();
			return adr;		
	}	
}

template<uint8_t Size, uint8_t specialCase> auto M68000::read(EffectiveAddress& ea) -> uint32_t {
	ea.address = fetch<Size, specialCase>(ea);
	ea.calculated = true;
	
	switch(ea.mode) {
		case DataRegisterDirect:
		case AddressRegisterDirect:
		case Immediate:
			return ea.address;
				
		case ProgramCounterIndirectWithDisplacement:
		case ProgramCounterIndirectWithIndex:
			state.data = false;
			break;
			
		default:
			state.data = true;
			break;
	}
	
	uint32_t data = read<Size>(ea.address);
	Base::updateRegA<Size>(ea);
	return data;
}

template<uint8_t Size> auto M68000::write(EffectiveAddress& ea, uint32_t data, bool lastBusCyle) -> void {
	
	switch (ea.mode) {
		case DataRegisterDirect:
			return Base::write<Size>(DataRegister{ea.reg}, data);
			
		default:
			write<Size>(ea.address, data, lastBusCyle);
			if (!ea.calculated) {
				Base::updateRegA<Size>(ea);
			}
			return;
	}		
}
