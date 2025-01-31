
//  This code is based on VIC-II cycle engine in VICE
//  You can get a copy of the original here: https://sourceforge.net/projects/vice-emu/

/*
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vicII.h"
#include "../expansionPort/expansionPort.h"

#define _fullAdr( __addr ) (((__addr) & 0x3fff) | system->vicBank)

namespace LIBC64 {
	
inline auto VicIICycle::fetchPhi1( uint32_t flags ) -> uint8_t {
	uint8_t sprPos;
	uint8_t value;
	
    if ( isFetchG(flags) )
		value = !idleModeTemp ? fetchG() : fetchIdleG();    
	
    else if ( isSprFirstCycle(flags) ) {
		sprPos = getSpr(flags);		
        value = fetchSpriteP( sprPos );    
	}	
    else if ( isSprSecondCycle(flags) ) {
		sprPos = getSpr(flags);		
        value = fetchSpriteS1( sprPos );   
	}	
    else if ( isRefresh(flags) )
        value = readPhi<true>( _fullAdr(0x3f00 | refreshCounter--) );

	else // idle cycle			
		value = readPhi<true>( _fullAdr(0x3fff) );

	if (baLow && isFetchC(flags))
		fetchC();
	
	return value;
}

inline auto VicIICycle::fetchSprPhi2( uint32_t flags ) -> void {
	uint8_t sprPos;
	
	if (isSprFirstCycle(flags)) {
		sprPos = getSpr(flags);
		fetchSpriteS0(sprPos);
		
	} else if (isSprSecondCycle(flags)) {
		sprPos = getSpr(flags);
		fetchSpriteS2(sprPos);
	}
}

inline auto VicIICycle::fetchSpriteP( uint8_t pos ) -> uint8_t {
    
    sprite[pos].dataP = readPhi<true>( _fullAdr((vm << 10) | 0x3f8 | pos) );
	
	return sprite[pos].dataP;
}

inline auto VicIICycle::sprHasDma(uint8_t pos) -> bool {
    return spriteDma & (1 << pos);
}

auto VicIICycle::fetchSpriteS1(uint8_t pos) -> uint8_t {	
    uint8_t sprdata;

    if (sprHasDma(pos)) {
        sprdata = readPhi<true>( _fullAdr((sprite[pos].dataP << 6) | sprite[pos].mc) );

        sprite[pos].mc++;
        sprite[pos].mc &= 0x3f;
		
    } else {
        sprdata = readPhi<true>( _fullAdr(0x3fff) );
    }

    sprite[pos].dataS &= 0xff00ff;
    sprite[pos].dataS |= sprdata << 8;

    return sprdata;
}

inline auto VicIICycle::fetchSpriteS0(uint8_t pos) -> void {
    uint8_t value = lastBusPhi2;

    if ( sprHasDma(pos) ) {
        if (!aecDelay) {
            value = readPhi<false>( _fullAdr((sprite[pos].dataP << 6) | sprite[pos].mc) );
		}

        sprite[pos].mc++;
        sprite[pos].mc &= 0x3f;
    }

    sprite[pos].dataS &= 0x00ffff;
    sprite[pos].dataS |= value << 16;
}

inline auto VicIICycle::fetchSpriteS2(uint8_t pos) -> void {
    uint8_t value = lastBusPhi2;
	
    if ( sprHasDma(pos) ) {
        if (!aecDelay) {
			value = readPhi<false>( _fullAdr((sprite[pos].dataP << 6) | sprite[pos].mc) );
		}
		
        sprite[pos].mc++;
        sprite[pos].mc &= 0x3f;
    }

    sprite[pos].dataS &= 0xffff00;
    sprite[pos].dataS |= value;
	
	sprite[pos].dataShiftReg = sprite[pos].dataS;
}

auto VicIICycle::fetchC() -> void {
	uint8_t _color;
	uint8_t _dataC;
	
	if ( !aecDelay ) {
		_color = system->colorRam[ vc ] & 0xf;
		_dataC = readPhi<false>( _fullAdr((vm << 10) | vc) );
	} else if (expansionPort->haltMainCpu()) {
		_color = 0;
		if (expansionPort->hasIoOnHost())
			_dataC = readPhi<false>( _fullAdr((vm << 10) | vc) );
		else
			_dataC = 0xff;
	} else {
		_color = readCpu() & 0xf;
		_dataC = 0xff;
	}
		
	cBuffer[ vmli ] = (_color << 8) | _dataC;
}

auto VicIICycle::addrG( uint8_t useMode ) -> uint16_t {
    
    uint16_t addr;

    if (VIC_MODE_BMM( useMode ) ) {
        addr = (vc << 3) | rc;
        addr |= (cb & 4) << 11;
		
    } else {        
        addr = ((cBuffer[ vmli ] & 0xff) << 3) | rc;
        addr |= cb << 11;
    }

    if (VIC_MODE_ECM( useMode ) )
        addr &= 0x39ff;   

    return addr;
}

auto VicIICycle::fetchIdleG() -> uint8_t {
	uint8_t data;

	if (rev65)
		data = modeEcmBmm;
	else
		data = modeEcmBmmDma; //is delayed one cycle for 85xx chips

	if (VIC_MODE_ECM(data) )
		data = readPhi<true>( _fullAdr(0x39ff) );
	else
		data = readPhi<true>( _fullAdr(0x3fff) );
	
	gBuffer = data;
	
	if (gBufferUse) {
		gBufferPipe1 = gBuffer;
		gBufferUse = false;
	}
	
	return data;
}

auto VicIICycle::fetchG() -> uint8_t {            
    uint16_t addr;
    uint8_t data;
        
	if (rev65) {
		addr = addrG( modeEcmBmm | (modeEcmBmmDma & 8) );

		// when Bmm changes
		if ( (modeEcmBmm ^ modeEcmBmmDma) & 8 ) {
			uint16_t addrFrom = addrG( modeEcmBmmDma );
			uint16_t addrTo = addrG( modeEcmBmm );

			if ( !isCharRomAccessed( addrFrom ) && isCharRomAccessed( addrTo ) ) 
				addr = (addrFrom & 0xff) | (addrTo & 0x3f00);
		}

	} else
		addr = addrG( modeEcmBmmDma );

	vmli++;
	vc++;
	vc &= 0x3ff;	
           
    data = readPhi<true>( _fullAdr(addr) );
	
	gBuffer = data;

	if (gBufferUse) {
		gBufferPipe1 = gBuffer;
		gBufferUse = false;
	}

    return data;
}

inline auto VicIICycle::isCharRomAccessed(uint16_t addr) -> bool {
	addr = (addr & 0x3fff) | system->vicBank;

    return !ultimaxPhi1 && ((addr & 0x7000) == 0x1000);
}

inline auto VicIICycle::readCpu() -> uint8_t {
	// we are in second half cycle and VIC pulled BA low but doesn't own BUS.
	// it takes 3 further cycles till VIC can access BUS in second half cycle.
	// so this function is called for 3 cycles in a row.
	// first we need to find out who is BUS Master? CPU or expansion port ?
	if ( !expansionPort->isDma() )
		// at this point CPU is only halted by BA(RDY) when entering a read cycle.
		// even when cpu is halted the address is selected on BUS and the VIC reads
		// in second half cycle from this address but not the CPU.            
		return system->memoryCpu.read( cpu.addressBus() );

	// expansion port is BUS Master... same explanation as above
	return system->memoryCpu.read( expansionPort->addressBus() );            
}

template<bool phi1> inline auto VicIICycle::readPhi(uint16_t addr) -> uint8_t {

    if ((phi1 && !ultimaxPhi1) || (!phi1 && !ultimaxPhi2)) {
        if ((addr & 0x7000) == 0x1000)
            return system->charRom[ addr & 0xfff ];

        return *(system->ram + addr);
    }

    if ((addr & 0x3000) == 0x3000)
        return expansionPort->readRomH( 0x1000 | (addr & 0xfff) );

    // todo: a cartridge could modify address bus and prevent VIC in Ultimax mode from reading C64 memory,
    // instead provide data for it on expansion port.
    // will be implemented when needed, i.e. Turbo Chameleon doing this ? other expansions ?
    return *(system->ram + addr);
}

}
