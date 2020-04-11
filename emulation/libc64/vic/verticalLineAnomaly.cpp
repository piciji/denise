
#include "vicII.h"

namespace LIBC64 {
    
auto VicII::insertVerticalLineAnomaly(unsigned start, unsigned end) -> void {
	
	if (!leftLineAnomaly.permanent && (start == 0) ) {
		leftLineAnomaly.framePos--;
		if (leftLineAnomaly.framePos == 0)
			leftLineAnomaly.permanent = true;							
	}
	
	if (leftLineAnomaly.permanent)
		insertVerticalLineAnomaly<true>( start, end );
	else
		insertVerticalLineAnomaly<false>( start, end );
}

template<bool permanent> auto VicII::insertVerticalLineAnomaly(unsigned start, unsigned end) -> void {
    
    uint16_t* ptr = frameBuffer + (start * VIC_MAX_LINE_LENGTH) + firstVisiblePixel + 1;
    uint8_t _colorReg = leftLineAnomaly.mode == 1 ? 1 : colorReg[ 0x23 ];
	
	unsigned moreLikely = 0;
	
    for(unsigned i = start; i < end; i++) {        
        
		if (permanent) {	
			*ptr = _colorReg;
			*(ptr + 1) = _colorReg;
		
		} else if (leftLineAnomaly.mode == 1) {
			*ptr = _colorReg;
			if (leftLineAnomaly.framePos < LEFT_LINE_ANOMALY_ONE_PIX)
				*(ptr + 1) = _colorReg;
			
		} else {			
			
			if (moreLikely) {
				moreLikely--;
				
				if (moreLikely & 1) {
					*ptr = _colorReg;							
					*(ptr + 1) = _colorReg;	
				} else {
					if ((rand() % 2) == 0) {
						*ptr = _colorReg;
                    }
				}					
					
				goto Next;
			}

			if ( (rand() % 50 ) == 0  ) {
				moreLikely = rand() % 6;
				if (moreLikely & 1)
					moreLikely++;
				
				if ((rand() % 2) == 0) {
					*ptr = _colorReg;
                }
				
			} else {				
				*ptr = _colorReg;								
				*(ptr + 1) = _colorReg;					
			}
		}
		   
		Next:
        ptr += VIC_MAX_LINE_LENGTH;
    }	
}
   
}
