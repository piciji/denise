
#include "wd1770.h"

namespace LIBC64 {

auto WD1770::read(uint16_t address) -> uint8_t {
    uint8_t value;

    switch (address & 3) {
        case 0:
            value = status;
            // todo release irq
            break;
        case 1:
            value = track;
            break;
        case 2:
            value = sector;
            break;
        case 3:
            value = data;
            status &= ~DATA_REQUEST;
            break;
    }

    return value;
}

auto WD1770::write(uint16_t address, uint8_t value) -> void {

    switch (address & 3) {
        case 0: {
            // todo release irq
            command = value;

            if ((status & BUSY) && (baseCommand() != FORCE_INTERRUPT))
                return;

            commandStage = 0;
            commandRegisterPrevious = commandRegister;
            commandRegister = value;

            status = 0;

            switch (command) {
                case RESTORE:
                case SEEK:
                case STEP:
                case STEP + 1:
                case STEP_IN:
                case STEP_IN + 1:
                case STEP_OUT:
                case STEP_OUT + 1:
                    // todo
                    commandType = 1;
                    break;

                case READ_SECTOR:
                case READ_SECTOR + 1:

                    if (value & 4)
                        settleDelay = 30000 * 8;	// 30ms
                    else
                        settleDelay = 0;

                    readAddressState = SEARCHING_FOR_NEXT_ID;

                    commandType = 2;
                    break;

                case WRITE_SECTOR:
                case WRITE_SECTOR + 1:

                    statusRegister = INDEX_DATAREQUEST;

                    if (value & 4)
                        settleDelay = 30000 * 8;	// 30ms
                    else
                        settleDelay = 0;

                    readAddressState = SEARCHING_FOR_NEXT_ID;

                    commandType = 2;
                    break;

                case READ_ADDRESS:
                    settleDelay = 0;

                    readAddressState = SEARCHING_FOR_NEXT_ID;

                    commandType = 3;
                    break;

                case READ_TRACK:
                    commandType = 3;
                    break;

                case WRITE_TRACK:
                    commandType = 3;
                    break;

                case FORCE_INTERRUPT:

                    CommandComplete();
                    // todo IRQ handling

                    commandType = 4;
                    break;
            }
        } break;

        case 1:
            track = value;
            break;
        case 2:
            sector = value;
            break;
        case 3:
            data = value;
            status &= ~DATA_REQUEST;
            break;
    }
}

}