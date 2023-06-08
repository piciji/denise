
// Floppy Disc Controller

// Paula communicates with the drives via 3 lines. Read, Write, Write enable
// Each change in the magnetic flux causes an edge on the reading pin.
// Paula now measures the elapsed time to determine whether the bit sequence 101, 1001 or 10001 was received.
// Depending on whether "fast" or "slow" is selected in "adkcon", the time until a zero is pushed into the bitstream varies
// if there are no flux changes.
// Paula knows the Agnus beat. (the drive doesn't) Thus, the measured times are multiples of ~280 ns (differs between PAL/NTSC).
// Prolonged sequences without magnetization, such as 100001, are dangerous because they can cause random flux changes (not on disc).
// Therefore, these sequences are not valid GCR or MFM (weak bits) and the reason why GCR/MFM encoding is needed at all.
//
// Paula doesn't know which drives are connected or selected. There is simply no data coming from/going to these drives.
// Write operations are possible simultaneously to multiple drives. Reading multiple drives at the same time does not lead to a meaningful result.
//
// For the sake of simplicity, the rotation of the floppy disk is driven here. In reality, the drive does not know the Agnus clock
// and rotates at 300 RPM. Depending on the width of the bitcells on the floppy disk, a certain number of flux changes become visible to Paula.
// Based on the elapsed Agnus cycles, the rotation time can be calculated without Paula being able to impose a speed on the drive.

// The controller writes one bit in 7 DMA cycles. ADFs are copyable and must therefore meet this standard.
// Therefore, each ADF track is brought to the following number of bytes when reading in.

// speed: floppy speed is the same for PAL/NTSC: 0,2 sec per revolution (full track rotation)
// access: differs between PAL/NTSC. a bitcell access is connected to system clock (in context of Paula, not drive) and takes roughly 2 us
// PAL: 7.09379 MHz, NTSC: 7.15909 MHz
// PAL bitcell time:  140.96837 ns (1 cycle) * 14 (nearest integral) = 1.97356 us
// NTSC bitcell time:  139.68256 ns (1 cycle) * 14 (nearest integral) = 1.95556 us
// PAL: 0.2 s / 1.97356 us = 101339 bits = 12668 bytes per revolution
// NTSC: 0.2 s / 1.95556 us = 102272 bits = 12784 bytes per revolution

// all above is valid only for reading standard amiga written disks or writing disks.
// a sophisticated device for generating original disks can write bits with a variable bitcell width,
// so timing can change each bitcell. The amiga drives can only write a fixed bitcell width, see above
// e.g. PAL: 1.97356 us or 3.94712 us (if slow mode is selected in controller)

// precompensation is uninteresting for emulation, beacuse it doesn't change bitcell width but
// advance or delays the flux transition within the bitcell in order to optimize the distance
// between 2 adjacent transitions

// Since ADF's do not contain copy protections, it is sufficient to transfer them byte by byte, especially in terms of emulation speed.

// In contrast to the ADF, EXT ADF does not contain the user data, but the encoded data (GCR or MFM).
// These are not flux change data, but some copy protections can already be handled.
// This is done by the fact that due to the track bit lengths given in the header of the file, the bitcell width can be determined,
// which may well deviate from the standard bitcell width, see ADF. The emulation takes this into account and allows
// the duration of the bitcells determined in this way to elapse before the next bit can be read.
// The bitcell width is manipulated by setting additional bits in the GAP area of the disc.
// Copy protections that test the sectors for a certain reading time (different from the fixed one, that the drive can write)
// can be successfully operated with this.
// However, the EXT format does not contain any information about reconstructing variable bitcell widths within a track.
// Like G64, EXT ADF can only handle one custom bitcell width per track.
// Fuzzy bits also cannot be imaged with EXT ADF.
// Both requires a real flux change format.

// fuzzy bits
// The controller can only determine the correct number of zeros between two flux changes,
// if the bitcell width does not deviate too far from the standard. Some copy protections take advantage of this by writing flux changes
// at a distance of 5 us. The controller reads this in as episode 101 or as episode 1001. The controller itself is purely digital
// and does not cause fluctuations but the drive motor does. If the data is written regularly, i.e. with about 4 us,
// the controller always reads back the same sequence and a copy can thus be recognized.

