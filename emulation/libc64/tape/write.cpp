
#include "tape.h"
#include "../system/system.h"

namespace LIBC64 {

auto Tape::setWriteProtect(bool state) -> void {
	
	writeProtect = state;
    writeQuestionState = 0;
}

auto Tape::isWriteProtected() -> bool {
    return writeProtect;
}

auto Tape::writeIn(bool bit) -> void {
    bool _wb = writeBit;
    writeBit = bit;

    if (!enabled || !loaded || rawData || !motorIn || mode != Mode::Record)
        return;

    if (writeProtect) // mechanical protection, record button isn't pressable
        return;

    if (!writeBit || (writeBit == _wb)) {
        return advanceWriteCounter();
    }

    unsigned cyclesElapsed = sysTimer.fallBackCycles( writeClock );

    if (cyclesElapsed <= 7)
        return advanceWriteCounter();

    if (writeQuestionState == 1)
        return advanceWriteCounter();
       
    if (!writeQuestionState) {
        if (!system->interface->questionToWrite(media)) {
            writeQuestionState = 1; // don't ask again
            return advanceWriteCounter();
        }
        writeQuestionState = 2; 
    }
    
    if (cyclesElapsed <= (255 * 8 + 7) ) {
		addByteToWriteBuffer( (uint8_t)(cyclesElapsed / 8) );
    } else { // long gap
		addByteToWriteBuffer( 0 );
		addByteToWriteBuffer( cyclesElapsed & 0xff );
		addByteToWriteBuffer( (cyclesElapsed >> 8) & 0xff );
		addByteToWriteBuffer( (cyclesElapsed >> 16) & 0xff );
    }

    writeClock = sysTimer.clock;

    advanceWriteCounter();
}

auto Tape::advanceWriteCounter() -> void {
    unsigned cyclesElapsed = sysTimer.fallBackCycles( writeCounterClock );

    if (cyclesElapsed < 100000)
        return;

    cycles += cyclesElapsed;
    if (cycles > cyclesTotal)
        cyclesTotal = cycles;

    updateCounter();

    writeCounterClock = sysTimer.clock;
}

auto Tape::addByteToWriteBuffer(uint8_t byte) -> void {
	
	writeData[writePos++] = byte;
	
	if (writePos == TAPE_WRITE_SIZE )
		writeBuffer();
}

auto Tape::writeBuffer() -> void {
	if (rawData || (writePos == 0) )
		return;
	
	unsigned writeSize = write(writeData, writePos, curPos );
    curPos += writeSize;
		
	if (writeSize != writePos) {
        writePos = 0;
		setMode( Mode::Stop ); // something went wrong
    }
	
	writePos = 0;
	
	if (curPos <= rawSize)
		return;
	// file has increased
    rawSize = curPos;
	// write new file size to header
	uint8_t entry[4];
	uint32_t hSize = rawSize - 20;
	
	entry[0] = hSize & 0xff;
	entry[1] = (hSize >> 8) & 0xff;
	entry[2] = (hSize >> 16) & 0xff;
	entry[3] = (hSize >> 24) & 0xff;
	
	write( &entry[0], 4, 0x10 );		
}

auto Tape::createTap( unsigned& imageSize ) -> uint8_t* {
	
    imageSize = 24; // 20 byte header + 4 byte data
    
	uint8_t* buffer = new uint8_t[imageSize];
	
	std::memcpy( buffer, "C64-TAPE-RAW", 12 );
	
	buffer[12] = 1; // version
	
	buffer[13] = buffer[14] = buffer[15] = 0; // future expansion
	
	buffer[16] = 4; // file size
	
	buffer[17] = buffer[18] = buffer[19] = 0; // file size
	
	buffer[20] = buffer[21] = buffer[22] = buffer[23] = 0;	    
    
	return buffer;
}

}