
#pragma once

namespace Emulator {
    struct Serializer;
}

namespace LIBC64 {

struct Keyboard {
	Keyboard(Emulator::Interface* interface) : interface(interface) {}

    Emulator::Interface* interface;
    Emulator::Interface::Device* device = nullptr;
        
	bool shiftLockPressed = false;
	bool shiftLock = false;
	
	uint8_t id[8][8] = { {42, 43, 40, 39, 36, 37, 38, 41 }, {3, 32, 10, 4, 35, 28, 14, 46},
	{5, 27, 13, 6, 12, 15, 29, 33}, {7, 34, 16, 8, 11, 17, 30, 31},
	{9, 18, 19, 0, 22, 20, 24, 23}, {44, 25, 21, 45, 48, 50, 63, 49},
	{52, 53, 51, 56, 47, 57, 60, 61}, {1, 55, 62, 2, 54, 58, 26, 59}};
    
    uint8_t cols[8];
    uint8_t rows[8];
    bool suppressPoll;
	    
    auto setDevice( Emulator::Interface::Device* device ) -> void {
        
        if (!device->isKeyboard())
            return;

        this->device = device;
    }

    auto setKeycode(uint8_t row, uint8_t col) -> void {
        rows[row] |= 1 << col;
        cols[col] |= 1 << row;
        suppressPoll = true;
    }

    auto poll() -> void {
        if (suppressPoll) {
            suppressPoll = false;
            return;
        }
        // update pressed key state from host, one time per frame
        bool state;
        
        for( unsigned col = 0; col < 8; col++ ) {
            for( unsigned row = 0; row < 8; row++ ) {
                state = interface->inputPoll( device->id, id[row][col] ) & 1;
                
                if (state) {                    
                    rows[row] |= 1 << col;
                    cols[col] |= 1 << row;                    
                    
                } else {                    
                    rows[row] &= ~(1 << col);
                    cols[col] &= ~(1 << row);                    
                }    
            }
        }  
        
        bool shiftLockPressedBefore = shiftLockPressed;
		
		shiftLockPressed = interface->inputPoll( device->id, 64 );
		
		if (shiftLockPressed && !shiftLockPressedBefore)
			shiftLock ^= 1;
		
		if (shiftLock) {                    
			rows[1] |= 1 << 7;
			cols[7] |= 1 << 1;
		}			
    }
    // there is a 8 x 8 gird of resistors
    // cia1 port A access the rows and cia1 port B the columns
    // by pressed keys rows and columns influence each other, so reading port A is
    // influenced by port B and vice versa.
    // e.g. for a row we need to find out, which keys are pressed in order to get the 
    // respective columns. now we check for pressed keys on these columns and get rows.
    // basically we have to follow each intersection (pressed key) to get all involved
    // rows/columns for a single start row or column.
    // following each intersection generates ghost keys, when too much keys are pressed
    // at same time.
    // imagine three keys, 2 on the same row and a third on another row but same column 
    // like the first or second. A forth ghost key would activate on the row of the third
    // key and the column of the first or sesond key.

    auto matrixResolveRow( uint8_t pos, uint8_t& linkedRows, uint8_t& linkedCols ) -> void {
        matrixResolve<true>(pos, linkedRows, linkedCols );
    }

    auto matrixResolveCol( uint8_t pos, uint8_t& linkedRows, uint8_t& linkedCols ) -> void {
        matrixResolve<false>(pos, linkedRows, linkedCols );
    }

    template<bool followRow> inline auto matrixResolve( uint8_t pos, uint8_t& linkedRows, uint8_t& linkedCols ) -> void {
        if ( (1 << pos) & (followRow ? linkedRows : linkedCols) ) // already checked
            return;

        if (followRow)
            linkedRows |= 1 << pos;
        else
            linkedCols |= 1 << pos;

        uint8_t mask = followRow ? rows[ pos ] : cols[ pos ];
        if ((mask & (followRow ? ~linkedCols : ~linkedRows)) == 0)
            return;

        for( pos = 0; pos < 8; pos++ ) {
            if (followRow) {
                if (mask & (1 << pos) & ~linkedCols)
                    matrixResolve<false>(pos, linkedRows, linkedCols);
            } else {
                if ( mask & (1 << pos) & ~linkedRows )
                    matrixResolve<true>( pos, linkedRows, linkedCols );
            }
        }
    }
        
	auto reset() -> void {
		shiftLockPressed = false;
		shiftLock = false;
        suppressPoll = false;
        std::memset(cols, 0, 8);
        std::memset(rows, 0, 8);
	}
	
	auto restore() -> bool {
		
		return interface->inputPoll( device->id, 65 ) & 1;
	}
    
    auto serialize( Emulator::Serializer& s ) -> void {
        
        s.integer( shiftLockPressed );
        s.integer( shiftLock );
        s.array( cols );
        s.array( rows );
    }

};

}