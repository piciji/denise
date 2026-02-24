
#include "vicII.h"
#include "../expansionPort/expansionPort.h"

#define _fullAdr( __addr ) (((__addr) & 0x3fff) | vicBank)

namespace LIBC64 {
	
template<bool logDma> inline auto VicIICycle::fetchPhi1( uint32_t flags ) -> uint8_t {
	uint8_t sprPos;
	uint8_t value;
	
    if ( isFetchG(flags) ) {
        value = !idleModeTemp ? fetchG<logDma>() : fetchIdleG<logDma>();
        if constexpr (logDma)
            debugger.dma[cycle].usage = DMA_GRAPHICS;
    } else if ( isSprFirstCycle(flags) ) {
		sprPos = getSpr(flags);
        value = fetchSpriteP<logDma>( sprPos );
	} else if ( isSprSecondCycle(flags) ) {
		sprPos = getSpr(flags);
        value = fetchSpriteS1<logDma>( sprPos );
	} else if ( isRefresh(flags) ) {
        value = readPhi<true, logDma>( _fullAdr(0x3f00 | refreshCounter--) );
	    if constexpr (logDma)
            debugger.dma[cycle].usage = DMA_REFRESH;
    } else { // idle cycle
        value = readPhi<true, logDma>( _fullAdr(0x3fff) );
        if constexpr (logDma)
            debugger.dma[cycle].usage = DMA_IDLE;
    }

    if constexpr (logDma) {
        debugger.dma[cycle].data = value;
        debugger.dma[cycle].usageCpu = 0;
    }

	if (baLow) {
	    if (isFetchC(flags)) {
	        fetchC<logDma>();
	    }

	    if constexpr (!logDma) {
	        if (debugger.action == DebuggerAction::HaltCPU) {
	            oneTimeDebuggerAction();
	        }
	    }
	}
	
	return value;
}

auto VicIICycle::fetchSprPhi2( uint32_t flags ) -> void {
	uint8_t sprPos;
	
	if (isSprFirstCycle(flags)) {
		sprPos = getSpr(flags);
		fetchSpriteS0(sprPos);
		
	} else if (isSprSecondCycle(flags)) {
		sprPos = getSpr(flags);
		fetchSpriteS2(sprPos);
	}
}

template<bool logDma> inline auto VicIICycle::fetchSpriteP( uint8_t pos ) -> uint8_t {
    
    sprite[pos].dataP = readPhi<true, logDma>( _fullAdr((vm << 10) | 0x3f8 | pos) );

    if constexpr (logDma)
        debugger.dma[cycle].usage = DMA_SPR_PTR;
	
	return sprite[pos].dataP;
}

inline auto VicIICycle::sprHasDma(uint8_t pos) -> bool {
    return spriteDma & (1 << pos);
}

template<bool logDma> auto VicIICycle::fetchSpriteS1(uint8_t pos) -> uint8_t {
    uint8_t sprdata;
    Sprite& spr = sprite[pos];

    if (sprHasDma(pos)) {
        sprdata = readPhi<true, logDma>( _fullAdr((spr.dataP << 6) | spr.mc) );

        spr.mc++;
        spr.mc &= 0x3f;
		
    } else {
        sprdata = readPhi<true, logDma>( _fullAdr(0x3fff) );
    }

    if constexpr (logDma)
        debugger.dma[cycle].usage = DMA_SPR_DATA;

    spr.dataS &= 0xff00ff;
    spr.dataS |= sprdata << 8;

    return sprdata;
}

inline auto VicIICycle::fetchSpriteS0(uint8_t pos) -> void {
    uint8_t value = lastBusPhi2;
    Sprite& spr = sprite[pos];

    if ( sprHasDma(pos) ) {
        if (!aecDelay) {
            if (debugger.dmaLog) {
                value = readPhi<false, true>( _fullAdr((spr.dataP << 6) | spr.mc) );
                debugger.dma[cycle].usageCpu = DMA_SPR_DATA;
                debugger.dma[cycle].dataCpu = value;
            } else
                value = readPhi<false, false>( _fullAdr((spr.dataP << 6) | spr.mc) );
		}

        spr.mc++;
        spr.mc &= 0x3f;
    }

    spr.dataS &= 0x00ffff;
    spr.dataS |= value << 16;
}

inline auto VicIICycle::fetchSpriteS2(uint8_t pos) -> void {
    uint8_t value = lastBusPhi2;
    Sprite& spr = sprite[pos];
	
    if ( sprHasDma(pos) ) {
        if (!aecDelay) {
            if (debugger.dmaLog) {
                value = readPhi<false, true>( _fullAdr((spr.dataP << 6) | spr.mc) );
                debugger.dma[cycle].usageCpu = DMA_SPR_DATA;
                debugger.dma[cycle].dataCpu = value;
            } else
                value = readPhi<false, false>( _fullAdr((spr.dataP << 6) | spr.mc) );
		}
		
        spr.mc++;
        spr.mc &= 0x3f;
    }

    spr.dataS &= 0xffff00;
    spr.dataS |= value;
	
	spr.dataShiftReg = spr.dataS;

    if (debugger.storeSprites && (spritePending & (1 << pos)))
        storeSprite(spr);
}

template<bool logDma> auto VicIICycle::fetchC() -> void {
	uint8_t _color;
	uint8_t _dataC;
	
	if ( !aecDelay ) {
		_color = system->colorRam[ vc ] & 0xf;
		_dataC = readPhi<false, logDma>( _fullAdr((vm << 10) | vc) );
	    if constexpr (logDma)
	        debugger.dma[cycle].usageCpu = DMA_CHARACTER;
	} else if (expansionPort->haltMainCpu()) {
		_color = 0;
		if (expansionPort->hasIoOnHost()) {
		    _dataC = readPhi<false, logDma>( _fullAdr((vm << 10) | vc) );
		    if constexpr (logDma)
		        debugger.dma[cycle].usageCpu = DMA_CHARACTER;
		} else
			_dataC = 0xff;
	} else {
		_color = readCpu() & 0xf;
		_dataC = 0xff;
	}

    if constexpr (logDma)
        debugger.dma[cycle].dataCpu = _dataC;

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

template<bool logDma> auto VicIICycle::fetchIdleG() -> uint8_t {
	uint8_t data;

	if (rev65)
		data = modeEcmBmm;
	else
		data = modeEcmBmmDma; //is delayed one cycle for 85xx chips

	if (badLine && yScroll)
		_addrG = rev65 ? 0x38ff : 0x3807;
	else if (VIC_MODE_ECM(data) )
		_addrG = 0x39ff;
	else
		_addrG = 0x3fff;

	gBuffer = readPhi<true, logDma>(_fullAdr(_addrG));
	
	if (gBufferUse) {
		gBufferPipe1 = gBuffer;
		gBufferUse = false;
	}
	
	return gBuffer;
}

template<bool logDma> auto VicIICycle::fetchG() -> uint8_t {
    uint8_t data;
        
	if (rev65) {
		_addrG = addrG( modeEcmBmm | (modeEcmBmmDma & 8) );

		// when Bmm changes
		if ( (modeEcmBmm ^ modeEcmBmmDma) & 8 ) {
			uint16_t addrFrom = addrG( modeEcmBmmDma );
			uint16_t addrTo = addrG( modeEcmBmm );

			if ( !isCharRomAccessed( addrFrom ) && isCharRomAccessed( addrTo ) ) 
				_addrG = (addrFrom & 0xff) | (addrTo & 0x3f00);
		}

	} else
		_addrG = addrG( modeEcmBmmDma );

	vmli++;
	vc++;
	vc &= 0x3ff;
           
    data = readPhi<true, logDma>( _fullAdr(_addrG) );
	
	gBuffer = data;

	if (gBufferUse) {
		gBufferPipe1 = gBuffer;
		gBufferUse = false;
	}

    return data;
}

inline auto VicIICycle::isCharRomAccessed(uint16_t addr) -> bool {
	addr = (addr & 0x3fff) | vicBank;

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

template<bool phi1, bool logDma, bool peek> inline auto VicIICycle::readPhi(uint16_t addr) -> uint8_t {

    if constexpr (logDma) {
        if constexpr (phi1)
            debugger.dma[cycle].address = addr;
        else
            debugger.dma[cycle].addrCpu = addr;
    }

    if ((phi1 && !ultimaxPhi1) || (!phi1 && !ultimaxPhi2)) {
        if ((addr & 0x7000) == 0x1000)
            return system->charRom[ addr & 0xfff ];

        return *(system->ram + addr);
    }

    if ((addr & 0x3000) == 0x3000) {
        if constexpr (peek)
            return expansionPort->peekRomH( 0x1000 | (addr & 0xfff) );

        return expansionPort->readRomH( 0x1000 | (addr & 0xfff) );
    }

    // todo: a cartridge could modify address bus and prevent VIC in Ultimax mode from reading C64 memory,
    // instead provide data for it on expansion port.
    // will be implemented when needed, i.e. Turbo Chameleon doing this ? other expansions ?
    return *(system->ram + addr);
}

}
