
#include "vicIIFast.h"
#include "../../system/system.h"
#include "../../expansionPort/expansionPort.h"

namespace LIBC64 {  

auto VicIIFast::scanline() -> void {    
    
    if (den && (vCounter == borderTop)) {
        vFlipFlop = false;
        if (system->mhz2 & 2)
            cpu.setClock(false);
    } else if (vCounter == borderBottom) {
        vFlipFlop = true;
        if (system->mhz2 & 2)
            cpu.setClock(true);
    }
    
    vc = vcBase;   
    
	if (!hFlipFlop)
		std::memset(linePtr + firstVisiblePixel, colorReg[ 0x21 ], hWidth);	
	
    ecmBmmMcm = modeEcmBmm | modeMcm;
    vicBank = system->vicBank;
    linePos = firstVisiblePixel + (ntscBorder ? 56 : 46);

    if (xScroll) {
        uint8_t _col = colorReg[0x21];
        for (unsigned i = 0; i < xScroll; i++)
            *( linePtr + linePos++ ) = _col;        
    }

    switch(ecmBmmMcm) {
        case 0: mode0(); break;
        case 1: mode1(); break;
        case 2: mode2(); break;
        case 3: mode3(); break;
        case 4: mode4(); break;
        case 5: mode5(); break;
        case 6: mode6(); break;
        case 7: mode7(); break;
    }     

    applySprites();

    if (!vFlipFlop) {
		if (hFlipFlop)
			applyBorder();
    } else
        std::memset(linePtr + firstVisiblePixel, colorReg[ 0x20 ], hWidth);       
    
    if (addMeta)
        applyMeta();  
	
	hFlipFlop = 1;
}

inline auto VicIIFast::fetch(unsigned i) -> void {
    static uint16_t addr;
    
    if (badLine) {
        color = 0x80 | (system->colorRam[ vc & 0x3ff ] & 0xf);
        dataC = readPhi<false>((((vm << 10) | vc) & 0x3fff) | vicBank);
        cBuffer[i] = (color<< 8) | dataC;
    } else {
        uint16_t value = cBuffer[i];
        color = value >> 8;
        dataC = value & 0xff;
    }

    if (idleMode) {
        color = 0x80;
        dataC = 0;
        addr = (ecmBmmMcm & 4) ? 0x39ff : 0x3fff;

    } else {
        addr = rc;

        if (ecmBmmMcm & 2) {
            addr |= vc << 3;
            addr |= (cb & 4) << 11;

        } else {
            addr |= dataC << 3;
            addr |= cb << 11;
        }

        if (ecmBmmMcm & 4)
            addr &= 0x39ff;

        vc++;
        vc &= 0x3ff;
    }

    if (!vFlipFlop)
        dataG = readPhi<true>((addr & 0x3fff) | vicBank);
    else 
        dataG = 0;
    
    if (dmaDelay)
        if (--dmaDelay == 0) {
            badLine = allowBadlines && (yScroll == (vCounter & 7));  
            if (badLine)
                idleMode = false;
        }
}

inline auto VicIIFast::mode0() -> void {
    
    uint8_t _background = colorReg[0x21];
    uint8_t* ptr = linePtr + linePos;
    
    for (unsigned i = 0; i < 40; i++) {
        
        fetch(i);        
        
        *ptr++ = (dataG & 0x80) ? color : _background;
        *ptr++ = (dataG & 0x40) ? color : _background;
        *ptr++ = (dataG & 0x20) ? color : _background;
        *ptr++ = (dataG & 0x10) ? color : _background;
        *ptr++ = (dataG & 0x8) ? color : _background;
        *ptr++ = (dataG & 0x4) ? color : _background;
        *ptr++ = (dataG & 0x2) ? color : _background;
        *ptr++ = (dataG & 0x1) ? color : _background;
    }    
}

inline auto VicIIFast::mode1() -> void {
    uint8_t _col;
    uint8_t _useCol;    
    uint8_t gBits;
    uint8_t _background = colorReg[0x21];
    uint8_t* ptr = linePtr + linePos;
    
    for (unsigned i = 0; i < 40; i++) {
        
        fetch(i);            
        
        _col = color & 0x87;    
        
        if (color & 8) {

            gBits = dataG >> 6;
            _useCol = (gBits == 3) ? _col : colorReg[ 0x21 + gBits ];
            _useCol |= (gBits & 2) << 6;
            *ptr++ = _useCol;
            *ptr++ = _useCol;

            gBits = (dataG >> 4) & 3;
            _useCol = (gBits == 3) ? _col : colorReg[ 0x21 + gBits ];
            _useCol |= (gBits & 2) << 6;
            *ptr++ = _useCol;
            *ptr++ = _useCol;

            gBits = (dataG >> 2) & 3;
            _useCol = (gBits == 3) ? _col : colorReg[ 0x21 + gBits ];
            _useCol |= (gBits & 2) << 6;
            *ptr++ = _useCol;
            *ptr++ = _useCol;

            gBits = dataG & 3;
            _useCol = (gBits == 3) ? _col : colorReg[ 0x21 + gBits ];
            _useCol |= (gBits & 2) << 6;
            *ptr++ = _useCol;
            *ptr++ = _useCol;
        } else {
            *ptr++ = (dataG & 0x80) ? _col : _background;
            *ptr++ = (dataG & 0x40) ? _col : _background;
            *ptr++ = (dataG & 0x20) ? _col : _background;
            *ptr++ = (dataG & 0x10) ? _col : _background;
            *ptr++ = (dataG & 0x8) ? _col : _background;
            *ptr++ = (dataG & 0x4) ? _col : _background;
            *ptr++ = (dataG & 0x2) ? _col : _background;
            *ptr++ = (dataG & 0x1) ? _col : _background;
        }          
    }    
}

inline auto VicIIFast::mode2() -> void {
    
    uint8_t _col1;
    uint8_t _col2;
    uint8_t* ptr = linePtr + linePos;
    
    for (unsigned i = 0; i < 40; i++) {
        
        fetch(i);       
        _col1 = 0x80 | ((dataC >> 4) & 15);
        _col2 = dataC & 15;
        
        *ptr++ = (dataG & 0x80) ? _col1 : _col2;
        *ptr++ = (dataG & 0x40) ? _col1 : _col2;
        *ptr++ = (dataG & 0x20) ? _col1 : _col2;
        *ptr++ = (dataG & 0x10) ? _col1 : _col2;
        *ptr++ = (dataG & 0x8) ? _col1 : _col2;
        *ptr++ = (dataG & 0x4) ? _col1 : _col2;
        *ptr++ = (dataG & 0x2) ? _col1 : _col2;
        *ptr++ = (dataG & 0x1) ? _col1 : _col2;
    }    
}

inline auto VicIIFast::mode3() -> void {
    uint8_t _col, _col1, _col2, _useCol;
    uint8_t _background = colorReg[0x21];
    uint8_t gBits;
    uint8_t* ptr = linePtr + linePos;

    for (unsigned i = 0; i < 40; i++) {

        fetch(i);

        _col = color;
        _col1 = (dataC >> 4) & 15;
        _col2 = 0x80 | (dataC & 15);
        
        for (unsigned p = 0; p < 4; p++) {
                 
            gBits = dataG >> 6;

            if (gBits == 1) _useCol = _col1;                
            else if (gBits == 2) _useCol = _col2;
            else if (gBits == 3) _useCol = _col;
            else _useCol = _background;
                        
            *ptr++ = _useCol;
            *ptr++ = _useCol;

            dataG <<= 2;
        }
    } 
}

inline auto VicIIFast::mode4() -> void {
    
    uint8_t _col1;
    uint8_t* ptr = linePtr + linePos;
    
    for (unsigned i = 0; i < 40; i++) {
        
        fetch(i);        
        _col1 = colorReg[ 0x21 + ((dataC >> 6) & 3)]; 
        
        *ptr++ = (dataG & 0x80) ? color : _col1;
        *ptr++ = (dataG & 0x40) ? color : _col1;
        *ptr++ = (dataG & 0x20) ? color : _col1;
        *ptr++ = (dataG & 0x10) ? color : _col1;
        *ptr++ = (dataG & 0x8) ? color : _col1;
        *ptr++ = (dataG & 0x4) ? color : _col1;
        *ptr++ = (dataG & 0x2) ? color : _col1;
        *ptr++ = (dataG & 0x1) ? color : _col1;
    }    
}

inline auto VicIIFast::mode5() -> void {
    uint8_t _useCol;  
    uint8_t* ptr = linePtr + linePos;
    
    for (unsigned i = 0; i < 40; i++) {
        
        fetch(i);  
        
        if (color & 8) {

            _useCol = dataG & 0x80;
            *ptr++ = _useCol;
            *ptr++ = _useCol;
            dataG <<= 2;

            _useCol = dataG & 0x80;
            *ptr++ = _useCol;
            *ptr++ = _useCol;
            dataG <<= 2;
            
            _useCol = dataG & 0x80;
            *ptr++ = _useCol;
            *ptr++ = _useCol;
            dataG <<= 2;
            
            _useCol = dataG & 0x80;
            *ptr++ = _useCol;
            *ptr++ = _useCol;
            dataG <<= 2;
        } else {
            *ptr++ = (dataG & 0x80) ? 0x80 : 0;
            *ptr++ = (dataG & 0x40) ? 0x80 : 0;
            *ptr++ = (dataG & 0x20) ? 0x80 : 0;
            *ptr++ = (dataG & 0x10) ? 0x80 : 0;
            *ptr++ = (dataG & 0x8) ? 0x80 : 0;
            *ptr++ = (dataG & 0x4) ? 0x80 : 0;
            *ptr++ = (dataG & 0x2) ? 0x80 : 0;
            *ptr++ = (dataG & 0x1) ? 0x80 : 0;
        }
    }    
}

inline auto VicIIFast::mode6() -> void {
    
    uint8_t* ptr = linePtr + linePos;
    
    for (unsigned i = 0; i < 40; i++) {
        
        fetch(i);        
        
        *ptr++ = (dataG & 0x80) ? 0x80 : 0;
        *ptr++ = (dataG & 0x40) ? 0x80 : 0;
        *ptr++ = (dataG & 0x20) ? 0x80 : 0;
        *ptr++ = (dataG & 0x10) ? 0x80 : 0;
        *ptr++ = (dataG & 0x8) ? 0x80 : 0;
        *ptr++ = (dataG & 0x4) ? 0x80 : 0;
        *ptr++ = (dataG & 0x2) ? 0x80 : 0;
        *ptr++ = (dataG & 0x1) ? 0x80 : 0;
    }    
}

inline auto VicIIFast::mode7() -> void {
    uint8_t _useCol;
    uint8_t* ptr = linePtr + linePos;

    for (unsigned i = 0; i < 40; i++) {

        fetch(i);
        
        for (unsigned p = 0; p < 4; p++) {
                             
            _useCol = dataG & 0x80;
                        
            *ptr++ = _useCol;
            *ptr++ = _useCol;

            dataG <<= 2;
        }
    } 
}

auto VicIIFast::dmaSprites() -> void {
    uint16_t dataP;     
    vicBank = system->vicBank;
    
    for (unsigned pos = 0; pos < 8; pos++) {
        Sprite* spr = &sprite[pos];
        uint8_t mask = 1 << pos;
		
        if (spr->enabled && !(spriteDma & mask) && ( (vCounter & 0xff) == spr->y ) ) {
            spriteDma |= mask;
            spr->mcBase = 0;
            spr->expandYFlop = true;
        }
        
        spr->mc = spr->mcBase;
        
        if (spriteDma & mask) {
            if (spr->expandY)
                spr->expandYFlop ^= 1;
            
            dataP = readPhi<true>( (((vm << 10) | 0x3f8 | pos) & 0x3fff) | vicBank ) << 6;
            spr->dataS = readPhi<false>( ((dataP | spr->mc) & 0x3fff) | vicBank ) << 16;
            spr->mc++;
            spr->mc &= 63;
            spr->dataS |= readPhi<true>( ((dataP | spr->mc) & 0x3fff) | vicBank ) << 8;
            spr->mc++;
            spr->mc &= 63;
            spr->dataS |= readPhi<false>( ((dataP | spr->mc) & 0x3fff) | vicBank );
            spr->mc++;
            spr->mc &= 63;
        }            
    }
}

auto VicIIFast::dmaSpritesOff() -> void {  
    	
	spriteActive = spriteDma;
    for (unsigned pos = 0; pos < 8; pos++) {
        Sprite* spr = &sprite[pos];
        spr->dataShiftReg = spr->dataS;
        
        if (spr->expandYFlop) {
            spr->mcBase = spr->mc;
            if (spr->mcBase == 63) {
                spriteDma &= ~(1 << pos);
            }
        }
    }
}

auto VicIIFast::applySprites() -> void {

    std::fill_n(drawSprites, VIC_MAX_LINE_LENGTH, nullptr);
    uint8_t* ptr;
    Sprite* sprBefore;
    uint32_t dataShiftReg;
	uint8_t mask;
	unsigned xPos;
    
    for (unsigned sprPos = 0; sprPos < 8; sprPos++) {
        Sprite* spr = &sprite[sprPos];
		mask = 1 << sprPos;
        
		if (!(spriteActive & mask))
            continue;
        
        xPos = spr->xPos; 
                                            
        spr->expandXFlop = true;
        spr->mcFlop = true;
        
        dataShiftReg = spr->dataShiftReg & spr->mask;                
        
        while(dataShiftReg) {
            
            if (spr->expandXFlop) {
                if (spr->multiColor) {
                    // 2 bits per pixel (repeated)
                    if (spr->mcFlop)
                        spr->shiftOut = (dataShiftReg >> 22) & 3;

                    spr->mcFlop ^= 1; //repeat last shift out for second pixel
                } else
                    spr->shiftOut = ((dataShiftReg >> 23) & 1) << 1; // 2: sprite color, 0: transparent
            }

            if (spr->expandXFlop)
                // pixel is repeated, so "shift out" every second time
                dataShiftReg <<= 1;

            if (spr->expandX)
                spr->expandXFlop ^= 1;
            else
                spr->expandXFlop = 1;
                                                                
            if (spr->shiftOut) {
            
                ptr = linePtr + xPos;
                sprBefore = drawSprites[xPos];
                
                bool _fg = *ptr & 0x80;
                
                if (sprBefore) {                    
                    spriteSpriteCollided |= 1 << sprBefore->position;
                    spriteSpriteCollided |= mask;
                } else {
                    if (!spr->prioMD || !_fg) {                           
                        if (spr->shiftOut == 1) *ptr = colorReg[0x25]; // mc color register 1
                        else if (spr->shiftOut == 3) *ptr = colorReg[0x26]; // mc color register 2
                        else /*if (spr->shiftOut == 2)*/ *ptr = colorReg[spr->colorCode]; // sprite color                                                 
                    }                    
                }

                if (_fg)
                    spriteForegroundCollided |= mask;
                                    
                drawSprites[xPos] = spr;                                
            } else
				dataShiftReg &= 0xffffff;
            
            xPos++;
        }        
    }
}

template<bool phi1> inline auto VicIIFast::readPhi(uint16_t addr) -> uint8_t {

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
