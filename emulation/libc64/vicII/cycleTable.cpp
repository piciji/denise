
#include "base.h"

namespace LIBC64 {

#define M_PAL (_m == C_PAL)
#define M_NTSC (_m == C_NTSC)
#define M_NTSC_OLD (_m == C_NTSC_OLD)

// [0 - 7]
#define SprBA1(x)		( 1 << x )
#define SprBA2(x, y)	( (1 << x) | (1 << y) )
#define SprBA3(x, y, z)	( (1 << x) | (1 << y) | (1 << z) )
// [8]
#define BgBA			( 1 << 8 )
// [9 - 11] -> sprite pos
// [12]
#define SprFirstCycle(p)	((0x8 | p) << 9)
// [13]
#define SprSecondCycle(p)	((0x10 | p) << 9)
// [14]
#define Refresh				(0x20 << 9)
// [15]
#define FetchC				(0x40 << 9)
// [16]
#define FetchG				(0x80 << 9)
// [17]
#define SprDma				(0x100 << 9)
// [18]
#define SprExp				(0x200 << 9)
// [19]
#define SprDisp				(0x400 << 9)
#define StartPhi2			(0x400 << 9)	// alternate for scanline renderer
// [20]
#define Visible				(0x800 << 9)
	
// [21]
#define ScanlineRender		(0x1000 << 9)
// [22]
#define ScanlineRenderFin	(0x2000 << 9)
	
// [23 - 25] only one of the following can happen same cycle, so we don't need a single bit for each task
#define UpdateVC			(1 << 23)
#define SprMCBase			(2 << 23)
#define BrdLeftFirst		(3 << 23)
#define BrdLeftSecond		(4 << 23)
#define BrdRightFirst		(5 << 23)
#define BrdRightSecond		(6 << 23)
#define UpdateRc			(7 << 23)
#define TaskMask			(7 << 23)
	
// [26 - 31] 6 bit for sprite xpos
#define SprXpos(p)			(((p >> 3) & 0x3f) << 26)


auto VicIIBase::generateCycleTable(CycleMode _m) -> void {
	uint8_t _cycle = 1;
	uint32_t flags;
	unsigned xPos = M_PAL ? 0x198 : 0x1a0;
	
	while(_cycle <= 65) {
		
		switch(_cycle) {
			case 1:
				if ( M_NTSC )			flags = SprSecondCycle(3) | SprBA3(3, 4, 5);
				else/*PAL, NTSC OLD*/	flags = SprFirstCycle(3) | SprBA2(3, 4);
				break;
			case 2:
				if ( M_NTSC )			flags = SprFirstCycle(4) | SprBA2(4, 5);
				else/*PAL, NTSC OLD*/	flags = SprSecondCycle(3) | SprBA3(3, 4, 5);
				break;	
			case 3:
				if ( M_NTSC )			flags = SprSecondCycle(4) | SprBA3(4, 5, 6);
				else/*PAL, NTSC OLD*/	flags = SprFirstCycle(4) | SprBA2(4, 5);
				break;	
			case 4:
				if ( M_NTSC )			flags = SprFirstCycle(5) | SprBA2(5, 6);
				else/*PAL, NTSC OLD*/	flags = SprSecondCycle(4) | SprBA3(4, 5, 6);
				break;	
			case 5:
				if ( M_NTSC )			flags = SprSecondCycle(5) | SprBA3(5, 6, 7);
				else/*PAL, NTSC OLD*/	flags = SprFirstCycle(5) | SprBA2(5, 6);
				break;
			case 6:
				if ( M_NTSC )			flags = SprFirstCycle(6) | SprBA2(6, 7);
				else/*PAL, NTSC OLD*/	flags = SprSecondCycle(5) | SprBA3(5, 6, 7);
				break;		
			case 7:
				if ( M_NTSC )			flags = SprSecondCycle(6) | SprBA2(6, 7);
				else/*PAL, NTSC OLD*/	flags = SprFirstCycle(6) | SprBA2(6, 7);
				break;
			case 8:
				if ( M_NTSC )			flags = SprFirstCycle(7) | SprBA1(7);
				else/*PAL, NTSC OLD*/	flags = SprSecondCycle(6) | SprBA2(6, 7);
				break;
			case 9:
				if ( M_NTSC )			flags = SprSecondCycle(7) | SprBA1(7);
				else/*PAL, NTSC OLD*/	flags = SprFirstCycle(7) | SprBA1(7);
				break;			
			case 10:
				if ( M_NTSC )			flags = 0; // phi1 idle cycle
				else/*PAL, NTSC OLD*/	flags = SprSecondCycle(7) | SprBA1(7);
				break;	
			case 11:
				flags = Refresh;
				break;
			case 12:
				flags = Refresh | BgBA;
				break;
			case 13:
				flags = Refresh | BgBA;
				break;
			case 14:
				flags = Refresh | BgBA | UpdateVC;
				break;
			case 15:
				flags = Refresh | FetchC | BgBA;
				break;
			case 16:
				flags = FetchG | FetchC | BgBA | SprMCBase | Visible;
				break;
			case 17: 
				flags = FetchG | FetchC | BgBA | BrdLeftFirst | Visible;
				break;
			case 18: 
				flags = FetchG | FetchC | BgBA | BrdLeftSecond | Visible;
				break;
			case 19:
			case 20: case 21: case 22: case 23: case 24:
			case 25: case 26: case 27: case 28: case 29:
			case 30: case 31: case 32: case 33: case 34:
			case 35: case 36: case 37: case 38: case 39:
			case 40: case 41: case 42: case 43: case 44:
			case 45: case 46: case 47: case 48: case 49:
			case 50: case 51: case 52: case 53: case 54:
				flags = FetchG | FetchC | BgBA | Visible;
				break;
			// Sprites
			case 55:
				flags = FetchG | Visible;
				if ( M_PAL )		flags |= SprDma | SprBA1(0);								
				break;				
			case 56:	// phi1 idle cycle
				flags = SprDma | BrdRightFirst | SprExp | SprBA1(0);
				break;
			case 57:	// phi1 idle cycle
				flags = BrdRightSecond;
				if ( M_PAL )		flags |= SprBA2(0, 1);
				else				flags |= SprDma | SprBA1(0);
				break;
			case 58:
				flags = UpdateRc;
				if ( M_PAL )		flags |= SprFirstCycle(0) | SprBA2(0, 1);
				else				flags |= SprBA2(0, 1);	// phi1 idle cycle 
				if ( !M_NTSC )		flags |= SprDisp;
				break;
			case 59:
				if ( M_PAL )		flags = SprSecondCycle(0) | SprBA3(0, 1, 2);
				else				flags = SprFirstCycle(0) | SprBA2(0, 1);
				if ( M_NTSC )		flags |= SprDisp;
				break;
			case 60:
				if ( M_PAL )		flags = SprFirstCycle(1) | SprBA2(1, 2);
				else				flags = SprSecondCycle(0) | SprBA3(0, 1, 2);
				break;
			case 61:
				if ( M_PAL )		flags = SprSecondCycle(1) | SprBA3(1, 2, 3);
				else				flags = SprFirstCycle(1) | SprBA2(1, 2);
				break;
			case 62:
				if ( M_PAL )		flags = SprFirstCycle(2) | SprBA2(2, 3);
				else				flags = SprSecondCycle(1) | SprBA3(1, 2, 3);
				break;
			case 63:
				if ( M_PAL )		flags = SprSecondCycle(2) | SprBA3(2, 3, 4);
				else				flags = SprFirstCycle(2) | SprBA2(2, 3);
				break;
			// ntsc only
			case 64:
				flags = SprSecondCycle(2) | SprBA3(2, 3, 4);
				break;
			case 65:
				flags = SprFirstCycle(3) | SprBA2(3, 4);
				break;				
		}
		
		if ( isScanlineRenderer() ) {
			flags &= ~SprDisp; // unused for scanline renderer
			
			if (_cycle == 33)
				flags |= ScanlineRender; // scanline render cycle 
			else if (_cycle == 55)
				flags |= ScanlineRenderFin; // scanline render finish cycle 
			else if (_cycle == 12)
				flags |= StartPhi2;
		}
		
		flags |= SprXpos( xPos );
		
		if ( M_NTSC && (_cycle == 61) );
		else
			xPos += 8;				
		
		if (xPos == (M_PAL ? 0x1f8 : 0x200))
			xPos = 0;
		
		cycleTab[_cycle - 1] = flags;				
		_cycle++;
	}
}

#undef M_PAL
#undef M_NTSC
#undef M_NTSC_OLD
#undef SprBA1
#undef SprBA2
#undef SprBA3
#undef BgBA
#undef SprFirstCycle
#undef SprSecondCycle
#undef FetchC
#undef FetchG
#undef Refresh
#undef SprDma
#undef SprExp
#undef SprDisp
#undef Visible
#undef UpdateVC
#undef SprMCBase
#undef BrdLeftFirst
#undef BrdLeftSecond
#undef BrdRightFirst
#undef BrdRightSecond
#undef UpdateRc
#undef SprXpos

}
