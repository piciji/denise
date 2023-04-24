
#include "m6502.h"
#include "../drive/drive.h"
#include "../../system/system.h"

namespace LIBC64 {	
	
#include "opcodes.cpp"		

auto M6502::power() -> void {

	regS = 0x00;
	//some of these values could be random on first power on
	regX = 0x00;
	regY = 0x00;
	regA = 0xaa;
	pc = 0x00ff;	
	SET_STATUS( 0x02 )

	reset();
}

auto M6502::reset() -> void {
	
	irqPending = interruptSampled = false;

    nmiPending = false;
	
    soSample = 0;
    
	killed = false;
	
    step = 0;
    
    readNext = true;
    
    soBlock = 0;
    
	resetRoutine();
}

auto M6502::resetRoutine() -> void {
	READ( pc )
	READ( pc )
	READ( pc )
	READ( 0x100 | regS-- )
	READ( 0x100 | regS-- )
	READ( 0x100 | regS-- )
		
	READ( 0xfffc )
	pc = dataBus;
	READ( 0xfffd )
	pc |= dataBus << 8;
	SET_FLAG_I( 1 )	
}

template<bool software> inline auto M6502::interrupt() -> void {	
	
	if (!software) {
		READ( pc )
		READ( pc )
	} else {
		READ_PC_INC
	}
	
	PUSH((pc >> 8) & 0xff);
	PUSH( pc & 0xff);

	uint16_t vector = 0xfffe;

    if (nmiPending) {
        nmiPending = false;
        vector = 0xfffa; // a late nmi can hijack irq
    }

	SET_FLAG_B( software )
	
    PUSH_STATUS
	
	READ( vector )
	
	pc = dataBus;
	
	SET_FLAG_I( 1 )

	READ( vector + 1 )
		
	pc |= dataBus << 8;
	
	/**	 
	 * no interrupt polling at the end of this service routine (software break too)
	 * so at least one opcode is following before could interrupted again by nmi
	 */

	interruptSampled = false;
}

auto M6502::setIrq(bool state) -> void {
	// level sensitive
	irqPending = state;
}

auto M6502::setNmi() -> void {
    // edge sensitive ( triggers only: 0 -> 1)
    nmiPending = true;
}

auto M6502::setMagicForAne(uint8_t magicAne) -> void {
	this->magicAne = magicAne;
}

auto M6502::setMagicForLax(uint8_t magicLax) -> void {
	this->magicLax = magicLax;
}

auto M6502::triggerSO(uint8_t delay) -> void {
    // edge sensitive
    this->soSample = delay;
}

auto M6502::handleSo() -> void {
    // a sampled external SO is executed in following first half cycle. when cpu accesses v flag
    // internally in second half cycle, the external change in first half cycle is wasted.
    // instructions like adc, sbc don't calculate in last opcode cycle. there is
    // simply no time because the data fetch for calculation happens in last half cycle.
    // the calculation is done in first cycle of next instruction during the opcode fetch.
    // for simplicity in emulation the calculation is done in context of last instruction cycle.
    // so we need to take care in case of external SO in emulation, because it would
    // execute in wrong order. thats why we use the following "Block" variable,
    // seted in the end of overflow accessing instructions.

    if (soBlock) {
        // external overflow is not delayed but prevented
        soBlock--;

        if (soSample) // only a delayed sample could survive this block, because the delay happens outside of CPU (here emulated inside)
            soSample--;

    } else if (soSample) {
        soSample--;

        if (!soSample) {
            SET_FLAG_V(1)
        }
    }
}


auto M6502::serialize(Emulator::Serializer& s) -> void {
	
	s.integer( irqPending );
	s.integer( interruptSampled );
    s.integer( nmiPending );
	s.integer( killed );
	s.integer( pc );
    s.integer( IR );
	s.integer( regX );
	s.integer( regY );
	s.integer( regA );
	s.integer( regS );
	s.integer( regP );
	s.integer( flagZ );
	s.integer( flagN );
	s.integer( magicAne );
	s.integer( magicLax );
    
    s.integer( step );
    s.integer( readNext );
    
    s.integer( soBlock );
    s.integer( soSample );
    
    s.integer( zeroPage );
    s.integer( absolute );
    s.integer( absIndexed );
    s.integer( dataBus );
    s.integer( _value );
}

}

