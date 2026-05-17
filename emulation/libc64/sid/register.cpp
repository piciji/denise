
#include "reSid.h"

namespace LIBC64 {

auto ReSid::peekIO( uint8_t addr ) -> uint8_t {
    addr &= 0x1f;

    switch( addr ) {
        case 0x19:
        case 0x1a:
            if (!sysTimer.has( &sidManager.callPotUpdate )) {
                sidManager.potX = sidManager.getPotX();
                sidManager.potY = sidManager.getPotY();
                sysTimer.add( &sidManager.callPotUpdate, 512, Emulator::SystemTimer::Action::WhenNotExistsOnly );
            }
            return addr == 0x19 ? sidManager.potX : sidManager.potY;
        case 0x1b:
            return voice[2].osc3 >> 4;
        case 0x1c:
            return envelope[2].env3;
    }

    return lastBusValue;
}

auto ReSid::readIO( uint8_t addr ) -> uint8_t {
    addr &= 0x1f;
    
    switch( addr ) {
        case 0x19:
        case 0x1a:
            // sid has two AD converters. AD conversion is not emulated directly,
            // because we simply don't have a voltage to convert from.
            // we fetch resulting digital value directly from input emulation.
            if (!sysTimer.has( &sidManager.callPotUpdate )) {
                sidManager.potX = sidManager.getPotX();
                sidManager.potY = sidManager.getPotY();
                sysTimer.add( &sidManager.callPotUpdate, 512, Emulator::SystemTimer::Action::WhenNotExistsOnly );
            }
            lastBusValue = addr == 0x19 ? sidManager.potX : sidManager.potY;
            databusDecay = databusDecayTime;
            break;
		case 0x1b:
			lastBusValue = voice[2].osc3 >> 4;
            databusDecay = databusDecayTime;
            break;
        case 0x1c:
            lastBusValue = envelope[2].env3;
            databusDecay = databusDecayTime;
            break;
    }
    
    return lastBusValue;
}

auto ReSid::writeIO( uint8_t addr, uint8_t value ) -> void {
 
    addr &= 0x1f;
    lastBusValue = value;
    databusDecay = databusDecayTime;

    if (usbSIDPico.enabled && system->displayFrame())
        usbSIDPico.store(addr, value, nr);
    
    switch( addr ) {
        
        case 0x0:
            voice[0].setFrequencyLo( value );
            return;
            
        case 0x1:
            voice[0].setFrequencyHi( value );
            return;
        
        case 0x2:
            voice[0].setPwLo( value );
            return;
            
        case 0x3:
            voice[0].setPwHi( value );
            return;
            
        case 0x4:
            envelope[0].control( value & 1 );
            voice[0].setControl( value );       
            return;
            
        case 0x5:
            envelope[0].setAttackDecay( value );
            return;
            
        case 0x6:
            envelope[0].setSustainRelease( value );
            return;

        case 0x7:
            voice[1].setFrequencyLo( value );
            return;
            
        case 0x8:
            voice[1].setFrequencyHi( value );
            return;
            
        case 0x9:
            voice[1].setPwLo( value );
            return;
            
        case 0xa:
            voice[1].setPwHi( value );
            return;
            
        case 0xb:
            envelope[1].control( value & 1 );
            voice[1].setControl( value );
            return;
            
        case 0xc:
            envelope[1].setAttackDecay( value );
            return;
            
        case 0xd:
            envelope[1].setSustainRelease( value );
            return;

        case 0xe:
            voice[2].setFrequencyLo( value );
            return;
            
        case 0xf:
            voice[2].setFrequencyHi( value );
            return;
            
        case 0x10:
            voice[2].setPwLo( value );
            return;
            
        case 0x11:
            voice[2].setPwHi( value );
            return;
            
        case 0x12:
            envelope[2].control( value & 1 );
            voice[2].setControl( value );
            return;
            
        case 0x13:
            envelope[2].setAttackDecay( value );
            return;
            
        case 0x14:
            envelope[2].setSustainRelease( value );
            return;		
    }

    writeIOFilter( addr, value );
}

auto ReSid::writeIOFilter( uint8_t addr, uint8_t value ) -> void {
	//addr &= 0x1f;
	
	switch(addr) {
		case 0x15:
			filter.writeFcLow( value );
            chamberlinFilter.setSVF(sampleRate);
			break;
		case 0x16:
			filter.writeFcHi( value );
            chamberlinFilter.setSVF(sampleRate);
			break;
		case 0x17:
			filter.writeResFilt( value );
            chamberlinFilter.setSVF(sampleRate);
			break;
		case 0x18:            
			filter.writeModeVol( value );
			break;
	}
}

}
