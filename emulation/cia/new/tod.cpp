
#include "cia.h"

template<uint8_t model>
auto Cia<model>::tod() -> void {
    // calling this function means "rising edge"
    if (!todActive)
        return;

    if constexpr(model == MOS_6526) {
        // todo: calculation needs a lot of cycles
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

    } else {

        tickCounter = 1;
    }
}

template<uint8_t model>
auto Cia<model>::todIncrement() -> void {
    if constexpr(model == MOS_8520) {

        switch(tickCounter) {
            case 1:     // align to 4-th clock
                tickCounter = 2;
                break;
            case 2:     // 4 cycles
                if ((todc & 0xfff) == 0xfff) {
                    tickCounter = 3;
                    todc &= ~0xfff;
                } else {
                    tickCounter = 4;
                    todc++;
                    todc &= 0xffffff;
                }
                break;
            case 3:     // 8 cycles
                if (todc == alarm) //tod bug
                    intIncomming |= 4;

                tickCounter = 5;
                break;
            case 4:     // 8 cycles
                tickCounter = 6;
                break;

            case 5:     // 12 cycles
                todc += 1 << 12;
                todc &= 0xffffff;

                if (todc == alarm)
                    intIncomming |= 4;

                tickCounter = 0;
                break;

            case 6:     // 12 cycles
                if (todc == alarm)
                    intIncomming |= 4;

                tickCounter = 0;
                break;
        }

    } else {
        // todo
    }
}