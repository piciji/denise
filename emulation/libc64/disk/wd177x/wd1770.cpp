
#include "wd1770.h"
#include <random>
#include "../../system/system.h"
#include "../../../tools/macros.h"

#define CyclesPerRevolution300Rpm 3200000

namespace LIBC64 {

auto WD1770::setMode(WD1770::Mode mode) -> void {
    this->mode = mode;
}

auto WD1770::setType(WD1770::Type type) -> void {
    this->type = type;
}

auto WD1770::setTrack(DiskStructure::MTrack* trackPtr, bool dummyTrack) -> void {
    this->trackPtr = trackPtr;
    this->dummyTrack = dummyTrack;
}

auto WD1770::setPulseIndex(int index, unsigned delta) -> void {
    this->pulseIndex = index;
    this->pulseDelta = delta;
}

auto WD1770::setRateInMhz(uint8_t caller, uint8_t fluxSamplingRate) -> void {
    this->callerMhz = (caller < 1) ? 1 : caller;
    this->callerHz = this->callerMhz * 1000000;
    this->refCycles = fluxSamplingRate >> getTimeFactor();
}

auto WD1770::setWriteProtected(bool state) -> void {
    writeProtected = state;
}

auto WD1770::setDiskAccessible(bool state) -> void { // if motor is not controlled by WD177x
    motorAdvance = state;
    indexHole = !motorAdvance && (commandType == 1 || commandType == 4);
}

auto WD1770::read(uint16_t address) -> uint8_t {

    switch (address & 3) {
        case 0:
            return status;
        case 1:
            return trackReg;
        case 2:
            return sectorReg;
        case 3:
            status &= ~DATA_REQUEST_INDEX;
            return dataReg;
    }

    _unreachable
}

#define SET_TYPE(_type, _ident) \
    /*system->interface->log("type "#_type" "#_ident);*/ \
    commandType = _type;        \
    indexHole = !motorAdvance && (commandType == 1 || commandType == 4);

auto WD1770::write(uint16_t address, uint8_t value) -> void {

    switch (address & 3) {
        case 0: {
            if (status & BUSY) {
                if ((value >> 4) == FORCE_INTERRUPT) {
                    if (!forceInterruptDelay)
                        forceInterruptDelay = 16 << getTimeFactor();
                } else
                    // too soon: nullifies pending forced interrupt, old command keeps running
                    forceInterruptDelay = 0;

                return;
            }

            command = value;
            commandStage = 1;
            commandSubStage = 0;

            switch (baseCommand()) {
                case RESTORE:           SET_TYPE(1, RESTORE) break;
                case SEEK:              SET_TYPE(1, SEEK) break;
                case STEP:
                case STEP + 1:          SET_TYPE(1, STEP) break;
                case STEP_IN:
                case STEP_IN + 1:       SET_TYPE(1, STEPIN) break;
                case STEP_OUT:
                case STEP_OUT + 1:      SET_TYPE(1, STEPOUT) break;

                case READ_SECTOR:       SET_TYPE(2, READ_SECTOR) break;
                case READ_SECTOR + 1:   SET_TYPE(2, READ_SECTOR_MULTI) break;
                case WRITE_SECTOR:      SET_TYPE(2, WRITE_SECTOR) break;
                case WRITE_SECTOR + 1:  SET_TYPE(2, WRITE_SECTOR_MULTI) break;

                case READ_ADDRESS:      SET_TYPE(3, READ_ADDRESS) break;
                case READ_TRACK:        SET_TYPE(3, READ_TRACK) break;
                case WRITE_TRACK:       SET_TYPE(3, WRITE_TRACK) break;

                case FORCE_INTERRUPT:   SET_TYPE(4, FORCE_INTERRUPT)
                    // hint: command type 4 reflects the status register of command type 1 (track zero and index hole)
                    // todo: is motor off delay canceled too ?
                    commandStage = 0;
                    break;
            }

        } break;

        case 1:
            //if (status & BUSY)
              //  break;
            trackReg = value;
            break;
        case 2:
         //   if (status & BUSY)
           //     break;
            sectorReg = value;
            break;
        case 3:
            dataReg = value;
            status &= ~DATA_REQUEST_INDEX;
            break;
    }
}

auto WD1770::reset() -> void {
    commandType = 0;
    command = 0;
    status = 0;
    trackReg = 0;
    sectorReg = 0;
    dataReg = 0;
    indexHoleCounter = 0;
    syncMarkDetector = false;
    syncMarkDetectorC2 = false;
    syncMark = false;
    delay = 0;
    commandStage = 0;
    commandSubStage = 0;
    bitCounter = 0;
    clockBit = true;
    byteReady = false;
    pulseDelta = 1;
    pulseIndex = -1;
    pulseDuration = 0;
    pulseWidth = 0;
    byteCount = 0;
    crc = 0xcdb4;
    crcFetched = 0;
    readBuffer = 0;
    DSR = 0;
    motorAdvance = false;
    sectorLength = 128;
    fluxPending = false;
    lastMfmPattern = M10;
    writeGate = false;
    F7WriteMode = 0;
    fluxRemove = false;
    accum = 0;
    headOffset = 0;
    direction = 0;
    indexHole = false;
    indexHoleTransition = false;
    indexHoleWaitBegin = false;
    trackZero = false;
    forceInterruptDelay = 0;
    indexHoleDelay = 0;
}

// real chip kill commands between micro cycles and not after a fixed delay.
// exact amount of cycles for checksum calculations and comparisons are unknown. it needs to be done before next byte is ready.
// for time being, we delay for documented worst case instead of interrupting without any delay.
// includes a hack to not interrupt internal CRC writing, otherwise Big Blue Reader interrupts shortly before second CRC byte is written
// maybe the documented 16 micro are not the worst case delay in all situations.
#define CHECK_FORCE_INTERRUPT \
    if (forceInterruptDelay) { \
        if (--forceInterruptDelay == 0) { \
            if (commandSubStage == 16) {    /*hack*/ \
                forceInterruptDelay = 1;    \
            } else if ( (mode != USERDATA) && (commandSubStage == 17) && (bitCounter < 2))  { /* in flux mode, mfm encoding delays write */ \
                forceInterruptDelay = 1;      \
            } else {                  \
                complete(); \
                delay = 1; \
                commandType = 4;  \
                break; \
            }   \
        } \
    }

#define CHECK_VERIFY \
    if (command & 4) {  \
        syncMarkDetector = true; \
        delay = 30000 << getTimeFactor(); \
        commandSubStage = 5; \
    } else  \
        complete();

auto WD1770::clock() -> void {

    if (!commandType)
        return;

    if (writeGate) {
        if (mode == Mode::FLUX)
            writeFlux();
        else
            writeUserData();
    } else {
        if (mode == Mode::FLUX)
            readFlux();
        else
            readUserData();
    }

    switch (commandStage) {
        default:
        case 0: // idle
            return;
        case 1:
            // internal delay to prepare commands
            delay = 5 << getTimeFactor();
            commandStage = 2;
            break;

        case 2:
            delay--;
            if (!delay) {
                if (commandType == 1)
                    status &= ~0x5f; // keep motor and spin up
                else
                    status &= ~0x7f; // keep motor

                status |= Status::BUSY;
                commandStage = 3;
            }
            break;
        case 3:
            if (((status & MOTOR_ON) == 0) && ((command & 8) == 0)) { // H Flag
                // emulate spin up delay, doesn't matter if MO line is connected or not
                status |= MOTOR_ON;
                indexHoleCounter = 0;
                commandStage = 4;
            } else {
                commandStage = 5;
            }
            break;

        case 4: CHECK_FORCE_INTERRUPT
            if (indexHoleCounter == 6) {
                if (commandType == 1)
                    status |= RECORD_TYPE_SPIN;

                commandStage = 5;
            }
            break;

        case 5:
            commandStage = 7;
            if (commandType == 2 || commandType == 3) {
                if (command & 4) { // E Flag
                    delay = 30000 << getTimeFactor();
                    commandStage = 6;
                }
            }
            break;

        case 6: CHECK_FORCE_INTERRUPT
            delay--;
            if (!delay) {
                commandStage = 7;
            }
            break;

        case 7:
            if ((baseCommand() == WRITE_TRACK) || ((baseCommand() & 0xe) == WRITE_SECTOR)) {
                if (writeProtected) {
                    status |= WRITE_PROTECT;
                    complete();
                    break;
                }
            }
            indexHoleCounter = 0;
            syncMarkDetector = (commandType != 1) && (baseCommand() != WRITE_TRACK);
            syncMarkDetectorC2 = baseCommand() == READ_TRACK;
            commandStage = 8;

        case 8: CHECK_FORCE_INTERRUPT

            if (commandType == 1) {
                if (commandSubStage >= 6) {
                    if (indexHoleCounter == 5) {
                        status |= RECORD_NOT_FOUND;
                        complete();
                        break;
                    }
                }

                switch (commandSubStage) {
                    case 0:
                        commandSubStage = 2;
                        switch(baseCommand()) {
                            case STEP_IN:
                                direction = 1;
                                commandSubStage = 3;
                                break;
                            case STEP_IN + 1:
                                direction = 1;
                                commandSubStage = 2;
                                break;
                            case STEP_OUT:
                                direction = 0;
                                commandSubStage = 3;
                                break;
                            case STEP_OUT + 1:
                                direction = 0;
                                commandSubStage = 2;
                                break;
                            case STEP:
                                commandSubStage = 3;
                                break;
                            case STEP + 1:
                                commandSubStage = 2;
                                break;
                            default:
                            case RESTORE:
                                trackReg = 0xff;
                                dataReg = 0;
                                commandSubStage = 1;
                                break;
                            case SEEK:
                                commandSubStage = 1;
                                break;
                        }
                        break;

                    case 1:
                        if (trackReg == dataReg) {
                            CHECK_VERIFY
                        } else {
                            if (dataReg < trackReg) {
                                direction = 1;
                            } else
                                direction = 0;

                            commandSubStage = 2;
                        }
                        break;

                    case 2:
                        if (direction)
                            trackReg--;
                        else
                            trackReg++;

                        stepCall(direction);

                        commandSubStage = 3;
                        break;

                    case 3:
                        if (!direction && trackZero) {
                            trackReg = 0;
                            CHECK_VERIFY
                        } else {
                            switch(command & 3) {
                                default:
                                case 0: delay = 6000 << getTimeFactor(); break;
                                case 1: delay = 12000 << getTimeFactor(); break;
                                case 2: delay = (type == T1772 ? 2000 : 20000) << getTimeFactor(); break;
                                case 3: delay = (type == T1772 ? 3000 : 30000) << getTimeFactor(); break;
                            }
                            commandSubStage = 4;
                        }
                        break;

                    case 4:
                        delay--;
                        if (!delay) {
                            if ( (baseCommand() == SEEK) || (baseCommand() == RESTORE)) {
                                commandSubStage = 1;
                            } else {
                                CHECK_VERIFY
                            }
                        }
                        break;

                    // verify
                    case 5:
                        delay--;
                        if (!delay) {
                            commandSubStage = 6;
                            indexHoleCounter = 0;
                        }
                        break;
                    case 6:
                        if (syncMark) {
                            crc = 0xcdb4;
                            commandSubStage = 7;
                            byteCount = 0;
                            byteReady = false;
                        }
                        break;
                    case 7:
                        if (newByte()) {
                            if (syncMark && (DSR == 0xa1)) {
                                byteCount++;
                                crc = 0xcdb4;

                                if (byteCount == 2) {
                                    byteCount = 0;
                                    commandSubStage = 8;
                                }
                            } else {
                                commandSubStage = 6;
                            }
                        }
                        break;
                    case 8:
                        if (newByte()) {
                            if (DSR == 0xfc || DSR == 0xfd || DSR == 0xfe || DSR == 0xff) {
                                updateCRC();
                                commandSubStage = 9;
                                byteCount = 0;
                                syncMarkDetector = false;
                            } else
                                commandSubStage = 6;
                        }
                        break;
                    case 9:
                        if (newByte()) {
                            byteCount++;

                            if (byteCount == 1) {
                                if (trackReg != DSR) {
                                    commandSubStage = 6;
                                    syncMarkDetector = true;
                                } else
                                    updateCRC();
                            } else if (byteCount == 5) {
                                crcFetched = DSR << 8;
                            } else if (byteCount == 6) {
                                crcFetched |= DSR;

                                if (crc != crcFetched) {
                                    status |= CRC_ERROR;
                                    commandSubStage = 6;
                                    syncMarkDetector = true;
                                } else
                                    complete();
                            } else {
                                updateCRC();
                            }
                        }
                        break;
                }

            } else if (commandType == 2) { // Type 2 commands: Read/Write Sector

                if (commandSubStage < 7) {
                    if (indexHoleCounter == 5) {
                        status |= RECORD_NOT_FOUND;
                        complete();
                        break;
                    }
                }

                switch (commandSubStage) {
                    case 0:
                        if (syncMark) {
                            crc = 0xcdb4;
                            commandSubStage = 1;
                            byteCount = 0;
                            byteReady = false;
                        }
                        break;
                    case 1:
                        if (newByte()) {
                            if (syncMark && (DSR == 0xa1)) {
                                byteCount++;
                                crc = 0xcdb4;

                                if (byteCount == 2) {
                                    byteCount = 0;
                                    commandSubStage = 2;
                                }
                            } else {
                                commandSubStage = 0;
                            }
                        }
                        break;
                    case 2:
                        if (newByte()) {
                            if (DSR == 0xfc || DSR == 0xfd || DSR == 0xfe || DSR == 0xff) {
                                updateCRC();
                                commandSubStage = 3;
                                byteCount = 0;
                                syncMarkDetector = false;
                            } else
                                commandSubStage = 0;
                        }
                        break;
                    case 3:
                        if (newByte()) {
                            // track, side, sector, sector length, crc1, crc2
                            byteCount++;

                            if (byteCount == 1) {
                                updateCRC();
                                if (trackReg != DSR) {
                                    commandSubStage = 0;
                                    syncMarkDetector = true;
                                    break;
                                }
                            } else if (byteCount == 3) {
                                updateCRC();
                                if (sectorReg != DSR) {
                                    commandSubStage = 0;
                                    syncMarkDetector = true;
                                    break;
                                }

                            } else if (byteCount == 4) {
                                DSR &= 3;
                                if (DSR == 0) sectorLength = 128;
                                else if (DSR == 1) sectorLength = 256;
                                else if (DSR == 2) sectorLength = 512;
                                else sectorLength = 1024;

                                updateCRC();
                            } else if (byteCount == 5) {
                                crcFetched = DSR << 8;
                            } else if (byteCount == 6) {
                                crcFetched |= DSR;

                                if (crc != crcFetched) {
                                    status |= CRC_ERROR;
                                    commandSubStage = 0;
                                    syncMarkDetector = true;
                                } else {
                                    commandSubStage = (command & 0x20) ? 10 /*write*/ : 4 /*read*/;
                                    byteCount = 0;
                                    delay = 0; // limit amount of readed bytes to find DAM

                                    syncMarkDetector = command & 0x20 ? false : true;
                                }

                            } else {
                                updateCRC();
                            }
                        }
                        break;

                    case 4:
                        if (newByte()) {
                            if (++delay == 44) {
                                commandSubStage = 0;
                                break;
                            }
                        }

                        if (syncMark) {
                            crc = 0xcdb4;
                            commandSubStage = 5;  // read
                            byteCount = 0;
                        }
                        break;
                    case 5:
                        if (newByte()) {
                            if (++delay == 44) {
                                commandSubStage = 0;
                                break;
                            }

                            if (syncMark && (DSR == 0xa1)) {
                                byteCount++;
                                crc = 0xcdb4;

                                if (byteCount == 2) {
                                    byteCount = 0;
                                    commandSubStage = 6;
                                }
                            } else {
                                commandSubStage = 4;
                            }
                        }
                        break;
                    case 6:
                        if (newByte()) {
                            if (++delay == 44) {
                                commandSubStage = 0;
                                break;
                            }

                            if (DSR == 0xf8 || DSR == 0xf9 || DSR == 0xfa || DSR == 0xfb) {
                                commandSubStage = 7;
                                updateCRC();
                                syncMarkDetector = false;
                                byteCount = 0;
                                if (DSR == 0xf8 || DSR == 0xf9)
                                    status |= RECORD_TYPE_SPIN; // DDAM
                            } else
                                commandSubStage = 4;
                        }
                        break;
                    case 7:
                        if (newByte()) {
                            updateCRC();

                            if (++byteCount == sectorLength) {
                                commandSubStage = 8;
                            }

                            updateDR();
                        }
                        break;
                    case 8:
                        if (newByte()) {
                            crcFetched = DSR << 8;
                            commandSubStage = 9;
                        }
                        break;

                    case 9:
                        if (newByte()) {
                            crcFetched |= DSR;

                            if (crc != crcFetched) {
                                status |= CRC_ERROR;
                                complete();
                            } else {
                                if (command & 0x10) {
                                    commandSubStage = 0;
                                    indexHoleCounter = 0;
                                    sectorReg += 1;
                                } else {
                                    complete();
                                }
                            }
                        }
                        break;
                    case 10: // write sector
                        if (newByte()) {
                            byteCount++;
                            if (byteCount == 2) {
                                status |= DATA_REQUEST_INDEX;
                            } else if (byteCount == 11) {
                                if (status & DATA_REQUEST_INDEX) {
                                    status |= TRACKZERO_LOST;
                                    complete();
                                }
                            } else if (byteCount == 22) {
                                commandSubStage = 11;
                                byteCount = 0;
                                bitCounter = 0;
                                fluxPending = false;
                                lastMfmPattern = M10;
                                pulseDuration = 1;
                                writeGate = true;
                                F7WriteMode = 0;
                                DSR = 0;
                            }
                        }
                        break;
                    case 11:
                        if (newByte()) {
                            byteCount++;
                            if (byteCount == 12) {
                                DSR = 0xa1;
                                fluxRemove = true;
                                commandSubStage = 12;
                                byteCount = 0;
                            }
                        }
                        break;

                    case 12:
                        if (newByte()) {
                            if (++byteCount == 3) {
                                DSR = (command & 1) ? 0xf8 : 0xfb;
                                updateCRC();
                                byteCount = 0;
                                commandSubStage = 13;
                            } else {
                                DSR = 0xa1;
                                fluxRemove = true;
                                crc = 0xcdb4;
                            }
                        }
                        break;
                    case 13:
                        if (newByte()) {
                            DSR = dataReg;
                            status |= DATA_REQUEST_INDEX;
                            byteCount = 0;
                            updateCRC();
                            commandSubStage = 14;
                        }

                        break;
                    case 14:
                        if (newByte()) {

                            if (++byteCount == sectorLength) {

                                DSR = (crc >> 8) & 0xff;
                                // Big Blue Reader reads data register during Controller writes the second CRC byte and expects to
                                // read zero. Afterwards the write operation will be canceled with a type 4 command.
                                // this effectively prevents the controller from writing the final 0xff after the second CRC byte.
                                // I am not sure if this is the intention of this action but it seems the last sector byte
                                // will not read back from data register after it was written.
                                dataReg = 0;
                                commandSubStage = 15;
                            } else {
                                DSR = dataReg;
                                if (status & DATA_REQUEST_INDEX) {
                                    status |= TRACKZERO_LOST;
                                    DSR = 0;
                                }

                                updateCRC();
                                status |= DATA_REQUEST_INDEX;
                            }
                        }
                        break;
                    case 15:
                        if (newByte()) {
                            DSR = crc & 0xff;
                            commandSubStage = 16;
                        }
                        break;
                    case 16:
                        if (newByte()) {
                            DSR = 0xff;
                            commandSubStage = 17;
                        }
                        break;
                    case 17:
                        if (newByte()) {
                            writeGate = false;
                            if (command & 0x10) {
                                commandSubStage = 0;
                                indexHoleCounter = 0;
                                sectorReg += 1;
                            } else {
                                complete();
                            }
                        }
                        break;
                }
            } else if (commandType == 3) {

                switch (baseCommand()) {
                    case READ_ADDRESS: {
                        switch (commandSubStage) {
                            case 0:
                                if (syncMark) {
                                    crc = 0xcdb4;
                                    commandSubStage = 1;
                                    byteCount = 0;
                                    byteReady = false;
                                }
                                break;
                            case 1:
                                if (newByte()) {
                                    if (syncMark && (DSR == 0xa1)) {
                                        byteCount++;
                                        crc = 0xcdb4;

                                        if (byteCount == 2) {
                                            byteCount = 0;
                                            commandSubStage = 2;
                                        }
                                    } else {
                                        commandSubStage = 0;
                                    }
                                }
                                break;
                            case 2:
                                if (newByte()) {
                                    if (DSR == 0xfc || DSR == 0xfd || DSR == 0xfe || DSR == 0xff) {
                                        updateCRC();
                                        commandSubStage = 3;
                                        byteCount = 0;
                                        syncMarkDetector = false;
                                    } else
                                        commandSubStage = 0;
                                }
                                break;
                            case 3:
                                if (newByte()) {
                                    updateDR(); // track, side, sector, sector length, crc1, crc2
                                    byteCount++;

                                    if (byteCount == 1) {
                                        updateCRC();
                                        sectorReg = DSR; // track adr to sector reg for comparisons
                                    } else if (byteCount == 5) {
                                        crcFetched = DSR << 8;
                                    } else if (byteCount == 6) {
                                        crcFetched |= DSR;

                                        if (crc != crcFetched) {
                                            status |= CRC_ERROR;
                                        }
                                        complete();
                                    } else {
                                        updateCRC();
                                    }
                                }
                                break;
                        }

                    } break;

                    case READ_TRACK: {
                        switch (commandSubStage) {
                            case 0:
                                if (indexHoleTransition) {
                                    indexHoleTransition = false;
                                    commandSubStage = 1;
                                    byteReady = false;
                                }
                                break;
                            case 1:
                                // AM detector is always active during read track
                                if (indexHoleTransition) {
                                    indexHoleTransition = false;
                                    complete();
                                } else if (newByte()) {
                                    updateDR();
                                }
                                break;
                        }
                    } break;

                    case WRITE_TRACK: {
                        switch (commandSubStage) {
                            case 0:
                                status |= DATA_REQUEST_INDEX;
                                commandSubStage = 1;
                                byteCount = 0;
                                byteReady = false;
                                break;
                            case 1:
                                if (newByte()) {
                                    if (++byteCount == 3) {
                                        if (status & DATA_REQUEST_INDEX) {
                                            status |= TRACKZERO_LOST;
                                            complete();
                                        } else {
                                            byteCount = 0;
                                            commandSubStage = 2;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                if (indexHoleTransition) {
                                    indexHoleTransition = false;
                                    commandSubStage = 3;
                                    bitCounter = 0;
                                    fluxPending = false;
                                    lastMfmPattern = M10;
                                    pulseDuration = 1;
                                    writeGate = true;
                                    F7WriteMode = 0;
                                    byteReady = false;
                                    prepareNextByteToWrite();
                                }
                                break;
                            case 3:
                                if (indexHoleTransition) {
                                    indexHoleTransition = false;
                                    writeGate = false;
                                    complete();
                                } else if (newByte()) {
                                    prepareNextByteToWrite();
                                }
                                break;
                        }
                    } break;
                }
            }
            break;
        case 9:
            delay--;
            if (!delay) {
                status &= ~Status::BUSY;
                if (status & MOTOR_ON) {
                    commandStage = 10;
                } else
                    commandStage = 0;
            }
            break;
        case 10:
            if (indexHoleCounter == 9) {
                status &= ~MOTOR_ON;
                if (commandType == 1 || commandType == 4)
                    status &= ~RECORD_TYPE_SPIN;
                commandStage = 0;
            }
            break;
    }
}

inline auto WD1770::newByte() -> bool {
    if (!byteReady)
        return false;

    byteReady = false;

    return true;
}

auto WD1770::complete() -> void {
    writeGate = false;
    syncMarkDetector = false;
    syncMarkDetectorC2 = false;
    commandStage = 9;
    indexHoleCounter = 0;
    delay = 16 << getTimeFactor();
}

auto WD1770::updateDR() -> void {
    if (status & DATA_REQUEST_INDEX) {
        status |= TRACKZERO_LOST;
    }

    status |= DATA_REQUEST_INDEX;
    dataReg = DSR;
}

auto WD1770::prepareNextByteToWrite() -> void {

    if (F7WriteMode == 1) {
        F7WriteMode = 2;
        DSR = crc & 0xff;

    } else {
        DSR = dataReg;

        if (status & DATA_REQUEST_INDEX) {
            status |= TRACKZERO_LOST;
            DSR = 0;
        }

        if (F7WriteMode == 0) {
            if (DSR == 0xf7) {
                DSR = (crc >> 8) & 0xff;
                F7WriteMode = 1;
                return;
            } else if (DSR == 0xf5) {
                DSR = 0xa1;
                crc = 0xcdb4;
                fluxRemove = true;
            } else if (DSR == 0xf6) {
                DSR = 0xc2;
                fluxRemove = true;
            } else {
                updateCRC();
            }

        } else {
            // escape 0xf5, 0xf6, 0xf7
            updateCRC();
            if (DSR != 0xf7)
                F7WriteMode = 0;
        }
    }

    status |= DATA_REQUEST_INDEX;
}

auto WD1770::readUserData() -> void {

    if (!motorAdvance)
        return;

    accum += 31250; // 250 (kbits per second)

    if (accum < this->callerHz)
        return;
    // one byte has moved
    accum -= this->callerHz;

    uint8_t* ptr = trackPtr->data;
    unsigned _maxPos = 6250;

    if (ptr) {
        if (trackPtr->size < 6250)
            _maxPos = trackPtr->size;
    }

    if (++headOffset >= _maxPos) {
        headOffset = 0;
        indexHole = true;
        indexHoleTransition = true;
        indexHoleCounter++;
        if (commandType == 1 || commandType == 4)
            status |= DATA_REQUEST_INDEX;
    } else if (indexHole && (headOffset == 100)) {
        indexHole = false;
        indexHoleTransition = false;
        if (commandType == 1 || commandType == 4)
            status &= ~DATA_REQUEST_INDEX;
    }

    if (!ptr || (mode == None)) {
        DSR = 0;
        syncMark = false;

    } else {
        DSR = ptr[ headOffset ];

        syncMark = syncMarkDetector && ((trackPtr->mfmSync[headOffset >> 3] & (1 << (headOffset & 7))) != 0);

//        if (syncMark && indexHole) {
//            indexHole = false;
//            if (commandType == 1 || commandType == 4)
//                status &= ~DATA_REQUEST_INDEX;
//        }
    }

    byteReady = true;
}

auto WD1770::writeUserData() -> void {

    if (!motorAdvance)
        return;

    accum += 31250;

    if (accum < this->callerHz)
        return;
    // one byte has moved
    accum -= this->callerHz;

    uint8_t* ptr = trackPtr->data;
    unsigned _maxPos = 6250;

    if (ptr) {
        if (trackPtr->size < 6250)
            _maxPos = trackPtr->size;
    }

    if (++headOffset >= _maxPos) {
        headOffset = 0;
        indexHole = true;
        indexHoleTransition = true;
        indexHoleCounter++;
        if (commandType == 1 || commandType == 4)
            status |= DATA_REQUEST_INDEX;
    } else if (indexHole && (headOffset == 100)) {
        indexHole = false;
        indexHoleTransition = false;
        if (commandType == 1 || commandType == 4)
            status &= ~DATA_REQUEST_INDEX;
    }

    if ( (mode == None) || dummyTrack) {

    } else {
        ptr[ headOffset ] = DSR;

        if (fluxRemove) {
            fluxRemove = false;
            trackPtr->mfmSync[headOffset >> 3] |= 1 << (headOffset & 7);
        } else {
            trackPtr->mfmSync[headOffset >> 3] &= ~(1 << (headOffset & 7));
        }

        if (!written)
            written = true;

        trackPtr->written = 0x81;
    }

    byteReady = true;
}

auto WD1770::readFlux() -> void {
    uint8_t useCycles = refCycles;
    unsigned todo;

    do {
        if (motorAdvance) {
            todo = pulseDelta;

            if (useCycles < todo)
                todo = useCycles;

            if (indexHoleDelay && (indexHoleDelay < todo) )
                todo = indexHoleDelay;

        } else
            todo = useCycles;

        if (pulseDuration && (pulseDuration < todo)) {
            todo = pulseDuration;
        }

        if (indexHoleDelay) {
            indexHoleDelay -= todo;
            if (!indexHoleDelay) {
                if (indexHoleWaitBegin && !syncMark) {
                    indexHoleWaitBegin = false;
                    indexHoleDelay = 51555;
                    indexHole = true;
                    indexHoleTransition = true;
                    indexHoleCounter++;

                    if (commandType == 1 || commandType == 4)
                        status |= DATA_REQUEST_INDEX;
                } else {
                    indexHole = false;
                    indexHoleTransition = false;
                    indexHoleWaitBegin = false;
                    if (commandType == 1 || commandType == 4)
                        status &= ~DATA_REQUEST_INDEX;
                }
            }
        }

        if (pulseDuration) {
            pulseDuration -= todo;

            if (!pulseDuration) {

                switch(pulseWidth++) {
                    case 0: pulseDuration = 16; break;  // 2.375 + 1.0 = 3.375
                    case 1: pulseDuration = 16; decodeMFM(0); break;  // 3.375 + 1.0 = 4.375
                    // fuzzy bits (4 or 6 micro)
                    case 2: pulseDuration = 16; break;  // 4.375 + 1.0 = 5.375
                    case 3: pulseDuration = 16; decodeMFM(0); break; // 5.375 + 1.0 = 6.375
                    // fuzzy bits (6 or 8 micro)
                    case 4: pulseDuration = 16; break;  // 6.375 + 1.0 = 7.375
                    case 5: pulseDuration = 16; decodeMFM(0); break; // 7.375 + 1.0 = 8.375

                    case 6: pulseDuration = 16; if (rand() & 1) pulseWidth = 9; break;  // 8.375 + 1.0 = 9.375
                    case 7: pulseDuration = 16; decodeMFM(0); break; // 9.375 + 1.0 = 10.375
                    case 8: pulseDuration = 16; break;  // 10.375 + 1.0 = 11.375
                    case 9: pulseDuration = 16; decodeMFM(0); break; // 11.375 + 1.0 = 12.375
                    case 10: pulseDuration = 16; if (rand() & 1) pulseWidth = 13; break;  // 12.375 + 1.0 = 13.375
                    case 11: pulseDuration = 16; decodeMFM(0); break; // 13.375 + 1.0 = 14.375
                    case 12: pulseDuration = 16; break;  // 14.375 + 1.0 = 15.375
                    case 13: pulseDuration = 16; decodeMFM(1); pulseWidth = 0; break; // 15.375 + 1.0 = 16.375
                }
            }
        }

        if (motorAdvance) {
            pulseDelta -= todo;
            if (!pulseDelta) {
                DiskStructure::Pulse& pulse = trackPtr->pulses[pulseIndex];

                pulseIndex = pulse.next;

                if (pulseIndex >= 0) {
                    pulseDelta = trackPtr->pulses[pulseIndex].position - pulse.position;
                } else {
                    pulseIndex = trackPtr->firstPulse;

                    pulseDelta = trackPtr->pulses[pulseIndex].position + (CyclesPerRevolution300Rpm - pulse.position);
                    indexHole = false;
                    indexHoleWaitBegin = true;
                    indexHoleDelay = CyclesPerRevolution300Rpm - pulse.position;
                }

                if ((pulse.strength == 0xffffffff) || (rand() < pulse.strength)) {
                    if (pulseWidth & 1) {
                        // fuzzy bits
                        if (rand() & 1)
                            // todo: adjust randomness by distance to border of inspection window
                            decodeMFM(0);

                        decodeMFM(1);
                    } else if (pulseWidth == 0) {
                        decodeMFM(0); // no flux area
                    } else
                        decodeMFM(1);

                    pulseDuration = 38;    // 2.375
                    pulseWidth = 0;
                }
            }
        }

        useCycles -= todo;
    } while (useCycles);
}

auto WD1770::decodeMFM( bool bit ) -> void {
    byteReady = false;
    readBuffer <<= 1;
    readBuffer |= bit;
    syncMark = false;

    if (syncMarkDetector) {
        if ((readBuffer & 0x7fff) == 0x4489) {  // a1
            //A1 (missing clock between 4 & 5), no data can produce this unique pattern
            syncMark = true;
        }
    }

    if (syncMarkDetectorC2) {
        if ((readBuffer & 0x7fff) == 0x5224) { // c2
            // C2 (missing clock between 3 & 4), a bug in Controller because data can produce this pattern
            // only a problem during read track, because during read sector AM detector will be disabled on datafield
            syncMark = true;
        } else if ((readBuffer & 0x1ff) == 0x29) {
            // Bug in Controller
            syncMark = true;
        }
    }

    if (!clockBit) {
        DSR <<= 1;
        DSR |= readBuffer & 1;

        if (++bitCounter == 8) {
            bitCounter = 0;
            byteReady = true;
        }
    }
    clockBit ^= 1;

    if (syncMark) {
//        if (indexHole) {
//            indexHole = false;
//            indexHoleWaitBegin = false;
//            indexHoleDelay = 0;
//            if (commandType == 1 || commandType == 4)
//                status &= ~DATA_REQUEST_INDEX;
//        }
        clockBit = true;
        bitCounter = 0;
    }
}

auto WD1770::writeFlux() -> void {

    uint8_t useCycles = refCycles;
    bool flux;
    unsigned todo;

    do {
        flux = false;

        if (motorAdvance) {
            todo = pulseDelta;

            if (useCycles < todo)
                todo = useCycles;

            if (indexHoleDelay && (indexHoleDelay < todo) )
                todo = indexHoleDelay;
        } else
            todo = useCycles;

        if (indexHoleDelay) {
            indexHoleDelay -= todo;
            if (!indexHoleDelay) {
                if (indexHoleWaitBegin && !fluxRemove) {
                    indexHoleWaitBegin = false;
                    indexHoleDelay = 51555;
                    indexHole = true;
                    indexHoleTransition = true;
                    indexHoleCounter++;

                    if (commandType == 1 || commandType == 4)
                        status |= DATA_REQUEST_INDEX;
                } else {
                    indexHole = false;
                    indexHoleTransition = false;
                    indexHoleWaitBegin = false;
                    if (commandType == 1 || commandType == 4)
                        status &= ~DATA_REQUEST_INDEX;
                }
            }
        }

        if (pulseDuration && (pulseDuration < todo))
            todo = pulseDuration;

        pulseDuration -= todo;
        if (!pulseDuration) {

            if (fluxPending) {
                flux = true;
                pulseDuration = 4 * 16;
                fluxPending = false;
            } else {
                bool bit = (DSR & 0x80) != 0;
                DSR <<= 1;

                if (++bitCounter == 8) {
                    bitCounter = 0;
                    byteReady = true;
                }

                if (bit) {
                    if (lastMfmPattern == M01) { // 0101
                        flux = true;
                        pulseDuration = 4 * 16;
                    } else if (lastMfmPattern == M00) {  //10001
                        fluxPending = true;
                        pulseDuration = 2 * 16;
                    } else /*if (lastMfmPattern == M10)*/ { // 1001
                        fluxPending = true;
                        pulseDuration = 2 * 16;
                    }

                    lastMfmPattern = M01;
                } else {
                    if (lastMfmPattern == M01) { // 0100x
                        pulseDuration = 2 * 16;
                        lastMfmPattern = M00;
                    } else if (lastMfmPattern == M00) {  //10010
                        flux = true;
                        pulseDuration = 4 * 16;
                        lastMfmPattern = M10;
                    } else /*if (lastMfmPattern == M10)*/ { // 1010
                        if (!fluxRemove)
                            flux = true;
                        else
                            fluxRemove = false;

                        pulseDuration = 4 * 16;
                        lastMfmPattern = M10;
                    }
                }
            }
        }

        if (motorAdvance) {
            pulseDelta -= todo;

            if (pulseDelta) {
                if (flux) {
                    if (!writeProtected) {
                        DiskStructure::Pulse& pulse = trackPtr->pulses[pulseIndex];
                        unsigned position;

                        if (pulseDelta >= pulse.position)
                            position = CyclesPerRevolution300Rpm - (pulseDelta - pulse.position);
                        else
                            position = pulse.position - pulseDelta;

                        if (!dummyTrack) {
                            DiskStructure::addPulse(trackPtr, position, 0xffffffff);

                            if (!written)
                                written = true;

                            trackPtr->written = 0x81;
                        }
                    }
                }
                // else
                // no new flux at this position ... there is already no flux here ... nothing to do
            } else {

                DiskStructure::Pulse& pulse = trackPtr->pulses[pulseIndex];

                if (!writeProtected && !dummyTrack) {
                    if (flux) {
                        if (pulse.strength != 0xffffffff)
                            // 1541 always write strong pulses
                            pulse.strength = 0xffffffff;

                    } else
                        DiskStructure::freePulse(trackPtr, pulseIndex);

                    if (!written)
                        written = true;

                    trackPtr->written = 0x81;
                }

                pulseIndex = pulse.next;

                if (pulseIndex >= 0) {
                    pulseDelta = trackPtr->pulses[pulseIndex].position - pulse.position;
                } else {
                    pulseIndex = trackPtr->firstPulse;

                    pulseDelta = trackPtr->pulses[pulseIndex].position + (CyclesPerRevolution300Rpm - pulse.position);

                    indexHole = false;
                    indexHoleWaitBegin = true;
                    indexHoleDelay = CyclesPerRevolution300Rpm - pulse.position;
                }
            }
        }

        useCycles -= todo;
    } while(useCycles);
}

auto WD1770::setTrackZero(bool state) -> void {
    trackZero = state;

    if (commandType == 1 || commandType == 4) {
        if (trackZero) {
            status |= TRACKZERO_LOST;
        } else {
            status &= ~TRACKZERO_LOST;
        }
    }
}

inline auto WD1770::getTimeFactor() -> uint8_t {
    return callerMhz - 1;
}

inline auto WD1770::updateCRC() -> void {
    crc = CRC1021[(crc >> 8) ^ DSR] ^ (crc << 8);
}

uint16_t WD1770::CRC1021[] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

auto WD1770::serialize(Emulator::Serializer& s) -> void {
    s.integer(mode);
    s.integer((uint8_t&)type);
    s.integer(commandType);
    s.integer(command);
    s.integer(status);
    s.integer(trackReg);
    s.integer(sectorReg);
    s.integer(dataReg);
    s.integer(syncMarkDetector);
    s.integer(syncMarkDetectorC2);
    s.integer(syncMark);
    s.integer(delay);
    s.integer(commandStage);
    s.integer(commandSubStage);
    s.integer(bitCounter);
    s.integer(clockBit);
    s.integer(byteReady);
    s.integer(pulseDelta);
    s.integer(pulseIndex);
    s.integer(pulseDuration);
    s.integer(pulseWidth);
    s.integer(byteCount);
    s.integer(crc);
    s.integer(crcFetched);
    s.integer(readBuffer);
    s.integer(DSR);

    s.integer(writeProtected);

    s.integer(sectorLength);
    s.integer(fluxPending);
    s.integer(lastMfmPattern);
    s.integer(written);
    s.integer(writeGate);
    s.integer(F7WriteMode);
    s.integer(fluxRemove);
    s.integer(accum);
    s.integer(headOffset);
    s.integer(direction);

    s.integer(indexHoleCounter);
    s.integer(indexHole);
    s.integer(indexHoleTransition);
    s.integer(indexHoleWaitBegin);
    s.integer(trackZero);
    s.integer(forceInterruptDelay);
    s.integer(indexHoleDelay);
}

}
