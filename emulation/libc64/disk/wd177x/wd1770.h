
// for 1571 following Pins are not used
// Motor, Step, Direction, Drq, Intrq, Tr00, DDEN (force to MFM)

// for 1581 following Pins are not used
// Motor, Drq, Intrq, DDEN (force to MFM)

// unused stuff is not emulated at the moment

/*
data	0xA1	1   0   1   0   0   0   0   1
MFM sync	    1 0 0 0 1 0 0 1 0 1 0 1 0 0 1 ?
                                  - (removed clock bit)
data	0xC2	1   1   0   0   0   0   1   0
MFM sync	    1 0 1 0 0 1 0 1 0 1 0 0 1 0 0 ?
                              - (removed clock bit)
*/

#pragma once

#include "../structure/structure.h"
#include "../../../tools/rand.h"

namespace LIBC64 {

struct WD1770 {

    enum Type {T1770, T1772};

    enum Mode { None = 0, DECODED = 1, ENCODED = 2, FLUX = 4 };
    // MFM encoding
    // 1 -> 01
    // 0 -> 00 (when 1 before) or 10 (when 0 before)
    enum MFM_PATTERN { M00, M10, M01 };

    enum Status { BUSY = 1, DATA_REQUEST_INDEX = 2, TRACKZERO_LOST = 4, CRC_ERROR = 8,
                    RECORD_NOT_FOUND = 0x10, RECORD_TYPE_SPIN = 0x20, WRITE_PROTECT = 0x40, MOTOR_ON = 0x80 };

    enum Command { RESTORE = 0, SEEK = 1, STEP = 2, STEP_IN = 4,
                    STEP_OUT = 6, READ_SECTOR = 8, WRITE_SECTOR = 0xa, READ_ADDRESS = 0xc,
                    FORCE_INTERRUPT = 0xd, READ_TRACK = 0xe, WRITE_TRACK = 0xf };

    static uint16_t CRC1021[256];

    auto setDiskAccessible(bool state) -> void;
    auto read(uint16_t address) -> uint8_t;
    auto write(uint16_t address, uint8_t value) -> void;
    auto rotate(unsigned revolutionCycles) -> void;
    auto reset() -> void;
    auto wasWritten() -> bool { return written; }
    auto resetWritten() -> void { written = false; }
    auto setTrack(DiskStructure::MTrack* trackPtr) -> void;
    auto setPulseIndex(int index, unsigned delta) -> void;
    auto set2Mhz(bool state) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto setWriteProtected(bool state) -> void;
    auto setMode(WD1770::Mode mode) -> void;
    auto setType(WD1770::Type type) -> void;
    auto setTrackZero(bool state) -> void;
    auto writeMode() -> bool { return writeGate; }
    auto getHeadOffset() -> unsigned { return headOffset; }
    auto setHeadOffset( unsigned offset ) -> void { headOffset = offset; }

    std::function<void (bool direction)> stepCall;
    std::function<void ()> toggleWrite;

protected:
    uint8_t mode;
    Type type = T1770;
    uint8_t refCycles = 8;
    bool cpu2Mhz = true;

    uint8_t commandType;
    uint8_t command;
    uint8_t status;
    uint8_t trackReg;
    uint8_t sectorReg;
    uint8_t dataReg;

    bool syncMarkDetector;
    bool syncMarkDetectorC2;
    bool syncMark;

    unsigned delay;
    uint8_t commandStage;
    uint8_t commandSubStage;

    uint8_t bitCounter;
    bool clockBit;
    bool byteReady;

    unsigned pulseDelta;
    int pulseIndex;
    unsigned pulseDuration;
    uint8_t pulseWidth;

    uint16_t byteCount;
    uint16_t crc;
    uint16_t crcFetched;

    DiskStructure::MTrack* trackPtr = nullptr;

    uint16_t readBuffer;
    uint8_t DSR;

    bool writeProtected = false;
    bool diskInserted = false;

    uint16_t sectorLength;

    bool fluxPending;
    uint8_t lastMfmPattern;
    bool written;
    bool writeGate;
    uint8_t F7WriteMode;
    bool clockRemove;

    uint32_t accum;
    unsigned headOffset;

    bool direction;

    uint8_t indexHoleCounter;
    bool indexHoleTransition;
    bool indexHole;
    bool indexHoleWaitBegin;
    bool trackZero;

    unsigned forceInterruptDelay;
    unsigned indexHoleDelay;
    Emulator::Rand randomizer;

    auto readFlux(unsigned revolutionCycles) -> void;
    auto readEncoded(unsigned revolutionCycles) -> void;
    auto readDecoded(unsigned revolutionCycles) -> void;
    auto writeFlux(unsigned revolutionCycles) -> void;
    auto writeEncoded(unsigned revolutionCycles) -> void;
    auto writeDecoded(unsigned revolutionCycles) -> void;
    auto complete() -> void;
    auto updateDR() -> void;
    auto newByte() -> bool;
    auto prepareNextByteToWrite() -> void;
    auto encodeBit() -> bool;

    auto baseCommand() -> uint8_t { return command >> 4; }
    auto decodeMFM( bool bit ) -> void;
    auto updateCRC() -> void;
    auto getTimeFactor() -> uint8_t;
};

}
