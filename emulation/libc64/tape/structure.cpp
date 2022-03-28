
#include "structure.h"
#include "tape.h"

#define SHORT_PULSE(p)          (p >= 36 && p <= 54)
#define MIDDLE_PULSE(p)         (p >= 55 && p <= 73)
#define LONG_PULSE(p)           (p >= 74 && p <= 100)

#define TT_SHORT_PULSE(p)       (p >= 10 && p <= 34)
#define TT_LONG_PULSE(p)        (p >= 35 && p <= 54)

#define UNKNOWN 0
#define CBM_TAPE 1
#define TURBO_TAPE 2

namespace LIBC64 {

TapeStructure::TapeStructure(Tape* tape) {
    this->tape = tape;
    fetchData = new uint8_t[ TAPE_FETCH_SIZE ];
    fetchPos = 0;
    fetchSize = 0;
    curPos = 0;
}

TapeStructure::~TapeStructure() {
    delete[] fetchData;
    clearBuffer();
}

auto TapeStructure::clearBuffer() -> void {
    for (auto& f : fileEntries) {
        if (f.buffer)
            delete[] f.buffer;
        if (f.header)
            delete[] f.header;
    }
}

auto TapeStructure::getListing( ) -> std::vector<Emulator::Interface::Listing>& {
    FileEntry fileEntry;
    Emulator::C64Listing listing;
    uint8_t head[16] = {'C','6','4','-','T', 'A', 'P', 'E','-','R','A','W',' ',' ',' ',' '};

    setPosition(0x14);
    unsigned id = 0;

    listings.clear();

    clearBuffer();

    fileEntries.clear();

    curFileEntry = nullptr;

    listing.convertToScreencode = system->interface->convertToScreencode;

    listings.push_back( {id++, listing.buildHeadline( &head[0] ) } );

    while( nextFile(fileEntry) ) {

        uint8_t type = (fileEntry.type == 4) ? 1 : 2 ;
        type |= 0x20;
        if (fileEntry.turoTape)
            type |= 0x10;

        unsigned size = 0;
        if (fileEntry.type != 4) {
            if (fileEntry.startAddr < fileEntry.endAddr)
                size = (fileEntry.endAddr - fileEntry.startAddr + 253) / 254; // round up in case of fractional block
        }

  //      system->interface->log("header");
        uint8_t* name = new uint8_t[16];
        std::memset(name, 0x20, 16);

        bool invisibleTextColor = false;
        for(unsigned i = 0; i < 16; i++) {
            uint8_t* pos = fileEntry.header + 5 + i;

//            system->interface->log( *(pos), 0, 1 );

            if (*pos == 0x1f) { // control to change text color "blue" ... means inivisible chars
                invisibleTextColor = true;
                continue;
            }

            if (invisibleTextColor && (
                    (*pos == 0x05) || (*pos == 0x1c) || (*pos == 0x1e) || (*pos == 0x81) || (*pos == 0x90) || (*pos == 0x95)
                ||  (*pos == 0x96) || (*pos == 0x97) || (*pos == 0x98) || (*pos == 0x99) || (*pos == 0x9a) || (*pos == 0x9b)
                ||  (*pos == 0x9c) || (*pos == 0x9e) || (*pos == 0x9f) )) {
                invisibleTextColor = false;
                continue;
            }

            if (invisibleTextColor)
                continue;

            if (*pos <= 0x1f) // control
                name[i] = 0x20;
            else if (*pos >= 0x60 && *pos <= 0x7a)
                name[i] = *pos - 0x60;
            else if (*pos >= 0x80 && *pos <= 0x9f) // control
                name[i] = 0x20;
            else
                name[i] = *pos;
        }

        listings.push_back( {id++, listing.buildListing( name, size, type ) });

        fileEntries.push_back(fileEntry);

        delete[] name;
    }

    if (fileEntries.size())
        curFileEntry = &(fileEntries[0]);

    setPosition(0x14);

    return listings;
}

auto TapeStructure::setFile( unsigned fileNumber ) -> bool {
    for(auto& fileEntry : fileEntries) {
        if (fileEntry.number == fileNumber) {
            curFileEntry = &fileEntry;
            return true;
        }
    }

    return false;
}

auto TapeStructure::getCurFile() -> FileEntry* {
    return curFileEntry;
}

auto TapeStructure::analyzeFile() -> int {
    int pulse;
    unsigned hintCBM = 0;
    unsigned startCBM = curPos;
    unsigned hintTT = 0;
    unsigned startTT = curPos;
    unsigned posBefore;

    while (true) {
        posBefore = curPos;
        pulse = fetchPulse();

        if (pulse == -1) // end of tape
            break;

        if (SHORT_PULSE(pulse)) {
            hintCBM++;
        } else {
            hintCBM = 0;
            startCBM = curPos;
        }

        if ((hintTT & 7) == 0) {
            if (TT_LONG_PULSE(pulse)) {
                hintTT++;
            } else {
                startTT = curPos;
                hintTT = 0;
            }
        } else {
            if (TT_SHORT_PULSE(pulse)) {
                hintTT++;
            } else if (TT_LONG_PULSE(pulse)) {
                startTT = posBefore;
                hintTT = 1;
            } else {
                startTT = curPos;
                hintTT = 0;
            }
        }

        if (hintTT >= 1500) {
            setPosition( startTT + 2 );
            return TURBO_TAPE;
        }

        if (hintCBM >= 1000) {
            setPosition( startCBM );
            return CBM_TAPE;
        }
    }

    return UNKNOWN;
}

auto TapeStructure::jumpOverCbmGap() -> bool {
    int data;
    unsigned tempPos;
    unsigned tempPos2;
    unsigned tries = 0;
    bool found = false;

    while (1) {
        tempPos = curPos;
        data = fetchPulse( );
        tempPos2 = curPos;

        if (data < 0)
            return false;

        if (found) {
            if (LONG_PULSE(data)) {
                setPosition(tempPos);
                data = getByte();

                if (data == -1)
                    return false;

                if (data < -1) {
                    if (++tries > 30) {
                        break;
                    }

                    setPosition(tempPos2);
                } else {
                    setPosition(tempPos);
                    break;
                }
            } else if (!SHORT_PULSE(data)) {
                if (++tries > 30) {
                    break;
                };
            }
        } else {
            if (SHORT_PULSE(data)) {
                if (++tries == 32) {
                    tries = 0;
                    found = true;
                }
            } else
                tries = 0;
        }
    }

    return true;
}

auto TapeStructure::jumpOverCbmFile(bool seq) -> bool {
    if (seq) {
        allocatedSize = 193;
        unsigned _size = allocatedSize;
        uint8_t buffer[allocatedSize];
        unsigned tempPos;

        while (1) {
            tempPos = curPos;

            bool state = readCbmBlock(buffer, _size, false);
            if (!state || (buffer[0] != 2) ) {
                setPosition( tempPos );
                break;
            }
        }
    } else {
        if (!jumpOverCbmGap()) // data
            return false;
        if (!jumpOverCbmGap()) // repeated data
            return false;
    }

    return true;
}

auto TapeStructure::jumpOverTTGap() -> bool {
    int data;

    while(1) {
        unsigned tempPos = curPos;
        data = getTTByte();
        if (data < 0)
            return false;

        if (data != 2) {
            setPosition( tempPos );
            break;
        }
    }

    return true;
}

auto TapeStructure::nextFile(FileEntry& fileEntry) -> bool {
    unsigned fileType;

    while (true) {
        if ((fileType = analyzeFile()) == UNKNOWN)
            return false;

        fileEntry.turoTape = fileType == TURBO_TAPE;
        fileEntry.header = nullptr;
        fileEntry.buffer = nullptr;

        fileEntry.offset = curPos;

        if (fileEntry.turoTape) {
            if (!readTTHeader(fileEntry)) {
                setPosition( fileEntry.offset );
                jumpOverTTGap();
                continue;
            }

        } else {
            if (!readCbmHeader(fileEntry)) {
                setPosition( fileEntry.offset );
                jumpOverCbmGap();
                continue;
            }
        }

        if (fileEntry.type == 5) // end of tape marker
            return false;

        break;
    }

    fileEntry.dataOffset = curPos;
    fileEntry.number++;

    // jump over the file data, if not successfull still use the header data for UI listing
    if (fileEntry.turoTape)
        readTTBlock(nullptr, fileEntry.endAddr - fileEntry.startAddr + 1);
    else
        jumpOverCbmFile(fileEntry.type == 4);

    return true;
}

auto TapeStructure::readCbmHeader(FileEntry& fileEntry) -> bool {
    allocatedSize = 1024; // maximum
    if (!fileEntry.header) {
        // will allocate enough space in case of "end of file" marker is non standard
        fileEntry.header = new uint8_t[allocatedSize];
    }
    fileEntry.headerSize = 192; // typical

    if (!readCbmBlock(fileEntry.header, fileEntry.headerSize, false))
        return false;

    if (fileEntry.headerSize < (16 + 5))
        return false;

    if (fileEntry.header[0] != 1 && fileEntry.header[0] != 3 && fileEntry.header[0] != 4) // type 2 and 5 are no header blocks
        return false;

    // $01 relocatable program
    // $02 Data block for SEQ file
    // $03 non-relocatable program
    // $04 SEQ file header
    // $05 End-of-tape marker
    fileEntry.type = fileEntry.header[0];
    fileEntry.startAddr = (fileEntry.header[2] << 8) | fileEntry.header[1];
    fileEntry.endAddr = (fileEntry.header[4] << 8) | fileEntry.header[3];
    return true;
}

auto TapeStructure::readTTHeader(FileEntry& fileEntry) -> bool {
    if (!fileEntry.header) {
        fileEntry.header = new uint8_t[192];
    }

    fileEntry.headerSize = 192;

    if (!readTTBlock(fileEntry.header, fileEntry.headerSize, true))
        return false;

    fileEntry.type = 3; // TPRG  // pilot byte 2: non-relocatable program, todo pilot byte 1: relocatable
    fileEntry.startAddr = (fileEntry.header[1] << 8) | fileEntry.header[0];
    fileEntry.endAddr = (fileEntry.header[3] << 8) | fileEntry.header[2];
    return true;
}

auto TapeStructure::readTTBlock(uint8_t* buffer, unsigned size, bool header) -> bool {
    int data;

    if (!jumpOverTTGap())
        return false;

    for (int countdown = 9; countdown > 0; countdown--) {
        data = getTTByte();

        if ( (data < 0) || (data != countdown))
            return false;
    }

    data = getTTByte();
    if (data == -1)
        return false;

    if (header && ((data == 1) || (data == 2) )); // type: 1 => header block
    else if (!header && (data == 0)); // type: 0 => data block
    else
        return false;

    for (unsigned i = 0; i < size; i++) {
        data = getTTByte();
        if (data < 0)
            return false;

        if (buffer)
            buffer[i] = (uint8_t)data;
    }

    if (!header) {
        data = getTTByte();
        if (data < 0)
            return false;

        if (buffer) {
            for (unsigned i = 0; i < size; i++)
                data ^= buffer[i];

            if (data != 0)
                return false; // wrong checksum
        }
    }

    return true;
}

auto TapeStructure::readCbmBlock(uint8_t* buffer, unsigned& size, bool forceSecondPassEvenFirstPassIsErrorFree) -> bool {
    std::vector<unsigned> errors;

    for (uint8_t pass = 1; pass <= 2; pass++) {
        int data = readCbmBlock(buffer, size, errors, pass); // pass could have been increased here

        if (data == -1)
            return false;

        if (!buffer) // second pass dummy mode
            return true;

        if (data < 0) {
            // pass was not successfull at all. clear all errors to indicate second pass not to check for double errors.
            // means that each single error in second pass let fail the whole block
            errors.clear();

        } else if (errors.size() == 0) {
            uint8_t parity = 0;
            for (unsigned i = 0; i < size; i++)
                parity ^= buffer[i];

            if (pass == 1) {
                if (!parity) {
                    // there were no errors in pass1, so jump over pass2 because we don't need any data from it
                    if (!forceSecondPassEvenFirstPassIsErrorFree)
                        return jumpOverCbmGap();
                    else // dummy run second pass in order to get the correct resume position for custom loaders
                        buffer = nullptr;
                }
            } else
                // we gracefully ignore second pass parity error, otherwise Goonies wouldn't work
                return true;
        }
    }
    return false;
}

auto TapeStructure::readCbmBlock(uint8_t* buffer, unsigned& size, std::vector<unsigned>& errors, uint8_t& pass) -> int {
    int data;
    uint8_t _pass;
    unsigned offset = 0;
    bool firstPass = pass == 1;

    if (!jumpOverCbmGap())
        return -1;

    for (uint8_t countDown = 9; countDown > 0; countDown--) {
        data = getByte();

        if (!buffer) // dummy mode
            continue;

        if (data == -1)
            return data;

        if (countDown != (data & 0x7f))
            return -2;

        if (countDown == 9)
            _pass = data & 0x80;

        else if (_pass != (data & 0x80)) // check if all countdowns belong to same pass
            return -2;
    }

    pass = _pass & 0x80 ? 1 : 2;

    while (1) {
        data = getByte();

        if (data == -1) // end of file
            return -1;

        if (data == -3) { // end of block
            size = offset;
            if (!firstPass)
                // possible errors from first pass were handled, so clear them
                errors.clear();
            return 0;
        }

        if (buffer && (data == -2)) { // other errors
            if (firstPass) {
                errors.push_back(offset);

                if (errors.size() == 30)
                    return -2;

            } else {
                if (errors.size() == 0)
                    // first round was not successful at all, otherwise we would not be in the 2nd round without at least one error.
                    return -2;

                // first block has some errors, make sure there are no double errors on specific positions
                for(auto& errorOffset : errors) {
                    if (errorOffset == offset)
                        return -2;
                }
            }
        } else {
            if (offset == allocatedSize) {
                return -2;
            }

            if (buffer)
                buffer[offset] = (uint8_t)data;
        }
        offset++;
    }
}

auto TapeStructure::getTTByte() -> int {
    int pulse;
    uint8_t byte = 0;

    for (uint8_t i = 0; i < 8; i++) {
        pulse = fetchPulse( );
        if (pulse < 0)
            return -1;

        byte <<= 1;
        if (TT_LONG_PULSE(pulse))
            byte |= 1;
        else if (!TT_SHORT_PULSE(pulse))
            return -2;
    }

    return byte;
}

auto TapeStructure::getByte() -> int {
    uint8_t byte = 0;

    int data = fetchPulse( );
    if (data < 0 || !LONG_PULSE(data))
        return -1;

    data = fetchPulse( );
    if (data < 0)
        return -1;

    if (SHORT_PULSE(data))
        return -3; // end data block

    if (LONG_PULSE(data))
        return -2;

    // L - M  : start byte
    int parity = 1;
    for (uint8_t i = 0; i < 8; i++) {
        byte >>= 1;
        data = getBit();
        if (data < 0)
            return data;

        if (data > 0)
            byte |= 0x80;

        parity ^= data;
    }

    data = getBit(); // get parity bit
    if (data < 0)
        return data;

    if (data != parity)
        return -2;

    return (int)byte;
}

auto TapeStructure::getBit() -> int {
    int pulse1 = fetchPulse();
    if (pulse1 < 0)
        return -1;

    int pulse2 = fetchPulse();
    if (pulse2 < 0)
        return -1;

    if (SHORT_PULSE(pulse1) && (MIDDLE_PULSE(pulse2) || LONG_PULSE(pulse2)))
        return 0; // S - M
    else if ((MIDDLE_PULSE(pulse1) || LONG_PULSE(pulse1)) && SHORT_PULSE(pulse2))
        return 1; // M - S

    return -2;
}

auto TapeStructure::fetchPulse( ) -> int {
    unsigned gap = 0;
    uint8_t byte;
    if (!readForward( byte ))
        return -1;

    if ( version == 0 || byte > 0 ) {
        return byte == 0 ? 256 : (int)byte;
    }

    for(unsigned i = 0; i < 3; i++) {
        if (!readForward( byte ))
            return -1;

        gap |= byte << (i << 3);
    }

    return (int)(gap >> 3);
}

auto TapeStructure::readForward( uint8_t& byte ) -> bool {

    if (rawData) {
        if (curPos == rawSize)
            return false;

        byte = rawData[curPos++];

        return true;
    }

    if (fetchPos == 0) {

        fetchSize = this->tape->read( fetchData, TAPE_FETCH_SIZE, curPos );

        if (fetchSize == 0)
            return false;
    }

    byte = fetchData[fetchPos++];
    curPos++;

    if (fetchPos == fetchSize)
        fetchPos = 0;

    return true;
}

auto TapeStructure::setData(uint8_t* data, unsigned size) -> void {
    rawData = data;
    rawSize = size;

    setPosition(0xc);

    if (!readForward(version))
        version = 1;
}

auto TapeStructure::setPosition( unsigned pos ) -> void {
    curPos = pos;
    fetchPos = 0;
}

auto TapeStructure::readCurFile() -> FileEntry* {

    if (!curFileEntry)
        return nullptr;

    if (curFileEntry->number < 0) {
        if (!nextFile(*curFileEntry))
            return nullptr;
    }

    if (curFileEntry->buffer)
        return curFileEntry;

    setPosition( curFileEntry->offset );

    if (readFile(*curFileEntry)) {
        curFileEntry->endOffset = curPos;
        return curFileEntry;
    }

    return nullptr;
}

auto TapeStructure::readFile(FileEntry& fileEntry) -> bool {

    if (fileEntry.turoTape)
        return readTTFile(fileEntry);

    return readCbmFile(fileEntry);
}

auto TapeStructure::readTTFile(FileEntry& fileEntry) -> bool {
    if (!readTTHeader(fileEntry))
        return false;

	if (fileEntry.endAddr < fileEntry.startAddr)
		return false;
	
    unsigned size = fileEntry.endAddr - fileEntry.startAddr + 1;

    fileEntry.size = size;
    fileEntry.buffer = new uint8_t[size];

    return readTTBlock(fileEntry.buffer, fileEntry.size);
}

auto TapeStructure::readCbmFile(FileEntry& fileEntry) -> bool {
    if (!readCbmHeader( fileEntry ))
        return false;

    switch(fileEntry.type) {
        case 1: // relocatable  (traps take care of this)
        case 3: // non relocatable
            return readCbmFilePrg(fileEntry);
        case 4:
            return readCbmFileSeq(fileEntry);
    }
    return false;
}

auto TapeStructure::readCbmFilePrg(FileEntry& fileEntry) -> bool {
    allocatedSize = 64 * 1024;
    unsigned size = fileEntry.endAddr - fileEntry.startAddr + 1;

    if (size < 0)
        return false;

    fileEntry.size = size;

    fileEntry.buffer = new uint8_t[allocatedSize];

    return readCbmBlock( fileEntry.buffer, fileEntry.size, true );
}

auto TapeStructure::readCbmFileSeq(FileEntry& fileEntry) -> bool {
    allocatedSize = 193;
    unsigned _size = allocatedSize;
    uint8_t* buffer = new uint8_t[allocatedSize];
    fileEntry.size = 0;

    while(true) {
        if (!readCbmBlock( buffer, _size, true )) {
            delete[] buffer;
            return false;
        }

        if (buffer[0] != 2)
            break;

        uint8_t* _buf = new uint8_t[fileEntry.size + 191];

        if (fileEntry.buffer) {
            std::memcpy( _buf, fileEntry.buffer, fileEntry.size );
            delete[] fileEntry.buffer;
        }

        std::memcpy( _buf + fileEntry.size, buffer + 1, 191 );
        fileEntry.size += 191;
        fileEntry.buffer = _buf;
    }

    delete[] buffer;

    return true;
}

}
