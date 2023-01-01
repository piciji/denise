
// Floppy Disc Controller

// Paula communicates with the drives via 3 lines. Read, Write, write enable
// Each change in the magnetic flux causes an edge on the reading pin.
// Paula now measures the elapsed time to determine whether the bit sequence 101, 1001 or 10001 was received.
// Paula knows the Agnus beat. (the drive doesn't) Thus, the measured times are multiples of ~280 ns (PAL).
// Prolonged sequences without magnetization, such as 100001, are dangerous because they can cause random flux changes (not on disc).
// Therefore, these sequences are not valid GCR or MFM (weak bits) and the reason why GCR/MFM encoding is needed at all.
//
// Paula doesn't know which drives are connected or selected. There is simply no data coming from/going to these drives.
// However, the emulation distinguishes 2 approaches in terms of accuracy for ADF and EXT ADF respectively. So we need to know it in context of FDC.
// If more than one drive is selected, there are too many edges at the read line and no meaningful data stream can be determined.
// Write operations are possible simultaneously to multiple drives. Reading multiple drives at the same time does not lead to a meaningful result.
//
// For the sake of simplicity, the rotation of the floppy disk is driven here. In reality, the drive does not know the Agnus clock
// and rotates at 300 RPM. Depending on the length of the bit cells on the floppy disk, a certain number of flux changes become visible to Paula.
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
// a sophisticated device for generating original disks can write bits with a variable bitcell
// width, so timing can change each bitcell. The amiga drives can only write a fixed bitcell width, see above
// e.g. PAL: 1.97356 us or 3.94712 us (if slow mode is selected in controller)

// precompensation is uninteresting for emulation, beacuse it doesn't change bitcell width but
// advance or delays the flux transition within the bit cell in order to optimize the distance
// between 2 adjacent transitions

// Since ADF's do not contain copy protections, it is sufficient to transfer them byte by byte, especially in terms of emulation speed.

// In contrast to the ADF, EXT ADF does not contain the user data, but the encoded data (GCR or MFM).
// Basically, these are not flux change data, but can be interpreted as such and thus allow the handling of various copy protections.
// This is done by the fact that due to the track bit lengths given in the header of the file, the bit cell width can be determined,
// which may well deviate from the standard bit cell width, see ADF. The emulation takes this into account and allows
// the duration of the bit cells determined in this way to elapse before the next bit can be read.
// The bit cell width is manipulated by setting additional bits in the GAP area of the disc.
// Copy protections that test the sectors for a certain reading time (different from the fixed one, that the drive can write)
// can be successfully operated with this.
// However, the EXT format does not contain any information about reconstructing variable bit cell widths within a track.
// This requires a real flux change format.
// Another limitation of the EXT format is writing to tracks that do not have a standard bit cell width.
// The emulation will no longer change the previously defined bit cell width when writing the track.
// This is technically complex/impossible, since a track can also be partially written.
// Then the track would have to be shortened to the standard bit cell width, which could lead to data loss.
// In this case, it is better to leave the current number of bits. Practically, this should not be a problem,
// since newly created EXT ADF discs already have the standard bit cell width and games with copy protections write savegames
// to empty or standard length tracks. Even if not, reading the data would not fail, since only the reading time varies.
// a real flux change format doesn't have this limitation. you can write tracks partially with a different bitcell width.

// The controller has no idea if it is an HD or DD disk. High density is only achieved by halving the rotational speed.
// This happens automatically when the drive detects an HD disk.
// Halving the speed means writing double amount of data on same surface.
// Reading needs halving the speed too, otherwise the Controller (DMA) could not handle the double amount of data.

#include "paula.h"
#include "../drive/diskDrive.h"