// Another limitation of the EXT format is writing to tracks that do not have a standard bitcell width
// or by writing a custom bitcell width because of adjusted motor speed.
// Adjusting motor speed is the only way to write a custom bitcell width with Amiga drives.
// The emulation will no longer change the previously defined bitcell width when writing the track later.
// This is technically complex/impossible, since a track can also be partially written.
// Then the track would have to be shortened to the standard bitcell width, which could lead to data loss.
// In addition, the correct amount of bits would have to be written into the GAP area of the track to get the corresponding bitcell width.
// To do this, you would have to know in advance how much is written. To display this in EXT format would be silly.
// In this case, it is better to leave the current number of bits. Practically, this should not be a problem,
// since newly created EXT ADF discs already have the standard bitcell width and games with copy protections write savegames
// to empty or standard length tracks. Even if not, reading the data would not fail, since only the reading time varies.
// a real flux change format doesn't have this limitation. you can write tracks partially with any bitcell
// width or motor adjusted bitcell width on tracks with a different bitcell width.

// For EXT ADF, the "adkcon" setting "slow" or "fast" only plays a role when writing. When reading, the bit sequence is already fixed.
// Only with a flux change format does this setting also play a role when reading. Based on this, the number of zeros is determined,
// which are pushed into the bit stream between 2 flux changes.

// The controller has no idea if it is an HD or DD disk. High density is only achieved by halving the rotational speed.
// This happens automatically when the HD drive detects an HD disk.
// Halving the speed means writing double amount of data on same surface.
// Reading needs halving the speed too, otherwise the Controller (DMA) could not handle the double amount of data.

#include "paula.h"
#include "../drive/diskDrive.h"

#define FDC_IDLE (56 << 3)

