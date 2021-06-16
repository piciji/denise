
#include "structure.h"

namespace LIBC64 {
	
auto Structure1541::createD64FromPRG( std::string name, uint8_t* prgData, unsigned prgSize ) -> uint8_t* {
	
	uint8_t buffer[256];
	uint8_t* data = createDxx("empty", 1);
	uint8_t track = 18;
	uint8_t sector = 0;
	uint8_t startTrack;
	uint8_t startSector;
	unsigned pos = 2;
	uint8_t* bamPtr;
	uint8_t* dirPtr;
	unsigned i;
	int sectors;
	Emulator::PetciiConversion conv;
	
	sectors = countSectors( 18, 0 );	
	bamPtr = data + (sectors << 8);
	
	sectors = countSectors( 18, 1 );	
	dirPtr = data + (sectors << 8);
	
	if (!allocateFreeSector(bamPtr, track, sector))
		goto fail;
	
	startTrack = track;
	startSector = sector;
	
	for (i = 0; i < prgSize; i++) {		
		
		if (pos == 256) {
			pos = 2;
			
			uint8_t curTrack = track;
			uint8_t curSector = sector;
			
			if (!allocateNextFreeSector(bamPtr, track, sector))
				goto fail;
			
			// target to next sector;
			buffer[0] = track;
			buffer[1] = sector;
			
			sectors = countSectors( curTrack, curSector );
			std::memcpy( data + (sectors << 8), buffer, 256 );
			
			if ( !(++dirPtr[0x1e]) )
				dirPtr[0x1f]++; 
		}				
		
		buffer[pos++] = prgData[i];
	}
	
	buffer[0] = 0;
	buffer[1] = pos - 1;
	
	sectors = countSectors( track, sector );
	std::memcpy( data + (sectors << 8), buffer, pos );

	if (!(++dirPtr[0x1e]))
		dirPtr[0x1f]++; 
	
	dirPtr[0] = 0;
	dirPtr[1] = 0Xff;
	dirPtr[2] = 0x82;
	dirPtr[3] = startTrack;
	dirPtr[4] = startSector;	
	std::memset( dirPtr + 5, 0xa0, 16 );
		
	for(i = 0; i < name.size(); i++) {
		if (i == 16)
			break;
		
		*(dirPtr + 5 + i) = conv.encode( name[i] );
	}	
	
	return data;
	
fail:
	
	delete[] data;
	
	return nullptr;		
}

auto Structure1541::allocateFreeSector(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool {
	
	unsigned maxSectors;
	int t, s;
	
	for (uint8_t distance = 0; distance <= 22; distance++) {
		
		t = 18 - distance;
		
		if (distance && t >= 1) {
			
			maxSectors = countSectors( t );
			
			for (s = 0; s < maxSectors; s++) {
				
				if (allocateSector(bamPtr, t, s)) {
					track = t;
					sector = s;
					return true;				
				}
			}
		}
		
		t = 18 + distance;
		
		if (distance && (t <= 35)) {
			maxSectors = countSectors( t );
			
			for (s = 0; s < maxSectors; s++) {

				if (allocateSector(bamPtr, t, s)) {
					track = t;
					sector = s;
					return true;
				}
			}
		}
	}
	return false;
}

auto Structure1541::allocateNextFreeSector(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool {
	unsigned maxSectors;
	int t, s;
	
	if (track == 18)
		return false;
	
	s = sector + 10;
	t = track;
	
	maxSectors = countSectors( t );

	if (s >= maxSectors) {
        s -= maxSectors;
        if (s != 0)
            s--;        
    }

	for (unsigned i = 0; i < maxSectors; i++) {
		
		if (allocateSector(bamPtr, t, s)) {
			track = t;
			sector = s;
			return true;
		}
		s++;
		
		if (s >= maxSectors)
			s = 0;		
	}
	
	sector = 0;

	if (track < 18) {
		if (allocateDown(bamPtr, track, sector))
			return true;
				
		track = 18 - 1;
		
		if (allocateDown(bamPtr, track, sector))
			return true;		
		
		track = 18 + 1;
		
		if (allocateUp(bamPtr, track, sector))
			return true;		
		
	} else {
		if (allocateUp(bamPtr, track, sector))
			return true;
		
		track = 18 + 1;
		
		if (allocateUp(bamPtr, track, sector)) 
			return true;
		
		track = 18 - 1;
		
		if (allocateDown(bamPtr, track, sector))
			return true;		
	}
	
	return false;
}

auto Structure1541::allocateDown(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool {
	unsigned maxSectors;

	for (unsigned t = track; t >= 1; t--) {
		maxSectors = countSectors( t );
		
		for (unsigned s = 0; s < maxSectors; s++) {
			
			if (allocateSector(bamPtr, t, s)) {
				track = t;
				sector = s;
				return true;
			}
		}
	}
	
	return false;
}

auto Structure1541::allocateUp(uint8_t* bamPtr, uint8_t& track, uint8_t& sector) -> bool {
	unsigned maxSectors;

	for (unsigned t = track; t <= 35; t++) {
		maxSectors = countSectors( t );
		
		for (unsigned s = 0; s < maxSectors; s++) {
			
			if (allocateSector(bamPtr, t, s)) {
				track = t;
				sector = s;
				return true;
			}
		}
	}
	
	return false;
}

auto Structure1541::allocateSector(uint8_t* bamPtr, uint8_t track, uint8_t sector) -> bool {
	
	uint8_t* bamTrackPtr = getBamTrackEntry( bamPtr, track );
	
	if (issetBam( bamTrackPtr, sector )) {
		*bamTrackPtr -= 1;
		clrBam( bamTrackPtr, sector );
		return true;
	}
	
	return false;
}

auto Structure1541::freeSector(uint8_t* bamPtr, uint8_t track, uint8_t sector) -> bool {
	
	uint8_t* bamTrackPtr = getBamTrackEntry( bamPtr, track );
	
	if (!issetBam( bamTrackPtr, sector )) {
		*bamTrackPtr += 1;
		setBam( bamTrackPtr, sector );
		return true;
	}
	
	return false;
}

auto Structure1541::issetBam(uint8_t* bamTrackPtr, unsigned sector) -> bool {
	
    return (bamTrackPtr[1 + sector / 8] & (1 << (sector % 8))) != 0;
}

auto Structure1541::setBam(uint8_t* bamTrackPtr, unsigned sector) -> void {
	
    bamTrackPtr[1 + sector / 8] |= (1 << (sector % 8));    
}

auto Structure1541::clrBam(uint8_t* bamTrackPtr, unsigned sector) -> void {
	
    bamTrackPtr[1 + sector / 8] &= ~(1 << (sector % 8)); 
}

auto Structure1541::getBamTrackEntry( uint8_t* bamPtr, uint8_t track ) -> uint8_t* {
	
	return track <= TYPICAL_TRACKS
            ? &bamPtr[4 + 4 * (track - 1)] 
            : &bamPtr[192 + 4 * (track - TYPICAL_TRACKS - 1)];
}

}
