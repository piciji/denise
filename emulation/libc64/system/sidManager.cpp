
#include "sidManager.h"
#include "system.h"
#include "../sid/clone.cpp"

namespace LIBC64 {

SidManager::SidManager(LIBC64::System* system) : system(system) {
    for (unsigned i = 0; i < 7; i++)
        sids[i] = new Sid( system, *this, Sid::Type::MOS_6581 );

    sid = new Sid( system, *this, Sid::Type::MOS_6581 );

    sampleCounter = 0;
    sampleLimit = 2;
    audioOut = true;
    useExternalFilter = true;
    serializationSizeForSevenMoreSids = 0;
    useVolumeCorrection = false;
    sysClock = 0;
    potX = 0xff;
    potY = 0xff;

    getPotX = []() { return 0xff; };
    getPotY = []() { return 0xff; };

    callPotUpdate = []() { };
    callAlarm = [this]() {
        updateClock();
    };

    audioOut = true;
    extraSids = false;
    leftSids = 0;
    rightSids = 0;
}

auto SidManager::setIoMask(int nr, uint8_t pos) -> void {
    if (nr == 0)
        sid->setIoMask(pos);
    else
        sids[nr-1]->setIoMask(pos);
}

auto SidManager::getIoPos(int nr) -> int {
    if (nr == 0)
        return sid->ioPos;

    return sids[nr-1]->ioPos;
}

auto SidManager::updateClock() -> void {
    int options = audioOut | (useExternalFilter << 1);
    if (sid->filterType == Sid::FilterType::Chamberlin) // same filter for all SID's
        options |= 4;

    switch(options) {
        case 0: updateClockT<0>(); break;
        case 1: updateClockT<1>(); break;
        case 2: updateClockT<2>(); break;
        case 3: updateClockT<3>(); break;
        case 4: updateClockT<4>(); break;
        case 5: updateClockT<5>(); break;
        case 6: updateClockT<6>(); break;
        case 7: updateClockT<7>(); break;
    }
}

template<int options> inline auto SidManager::updateClockT() -> void {
    system->sysTimer.add( &callAlarm, 200, Emulator::SystemTimer::Action::UpdateExisting );

    int _delay = system->sysTimer.fallBackCycles( sysClock );

    if (!_delay)
        return;

    if (extraSids)
        clockMultiChips<options>(_delay);
    else
        sampleCounter = sid->clock<options>(_delay, sampleCounter, sampleLimit);

    sysClock = system->sysTimer.clock;
}

auto SidManager::registerCallbacks() -> void {

    system->sysTimer.registerCallback( { { &callPotUpdate, 1 }, { &callAlarm, 1 } } );
}


auto SidManager::disableAudioOut(bool state) -> void {
//    if (moreAccuracy && registerWrite.pipelined) {
//        // wait for worker thread
//        while (ready.load()) {}
//        applyFilterWrite();
//    }

    audioOut = !state;
}

template<int options> auto SidManager::clockMultiChips(int cycles) -> void {
    constexpr bool _audioOut = options & 1;
    //constexpr bool _useExtFilter = options & 2;
    //constexpr bool _useChamberlain = options & 4;

    double sampleLeft, sampleRight;
    const int _limit = sampleLimit;

    for (int c = 0; c < cycles; c++) {
        sampleLeft = sampleRight = 0.0;

        if (_audioOut && (++sampleCounter == _limit)) {

            sampleCounter = 0;

            for (auto useSid : useSids) {

                useSid->clock<options | 8>();

                if (useSid->leftChannel)
                    sampleLeft += useSid->curSample;

                if (useSid->rightChannel)
                    sampleRight += useSid->curSample;
            }

            if (!leftSids) {
                sampleRight /= rightSids;
                sampleLeft = sampleRight;
                system->audioRefresh(Emulator::sclamp(16, sampleLeft));

            } else if (!rightSids) {
                sampleLeft /= leftSids;
                sampleRight = sampleLeft;
                system->audioRefresh(Emulator::sclamp(16, sampleLeft));

            } else {
                sampleLeft /= leftSids;
                sampleRight /= rightSids;
                system->audioRefreshStereo(Emulator::sclamp(16, sampleLeft), Emulator::sclamp(16, sampleRight));
            }

        } else {
            for (auto useSid : useSids)
                useSid->clock<options>();
        }
    }
}

auto SidManager::getSidByAdr(uint16_t addr, bool ioArea) -> Sid* {

    addr &= 0xffe0;

    for (auto useSid : useSids) {
        if (addr == useSid->ioMask)
            return useSid;
    }

    return ioArea ? nullptr : sid;
}

auto SidManager::writeSid(uint16_t addr, uint8_t value) -> void {

    uint16_t _addr = addr & 0xffe0;
    bool match = false;

    for (auto useSid : useSids) {
        if ( _addr == useSid->ioMask ) {
            useSid->writeIO(addr, value);
            match = true;
        }
    }

    if (match)
        return;

    for (auto useSid : useSids) {
        if (!useSid->ioMask) {
            useSid->writeIO(addr, value);
            match = true;
        }
    }

    if (!match)
        sid->writeIO( addr, value );
}

auto SidManager::writeSidIO(uint16_t addr, uint8_t value) -> void {

    uint16_t _addr = addr & 0xffe0;

    for (auto useSid : useSids) {
        if ( _addr == useSid->ioMask)
            useSid->writeIO( addr, value );
    }
}

auto SidManager::updateSidUsage() -> void {

    leftSids = rightSids = 0.0;
    useSids.clear();

    extraSids = system->requestedSids > 0;

    if (!extraSids) {
        system->updateStatsStereo();
        return;
    }

    if (sid->leftChannel || sid->rightChannel)
        useSids.push_back(sid);

    if (sid->leftChannel)
        leftSids += 1.0;

    if (sid->rightChannel)
        rightSids += 1.0;

    for (int i = 0; i < system->requestedSids; i++) {

        Sid* extraSid = sids[i];

        if (extraSid->leftChannel || extraSid->rightChannel)
            useSids.push_back( extraSid );

        if (extraSid->leftChannel)
            leftSids += 1.0;

        if (extraSid->rightChannel)
            rightSids += 1.0;
    }

    extraSids = !useSids.empty();

    system->updateStatsStereo();
}

auto SidManager::isStereo() -> bool {

    if (!extraSids)
        return false;

    if (!leftSids || !rightSids)
        return false;

    for (auto useSid : useSids) {

        if (!useSid->leftChannel || !useSid->rightChannel)
            return true;
    }

    return false;
}

auto SidManager::setEnableFilterAll( bool state ) -> void {
    sid->filter.setEnable( state );

    for (unsigned i = 0; i < 7; i++)
        sids[i]->filter.setEnable( state );
}

auto SidManager::isEnableFilter( ) -> bool {
    return sid->filter.enabled;
}

auto SidManager::setFilterTypeAll( Sid::FilterType filterType ) -> void {
    sid->setFilterType(filterType);
    sid->volumeCorrection(useVolumeCorrection);

    for (unsigned i = 0; i < 7; i++) {
        sids[i]->setFilterType(filterType);
        sids[i]->volumeCorrection(useVolumeCorrection);
    }
}

auto SidManager::getFilterType( ) -> Sid::FilterType {
    return sid->filterType;
}

auto SidManager::setFilterVolumeCorrection( bool state ) -> void {
    useVolumeCorrection = state;

    sid->volumeCorrection(state);

    for (int i = 0; i < 7; i++)
        sids[i]->volumeCorrection(state);
}

auto SidManager::setType( int nr, Sid::Type type ) -> void {
    if (nr == 0) {
        sid->setType(type);
        sid->volumeCorrection(useVolumeCorrection);
    } else {
        sids[nr-1]->setType( type );
        sids[nr-1]->volumeCorrection( useVolumeCorrection );
    }
}

auto SidManager::getType(int nr) -> Sid::Type {
    if (nr == 0)
        return sid->type;

    return sids[nr-1]->type;
}

auto SidManager::updateChamberlinFrequencyAll(double sampleRate) -> void {

    sid->chamberlinFilter.updateFrequency( sampleRate );

    for (unsigned i = 0; i < 7; i++)
        sids[i]->chamberlinFilter.updateFrequency( sampleRate );
}

auto SidManager::adjustFilterBias6581All(int value) -> void {
    sid->filter.adjustFilterBias6581( value );

    for (unsigned i = 0; i < 7; i++)
        sids[i]->filter.adjustFilterBias6581( value );
}

auto SidManager::getFilterBias6581() -> int {
    return sid->filter.bias6581;
}

auto SidManager::adjustFilterBias8580All(int value) -> void {
    sid->filter.adjustFilterBias8580( value );

    for (unsigned i = 0; i < 7; i++)
        sids[i]->filter.adjustFilterBias8580( value );
}

auto SidManager::getFilterBias8580() -> int {
    return sid->filter.bias8580;
}

auto SidManager::setDigiBoostAll( bool state ) -> void {
    sid->setDigiBoost( state );

    for (unsigned i = 0; i < 7; i++)
        sids[i]->setDigiBoost( state );
}

auto SidManager::getDigiBoost( ) -> bool {
    return sid->filter.digiBoost;
}

auto SidManager::resetAll() -> void {
    sysClock = 0;
    potX = potY = 0xff;
    system->sysTimer.add( &callAlarm, 300, Emulator::SystemTimer::Action::UpdateExisting );

    sid->reset();

    for (int i = 0; i < 7; i++)
        sids[i]->reset();

    sampleCounter = 0;
}

auto SidManager::calcSerializationSizeForSevenMoreSids() -> void {

    Emulator::Serializer s;

    sid->serialize( s, false );

    serializationSizeForSevenMoreSids = s.size() * 7;
}

auto SidManager::useLeftChannel(int nr, bool state) -> void {
    if (nr == 0)
        sid->useLeftChannel(state);
    else
        sids[nr-1]->useLeftChannel(state);

    updateSidUsage();
}

auto SidManager::hasLeftChannel(int nr) -> bool {
    if (nr == 0)
        return sid->leftChannel;

    return sids[nr-1]->leftChannel;
}

auto SidManager::useRightChannel(int nr, bool state) -> void {
    if (nr == 0)
        sid->useRightChannel(state);
    else
        sids[nr-1]->useRightChannel(state);

    updateSidUsage();
}

auto SidManager::hasRightChannel(int nr) -> bool {
    if (nr == 0)
        return sid->rightChannel;

    return sids[nr-1]->rightChannel;
}

auto SidManager::setResampleQuality( uint8_t val ) -> void {

    sampleCounter = 0;

    switch(val) {
        case 0: sampleLimit = 1; break;
        case 1: sampleLimit = 2; break;
        case 2: sampleLimit = 7; break;
        default:
        case 3:
            sampleLimit = 18; break;
    }
}

auto SidManager::getResampleQuality( ) -> uint8_t {

    switch(sampleLimit) {
        case 1: return 0;
        case 2: return 1;
        case 7: return 2;
        default:
        case 18:
            return 3;
    }

    _unreachable
}

auto SidManager::searializeActiveSids(Emulator::Serializer& s, bool light) -> void {

    s.integer( sysClock );
    s.integer( useVolumeCorrection );
    s.integer( potX );
    s.integer( potY );

    sid->serialize(s, light);

    if (system->requestedSids && (s.mode() != Emulator::Serializer::Mode::Size) ) {
        for (unsigned i = 0; i < system->requestedSids; i++)
            sids[i]->serialize(s, light);
    }

    uint8_t sampleLimitBefore = sampleLimit;

    if (!light)
        s.integer( sampleCounter );

    s.integer( sampleLimit );
    s.integer( useExternalFilter );

    if (!light && (s.mode() == Emulator::Serializer::Mode::Load) ) {
        updateSidUsage();

        if (sampleLimitBefore != sampleLimit)
            system->updateStats();
    }
}

}