namespace LIBAMI {

auto Paula::setDskLen(uint16_t value) -> void {
    uint16_t oldValue = dskLen;
    dskTransferLength = value & 0x3fff;
    dskLen = value;
    bool start = false;

    if (value & oldValue & 0x8000) {
        if ((diskState == DiskState::READ || diskState == DiskState::WAIT_SYNC_READ) && !(value & 0x4000))
            return; // keep state

        setDskState(wordSync() ? DiskState::WAIT_SYNC_READ : DiskState::READ);
        start = true;
    } else if (!(value & 0x8000)) {
        setDskState(DiskState::OFF);
        dskTransferLength = 0;
        processDiskIdleCycles();
        dmaCycles = FDC_IDLE;
        dskEventCycle = agnus.clock + dmaCycles;
        return;
    }

    if (!wordSync() && (dskTransferLength == 0)) { // todo recheck this
        processDiskIdleCycles();
        finishDMA();
        return;
    }

    if (value & oldValue & 0x4000) {
        if(dskTransferLength == 0)
            return;

        if(dskTransferLength == 1)
            return finishDMA();

        if (diskState == DiskState::WRITE || diskState == DiskState::WAIT_SYNC_WRITE)
            return; // keep state

        setDskState(wordSync() ? DiskState::WAIT_SYNC_WRITE : DiskState::WRITE);
        dskBytr &= ~0x8000;
        start = true;
    }

    if (start) {
        fifoPos = 0;
        fifoReady = false;
        dskShifter = 0;

        if (wordSync()) {
            if(dskShifter == dskSync)
                dskShifterPos = 15;
            else
                dskShifterPos &= 15;
        } else if (diskState == DiskState::WRITE) {
            dskShifterPos = 16;
        } else
            dskShifterPos &= 15;

        processDiskIdleCycles();
        useInstantDriveAccess() ? instantDriveAccess() : setFdcEvent();
    }
}

auto Paula::finishDMA() -> void {
    setDskBlkInt();
    dskLen = 0;
    agnus.dmal = 0; // prevent endless loop in turbo mode
    dskTransferLength = 0;
    setDskState(DiskState::OFF);
    dmaCycles = FDC_IDLE;
    dskEventCycle = agnus.clock + dmaCycles;
}

auto Paula::setFdcEvent() -> void {
    if ((activeDrive->structure.type == DiskStructure::ADF) || (activeDrive->structure.type == DiskStructure::Unknown))
        dmaCycles = 7 * 8;
    else
        dmaCycles = !fast() ? 14 : 7;

    dskEventCycle = agnus.clock + dmaCycles;
}

auto Paula::getDskBytR() -> uint16_t {
    uint16_t out = dskBytr & 0x80ff;
    dskBytr &= ~0x8000;
    if (dmaDisk && (diskState != DiskState::OFF)) out |= 0x4000;
    if (dskLen & 0x4000) out |= 0x2000;
    if (dskSyncCycle) {
        if((agnus.clock - dskSyncCycle) <= (!fast() ? 14 : 7) ) out |= 0x1000;
        else dskSyncCycle = 0;
    }
    return out;
}

auto Paula::setDskSync(uint16_t value) -> void {
    if (dskSync != value) {
        dskSync = value;
        if (dskSync == dskShifter)
            setDskSyncInt();
    }
}

auto Paula::setDskDat(uint16_t value) -> void {
    addToFifo(value);
}

auto Paula::dskDatR() -> uint16_t {
    uint8_t repeat = 1 << ((diskState == DiskState::READ) ? turbo : 0);
    uint16_t out = 0;

    do {
        if(getFromFifo(out)) {
            if (dskTransferLength) {
                if (!--dskTransferLength) {
                    finishDMA();
                    break;
                }
            }
            if (repeat == 1)
                break;
            agnus.fakeDiskDma(out);
            repeat--;
        }

        handleFDControllerRead<true>();
    } while (1);

    return out;
}

auto Paula::instantDriveAccess() -> void {
    uint8_t out = 0;

    switch(diskState) {
        case DiskState::WAIT_SYNC_READ:
        case DiskState::READ:
            if (system->isProcessFrame())
                out = activeDrive->instantRead(dskTransferLength, dskSync, diskState == DiskState::WAIT_SYNC_READ);
            break;
        case DiskState::WAIT_SYNC_WRITE:
        case DiskState::WRITE: { // no support for multi selected drives
            if (system->isProcessFrame())
                out = activeDrive->instantWrite(dskTransferLength, dskSync, diskState == DiskState::WAIT_SYNC_WRITE);
        } break;
        default:
            return;
    }

    if (out & 1)
        setDskSyncInt();

    if (out & 2) {
        dmaCycles = 120; // some more delay, to increase compatibility. e.g. Jim Power
        setDskState(DiskState::INSTANT_BLK_INT);
    } else {
        dmaCycles = FDC_IDLE;
        setDskState(DiskState::OFF);
    }

    dskLen = 0;
    dskTransferLength = 0;
    dskEventCycle = agnus.clock + dmaCycles;
}

template<bool readWord, bool waitTurbo> auto Paula::handleFDControllerRead() -> void {
    unsigned iterations;
    // Paula has no idea if a drive is selected or its motor is running.

    switch(activeDrive->structure.type) {
        case DiskStructure::Unknown:
        case DiskStructure::ADF: { // msbsync and !fast unemulated, because don't make sense in context of ADF
            uint8_t byte;
            if constexpr (readWord) iterations = 2;
            else if constexpr (waitTurbo) iterations = 1 << turbo;

            do {
                byte = activeDrive->readByte(dmaCycles, !(readWord || waitTurbo) );
                dskBytr = byte | 0x8000;

                for (int i = 7; i >= 0; i--) {
                    dskShifter = (dskShifter << 1) | ((byte >> i) & 1);

                    if (dskTransferLength && (dskShifterPos == 15) && dmaDisk && (diskState == DiskState::READ))
                        addToFifo(dskShifter);

                    if (dskShifter == dskSync) {
                        setDskSyncInt();
                        // When reading "DiskbytR" the sync status is displayed 2 (fast) or 4 (slow) micro seconds,
                        // i.e. until the next bit follows and the sync status is probably no longer given.
                        // Since whole bytes are read in one piece, we have to store a timestamp to calculate the duration of one bit.
                        // An exact emulation is not possible with ADF but also not necessary.
                        // It would be possible that the sync status still exists after the next bit.
                        // But this can only be determined every 8 bits. In EXT ADF, the behavior will be emulated exactly.
                        // However, it makes no practical sense to switch to bitwise because of such situations and thus slow down the emulation.
                        dskSyncCycle = agnus.clock;

                        if (diskState == DiskState::WAIT_SYNC_READ || diskState == DiskState::WAIT_SYNC_WRITE) {
                            if constexpr (waitTurbo) iterations = 1;
                            setDskState(diskState == DiskState::WAIT_SYNC_READ ? DiskState::READ : DiskState::WRITE);

                            if (diskState == DiskState::WRITE) {
                                dskShifterPos = 16;
                                break;
                            }
                        }

                        if (wordSync())
                            dskShifterPos = 15;
                    }

                    dskShifterPos++;
                    dskShifterPos &= 15;
                }
            } while((readWord || waitTurbo) && --iterations);
        } break;
        case DiskStructure::EXT:
        case DiskStructure::EXT2: {
            bool bit;
            bool _msb = msbSync();
            if constexpr (readWord) iterations = 16;
            else if constexpr (waitTurbo) iterations = 1 << turbo;

            do {
                if constexpr (readWord || waitTurbo) iterations--;
                bit = activeDrive->readBit(dmaCycles, !(readWord || waitTurbo));
                dskShifter <<= 1;
                dskShifter |= bit;

                if ( (dskShifterPos == 15) && dmaDisk && (diskState == DiskState::READ))
                    addToFifo(dskShifter);

                if (_msb) { // Apple GCR
                    // the MSB of each byte has to be a one, if not, "framing" is wrong and controller skips all zero bits.
                    if (((dskShifterPos & 7) == 0) && ((dskShifter & 1) == 0)) {
                        dskShifter >>= 1;
                        if (readWord && iterations) continue;
                        else break;
                    }
                }

                if ((dskShifterPos & 7) == 7) {
                    uint8_t byte = dskShifter & 0xff;
                    dskBytr = byte | 0x8000;
                }

                if ((dskShifter == dskSync) && !_msb) {
                    setDskSyncInt();
                    dskSyncCycle = agnus.clock;

                    if (diskState == DiskState::WAIT_SYNC_READ || diskState == DiskState::WAIT_SYNC_WRITE) {
                        setDskState(diskState == DiskState::WAIT_SYNC_READ ? DiskState::READ : DiskState::WRITE);

                        if (diskState == DiskState::WRITE) {
                            dskShifterPos = 16;
                            break;
                        }
                    }

                    if (wordSync())
                        dskShifterPos = 15;
                }

                dskShifterPos++;
                dskShifterPos &= 15;

            } while((readWord || waitTurbo) && iterations);
        } break;
    }
}

auto Paula::handleFDControllerWrite() -> void {
    // paula would send data to all connected and selected drives.
    // the controller can only write with two fixed speeds. copy protections recognize this by measuring time when reading back.
    // adjusting motor speed would result in different bit cell width too. (not emulated in ADF and EXT ADF ... simply not possible)

    if (!dmaDisk || !fifoReady)
        return;

    switch(activeDrive->structure.type) {
        case DiskStructure::Unknown:
        case DiskStructure::ADF: {
            uint8_t byte = 0;

            if (dskShifterPos != 16) {
                if (dskShifterPos == 0) {
                    byte = dskShifter >> 8;
                    if (dskLen & 0x8000)
                        dskBytr = 0x8000;
                } else {
                    byte = dskShifter & 0xff;
                }
            }

            if (disk0.connected)
                disk0.writeByte(byte);
            if (disk1.connected)
                disk1.writeByte(byte);
            if (disk2.connected)
                disk2.writeByte(byte);
            if (disk3.connected)
                disk3.writeByte(byte);

            if (dskShifterPos == 8) {
                if (dskTransferLength) {
                    if (!--dskTransferLength)
                        return finishDMA();
                }
            }

            if (dskShifterPos != 16)
                dskShifterPos += 8;
        } break;
        case DiskStructure::EXT:
        case DiskStructure::EXT2: {
            bool state = false;

            if (dskShifterPos != 16)
                state = dskShifter & (1 << (15 - dskShifterPos));

            if (disk0.connected)
                disk0.writeBit(state);
            if (disk1.connected)
                disk1.writeBit(state);
            if (disk2.connected)
                disk2.writeBit(state);
            if (disk3.connected)
                disk3.writeBit(state);

            if (((dskShifterPos & 7) == 7) && (dskLen & 0x8000))
                dskBytr = 0x8000;

            if (dskShifterPos != 16) {
                if (++dskShifterPos == 16) {
                    if (dskTransferLength) {
                        if (!--dskTransferLength)
                            return finishDMA();
                    }
                }
            }

        } break;
    }

    if (dskTransferLength && (dskShifterPos == 16)) {
        dskShifterPos = 0;

        if (!getFromFifo(dskShifter))
            dskShifter = 0;
    }
}

inline auto Paula::getFromFifo(uint16_t& data) -> bool {
    if (fifoPos < 1)
        return false; // underflow

    fifoPos -= 1;
    data = (fifo >> (fifoPos << 4)) & 0xffff;
    return true;
}

inline auto Paula::addToFifo(uint16_t data) -> void {
    if (fifoPos == 3)
        return; // overflow

    fifo = (fifo << 16) | data;
    fifoPos++;
    fifoReady = true;
}

auto Paula::setDskState(DiskState next) -> void {
    bool writeBefore = diskState == DiskState::WRITE || diskState == DiskState::WAIT_SYNC_WRITE;
    bool writeAfter = next == DiskState::WRITE || next == DiskState::WAIT_SYNC_WRITE;
    diskState = next;

    if (writeBefore != writeAfter) {
        if (disk0.connected) disk0.updateDeviceState();
        if (disk1.connected) disk1.updateDeviceState();
        if (disk2.connected) disk2.updateDeviceState();
        if (disk3.connected) disk3.updateDeviceState();
    }
}

auto Paula::processDiskIdleCycles() -> void {
    if (dskEventCycle < agnus.clock)
        return;

    int temp = (int)(dskEventCycle - agnus.clock);

    if (temp >= dmaCycles)
        temp = 0;
    else
        temp = dmaCycles - temp;

    activeDrive->rotate( temp, true );
}

}
