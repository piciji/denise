
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../sid/sid.h"
#include "../sid/usbSidPico.h"
#include "debuggerSnapshot.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBC64 {

struct System;

struct SidManager {
    SidManager(System* system);

    using Callback = std::function<void ()>;
    std::function<uint8_t ()> getPotX;
    std::function<uint8_t ()> getPotY;
    USBSIDPico usbSIDPico;

    System* system;
    bool audioOut;
    double leftSids;
    double rightSids;
    int sampleCounter;
    int sampleLimit;
    unsigned serializationSizeForSevenMoreSids;
    unsigned sysClock;
    bool useExternalFilter;
    bool useVolumeCorrection;
    bool extraSids;
    int optionsInUse;

    Callback callAlarm;
    Callback callPotUpdate;

    struct {
        bool allow = false;
        int offset = 0;
        bool trigger = true;
        Sid* delayedSid = nullptr;
    } offsetPseudoStereo;

    uint8_t potX;
    uint8_t potY;

    std::vector<Sid*> useSids;

    Sid* sid;
    Sid* sids[7];

    auto updateOptionsInUse() -> void;
    auto setExternalFilter(bool state) -> void;
    template<int options> auto clockMultiChips(int cycles) -> void;
    auto getSidByAdr(uint16_t addr, bool ioArea = false) -> Sid*;
    auto writeSid(uint16_t addr, uint8_t value) -> void;
    auto writeSidIO(uint16_t addr, uint8_t value) -> void;
    auto updateSidUsage() -> void;
    auto isStereo() -> bool;
    auto setEnableFilterAll( bool state ) -> void;
    auto isEnableFilter( ) -> bool;
    auto setFilterTypeAll( Sid::FilterType filterType ) -> void;
    auto getFilterType( ) -> Sid::FilterType;
    auto setFilterVolumeCorrection( bool state ) -> void;
    auto setType( int nr, Sid::Type type ) -> void;
    auto getType( int nr ) -> Sid::Type;
    auto updateChamberlinFrequencyAll(double sampleRate) -> void;
    auto adjustFilterBias6581All(int value) -> void;
    auto adjustFilterBias8580All(int value) -> void;
    auto getFilterBias6581() -> int;
    auto getFilterBias8580() -> int;
    auto setDigiBoostAll( bool state ) -> void;
    auto getDigiBoost( ) -> bool;
    auto resetAll() -> void;
    auto setIoMask(int nr, uint8_t pos) -> void;
    auto getIoPos(int nr) -> int;
    auto readSidReg(uint16_t addr) -> uint8_t;
    auto peekSidReg(uint16_t addr) -> uint8_t;
    auto writeSidReg(uint16_t addr, uint8_t value) -> void;
    auto readIo(uint16_t& addr, uint8_t& value) -> bool;
    auto peekIo(uint16_t& addr, uint8_t& value) -> bool;
    auto writeIo(uint16_t addr, uint8_t value) -> void;

    auto setResampleQuality( uint8_t val ) -> void;
    auto getResampleQuality( ) -> uint8_t;

    auto disableAudioOut(bool state) -> void;
    auto calcSerializationSizeForSevenMoreSids() -> void;
    auto searializeActiveSids(Emulator::Serializer& s, bool light = false) -> void;

    auto updateClock() -> void;
    template<int options> auto updateClockT() -> void;
    auto registerCallbacks() -> void;
    auto clone( uint8_t start, uint8_t end ) -> void;

    auto useLeftChannel(int nr, bool state) -> void;
    auto useRightChannel(int nr, bool state) -> void;
    auto hasLeftChannel(int nr) -> bool;
    auto hasRightChannel(int nr) -> bool;

    auto applyOffsetPseudoStereo() -> void;
    auto intensifyPseudoStereo(bool state) -> void;
    auto hasIntensifiedPseudoStereo() -> bool { return offsetPseudoStereo.allow; }
    auto setSeparateFilterInputs(bool state) -> void;
    auto hasSeparateFilterInputs() -> bool;

    auto enableUSBSID(bool state) -> void;
    auto setUSBSIDBuffSize(unsigned value) -> void;
    auto setUSBSIDDiffSize(unsigned value) -> void;    

    auto hasUSBSID() -> bool { return usbSIDPico.enabled; }
    auto getUSBSIDBuffSize() -> unsigned { return usbSIDPico.buffSize; }
    auto getUSBSIDDiffSize() -> unsigned { return usbSIDPico.diffSize; }

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;
};

}