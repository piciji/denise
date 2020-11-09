
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

inline auto getSpriteBA( uint32_t flags ) -> uint8_t {
	return flags & 0xff;
}	
	
inline auto isBgBA( uint32_t flags ) -> bool {
	return flags & BgBA;
}

inline auto isSprFirstCycle( uint32_t flags ) -> bool {
	return flags & SprFirstCycle(0);
}

inline auto isSprSecondCycle( uint32_t flags ) -> bool {
	return flags & SprSecondCycle(0);
}

inline auto getSpr( uint32_t flags ) -> uint8_t {
	return (flags >> 9) & 7;
}

inline auto isRefresh( uint32_t flags ) -> bool {
	return flags & Refresh;
}

inline auto isFetchC( uint32_t flags ) -> bool {
	return flags & FetchC;
}

inline auto isFetchG( uint32_t flags ) -> bool {
	return flags & FetchG;
}

inline auto isSprDma( uint32_t flags ) -> bool {
	return flags & SprDma;
}

inline auto isSprExp( uint32_t flags ) -> bool {
	return flags & SprExp;
}

inline auto isSprDisp( uint32_t flags ) -> bool {
	return flags & SprDisp;
}

inline auto isStartPhi2( uint32_t flags ) -> bool {
	return flags & StartPhi2;
}

inline auto isVisible( uint32_t flags ) -> bool {
	return flags & Visible;
}

inline auto isScanlineRender( uint32_t flags ) -> bool {
	return flags & ScanlineRender;
}

inline auto isScanlineRenderFin( uint32_t flags ) -> bool {
	return flags & ScanlineRenderFin;
}

inline auto isUpdateVc( uint32_t flags ) -> bool {
	return (flags & TaskMask) == UpdateVC;
}	

inline auto isSprMcBase( uint32_t flags ) -> bool {
	return (flags & TaskMask) == SprMCBase;
}	

inline auto isBrdLeftFirst( uint32_t flags ) -> bool {
	return (flags & TaskMask) == BrdLeftFirst;
}	

inline auto isBrdLeftSecond( uint32_t flags ) -> bool {
	return (flags & TaskMask) == BrdLeftSecond;
}

inline auto isBrdRightFirst( uint32_t flags ) -> bool {
	return (flags & TaskMask) == BrdRightFirst;
}

inline auto isBrdRightSecond( uint32_t flags ) -> bool {
	return (flags & TaskMask) == BrdRightSecond;
}
	
inline auto isUpdateRc( uint32_t flags ) -> bool {
	return (flags & TaskMask) == UpdateRc;
}

inline auto getXpos( uint32_t flags ) -> uint16_t {	
	return (flags >> 26) << 3;
}

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