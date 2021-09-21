
// for 1571 following PIN's are not connected and not emulated at the moment:
// Motor, Step, Direction, Drq, Intrq, Tr00, DDEN (FM support)
// no support for write precompensation ( needed to increase lifetime of real disks )
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

namespace LIBC64 {

struct WD1770 {

    enum Mode { None = 0, USERDATA = 1, ENCODED = 2, FLUX = 4 };
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

    // if motor is NOT controlled by WD177x, then this method is used to inform about a disk is inserted AND motor is running.
    // if motor is controlled by WD177x, then this method is used to inform about a disk is inserted.
    auto setDiskAccessible(bool state) -> void;
    auto read(uint16_t address) -> uint8_t;
    auto write(uint16_t address, uint8_t value) -> void;
    auto clock() -> void;
    auto reset() -> void;
    auto wasWritten() -> bool { return written; }
    auto resetWritten() -> void { written = false; }
    auto setTrack(DiskStructure::GcrTrack* trackPtr, bool dummyTrack = false) -> void;
    auto setPulseIndex(int index, unsigned delta) -> void;
    auto setRateInMhz(uint8_t caller, uint8_t fluxSamplingRate) -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto setWriteProtected(bool state) -> void;
    auto setMode(WD1770::Mode mode) -> void;
    auto setTrackZero(bool state) -> void;

protected:
    uint8_t mode;
    uint8_t refCycles = 8;
    uint8_t callerMhz = 2;
    unsigned callerHz = 2000000;

    uint8_t commandType;
    uint8_t command;
    uint8_t status;
    uint8_t trackReg;
    uint8_t sectorReg;
    uint8_t dataReg;

    bool syncMarkDetector;
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

    DiskStructure::GcrTrack* trackPtr = nullptr;
    bool dummyTrack = true;

    uint16_t readBuffer;
    uint8_t DSR;

    bool writeProtected = false;
    bool motorAdvance;

    uint16_t sectorLength;

    bool fluxPending;
    uint8_t lastMfmPattern;
    bool written;
    bool writeGate;
    uint8_t F7WriteMode;
    bool fluxRemove;

    uint32_t accum;
    unsigned headOffset;

    bool direction;

    uint8_t indexHoleCounter;
    bool indexHole;
    bool trackZero;

    unsigned forceInterruptDelay;
    unsigned indexHoleDelay;

    auto readFlux() -> void;
    auto readUserData() -> void;
    auto writeFlux() -> void;
    auto writeUserData() -> void;
    auto complete() -> void;
    auto updateDR() -> void;
    auto newByte() -> bool;
    auto prepareNextByteToWrite() -> void;

    auto baseCommand() -> uint8_t { return command >> 4; }
    auto decodeMFM( bool bit ) -> void;
    auto updateCRC() -> void;
    auto getTimeFactor() -> uint8_t;
};

}
