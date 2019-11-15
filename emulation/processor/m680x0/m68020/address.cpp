
template<uint8_t Size, bool calcEaOnly, uint8_t specialCase> auto M68020::fetch(EffectiveAddress& ea) -> uint32_t {
	if (ea.calculated) return ea.address;
	uint32_t adr;
	
	switch(ea.mode) {
		case DataRegisterDirect:
			return Base::read<Size>( DataRegister{ea.reg} );
			
		case AddressRegisterDirect:
			return Base::read<Size>( AddressRegister{ea.reg} );
			
		case AddressRegisterIndirect:
			addSequencer(calcEaOnly ? 2 : 1);
			return Base::read( AddressRegister{ea.reg} );
			
		case AddressRegisterIndirectWithPostIncrement:
			addSequencer(calcEaOnly ? 2 : 1);
			return Base::read( AddressRegister{ea.reg} );
			
		case AddressRegisterIndirectWithPreDecrement:
			addSequencer(2);
			adr = Base::read( AddressRegister{ea.reg} );
			adr -= bytes<Size>();
			if (Size == Byte && ea.reg == 7) adr -= 1;
			return adr;
			
		case AddressRegisterIndirectWithDisplacement:			
			adr = Base::read( AddressRegister{ea.reg} ) + (int16_t)ctx->stageC;
			if (specialCase != NoLastPrefetch) readExtensionWord();		
			addSequencer(2);
			if (calcEaOnly) syncToBus();
			return adr;
			
		case AddressRegisterIndirectWithIndex: {			
			adr = Base::read( AddressRegister{ea.reg} );
		d8Xn:
			addSequencer(2);
			uint16_t ew = ctx->stageC;
			
			uint8_t reg = (ew & 0x7000) >> 12;
			uint32_t dispReg = ew & 0x8000 
					? Base::read( AddressRegister{reg} )
					: Base::read( DataRegister{reg} );
			if ( !(ew & 0x800) ) dispReg = (int16_t)dispReg;	
													
			fullextensionAddressing(adr, dispReg, ew);
			if (specialCase != NoLastPrefetch) readExtensionWord();
			addSequencer(2);
			if (calcEaOnly) syncToBus();
			return adr;
		}
		case AbsoluteShort:
			adr = (int16_t)ctx->stageC;
			if (specialCase != NoLastPrefetch) readExtensionWord();
			addSequencer(calcEaOnly ? 2 : 1);
			if (calcEaOnly) syncToBus();
			return adr;
			
		case AbsoluteLong:
			adr = ctx->stageC << 16 | ctx->stageB;
			addSequencer(1);
			readExtensionWord();
			if (specialCase != NoLastPrefetch) readExtensionWord();
			if (calcEaOnly) addSequencer(3);
			if (calcEaOnly) syncToBus();
			return adr;
			
		case ProgramCounterIndirectWithDisplacement:			
			adr = ctx->pc + 2 + (int16_t)ctx->stageC;
			if (specialCase != NoLastPrefetch) readExtensionWord();
			addSequencer(2);
			if (calcEaOnly) syncToBus();
			return adr;
			
		case ProgramCounterIndirectWithIndex:			
			adr = ctx->pc + 2;
			goto d8Xn;
			
		case Immediate:
			if (Size == Byte || Size == Word) {
				adr = Size == Byte ? ctx->stageC & 0xff : ctx->stageC; 
				if (specialCase != NoLastPrefetch) readExtensionWord();
				addSequencer(2);
				syncToBus();
				return adr;
			}
			
			adr = ctx->stageC << 16 | ctx->stageB;
			addSequencer(2);
			readExtensionWord();
			if (specialCase != NoLastPrefetch) readExtensionWord();			
			addSequencer(2);
			syncToBus();
			return adr;
	}	
}

template<uint8_t Size> auto M68020::read(EffectiveAddress& ea) -> uint32_t {
	ea.address = fetch<Size, false>(ea);
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
	
	switch(ea.mode) {
		case AddressRegisterIndirect:
		case AddressRegisterIndirectWithPostIncrement:
		case AbsoluteShort:
			addSequencer(1);
			break;
			
		case AbsoluteLong:
			addSequencer(3);
			break;
	}
	
	syncToBus();
	Base::updateRegA<Size>(ea);
	return data;
}

template<uint8_t Size> auto M68020::write(EffectiveAddress& ea, uint32_t data) -> void {
	
	switch (ea.mode) {
		case Immediate:
			return;
			
		case DataRegisterDirect:
			return Base::write<Size>(DataRegister{ea.reg}, data);

		case AddressRegisterDirect:
			return Base::write(AddressRegister{ea.reg}, data);
			
		case ProgramCounterIndirectWithDisplacement:
		case ProgramCounterIndirectWithIndex:
			state.data = false;
			break;
			
		default:
			state.data = true;
			break;
	}
	
	write<Size>(ea.address, data);
			
	if (!ea.calculated) {
		Base::updateRegA<Size>(ea);
		
		switch(ea.mode) {
			case AddressRegisterIndirect:
			case AddressRegisterIndirectWithPostIncrement:
			case AbsoluteShort:
				addSequencer(1);
				break;

			case AbsoluteLong:
				addSequencer(3);
				break;
		}
		syncToBus();
	}
}
/* approximate timing */
auto M68020::fullextensionAddressing( uint32_t& adr, uint32_t dispReg, uint16_t ew ) -> void {
    
    dispReg <<= (ew >> 9) & 3; //scale
    
    if ( !(ew & 0x100)) { //brief extension word;		
        uint8_t displacement = ew & 0xff;
        adr += dispReg + (int8_t)displacement;
        
    } else { //use full extension word
        int32_t outer = 0, disp = 0;
        int32_t base = adr;
        
        if (ew & 0x80) base = 0; //surpress base
        if (ew & 0x40) dispReg = 0; //surpress index operand
        
        if ((ew & 0x30) == 0x20) { //word displacement
			addSequencer(2);
            readExtensionWord();
            disp = (int16_t)ctx->stageC;
			addSequencer(2);
            
        } else if ((ew & 0x30) == 0x30) { //long displacement
			addSequencer(2);
            readExtensionWord();
            disp = ctx->stageC << 16;
			addSequencer(2);
            readExtensionWord();
            disp |= ctx->stageC;
			addSequencer(2);
        }
        base += disp;
        
        if ((ew & 0x3) == 0x2) {
			addSequencer(2);
            readExtensionWord();
            outer = (int16_t)ctx->stageC;
			addSequencer(2);
            
        } else if ((ew & 0x3) == 0x3) {
			addSequencer(2);
            readExtensionWord();
            outer = ctx->stageC << 16;
			addSequencer(2);
            readExtensionWord();
            outer |= ctx->stageC;
			addSequencer(2);
        }
        
        if (!(ew & 4)) base += dispReg; //preindexed
        if (ew & 3) base = read<Long>(base);
        if (ew & 4) base += dispReg; //postindexed
        
        adr = base + outer;		
    }
}
