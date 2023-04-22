
#include "system.h"

namespace LIBAMI {

auto System::calcSerializationSize() -> void {

    Emulator::Serializer s;

    unsigned signature = 0;
    char version[16] = {};
    char reserved[256] = {};

    s.integer(signature);
    s.array(version);
    s.array(reserved);

    serializeAll(s);

    serializationSize = s.size();
}

auto System::serialize(unsigned& size) -> uint8_t* {

    for(auto& drive : diskDrives) {
        if (drive.connected && drive.written)
            drive.structure.updateSerializationSize();
    }

    Emulator::Serializer s( serializationSize );

    unsigned signature = 0x414d49; // always constant for each emulation core
    char version[16] = {};
    char reserved[256] = {}; // reserved, e.g. state description

    auto str = Interface::Version;
    str.copy( version, str.size() );

    s.integer(signature);
    s.array(version);
    s.array(reserved);

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
    char reserved[256] = {};

    s.integer(signature);
    s.array(version);
    s.array(reserved);

    if(signature != 0x414d49)
        return false;

    if( std::string(version) != Interface::Version)
        return false;

    serializeAll(s);

    updateDriveSounds();
    updateStats();

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

    s.integer( observer.stateChange );
    s.integer( observer.motor );
    s.integer( observer.inputFetches );
}

// for runahead
auto System::serializeLight() -> void {

    auto& s = runAhead.serializer;

    s.setData( serializationSize );
    s.setMode( Emulator::Serializer::Mode::Save );

    serialize(s);
    cpu.serialize(s);
    agnus.serialize(s, true);
    cia1.serialize(s);
    cia2.serialize(s);
    denise.serialize(s);
    paula.serialize(s, runAhead.frames > 1);
    input.serialize(s);

    for(auto& drive : diskDrives)
        drive.serialize(s, true);
}

auto System::unserializeLight() -> void {

    auto& s = runAhead.serializer;

    s.setMode( Emulator::Serializer::Mode::Load );

    serialize(s);
    cpu.serialize(s);
    agnus.serialize(s, true);
    cia1.serialize(s);
    cia2.serialize(s);
    denise.serialize(s);
    paula.serialize(s, runAhead.frames > 1);
    input.serialize(s);

    for(auto& drive : diskDrives)
        drive.serialize(s, true);
}

}
