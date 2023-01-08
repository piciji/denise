
#include "diskDrive.h"

namespace LIBAMI {

auto DiskDrive::instantWrite(unsigned words, uint16_t syncWord, bool needSync) -> uint8_t {
    // support for shifted sync words. could be happen in a ADF too, if track is written during emulation
    unsigned bytes = words << 1;
    uint8_t byte;
    uint16_t word;
    bool synced = !needSync;
    unsigned offset = 0;
    unsigned length = track->length;
    uint16_t shifter;
    bool overflow = false;
    int b;
    unsigned pos = 0;
    uint8_t out = 0;

    if (stepSettleClock) {
        stepSettleClock = 0;
        step(nextStep, true);
    }

    if (!motor || !inserted) {
        if (needSync)
            return syncWord ? 0 : 3;
        return 2;
    }

    while(!synced) {
        byte = track->data[offset];

        for(b = 7; b >= 0; b--) {
            shifter = (shifter << 1) | ((byte >> b) & 1);
            if (shifter == syncWord) {
                out |= 1;
                synced = true;
                shifter = 0;
                pos = b; // bit wise
                if (pos) {
                    byte = track->data[offset] >> pos;
                    goto Next;
                }
                break;
            }
        }

        if (++offset == length) {
            offset = 0;
            overflow = true;
            cia.setFlag();
        } else if (overflow && (offset > 1))
            return out; // looks for sync pattern forever, no dskblk intr
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

    written = true;
    track->written |= 1;
    return out | 2;
}

auto DiskDrive::instantRead(unsigned words, uint16_t syncWord, bool needSync) -> uint8_t {
    uint16_t word;
    bool synced = !needSync;
    unsigned offset = 0;
    unsigned length = track->length;
    uint16_t shifter;
    bool overflow = false;
    int b;
    unsigned pos = 0;
    uint8_t out = 0;

    if (stepSettleClock) {
        stepSettleClock = 0;
        step(nextStep, true);
    }

    if (!motor || !inserted) {
        if (needSync)
            return syncWord ? 0 : 3;
        return 2;
    }

    do {
        word = track->data[offset] << 8;
        if (++offset == length) {
            offset = 0;
            if (!synced) overflow = true;
            cia.setFlag();
        }
        word |= track->data[offset];
        if (++offset == length) {
            offset = 0;
            if (!synced) overflow = true;
            cia.setFlag();
        }

        for(b = 15; b >= 0; b--) {
            shifter = (shifter << 1) | ((word >> b) & 1);
            if (shifter == syncWord) {
                out |= 1;
                synced = true;
                pos = 0;
                continue;
            }
            if (++pos == 16) {
                pos = 0;
                if (synced) {
                    agnus.fakeDiskDmaNoTracking(shifter);
                    words--;
                }
            }
        }
        if (overflow && (offset > 1))
            return out; // looks for sync pattern forever, no dskblk intr

    } while (words);

    return out | 2;
}

}
