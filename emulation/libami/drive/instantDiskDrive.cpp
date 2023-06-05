
#include "diskDrive.h"

namespace LIBAMI {

auto DiskDrive::instantWrite(unsigned words, uint16_t syncWord, bool needSync) -> uint8_t {
    // support for shifted sync words. could be happen in a ADF too, if track is written during emulation
    unsigned bytes = words << 1;
    uint8_t byte;
    uint16_t word;
    bool synced = !needSync;
    unsigned offset = headOffset >> 3;
    unsigned length = track->length;
    uint16_t shifter;
    int overflow = 0;
    int b;
    unsigned pos = 0;
    uint8_t out = 0;

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
        byte = track->data[offset];

        for(b = 7; b >= 0; b--) {
            shifter = (shifter << 1) | (byte & 0x80);
            byte <<= 1;

            if (shifter == syncWord) {
                out |= 1;
                synced = true;
                shifter = 0;
                pos = b; // bit wise
                if (pos) {
                    byte = track->data[offset] >> pos;
                    pos = 8 - pos;
                    goto Next;
                }
                break;
            }
        }

        if (++offset == length) {
            offset = 0;
            if (!synced) {
                if (++overflow == 2)
                    break; // there was no sync pattern found
            }
            cia.setFlag();
        }
    }

    Next:

    if (structure.writeProtected)
        return out | 2; // write is wasted, controller don't know

    do {
        word = agnus.fakeDiskDma();
        for(b = 15; b >= 0; b--) {
            byte = (byte << 1) | ((word >> b) & 1);

            if (++pos == 8) {
                pos = 0;
                track->data[offset] = byte;
                if (++offset == length) {
                    offset = 0;
                    cia.setFlag();
                }
                bytes--;
            }
        }
    } while (bytes);

    if (pos)
        track->data[offset] = (track->data[offset] & ~(0xff << (8 - pos))) | byte << (8 - pos);

    headOffset = offset << 3;
    written = true;
    track->written |= 1;
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
            cia.setFlag();

            if (structure.type == DiskStructure::EXT || structure.type == DiskStructure::EXT2) {
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
            cia.setFlag();

            if (structure.type == DiskStructure::EXT || structure.type == DiskStructure::EXT2) {
                bitLimit = (track->length << 3) - track->bits;
                if (bitLimit > 7) bitLimit = 0;
            }
        }

        if (!word) {
            if (rand) { // continous oscilations
                for (int i = 0; i < 16; i++)
                    word |= ((randomizer.xorShift() >> 16 ) & 1) << i;
            } else {
                word |= 0x155;
                rand = 1; // first oscilation
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
