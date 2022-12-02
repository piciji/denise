
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

    // disk->updateSerializationSize();

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

    // remapCpu();

    // updateDriveSounds();

    return true;
}

auto System::serializeAll(Emulator::Serializer& s) -> void {



    serialize( s );
    cia1.serialize( s );
    cia2.serialize( s );
    denise.serialize( s );
    paula.serialize( s );

    //disk.serialize( s );
    input.serialize( s );
    //agnus.serialize( s );
}

auto System::serialize(Emulator::Serializer& s) -> void {

//    s.array( ram, 64 * 1024 );

    s.integer( serializationSize );

    // powerSupply.serialize( s );
    cpu.serialize( s );
}

// for runahead
auto System::serializeLight() -> void {

    auto& s = runAhead.serializer;

    s.setData( serializationSize );
    s.setMode( Emulator::Serializer::Mode::Save );

    serialize(s);
    cia1.serialize(s);
    cia2.serialize(s);
    denise.serialize(s);
    paula.serialize(s, runAhead.frames > 1);

    //disk.serializeLight(s);
    input.serialize(s);


    //sysTimer.serialize(s);
}

auto System::unserializeLight() -> void {

    auto& s = runAhead.serializer;

    s.setMode( Emulator::Serializer::Mode::Load );
    //uint8_t _mode = mode;

    serialize(s);
    cia1.serialize(s);
    cia2.serialize(s);
    denise.serialize(s);
    paula.serialize(s, runAhead.frames > 1);

    //disk.serializeLight(s);
    input.serialize(s);

    //sysTimer.serialize(s);

    //if (mode != _mode)
      //  remapCpu();
}

}
