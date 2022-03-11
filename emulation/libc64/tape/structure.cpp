
#include "structure.h"
#include "tape.h"

#define TAPE_FETCH_SIZE 50 * 1024

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
}

auto TapeStructure::getListing( ) -> std::vector<Emulator::Interface::Listing>& {
    FileEntry fileEntry;
    Emulator::C64Listing listing;
    uint8_t head[16] = {'C','6','4','-','T', 'A', 'P', 'E','-','R','A','W',' ',' ',' ',' '};

    setPosition(0x14);
    unsigned id = 0;

    listings.clear();

    listing.convertToScreencode = system->interface->convertToScreencode;

    listings.push_back( {id++, listing.buildHeadline( &head[0] ) } );

    while( nextFile(fileEntry) ) {

        uint8_t type = (fileEntry.type == 4) ? 1 : 2 ;

        // round up in case of fractional block
        unsigned size = (fileEntry.endAddr - fileEntry.startAddr + 253) / 254;

        listings.push_back( {id++, listing.buildListing( &(fileEntry.name[0]), size, type, fileEntry.turoTape ) });
    }

    return listings;
}

auto TapeStructure::getFilePosition( unsigned fileNumber ) -> unsigned {
    setPosition(0x14);
    FileEntry fileEntry;

    while(nextFile(fileEntry)) {

        if (fileEntry.number == fileNumber)
            break;
    }

    return curPos;
}

auto TapeStructure::analyzeFile() -> int {
    int pulse;
    unsigned hintCBM = 0;
    unsigned startCBM = 0;
    unsigned hintTT = 0;
    unsigned startTT = 0;
    unsigned posBefore;

    while ((hintCBM < 1000) && (hintTT < 1600)) {

        posBefore = curPos;
        pulse = fetchPulse();

        if (pulse == -1) // end of tape
            return UNKNOWN;

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
    }

    if (hintTT >= 1600) {
        setPosition( startTT + 2 );
        return TURBO_TAPE;
    }

    setPosition( startCBM );
    return CBM_TAPE;
}

auto TapeStructure::findCbm() -> bool {
    int pulse;
    unsigned hintCBM = 0;
    unsigned startCBM = 0;

    while (hintCBM < 32) {
        pulse = fetchPulse();

        if (pulse == -1) // end of tape
            return false;

        if (SHORT_PULSE(pulse)) {
            hintCBM++;
        } else {
            hintCBM = 0;
            startCBM = curPos;
        }
    }

    setPosition( startCBM );
    return true;
}

auto TapeStructure::skipCbm() -> bool {
    int data;
    unsigned tempPos;
    unsigned tempPos2;
    uint8_t tries = 0;

    while (1) {
        tempPos = curPos;
        data = fetchPulse( );
        tempPos2 = curPos;

        if (LONG_PULSE(data)) {
            setPosition( tempPos );
            data = getByte();

            if (data == -1)
                return false;

            if (data < -1) {
                if (++tries > 35) {
                    break;
                }

                setPosition( tempPos2 );
            } else {
                setPosition( tempPos );
                break;
            }
        } else if (data < 0) {
            return false;
        } else if (!SHORT_PULSE(data)) {
            break;
        }
    }

    return true;
}

auto TapeStructure::skipCbmFile(bool seq) -> bool {

    if (!skipCbm())
        return false;

    if (!findCbm()) // skip repeated header
        return false;

    if (!skipCbm())
        return false;

    if (seq) {
        uint8_t buffer[193];
        unsigned tempPos;

        while (1) {
            tempPos = curPos;

            if (!findCbm()) {
                setPosition( tempPos );
                break;
            }

            bool state = readCbmBlock(buffer, 193);
            if (!state || (buffer[0] != 2) ) {
                setPosition( tempPos );
                break;
            }
        }
    } else {
        if (!findCbm()) // skip data
            return false;

        if (!skipCbm())
            return false;

        if (!findCbm()) // skip repeated data
            return false;

        if (!skipCbm())
            return false;
    }

    return true;
}

