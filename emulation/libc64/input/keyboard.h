
#pragma once

#include "../system/system.h"

namespace LIBC64 {
	
struct System;	
	
struct Keyboard {
	
    Interface::Device* device = nullptr;  
        
	bool shiftLockPressed = false;
	bool shiftLock = false;
	
	uint8_t id[8][8] = { {42, 43, 40, 39, 36, 37, 38, 41 }, {3, 32, 10, 4, 35, 28, 14, 46},
	{5, 27, 13, 6, 12, 15, 29, 33}, {7, 34, 16, 8, 11, 17, 30, 31},
	{9, 18, 19, 0, 22, 20, 24, 23}, {44, 25, 21, 45, 48, 50, 63, 49},
	{52, 53, 51, 56, 47, 57, 60, 61}, {1, 55, 62, 2, 54, 58, 26, 59}};
    
    uint8_t cols[8];
    uint8_t rows[8];
	
	// force command
//	struct {
//		std::vector<uint8_t> feed;
//		unsigned pos;
//		unsigned repeat;
//		bool ready = false;
//	} command;
    
    auto setDevice( Interface::Device* device ) -> void {       
        
        if (!device->isKeyboard())
            return;

        this->device = device;
    }
	
    auto poll() -> void {
        // update pressed key state from host, one time per frame
        bool state;
        
        for( unsigned col = 0; col < 8; col++ ) {
            for( unsigned row = 0; row < 8; row++ ) {
                state = system->interface->inputPoll( device->id, id[row][col] ) & 1;
                
                if (state) {                    
                    rows[row] |= 1 << col;
                    cols[col] |= 1 << row;                    
                    
                } else {                    
                    rows[row] &= ~(1 << col);
                    cols[col] &= ~(1 << row);                    
                }    
            }
        }  
        
//        if (command.ready)
//            forceCommand( );
        
        bool shiftLockPressedBefore = shiftLockPressed;
		
		shiftLockPressed = system->interface->inputPoll( device->id, 64 );
		
		if (shiftLockPressed && !shiftLockPressedBefore)
			shiftLock ^= 1;
		
		if (shiftLock) {                    
			rows[1] |= 1 << 7;
			cols[7] |= 1 << 1;
		}			
    }
    // next we need a keyboard matrix resolver, there is a 8 x 8 gird of resistors
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

    auto matrixActivateRow( uint8_t row, uint8_t& activeRows, uint8_t& activeColumns ) -> void {
        // we have scanned this row already
        if ( (1 << row) & activeRows )
            return;

        activeRows |= 1 << row;
        
        // get all columns of this row, whose keys are pressed
        uint8_t colMask = rows[ row ];
        
        for( unsigned col = 0; col < 8; col++ )  
            if ( colMask & (1 << col) & ~activeColumns )
                matrixActivateColumn( col, activeRows, activeColumns );
    }
    
    auto matrixActivateColumn( uint8_t column, uint8_t& activeRows, uint8_t& activeColumns ) -> void {        
        // we have scanned this column already
        if ( (1 << column) & activeColumns )
            return;

        activeColumns |= 1 << column;
        
        // get all rows of this column, whose keys are pressed
        uint8_t rowMask = cols[ column ];
        
        for( unsigned row = 0; row < 8; row++ )  
            if ( rowMask & (1 << row) & ~activeRows )
                matrixActivateRow( row, activeRows, activeColumns );
    }

    auto matrixGetActiveRowsByColumn( uint8_t column ) -> uint8_t {
        uint8_t activeRows = 0;
        uint8_t activeColumns = 0;
        matrixActivateColumn( column, activeRows, activeColumns );
        return activeRows;
    }

    auto matrixGetActiveRowsByRow( uint8_t row ) -> uint8_t {
        uint8_t activeRows = 0;
        uint8_t activeColumns = 0;
        matrixActivateRow( row, activeRows, activeColumns );
        return activeRows;
    }

    auto matrixGetActiveColumnsByColumn( uint8_t column ) -> uint8_t {
        uint8_t activeRows = 0;
        uint8_t activeColumns = 0;
        matrixActivateColumn( column, activeRows, activeColumns );
        return activeColumns;
    }

    auto matrixGetActiveColumnsByRow( uint8_t row ) -> uint8_t {
        uint8_t activeRows = 0;
        uint8_t activeColumns = 0;
        matrixActivateRow( row, activeRows, activeColumns );
        return activeColumns;
    }
        
	auto reset() -> void {
		shiftLockPressed = false;
		shiftLock = false;
	}
	
	auto restore() -> bool {
		
		return system->interface->inputPoll( device->id, 65 ) & 1;
	}
   
	// use key ids defined in interface.cpp
//	auto setCommand( std::vector<uint8_t> data ) -> void {
//		command.feed = data;
//		command.pos = 0;
//		command.repeat = 0;
//		command.ready = true;
//	}
			
//	auto forceCommand( ) -> void {
//		
//		uint8_t _id = command.feed[ command.pos ];
//		
//		for (unsigned i = 0; i < 8; i++) {			
//			
//            for (unsigned j = 0; j < 8; j++) {
//
//                if (id[i][j] == _id) {
//
//                    rows[i] |= 1 << j;
//                    cols[j] |= 1 << i;
//
//                    // kernal expects key pressed some time
//                    if (++command.repeat >= 5) {
//                        command.repeat = 0;
//
//                        if (++command.pos >= command.feed.size() ) {
//                            command.ready = false;							
//                        }
//                    }
//
//                    return;
//                }											
//            }				
//
//		}
//	}
    
    auto serialize( Emulator::Serializer& s ) -> void {
        
        s.integer( shiftLockPressed );
        s.integer( shiftLock );
        s.array( cols );
        s.array( rows );
    }

};

}