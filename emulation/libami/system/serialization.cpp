
#include "system.h"

namespace LIBAMI {

auto System::calcSerializationSize() -> void {

    Emulator::Serializer s;

    unsigned signature = 0;
    char version[16] = {};

    s.integer(signature);
    s.array(version);

    serializeAll(s);

    serializationSize = s.size();
    serializationSizeLight = serializationSize - agnus.getMemorySize();
}

auto System::serialize(unsigned& size) -> uint8_t* {

    for(auto& drive : diskDrives) {
        if (drive.connected && drive.written)
            drive.structure.updateSerializationSize();
    }

    Emulator::Serializer s( serializationSize );

    unsigned signature = 0x414d49; // always constant for each emulation core
    char version[16] = {};

    auto str = Interface::Version;
    str.copy( version, str.size() );

    s.integer(signature);
    s.array(version);

    serializeAll(s);

    // avoid extra copy of state memory
    serializer = std::move( s );

    size = serializer.size();

    return serializer.data();
}

auto System::checkSerialization(uint8_t* data, unsigned size) -> bool {

    if (size < 20)
        return false;

    Emulator::Serializer s( data, 20 );

    unsigned signature = 0;
    char version[16] = {};

    s.integer(signature);
    s.array(version);

    if (signature != 0x414d49)
        return false;

    if (std::string(version) != Interface::Version)
        return false;

    return true;
}

auto System::unserialize(uint8_t* data, unsigned size) -> bool {

    Emulator::Serializer s( data, size );

    unsigned signature = 0;
    char version[16] = {};

    s.integer(signature);
    s.array(version);

    if(signature != 0x414d49)
        return false;

    if( std::string(version) != Interface::Version)
        return false;

    serializeAll(s);

    updateDriveSounds();
    updateStats();
    paula.informPowerLED(true);
    input.keyboard.informCapsLed();

    history.reset();
    return true;
}

auto System::serializeAll(Emulator::Serializer& s) -> void {
    serialize( s );
    cpu.serialize( s );
    agnus.serialize( s );
    cia1.serialize( s );
    cia2.serialize( s );
    denise.serialize( s );
    paula.serialize( s );
    input.serialize( s );
    rtc.serialize( s );

    for(auto& drive : diskDrives)
        drive.serialize(s);
}

auto System::serialize(Emulator::Serializer& s) -> void {
    s.integer( serializationSize );
    s.integer( serializationSizeLight );

    s.integer( observer.stateChange );
    s.integer( observer.motor );
    s.integer( observer.inputFetches );

    s.integer(ntsc);
    s.integer(asyncHDDAccess);
    s.integer((uint8_t&)dongle.type);
    if (dongle.type) {
        s.integer(dongle.clock);
        s.integer(dongle.control);
    }
}

// for runahead
auto System::serializeLight(MemState<uint32_t, uint16_t, 3>& memState, bool fromHistory) -> void {
    auto& s = memState.serializer;
    agnus.memState = &memState;

    s.setData( serializationSizeLight );
    s.setMode( Emulator::Serializer::Mode::Save );

    serialize(s);
    cpu.serialize(s);
    agnus.serialize(s, true);
    cia1.serialize(s);
    cia2.serialize(s);
    denise.serialize(s);
    paula.serialize(s, !fromHistory && (runAhead.frames > 1));
    input.serialize(s);
    if (fromHistory)
        rtc.serialize( s );

    for(auto& drive : diskDrives)
        drive.serialize(s, true);

    memState.trackers[TRACKER_CHIP].reset(agnus.chipMem);
    memState.trackers[TRACKER_SLOW].reset(agnus.slowMem);
    memState.trackers[TRACKER_FAST].reset(agnus.fastMem);
}

auto System::unserializeLight(MemState<uint32_t, uint16_t, 3>& memState, bool fromHistory) -> void {
    auto& s = memState.serializer;
    agnus.memState = nullptr;

    s.setMode( Emulator::Serializer::Mode::Load );

    serialize(s);
    cpu.serialize(s);
    agnus.serialize(s, true);
    cia1.serialize(s);
    cia2.serialize(s);
    denise.serialize(s);
    paula.serialize(s, !fromHistory && (runAhead.frames > 1));
    input.serialize(s);
    if (fromHistory)
        rtc.serialize( s );

    for(auto& drive : diskDrives)
        drive.serialize(s, true);

    memState.trackers[TRACKER_CHIP].apply();
    memState.trackers[TRACKER_SLOW].apply();
    memState.trackers[TRACKER_FAST].apply();
}

}
