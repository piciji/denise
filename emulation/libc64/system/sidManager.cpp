
#include "sidManager.h"
#include "system.h"
#include "../expansionPort/expansionPort.h"
#include "../sid/clone.cpp"
#include "../sid/reSid24.h"
#include "../sid/chamberlin.h"
#include "residfpHandler.h"
#include "../../../deps/libresidfp/src/State.h"

namespace LIBC64 {

std::vector<std::string> SidManager::adrOptions = {
"Default", "D400", "D420", "D440", "D460", "D480", "D4A0", "D4C0", "D4E0", "D500", "D520", "D540", "D560", "D580",
"D5A0", "D5C0", "D5E0", "D600", "D620", "D640", "D660", "D680", "D6A0", "D6C0", "D6E0", "D700", "D720", "D740", "D760", "D780",
"D7A0", "D7C0", "D7E0", "DE00", "DE20", "DE40", "DE60", "DE80", "DEA0", "DEC0", "DEE0", "DF00", "DF20", "DF40", "DF60", "DF80", "DFA0", "DFC0", "DFE0"
};

SidManager::SidManager(System* system) : system(system), usbSIDPico(*system) {
    for (unsigned i = 0; i < 8; i++)
        sids[i] = new ReSid( i, system, *this, usbSIDPico, ReSid::Type::MOS_8580 );

    sampleCounter = 0;
    sampleLimit = 2;
    audioOut = true;
    serializationSizeForSevenMoreSids = 0;
    sysClock = 0;
    potX = 0xff;
    potY = 0xff;

    getPotX = []() { return 0xff; };
    getPotY = []() { return 0xff; };

    callPotUpdate = []() { };
    callAlarm = [this]() {
        updateClock();
    };

    extraSids = false;
    leftSids = 0;
    rightSids = 0;
}

auto SidManager::intensifyPseudoStereo(bool state) -> void {
    offsetPseudoStereo.allow = state;
    offsetPseudoStereo.trigger = false;
    offsetPseudoStereo.offset = 0;
    offsetPseudoStereo.delayedSid = nullptr;

    if (state && system->powerOn)
        applyOffsetPseudoStereo();
}

auto SidManager::applyOffsetPseudoStereo() -> void {

    for (auto useSid : useSids) {
        if ((useSid->leftChannel == useSid->rightChannel) || (useSid == mainSid()) || useSid->ioMask)
            continue;

        offsetPseudoStereo.offset = (system->vicII->frequency() >> 1) + (Emulator::Rand::rand() & 0x3ffff);
        offsetPseudoStereo.delayedSid = useSid;
        break;
    }
}

auto SidManager::setIoMask(int nr, uint8_t pos) -> void {
    Sid* sid = sids[nr];
    if (pos >= adrOptions.size())
        return;

    sid->ioPos = pos;

    if (pos == 0) {
        sid->ioMask = 0;
        return;
    }

    auto str = adrOptions[pos];

    sid->ioMask = std::stoul(str, nullptr, 16);
}

auto SidManager::getIoPos(int nr) -> int {
    return sids[nr]->ioPos;
}

auto SidManager::readSidReg(uint16_t addr) -> uint8_t {
    updateClock();
    if (extraSids)
        return getSidByAdr( addr )->readIO( addr );

    return mainSid()->readIO( addr );
}

auto SidManager::peekSidReg(uint16_t addr) -> uint8_t {
    updateClock();
    if (extraSids)
        return getSidByAdr( addr )->peekIO( addr );

    return mainSid()->peekIO( addr );
}

auto SidManager::writeSidReg(uint16_t addr, uint8_t value) -> void {
    updateClock();
    if (extraSids)
        return writeSid( addr, value );

    mainSid()->writeIO( addr, value );
}

auto SidManager::readIo(uint16_t& addr, uint8_t& value) -> bool {
    if (extraSids) {
        Sid* _sid = getSidByAdr( addr, true );
        if (_sid) {
            updateClock();
            value = _sid->readIO( addr );
            return true;
        }
    }
    return false;
}

auto SidManager::peekIo(uint16_t& addr, uint8_t& value) -> bool {
    if (extraSids) {
        Sid* _sid = getSidByAdr( addr, true );
        if (_sid) {
            updateClock();
            value = _sid->peekIO( addr );
            return true;
        }
    }
    return false;
}

auto SidManager::writeIo(uint16_t addr, uint8_t value) -> void {
    if (extraSids) {
        updateClock();
        writeSidIO( addr, value );
    }
}

auto SidManager::updateClock() -> void {
    system->sysTimer.add( &callAlarm, UpdateCycles, Emulator::SystemTimer::Action::UpdateExisting );

    int _delay = system->sysTimer.fallBackCycles( sysClock );

    if (!_delay)
        return;

    if (extraSids) {
        if (offsetPseudoStereo.offset > 0)
            audioOut ? clockMultiChips<true, true>(_delay) : clockMultiChips<false, true>(_delay);
        else {
            audioOut ? clockMultiChips<true, false>(_delay) : clockMultiChips<false, false>(_delay);
        }
    } else {
        sampleCounter = mainSid()->clock(_delay, sampleCounter, sampleLimit, audioOut);
    }

    sysClock = system->sysTimer.clock;
}

auto SidManager::registerCallbacks() -> void {

    system->sysTimer.registerCallback( { { &callPotUpdate, 1 }, { &callAlarm, 1 } } );
}

auto SidManager::disableAudioOut(bool state) -> void {
    audioOut = !state;
}

template<bool _audioOut, bool _delayed> auto SidManager::clockMultiChips(int cycles) -> void {
    double sampleLeft, sampleRight;
    const int _limit = sampleLimit;

    for (int c = 0; c < cycles; c++) {
        sampleLeft = sampleRight = 0.0;

        if (_audioOut && (++sampleCounter == _limit)) {

            sampleCounter = 0;

            for (auto useSid : useSids) {

                if constexpr (_delayed) {
                    if (useSid != offsetPseudoStereo.delayedSid)
                        useSid->clock();
                } else
                    useSid->clock();

                if (useSid->leftChannel)
                    sampleLeft += useSid->getSample();

                if (useSid->rightChannel)
                    sampleRight += useSid->getSample();
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
            for (auto useSid : useSids) {
                if constexpr (_audioOut)
                    useSid->clock();
                else
                    useSid->clockSilent();
            }
        }
    }

    if constexpr(_delayed) {
        if (offsetPseudoStereo.offset > cycles)
            offsetPseudoStereo.offset -= cycles;
        else
            offsetPseudoStereo.offset = 0;
    }
}

auto SidManager::getSidByAdr(uint16_t addr, bool ioArea) -> Sid* {

    addr &= 0xffe0;

    for (auto useSid : useSids) {
        if (addr == useSid->ioMask)
            return useSid;
    }

    return ioArea ? nullptr : mainSid();
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
        mainSid()->writeIO( addr, value );
    else {
        if (offsetPseudoStereo.allow) {
            if ((addr & 0x1f) == 0x18) {
                value &= 0xf;
                if (offsetPseudoStereo.trigger) {
                    if (value > 4) {
                        applyOffsetPseudoStereo();
                        offsetPseudoStereo.trigger = false;
                    }
                } else
                    offsetPseudoStereo.trigger = value == 0;
            }
        }
    }
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
        usbSIDPico.updateStereo();
        return;
    }

    if (mainSid()->leftChannel || mainSid()->rightChannel)
        useSids.push_back(mainSid());

    if (mainSid()->leftChannel)
        leftSids += 1.0;

    if (mainSid()->rightChannel)
        rightSids += 1.0;

    for (int i = 0; i < system->requestedSids; i++) {

        Sid* extraSid = sids[i+1];

        if (extraSid->leftChannel || extraSid->rightChannel)
            useSids.push_back( extraSid );

        if (extraSid->leftChannel)
            leftSids += 1.0;

        if (extraSid->rightChannel)
            rightSids += 1.0;
    }

    extraSids = !useSids.empty();

    system->updateStatsStereo();
    usbSIDPico.updateStereo();
    system->history.reset();
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
    for (auto sid : sids)
        sid->enableFilter( state );
}

auto SidManager::isFilterEnabled( ) -> bool {
    return mainSid()->filterEnabled();
}

auto SidManager::setEngineAll( uint8_t newEngine ) -> void {
    engine = newEngine;
    rebuildSids(true);
    updateSidUsage();
    system->history.reset();
}

auto SidManager::rebuildSids(bool cloneOld) -> void {
    for (unsigned i = 0; i < 8; i++) {
        auto oldSid = sids[i];
        Sid* newSid;

        switch (engine) {
            default:
            case 0:
                newSid = new ReSid( i, system, *this, usbSIDPico, oldSid->type );
                break;
            case 1:
                newSid = new ResidfpHandler(i, system, *this, usbSIDPico, oldSid->type);
                break;
            case 2:
                newSid = new ReSid24( i, system, *this, usbSIDPico, oldSid->type );
                break;
            case 3:
                newSid = new Chamberlin( i, system, *this, usbSIDPico, oldSid->type );
                break;
        }

        if (cloneOld)
            newSid->clone( oldSid, false );
        sids[i] = newSid;
        delete oldSid;
    }
}

auto SidManager::getEngine( ) -> uint8_t {
    return engine;
}

auto SidManager::setType( int nr, Sid::Type type ) -> void {
    sids[nr]->setType( type );
}

auto SidManager::getType(int nr) -> Sid::Type {
    return sids[nr]->getType();
}

auto SidManager::updateChamberlinFrequencyAll(double sampleRate) -> void {
    for (auto sid : sids) {
        sid->setSampleRate(sampleRate);
    }
}

auto SidManager::adjustFilterCurve6581All(int value) -> void {
    for (auto sid : sids) {
        sid->adjustFilterCurve6581( value );
    }
}

auto SidManager::getFilterCurve6581() -> int {
    return mainSid()->getFilterCurve6581();
}

auto SidManager::adjustFilterCurve8580All(int value) -> void {
    for (auto sid : sids) {
        sid->adjustFilterCurve8580( value );
    }
}

auto SidManager::getFilterCurve8580() -> int {
    return mainSid()->getFilterCurve8580();
}

auto SidManager::adjustFilterRange6581All(int value) -> void {
    for (auto sid : sids) {
        sid->adjustFilterRange6581( value );
    }
}

auto SidManager::getFilterRange6581() -> int {
    return mainSid()->getFilterRange6581();
}

auto SidManager::setWaveformStrength(uint8_t value) -> void {
    for (auto sid : sids) {
        sid->setWaveformStrength( value );
    }
}

auto SidManager::getWaveformStrength() -> uint8_t {
    return mainSid()->getWaveformStrength();
}

auto SidManager::setDigiBoostAll( bool state ) -> void {
    for (auto sid : sids) {
        sid->setDigiBoost( state );
    }
}

auto SidManager::getDigiBoost( ) -> bool {
    return mainSid()->hasDigiBoost();
}

auto SidManager::resetAll() -> void {
    sysClock = 0;
    potX = potY = 0xff;
    system->sysTimer.add( &callAlarm, UpdateCycles, Emulator::SystemTimer::Action::UpdateExisting );

    if (usbSIDPico.enabled)
        usbSIDPico.reset();

    for (auto sid : sids)
        sid->reset();

    sampleCounter = 0;
    offsetPseudoStereo.offset = 0;
    offsetPseudoStereo.trigger = true;
    offsetPseudoStereo.delayedSid = nullptr;

    usbSIDPico.enabled ? (void)usbSIDPico.open() : usbSIDPico.close();
}

auto SidManager::calcSerializationSizeForSevenMoreSids() -> void {

    Emulator::Serializer s;

    mainSid()->serialize( s, false );

    auto s1 = s.size();
    auto s2 = sizeof(reSIDfp::State);

    if (s2 > s1)
        s1 = s2;

    serializationSizeForSevenMoreSids = s1 * 7;
}

auto SidManager::useLeftChannel(int nr, bool state) -> void {
    sids[nr]->useLeftChannel(state);

    updateSidUsage();
}

auto SidManager::hasLeftChannel(int nr) -> bool {
    return sids[nr]->leftChannel;
}

auto SidManager::useRightChannel(int nr, bool state) -> void {
    sids[nr]->useRightChannel(state);

    updateSidUsage();
}

auto SidManager::hasRightChannel(int nr) -> bool {
    return sids[nr]->rightChannel;
}

auto SidManager::setResampleQuality( uint8_t val ) -> void {
    sampleCounter = 0;

    switch(val) {
        case 0: sampleLimit = 1; break;
        default:
        case 1: sampleLimit = 2; break;
        case 2: sampleLimit = 7; break;
    }
    system->history.reset();
}

auto SidManager::getResampleQuality( ) -> uint8_t {
    switch(sampleLimit) {
        case 1: return 0;
        default:
        case 2: return 1;
        case 7: return 2;
    }

    _unreachable
}

auto SidManager::enableUSBSID(bool state) -> void {
    usbSIDPico.enabled = state;
    if (!state)
        usbSIDPico.close();
    else if (system->powerOn) {
        usbSIDPico.open();
    }
}

auto SidManager::setUSBSIDBuffSize(unsigned value) -> void {
    usbSIDPico.setBuffSize(value);
}

auto SidManager::setUSBSIDDiffSize(unsigned value) -> void {
    usbSIDPico.setDiffSize(value);
}

auto SidManager::searializeActiveSids(Emulator::Serializer& s, bool light) -> void {
    bool _loadState = !s.memUsage() && (s.mode() == Emulator::Serializer::Mode::Load);

    s.integer( sysClock );
    s.integer( potX );
    s.integer( potY );
    uint8_t _engine = engine;
    s.integer( engine );

    if (_loadState && _engine != engine) {
        rebuildSids(false);
    }

    mainSid()->serialize(s, light);

    if (system->requestedSids && (s.mode() != Emulator::Serializer::Mode::Size) ) {
        for (unsigned i = 0; i < system->requestedSids; i++)
            sids[i + 1]->serialize(s, light);
    }

    uint8_t sampleLimitBefore = sampleLimit;

    if (!light)
        s.integer( sampleCounter );

    s.integer( sampleLimit );

    if (_loadState) {
        updateSidUsage();

        if (sampleLimitBefore != sampleLimit)
            system->updateStats();
    }
}

auto SidManager::updateSnapshot(DebuggerSnapshot& snap) -> void {
    uint8_t _potX = getPotX();
    uint8_t _potY = getPotY();

    for (unsigned i = 0; i < 8; i++) {
        auto& s = snap.sids[i];

        s.active = (i == 0) || (i <= system->requestedSids);

        if (s.active) {
            sids[i]->updateSnapshot( snap );
            s.potX = _potX;
            s.potY = _potY;
        }
    }
}

auto SidManager::clone( uint8_t start, uint8_t end ) -> void {
    if (start >= end)
         return;

    for( ; start < end; start++ ) {
        Sid* sid = sids[start + 1];

        sid->clone( mainSid(), true );
    }
}

}