auto TapeStructure::skipTT() -> bool {
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

auto TapeStructure::skipTTFile() -> bool {
    uint8_t buffer[193];

    bool state = readTTBlock(buffer, 193, true);
    if (state)
        state = readTTBlock(nullptr, ((buffer[3] << 8) | buffer[2]) - ((buffer[1] << 8) | buffer[0]) + 1);

    return state;
}

auto TapeStructure::nextFile(FileEntry& fileEntry) -> bool {
    unsigned fileType;
    unsigned tempPos;

    if (fileEntry.number >= 0) {
        if (fileEntry.turoTape)
            skipTTFile();
        else
            skipCbmFile(fileEntry.type == 4);
    }

    while (true) {
        if ((fileType = analyzeFile()) == UNKNOWN)
            return false;

        fileEntry.turoTape = fileType == TURBO_TAPE;

        tempPos = curPos;

        if (fileEntry.turoTape) {
            if (!readTTHeader(fileEntry)) {
                setPosition( tempPos );
                skipTT();
                continue;
            }

        } else {
            if (!readCbmHeader(fileEntry)) {
                setPosition( tempPos );
                int pulse;

                do {
                    pulse = fetchPulse();
                } while (SHORT_PULSE(pulse));

                continue;
            }
        }

        if (fileEntry.type == 5) // end of tape marker
            return false;

        setPosition( tempPos );
        break;
    }

    fileEntry.number++;
    return true;
}

auto TapeStructure::readCbmHeader(FileEntry& fileEntry) -> bool {
    uint8_t buffer[255];

    if (!readCbmBlock(buffer, 255))
        return false;

    if (buffer[0] != 1 && buffer[0] != 3 && buffer[0] != 4)
        return false;

    fileEntry.type = buffer[0];  // 1, 3 PRG, 4 SEQ
    fileEntry.turoTape = false;
    fileEntry.startAddr = (buffer[2] << 8) | buffer[1];
    fileEntry.endAddr = (buffer[4] << 8) | buffer[3];
    std::memcpy(fileEntry.name, buffer + 5, 16);
    return true;
}

auto TapeStructure::readTTHeader(FileEntry& fileEntry) -> bool {
    uint8_t buffer[193];

    if (!readTTBlock(buffer, 193, true))
        return false;

    fileEntry.type = 1; // TPRG
    fileEntry.startAddr = (buffer[1] << 8) | buffer[0];
    fileEntry.endAddr = (buffer[3] << 8) | buffer[2];
    std::memcpy(fileEntry.name, buffer + 5, 16);
    return true;
}

auto TapeStructure::readTTBlock(uint8_t* buffer, unsigned size, bool header) -> bool {
    int data;

    if (!skipTT())
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

auto TapeStructure::readCbmBlock(uint8_t* buffer, unsigned size) -> bool {
    std::vector<unsigned> errors;

    for (uint8_t pass = 1; pass <= 2; pass++) {
        int data = readCbmBlock(buffer, size, errors, pass); // pass could have been increased here

        if (data == -1)
            return false;

        if (data < 0) {
            errors.clear();

        } else if (errors.size() == 0) {
            if (pass == 1) {
                bool res = findCbm();
                res &= skipCbm();

                if (!res)
                    return false;
            }

            uint8_t parity = 0;
            for (unsigned i = 0; i < size; i++)
                parity ^= buffer[i];

            return !parity;
        }

        if (pass < 2) {
            if (!findCbm())
                return false;
        }
    }

    return false;
}

auto TapeStructure::readCbmBlock(uint8_t* buffer, unsigned& size, std::vector<unsigned>& errors, uint8_t& pass) -> int {

    int data;
    uint8_t _pass;
    unsigned count = 0;
    bool firstPass = pass == 1;

    if (!skipCbm())
        return -1;

    for (uint8_t countDown = 9; countDown > 0; countDown--) {
        data = getByte();

        if (data < 0)
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
            size = count;
            if (!firstPass)
                // possible errors from first pass were handeld, so clear them
                errors.clear();
            return 0;
        }

        if (data == -2) { // other errors
            if (firstPass) {
                errors.push_back(count);

                if (errors.size() == 30)
                    return -5;

            } else {
                if (errors.size() == 0)
                    // first round was not successful at all, otherwise we would not be in the 2nd round without at least one error.
                    return -6;

                // first block has some errors, make sure there are no double errors on specific positions
                for(auto& error : errors) {
                    if (error == count)
                        return -6;
                }
            }

            count++;
        } else {
            if (count == size)
                return -4;

            buffer[count++] = (uint8_t)data;
        }
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
        return 0; // S-M
    else if ((MIDDLE_PULSE(pulse1) || LONG_PULSE(pulse1)) && SHORT_PULSE(pulse2))
        return 1; // M-S

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
        // tape image was fully loaded because of compressed file
        // tape image can't be written in this case
        if (curPos == rawSize)
            return false;

        byte = rawData[curPos++];

        return true;
    }

    // uncompressed files shouldn't load before
    // needed data will be loaded by callback in chunks
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

}
