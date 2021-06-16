
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

            // Type II Commands
            // Command          Bit 7     B6     B5     B4     B3     B2     B1     Bit 0
            // ------------     -----     --     --     --     --     --     --     -----
            // Read Sector      1         0      0      m      h      E      0      0
            // Write Sector     1         0      1      m      h      E      P      a0

            // m (Multiple Sectors) - If this bit = 0, the 177x reads or writes ("accesses") only one sector.
            //						If this bit = 1, the 177x sequentially accesses sectors up to and including the last sector on the track.
            //						A multiple-sector command will end prematurely when the CPU loads a Force Interrupt command into the Command Register.
            // E(Settling Delay) - If this flag is set, the head settles before command execution.
            //						The settling time is 15ms for the 1772 and 30ms for the 1770.
            // P (Write Precompensation) - On the 1770-02 and 1772-00, a 0 value in this bit enables automatic write precompensation.
            //							The FDDC delays or advances the write bit stream by one-eighth of a cycle according to the following table.
            // Previous          Current bit           Next bit
            // bits sent         sending               to be sent       Precompensation
            // ---------         -----------           ----------       ---------------
            // x       1         1                     0                Early
            // x       0         1                     1                Late
            // 0       0         0                     1                Early
            // 1       0         0                     0                Late
            //
            //							Programmers typically enable precompensation on the innermost tracks, where bit shifts usually occur and bit density is maximal.
            //							A 1 value for this flag disables write precompensation.
            // a0(Data Address Mark) - If this bit is 0, the 177x will write a normal data mark (0xfb).
            //							If this bit is 1, the 177x will write a deleted	data mark (0xf8).

            // Type III commands are Read Address, Read Track, and Write Track.
            // Command          Bit 7     B6     B5     B4     B3     B2     B1     Bit 0
            // ------------     -----     --     --     --     --     --     --     -----
            // Read Address     1         1      0      0      h      E      0      0
            // Read Track       1         1      1      0      h      E      0      0
            // Write Track      1         1      1      1      h      E      P      0

            // Read Address:
            // The 177x reads the next ID field it finds, then sends the CPU the following six bytes via the Data Register:
            // Byte #     Meaning                |     Sector length code     Sector
            // length
            // ------     ------------------     |     -------------------------------
            // 1          Track                  |     0                      128
            // 2          Side                   |     1                      256
            // 3          Sector                 |     2                      512
            // 4          Sector length code     |     3                      1024
            // 5          CRC byte 1             |
            // 6          CRC byte 2             |

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
                    // The controller waits for a sector ID field that has the correct track number, sector number, and CRC.
                    // The controller then checks for the Data Address Mark, which consists of 43 copies of the second byte of the CRC.
                    // If the controller does not find a sector with correct ID	field and address mark within 5 disk revolutions, the command ends.
                    // Once the 177x finds the desired sector, it loads the bytes of that sector into the data register.
                    // If there is a CRC error at the end of the data field, the 177x sets the CRC Error bit in the Status Register and ends the command regardless of the state of the "m" flag.

                    if (value & 4)
                        settleDelay = 30000 * 8;	// 30ms
                    else
                        settleDelay = 0;

                    readAddressState = SEARCHING_FOR_NEXT_ID;

                    commandType = 2;
                    break;

                case WRITE_SECTOR:
                case WRITE_SECTOR + 1:
                    // The 177x waits for a sector ID field with the correct track number, sector number, and CRC.The 177x then counts off 22 bytes from the CRC field.
                    // If the CPU has not loaded a byte into the Data Register before the end of this 22 - byte delay, the 177x ends the command.
                    // Assuming that the CPU has heeded the 177x's data request, the controller writes 12 bytes of zeroes.
                    // The 177x then writes a normal or deleted Data Address Mark according to the a0 flag of the command.
                    // Next, the 177x writes the byte which the CPU placed in the Data Register, and continues to request and write data bytes until the end of the sector.
                    // After the 177x writes the last byte, it calculates and writes the 16 - bit CRC.The chip then writes one $ff byte.
                    // The 177x interrupts the CPU 24 cycles after it writes the second byte of the CRC.
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