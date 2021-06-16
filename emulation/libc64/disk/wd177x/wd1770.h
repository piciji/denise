
// at the moment, only parts needed by 1571 will be emulated.
// for 1571 following PIN's are not connected: Motor, Step, Direction, Drq, Intrq, Tr00

#pragma once

namespace LIBC64 {

class WD1770 {

    enum Status { BUSY = 1, DATA_REQUEST = 2, TRACKZERO = 4, CRC_ERROR = 8,
                    SEEK_ERROR = 0x10, DATAMARK = 0x20, WRITE_PROTECT = 0x40, MOTOR_ON = 0x80 };

    enum Command { RESTORE = 0, SEEK = 1, STEP = 2, STEP_IN = 4,
                    STEP_OUT = 6, READ_SECTOR = 8, WRITE_SECTOR = 0xa, READ_ADDRESS = 0xc,
                    FORCE_INTERRUPT = 0xd, READ_TRACK = 0xe, WRITE_TRACK = 0xf };

    uint8_t commandType;
    uint8_t command;
    uint8_t status;

    uint8_t track;
    uint8_t sector;
    uint8_t data;

    unsigned settleDelay;

    auto read(uint16_t address) -> uint8_t;
    auto write(uint16_t address, uint8_t value) -> void;
    auto baseCommand() -> uint8_t { return command >> 4; };
};

}
