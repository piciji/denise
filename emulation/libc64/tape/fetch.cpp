
#include "tape.h"

#define TAPE_TRANSITION_RANDOMNESS 10

namespace LIBC64 {
    
auto Tape::nextGap() -> unsigned {
	
	if ( !loaded )
		return TAPE_ZERO_GAP;	
	
	bool _longGap = false;
	
	if ( directionForward )
		// read forward
		return fetchGap(_longGap);

	// backward is tricky, we don't know anymore if the previous gaps were long or not
	// what we know is, that our current position is aligned and not in between a long gap	
	
	// ok we need to find some way to roll back byte by byte till a position that we
	// recognize as aligned and then read from this position forward to the gap before
	// current one
	// a long gap begins with 0 followed by 3 bytes
	// a short gap is a value different from zero
	// first we check the byte 4 bytes before the current one
	// if this is non zero, we definitely know the previous gap have to be short
	// why? for a long gap it have to be 0 in order our current position is aligned
	
	uint8_t byte = 0;
	uint8_t byteBefore;
	
	if(!readBackward( byteBefore ))
		return 0;	
    
	if (version == 0) 
		// there are no long gaps possible
		return shortGap( byteBefore );
    
    unsigned rememberPos = pos;
	
	if ( !readBackward( byte, 3 ) || byte ) {
        // short gap because we have rewinded back completly and
        // there are no 4 bytes for a long gap
        // or byte is non zero, see explanation above
		
		// forward to position before current position
        while( rememberPos != pos )
            if(!readForward( byte ))
				return 0;
        
        return shortGap( byteBefore );
    }
	
	// if the byte, four bytes before is zero it could be the start
	// of a long gap ... wait maybe its one of the 3 bytes of a sooner
	// long gap, then the previous gap would be short too ... damn
	// so a non zero results in a short gap always but a zero could be
	// both, so how we find out that ?
	
	// we have to find 3 non zero bytes in a row, then the byte afterwards is always
	// aligned and we can safely read forward from there to the previous gap
	// if there are 3 non zero bytes in a row the latest possible long gap is the byte
	// before this row, means long gap is aligned after the row
	// if the long gap begins sooner or there is no long gap at all means the row ends
	// with short gaps and is aligned too.
	
	unsigned nonZeroInARow = 0;
	
	while( nonZeroInARow < 3 ) {
		if(!readBackward( byte ))
			// rewinded back complety and nothing was found
			// could be happen if we are near the beginning of the tape
			// no problem the first byte is aligned, of course
			break;
		
		if (byte)
			nonZeroInARow++;
		else
			nonZeroInARow = 0;
	}        	
		
    if (nonZeroInARow == 3) {
		// we move to the the end of the row to our well earned aligned position
        if(!readForward( byte, 3 ))
            return 0;        
    }	
	// in else case we are at the beginning of tape (0x14)
    
	unsigned gap;
	
	// read forward from known aligned position
	// remember if previous gap was long or not
	while( pos <= rememberPos )		
		gap = fetchGap( _longGap );
    
	//  move to beginning of previous gap, depends if long or short gap
    if(!readBackward( byte, _longGap ? 4 : 1 ))
        return 0;
	
	return gap;
}   

auto Tape::fetchGap( bool& _longGap ) -> unsigned {
	uint8_t byte;
	
	if ( !readForward( byte ) )
		return 0;
	
	if ( version == 0 || byte > 0 ) {
        _longGap = false;
        return shortGap( byte );        
    }       
        
    _longGap = true;
    
    return longGap();
}
  
auto Tape::shortGap( uint8_t byte ) -> unsigned {
    
    unsigned gap = byte == 0 ? TAPE_ZERO_GAP : (byte * 8);

    return randomizeGap( gap );
}

auto Tape::longGap( ) -> unsigned {
    uint8_t byte;
    unsigned gap = 0;
    
    for(unsigned i = 0; i < 3; i++) {

        if (!readForward( byte ))
            return 0;

        gap |= byte << (i << 3);
    }		

    if ( gap == 0 )
        gap = TAPE_ZERO_GAP;

    
    return randomizeGap( gap );
}

auto Tape::randomizeGap( unsigned gap ) -> unsigned {

    // unfortunately the random adjustment wouldn't only affect the cia flag trigger
	// time but the total tape length too
	// we will add the previous adjustment to the new gap in order to maintain total length
	// means only the flag trigger shifts slightly by the randomness below
    if (adjust > 0) {

		if ( (gap - adjust) < 1) {
			adjust -= gap - 1;
			// the last adjustment isn't completly balanced
			// we don't add anymore randomness till adjust is fully balanced
			return 1;
		}		
	}
	
	gap -= adjust;
    adjust = 0;    
    
    if ( mode != Mode::Play )
        return gap;
    
    // for realistic behaviour we need some randomness		
	adjust = (rand() % ( (TAPE_TRANSITION_RANDOMNESS << 1) + 1 ) ) - TAPE_TRANSITION_RANDOMNESS;

	if ( (adjust >= 0) || (gap > -adjust) ) {
		gap += adjust;

	} else {
		// gap would be zero or below	
		gap = 1;
		adjust = -1 * (gap - 1);
	}

	return gap;
}

auto Tape::readForward( uint8_t& byte, unsigned count ) -> bool {
    
    for( unsigned i = 0; i < count; i++ ) {

        if ( !readForward( byte ) )
            return false;        
    }

    return true;
}

auto Tape::readForward( uint8_t& byte ) -> bool {

    if (data) {
        // tape image was fully loaded because of compressed file
        // tape image can't be written in this case
        if (pos == size)
            return false;

        byte = data[pos++];		

        return true;
    }
    
    // uncompressed files shouldn't load before
    // needed data will be loaded by callback in chunks
   
    if (fetchPos == 0) {

        fetchSize = read( fetchData, TAPE_FETCH_SIZE, pos );

        if (fetchSize == 0)
            return false;
    }

    byte = fetchData[fetchPos++];
    pos++;

    if (fetchPos == fetchSize)
        fetchPos = 0;

    return true;	
}

auto Tape::readBackward( uint8_t& byte, unsigned count ) -> bool {
    
    for( unsigned i = 0; i < count; i++ ) {

        if ( !readBackward( byte ) )
            return false;        
    }

    return true;
}

auto Tape::readBackward( uint8_t& byte ) -> bool {
    
    if (pos == 0x14) //end of header
        return false;
    
    pos--;
            
    if (data) {

        byte = data[pos];		

        return true;
    }     
    // for uncompressed mode
    if (fetchPos == 0) {
        
        unsigned fetchStart = 0;
        fetchSize = pos + 1;

        if (pos > TAPE_FETCH_SIZE) {
            fetchStart = pos - TAPE_FETCH_SIZE + 1;
            fetchSize = TAPE_FETCH_SIZE;
        } 

        fetchSize = read(fetchData, fetchSize, fetchStart );

        if (fetchSize == 0)
            return false;
        
        fetchPos = fetchSize;
    }
	
    byte = fetchData[--fetchPos];	
    
    return true;
}
    
}