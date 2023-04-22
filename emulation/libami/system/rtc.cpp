
#include "rtc.h"
#include "../agnus/agnus.h"
#include "../../tools/serializer.h"

namespace LIBAMI {

auto RTC::reset(bool softReset) -> void {

    if (!softReset) {
        regs[0xd] = 1; // hold
        regs[0xe] = 0;
        regs[0xf] = 4;
    }

    // todo ... usefull ?
    // Normally, "diff" would have to be saved permanently and fed back in after emulator restart.
    // In this way, a deviating time (for whatever reason) can be held permanently.
}

auto RTC::write(uint8_t adr, uint8_t val) -> void {
    switch(adr) {
        case 0xd: regs[ adr & 0xf ] = val & (1 | 8); break;
        case 0xe: regs[ adr & 0xf ] = val & 0xf; break;
        case 0xf: regs[ adr & 0xf ] = val & 0xf; break;
        default:
            updateRegs();
            regs[ adr & 0xf ] = val & 0xf;
            calcDiff();
            break;
    }
}

auto RTC::read(uint8_t adr, bool update) -> uint8_t {
    if (update)
        updateRegs(); // only in RA processing frame
    return regs[ adr & 0xf ];
}

auto RTC::chipTime() -> time_t {
    time_t curTime = time(nullptr);
    int64_t clock = agnus.clock;

    // short intervals caused by warp could irritate the software. we do not measure real time, but calculate it from past cycles.
    if ((curTime - lastTime) < 5)
        curTime = lastTime + agnus.dmaCyclesToSec(clock - lastClock);
    else {
        lastTime = curTime;
        lastClock = clock;
    }

    return curTime + diff;
}

auto RTC::updateRegs() -> void {
    time_t time = chipTime();
    std::tm t;
    localtime_r(&time, &t);

    regs[0x0] = t.tm_sec % 10;
    regs[0x1] = t.tm_sec / 10;
    regs[0x2] = t.tm_min % 10;
    regs[0x3] = t.tm_min / 10;
    regs[0x4] = t.tm_hour % 10;
    regs[0x5] = t.tm_hour / 10;

    if ( (t.tm_hour >= 12) && ((regs[0xf] & 4) == 0)) {
        regs[4] = (t.tm_hour - 12) % 10;
        regs[5] = (t.tm_hour - 12) / 10;
        regs[5] |= 4; // AM/PM
    }

    regs[0x6] = t.tm_mday % 10;
    regs[0x7] = t.tm_mday / 10;
    regs[0x8] = (t.tm_mon + 1) % 10;
    regs[0x9] = (t.tm_mon + 1) / 10;
    regs[0xa] = t.tm_year % 10;
    regs[0xb] = t.tm_year / 10;
    regs[0xc] = t.tm_wday;
}

auto RTC::calcDiff() -> void {
    std::tm t;

    t.tm_sec  = regs[0x0] + 10 * regs[0x1];
    t.tm_min  = regs[0x2] + 10 * regs[0x3];
    t.tm_hour = regs[0x4] + 10 * regs[0x5];
    t.tm_mday = regs[0x6] + 10 * regs[0x7];
    t.tm_mon  = regs[0x8] + 10 * regs[0x9] - 1;
    t.tm_year = regs[0xa] + 10 * regs[0xb];

    time_t rtcTime = mktime(&t);
    diff = rtcTime - time(nullptr);
}

auto RTC::serialize(Emulator::Serializer& s) -> void {
    s.array( regs );
    s.integer( lastTime );
    s.integer( lastClock );
    s.integer( diff );
}

}
