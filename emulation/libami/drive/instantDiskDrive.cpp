
#include "diskDrive.h"

namespace LIBAMI {

auto DiskDrive::instantWrite(unsigned words, uint16_t syncWord, bool needSync) -> uint8_t {
    // support for shifted sync words. could be happen in a ADF too, if track is written during emulation
    int byte;
    int bit;
    bool synced = !needSync;
    int offset = headOffset;
    int length = track->length;
    int bits;
    int byteOffset;
    uint16_t word;
    uint8_t out = 0;
    uint16_t shifter = 0;
    int overflow = 0;
    int b;
    bool bitMode = structure.type == DiskStructure::EXT || structure.type == DiskStructure::EXT2 || structure.type == DiskStructure::IPF;

    if (bitMode) {
        bits = track->bits;
    } else {
        bits = length << 3;
        offset += 7;
        offset &= ~7;
    }

    if (stepSettleClock) {
        stepSettleClock = 0;
        step(nextStep, true);
    }

    if (!motor || !inserted || !selected) {
        if (needSync)
            return syncWord ? 0 : 3;
        return 2;
    }

    while(!synced) {
        byte = offset >> 3;
        bit = (~offset) & 7; // msb is next

        offset++;
        if ( offset >= bits ) {
            offset = 0;
            if (++overflow == 2)
                return out; // there was no sync pattern found

            cia.setFlag();
        }

        shifter = (shifter << 1) | ((track->data[byte] >> bit) & 1);

        if (shifter == syncWord) {
            out |= 1;
            synced = true;
            shifter = 0;
        }
    }

    if (structure.writeProtected)
        return out | 2; // write is wasted, controller don't know

    if ( (structure.type == DiskStructure::ADF) && ((offset & 7) == 0) ) { // speedup
        byteOffset = offset >> 3;
        do {
            word = agnus.fakeDiskDma();

            track->data[byteOffset] = word >> 8;
            if (++byteOffset == length) {
                byteOffset = 0;
                cia.setFlag();
            }

            track->data[byteOffset] = word & 0xff;
            if (++byteOffset == length) {
                byteOffset = 0;
                cia.setFlag();
            }
        } while (--words);
        offset = byteOffset << 3;

    } else {
        do {
            word = agnus.fakeDiskDma();

            for(b = 15; b >= 0; b--) {
                byte = offset >> 3;
                bit = (~offset) & 7; // msb is next

                offset++;
                if ( offset >= bits ) {
                    offset = 0;
                    cia.setFlag();
                }

                if (((word >> b) & 1))
                    track->data[byte] |= 1 << bit;
                else
                    track->data[byte] &= ~(1 << bit);
            }
        } while (--words);
    }

    headOffset = offset;
    written = true;
    track->options |= 1;
    return out | 2;
}

auto DiskDrive::instantRead(unsigned words, uint16_t syncWord, bool needSync) -> uint8_t {
    uint16_t word;
    bool synced = !needSync;
    unsigned offset = headOffset >> 3;
    unsigned length = track->length;
    uint16_t shifter;
    int overflow = 0;
    int b;
    unsigned pos = 0;
    uint8_t out = 0;
    bool rand = false;
    int bitLimit;
    bool bitMode = structure.type == DiskStructure::EXT || structure.type == DiskStructure::EXT2 || structure.type == DiskStructure::IPF;

    if (stepSettleClock) {
        stepSettleClock = 0;
        step(nextStep, true);
    }

    if (!motor || !inserted || !selected) {
        if (syncWord && needSync)
            return 0;

        do {
            offset += 2;
            if (offset >= length) {
                offset -= length;
                cia.setFlag();
            }

            agnus.fakeDiskDmaNoTracking(0);
        } while (--words);

        return !syncWord ? 3 : 2;
    }

    do {
        bitLimit = 0;
        word = track->data[offset] << 8;
        if (++offset == length) {
            offset = 0;
            if (!synced) {
                if (++overflow == 2)
                    break; // there was no sync pattern found
            }
            structure.loadNextRevIPF(*track);
            cia.setFlag();

            if (bitMode) {
                bitLimit = (track->length << 3) - track->bits;
                if (bitLimit > 7) bitLimit = 0;
                word &= 0xff00 << bitLimit;
            }
        }
        word |= track->data[offset] << bitLimit;
        if (++offset == length) {
            offset = 0;
            if (!synced) {
                if (++overflow == 2)
                    break;
            }
            structure.loadNextRevIPF(*track);
            cia.setFlag();

            if (bitMode) {
                bitLimit = (track->length << 3) - track->bits;
                if (bitLimit > 7) bitLimit = 0;
            }
        }

        if (!word) {
            if (rand) { // continuous oscillations
                for (int i = 0; i < 16; i++)
                    word |= ((randomizer.xorShift() >> 16 ) & 1) << i;
            } else {
                word |= 0x155;
                rand = 1; // first oscillation
            }
        } else
            rand = 0;

        for(b = 15; b >= bitLimit; b--) {
            shifter = (shifter << 1) | ((word >> b) & 1);
            if ((pos == 15) && synced) {
                agnus.fakeDiskDmaNoTracking(shifter);
                words--;
            }

            if (shifter == syncWord) {
                out |= 1;
                if (!synced)
                    synced = true;

                if(needSync)
                    pos = 15;
            }

            pos++;
            pos &= 15;
        }
    } while (words);

    headOffset = offset << 3; // increase compatibility, e.g. Licence to kill

    return out | (words == 0 ? 2 : 0);
}

}