namespace LIBAMI {

auto Paula::getDskBytR() -> uint16_t {
    uint16_t out = fifo & 0xff;

    if (fifoWritten) out |= 0x8000;
    if ((dskLen & 0x8000) && dmaDisk) out |= 0x4000;
    if (dskLen & 0x4000) out |= 0x2000;
    if (dskSyncCycle) {
        if((agnus.clock - dskSyncCycle) < (!fast() ? 15 : 8) ) out |= 0x1000;
        else dskSyncCycle = 0;
    }

    fifoWritten = false;
    return out;
}

auto Paula::setDskLen(uint16_t value) -> void {
    uint16_t oldValue = dskLen;
    dskTansferLength = value & 0x3fff;
    dskLen = value;

    if (value & oldValue & 0x8000) {
        if (dskTansferLength == 0) {
            setDskBlkInt();
            setDskState(DiskState::OFF);
            dskEventCycle = 0;
            return;
        }

        if (value & oldValue & 0x4000)
            setDskState(DiskState::WRITE);
        else
            setDskState(wordSync() ? DiskState::WAIT_SYNC : DiskState::READ);

        setFdcEvent();
    } else if (!(value & 0x8000)) {
        setDskState(DiskState::OFF);
        dskTansferLength = 0;
        dskEventCycle = 0;
    } else
        return;

    resetFifo();
}

auto Paula::setFdcEvent() -> void {
    dmaCycles = 0;
    uint8_t state = 0;

    if (disk0.connected && disk0.selected)
        state |= (disk0.structure.type == DiskStructure::EXT) ? 2 : 1;
    else if (disk1.connected && disk1.selected) {
        if (!state || (diskState == DiskState::WRITE))
            state |= (disk1.structure.type == DiskStructure::EXT) ? 2 : 1;
    } else if (disk2.connected && disk2.selected) {
        if (!state || (diskState == DiskState::WRITE))
            state |= (disk2.structure.type == DiskStructure::EXT) ? 2 : 1;
    } else if (disk3.connected && disk3.selected) {
        if (!state || (diskState == DiskState::WRITE))
            state |= (disk3.structure.type == DiskStructure::EXT) ? 2 : 1;
    }

    if (state & 2) dmaCycles = diskState == DiskState::WRITE ? 7 : 8;
    else if (state & 1) dmaCycles = 7 * 8;

    if (!fast() && (state & 2)) dmaCycles <<= 1;

    dskEventCycle = dmaCycles ? (agnus.clock + dmaCycles) : 0;
    pulseWidth = 0;
}

auto Paula::setDskSync(uint16_t value) -> void {
    dskSync = value;
}

auto Paula::setDskDat(uint16_t value) -> void {
    addToFifo(value);
}

auto Paula::dskDatR() -> uint16_t {
    uint16_t out = getFromFifo();

    if (dskTansferLength) {
        if (!--dskTansferLength) {
            setDskBlkInt();
            setDskState(DiskState::OFF);
        }
    }
    return out;
}

auto Paula::handleFDControllerRead(DiskDrive& disk) -> void {
    if (disk.structure.type == DiskStructure::ADF) {
        if (dmaCycles != (7 * 8)) {
            // Basically, it is possible to change the selected drive while reading.
            // This, of course, will not lead to any meaningful result. (for consistency only)
            dmaCycles = 7 * 8;
            dskShifterPos = 0;
        }
        // For ADF it is sufficient to read byte by byte. There is no reason to waste performance here.
        uint8_t data = disk.readADF();
        for(int i = 7; i >= 0; i--) {
            dskShifter = (dskShifter << 1) | ((data >> i) & 1);
            if ((dskShifter & 0xf) == 0)
                // weak bits: more than 3 zeros in a row could cause random flux
                // like EXT handling this should happen on drive side, not controller.
                // can only happen, if track is written later on
                dskShifter |= 1;

            if (dskShifter == dskSync) {
                setDskSyncInt();
                dskSyncCycle = agnus.clock;

                if (diskState == DiskState::WAIT_SYNC) {
                    setDskState(DiskState::READ);
                    resetFifo();
                    return;
                }
            }
        }
        dskShifterPos += 8;
        if (dskShifterPos == 16) {
            addToFifo(dskShifter);
            dskShifterPos = 0;
        }
        return;
    }
    // ext adf handling (GCR or MFM stream)
    if (dmaCycles == (7 * 8)) {
        // same as above (for consistency only)
        dmaCycles = fast() ? 8 : 16;
        pulseWidth = 0;
    }

    if (disk.readEXT(dmaCycles)) {
        if (pulseWidth & 1) {
            // fuzzy bits (todo check ranges)
            // The controller can only determine the correct number of zeros between two flux changes,
            // if the bit cell width does not deviate too far from the standard. Some copy protections take advantage of this by writing flux changes
            // at a distance of 5 us. The controller reads this in as episode 101 or as episode 1001. The controller itself is purely digital
            // and does not cause fluctuations but the drive motor does. If the data is written regularly, i.e. with about 4 us,
            // the controller always reads back the same sequence and a copy can thus be recognized.
            if (rand() & 1) addBit(false);

            addBit(true);
        } else if (pulseWidth == 0) {
            // too tight, flux not accepted
            addBit(false);
        } else
            addBit(true);

        dmaCycles = (fast() ? 8 : 16) - dmaCycles;
        pulseWidth = 0;
    } else {
        // for simplicity all example calculations are done with 280 ns per DMA cycle. emulation considers PAL and NTSC timing for reading/writing
        switch(pulseWidth++) {
            case 0: dmaCycles = 4; break;                           // 2.240 + 1.120 = 3.360
            case 1: dmaCycles = 4; addBit(false); break;            // 3.360 + 1.120 = 4.480    // 10x
                // fuzzy bits (4 or 6 micro)
            case 2: dmaCycles = 3; break;                           // 4.480 + 0.840 = 5.320
            case 3: dmaCycles = 4; addBit(false); break;            // 5.320 + 1.120 = 6.440    // 100x
                // fuzzy bits (6 or 8 micro)
            case 4: dmaCycles = 3; break;                           // 6.440 + 0.840 = 7.280
            case 5: dmaCycles = 4; addBit(false); break;            // 7.280 + 1.120 = 8.400    // 1000x
                // following cases missing magnetization for a long period
            case 6: dmaCycles = 3;                                  // 8.400 + 0.840 = 9.240
                // random amount of zeros until oscilation (flux transition not on disc)
                if (rand() & 1) pulseWidth = 9;
                break;
            case 7: dmaCycles = 4; addBit(false); break;            // 9.240 + 1.120 = 10.360
            case 8: dmaCycles = 3; break;                           // 10.360 + 0.840 = 11.200
            case 9: dmaCycles = 4; addBit(false); break;            // 11.200 + 1.120 = 12.320
            case 10: dmaCycles = 3;                                 // 12.320 + 0.840 = 13.160
                if (rand() & 1) pulseWidth = 13;
                break;
            case 11: dmaCycles = 4; addBit(false); break;           // 13.160 + 1.120 = 14.280
            case 12: dmaCycles = 3; break;                          // 14.280 + 0.840 = 15.120
            case 13: dmaCycles = 4;                                 // 15.120 + 1.120 = 16.240
                // random flux transition (weak bits)
                // this is not entirely true. it is not the controller that produces the random flux transitions, but the drive.
                // we handle it here for performance and complexity reasons
                addBit(true);
                pulseWidth = 0;
                break;
        }
        if (!fast()) {
            dmaCycles <<= 1;
            if (pulseWidth & 1) dmaCycles += 1;
        }
    }
}

auto Paula::handleFDControllerWrite() -> void {
    if (dmaCycles == 7 || dmaCycles == 14) { // EXT or ADF/EXT mixed
        if (dskShifterPos == 0)
            dskShifter = getFromFifo();

        // paula would send data to all connected drives, because it doesn't know which ones are selected.
        // we check this for performance reasons.
        // the controller can only write with two fixed speeds. copy protections recognize this by measuring time when reading back.
        if (disk0.connected && disk0.selected) {
            if (disk0.structure.type == DiskStructure::EXT) disk0.writeEXT(dmaCycles, dskShifter & (1 << (15 - dskShifterPos)));
            else if (dskShifterPos == 7)    disk0.writeADF(dskShifter >> 8);
            else if (dskShifterPos == 15)   disk0.writeADF(dskShifter & 0xff);
        } if (disk1.connected && disk1.selected) {
            if (disk1.structure.type == DiskStructure::EXT) disk1.writeEXT(dmaCycles, dskShifter & (1 << (15 - dskShifterPos)));
            else if (dskShifterPos == 7)    disk1.writeADF(dskShifter >> 8);
            else if (dskShifterPos == 15)   disk1.writeADF(dskShifter & 0xff);
        } if (disk2.connected && disk2.selected) {
            if (disk2.structure.type == DiskStructure::EXT) disk2.writeEXT(dmaCycles, dskShifter & (1 << (15 - dskShifterPos)));
            else if (dskShifterPos == 7)    disk2.writeADF(dskShifter >> 8);
            else if (dskShifterPos == 15)   disk2.writeADF(dskShifter & 0xff);
        } if (disk3.connected && disk3.selected) {
            if (disk3.structure.type == DiskStructure::EXT) disk3.writeEXT(dmaCycles, dskShifter & (1 << (15 - dskShifterPos)));
            else if (dskShifterPos == 7)    disk3.writeADF(dskShifter >> 8);
            else if (dskShifterPos == 15)   disk3.writeADF(dskShifter & 0xff);
        }

        if (++dskShifterPos == 16) {
            dskShifterPos = 0;
            if (dskTansferLength) {
                if (!--dskTansferLength) {
                    setDskBlkInt();
                    setDskState(DiskState::OFF);
                }
            }
        }
    } else { // ADF only
        // only if all writing devices use ADF discs
        uint8_t data;
        if (dskShifterPos == 0) {
            dskShifter = getFromFifo();
            data = dskShifter >> 8;
            dskShifterPos = 8;
        } else { // == 8
            data = dskShifter & 0xff;
            dskShifterPos = 0;
            if (dskTansferLength) {
                if (!--dskTansferLength) {
                    setDskBlkInt();
                    setDskState(DiskState::OFF);
                }
            }
        }
        if (disk0.connected && disk0.selected) {
            // if an EXT disc is selected while writing, but at the start time all discs were ADF,
            // it is now switched to bitwise. This certainly does not lead to any meaningful behavior even on a real device,
            // but must be taken into account for consistency reasons.
            if (disk0.structure.type == DiskStructure::EXT) dmaCycles = fast() ? 7 : 14;
            else disk0.writeADF(data);
        }
        if (disk1.connected && disk1.selected) {
            if (disk1.structure.type == DiskStructure::EXT) dmaCycles = fast() ? 7 : 14;
            else disk1.writeADF(data);
        }
        if (disk2.connected && disk2.selected) {
            if (disk2.structure.type == DiskStructure::EXT) dmaCycles = fast() ? 7 : 14;
            else disk2.writeADF(data);
        }
        if (disk3.connected && disk3.selected) {
            if (disk3.structure.type == DiskStructure::EXT) dmaCycles = fast() ? 7 : 14;
            else disk3.writeADF(data);
        }
    }
}

auto Paula::addBit(bool bit) -> void {
    dskShifter <<= 1;
    dskShifter |= bit;

    if (msbSync()) { // Apple GCR
        // the MSB of each byte has to be a one, if not, "framing" is wrong and controller skips all zero bits.
        if (((dskShifterPos & 7) == 0) && ((dskShifter & 1) == 0)) {
            dskShifter >>= 1;
            return;
        }
    } else if (dskShifter == dskSync) {
        setDskSyncInt();
        dskSyncCycle = agnus.clock;

        if (diskState == DiskState::WAIT_SYNC) {
            setDskState(DiskState::READ);
            resetFifo();
            return;
        }
    }

    if (++dskShifterPos == 16) {
        dskShifterPos = 0;
        addToFifo(dskShifter);
    }
}

auto Paula::getFromFifo() -> uint16_t {
    if (fifoPos < 1)
        return 0; // underflow

    fifoPos -= 1;
    return (fifo >> (fifoPos << 4)) & 0xffff;
}

auto Paula::addToFifo(uint16_t data) -> bool {
    if (fifoPos == 3)
        return false; // overflow

    fifo = (fifo << 16) | data;
    fifoPos++;
    fifoWritten = true;
    return true;
}

inline auto Paula::resetFifo() -> void {
    fifoPos = 0;
    dskShifterPos = 0;
    dskSyncCycle = 0;
}

auto Paula::setDskState(DiskState next) -> void {
    bool upd = false;
    if ((diskState != DiskState::WRITE) && (next == DiskState::WRITE))
        upd = true;
    else if ((diskState == DiskState::WRITE) && (next != DiskState::WRITE))
        upd = true;

    if (upd) {
        if (disk0.connected) disk0.updateDeviceState();
        if (disk1.connected) disk1.updateDeviceState();
        if (disk2.connected) disk2.updateDeviceState();
        if (disk3.connected) disk3.updateDeviceState();
    }

    diskState = next;
}

}
