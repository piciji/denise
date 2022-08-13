
#include "cia.h"

template<> auto Cia::tod<MOS_6526>() -> void {
    if (!todActive)
        return;

    ++tickCounter &= 7;

    if (tickCounter != ((timerA.control & 0x80) ? 5 : 6))
        return;

    tickCounter = 0;

    uint8_t ts = (todc >> 0) & 0x0f; // tenth second
    uint8_t sL = (todc >> 8) & 0x0f; // seconds [x0-x9]
    uint8_t sH = (todc >> 12) & 0x0f; // seconds [0x-5x]
    uint8_t mL = (todc >> 16) & 0x0f; // minutes [x0-x9]
    uint8_t mH = (todc >> 20) & 0x0f; // minutes [0x-5x]
    uint8_t hL = (todc >> 24) & 0x0f; // hours [x1-x9]
    uint8_t hH = (todc >> 28) & 0x01; // hours [0x-1x]
    uint8_t pm = (todc >> 24) & 0x80;

    /* tenth seconds (0-9) */
    ts = (ts + 1) & 0x0f;
    if (ts == 10) {
        ts = 0;
        // seconds
        sL = (sL + 1) & 0x0f; // x0-x9
        if (sL == 10) {
            sL = 0;
            sH = (sH + 1) & 0x07; // 0x-5x
            if (sH == 6) {
                sH = 0;
                // minutes
                mL = (mL + 1) & 0x0f; // x0-x9
                if (mL == 10) {
                    mL = 0;
                    mH = (mH + 1) & 0x07; // 0x-5x
                    if (mH == 6) {
                        mH = 0;
                        // hours 1-12
                        hL = (hL + 1) & 0x0f;
                        if (hH) {
                            // 11 -> 12
                            if (hL == 2)
                                pm ^= 0x80;
                            // 12 -> 1
                            if (hL == 3) {
                                hL = 1;
                                hH = 0;
                            }

                        } else if (hL == 10) {
                            hL = 0;
                            hH = 1;
                        }
                    }
                }
            }
        }
    }

    todc = ts | (sL << 8) | (sH << 12) | (mL << 16) | (mH << 20) | (hL << 24) | (hH << 28) | (pm << 24);

    if (todc == alarm)
        intIncomming |= 4;
}

template<> auto Cia::tod<MOS_8520>() -> void {
    if (!todActive)
        return;

    if ((todc & 0xfff) == 0xfff) {
        if ((todc & ~0xfff) == alarm) //tod bug
            intIncomming |= 4;
    }

    todc++;
    todc &= 0xffffff;

    if (todc == alarm)
        intIncomming |= 4;
}
