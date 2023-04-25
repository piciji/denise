
#include "fastloader.h"
#include "../../system/system.h"
#include "../../disk/iec.h"

namespace LIBC64 {

Fastloader::Fastloader(System* system) : via(3), ExpansionPort(system) {
    setId( Interface::ExpansionIdFastloader );

    pia.ca2Out = [system, this](bool direction) {
        if (system->secondDriveCable.parallelUse && !direction && (mode & FASTLOADER_OUTPINA)) {
            system->diskIdleOff();
            // Port B with parallel cable
            system->iecBus.writeParallelHandshake();
        }
    };

    pia.cb2Out = [system, this](bool direction) {
        // no use case at the moment
        if (system->secondDriveCable.parallelUse && !direction && (mode & FASTLOADER_OUTPINB)) {
            system->diskIdleOff();
            // Port B with parallel cable
            system->iecBus.writeParallelHandshake();
        }
    };

    pia.readPort = [system, this]( Emulator::Pia::Port port ) {

        if (port == Emulator::Pia::Port::A) { // PROLOGIC
            if (!system->secondDriveCable.parallelUse || (mode & FASTLOADER_PORTB) )
                return this->pia.ioa;

            system->diskIdleOff();

            return (uint8_t)(this->pia.ioa & system->iecBus.readParallel());
        }

        // Port B (no use case at the moment)
        if (!system->secondDriveCable.parallelExpansion || (mode & FASTLOADER_PORTA))
            return this->pia.iob;

        system->diskIdleOff();

        return (uint8_t)(this->pia.iob & system->iecBus.readParallel());
    };

    pia.writePort = [this]( Emulator::Pia::Port port, uint8_t data ) {
        // nothing todo here, because CA(B)2 is triggered and port value is latched within PIA context
    };

    via.ca2Out = [system, this]( bool direction ) {
        // no use case at the moment
        if (system->secondDriveCable.parallelUse && !direction && (mode & FASTLOADER_OUTPINA)) {
            system->diskIdleOff();
            // Port B with parallel cable
            system->iecBus.writeParallelHandshake();
        }
    };

    via.cb2Out = [system, this]( bool direction ) {
        if (system->secondDriveCable.parallelUse && !direction && (mode & FASTLOADER_OUTPINB)) {
            system->diskIdleOff();
            // Port B with parallel cable
            system->iecBus.writeParallelHandshake();
        }
    };

    via.readPort = [system, this]( Via::Port port, Via::Lines* lines ) {
        if (port == Via::Port::A) { // no use case at the moment
            if (!system->secondDriveCable.parallelUse || (mode & FASTLOADER_PORTB) )
                return lines->ioa;

            system->diskIdleOff();

            return (uint8_t)(lines->ioa & system->iecBus.readParallel());
        }

        // Port B
        if (!system->secondDriveCable.parallelExpansion || (mode & FASTLOADER_PORTA))
            return lines->iob;

        system->diskIdleOff();

        return (uint8_t)(lines->iob & system->iecBus.readParallel());
    };

    via.writePort = [this]( Via::Port port, Via::Lines* lines ) {
        // nothing todo here, because CA(B)2 is triggered and port value is latched within VIA context
    };
}

auto Fastloader::clock() -> void {
    if (mode & FASTLOADER_VIA) {
        via.process();
    }
}

auto Fastloader::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {

    this->media = (rom && romSize) ? media : nullptr;
    if (rom && (romSize & 2) ) {
        // simple check if prg header is attached
        romSize -= 2;
        rom += 2;
    }

    this->rom = rom;
    this->romSize = romSize;
}

auto Fastloader::writeIo1( uint16_t addr, uint8_t value ) -> void {
    addr &= 0xff;
    if ( (mode & FASTLOADER_PIA_IO1) == FASTLOADER_PIA_IO1) {
        if (addr == 0x5c || addr == 0x5d) {
            pia.write(addr & 3, value);
        }
    }
}

auto Fastloader::readIo1( uint16_t addr ) -> uint8_t {
    addr &= 0xff;
    if ( (mode & FASTLOADER_PIA_IO1) == FASTLOADER_PIA_IO1) {
        if (addr == 0x5c || addr == 0x5d) {
            return pia.read(addr & 3);
        }
    }

    return ExpansionPort::readIo1(addr);
}

auto Fastloader::writeIo2( uint16_t addr, uint8_t value ) -> void {
    if ( (mode & FASTLOADER_VIA_IO2) == FASTLOADER_VIA_IO2) {
        via.write( addr & 0xff, value );
    }
}

auto Fastloader::readIo2( uint16_t addr ) -> uint8_t {
    if ( (mode & FASTLOADER_VIA_IO2) == FASTLOADER_VIA_IO2) {
        return via.read( addr & 0xff );
    }

    return ExpansionPort::readIo1(addr);
}

auto Fastloader::hasHiramCableConnected() -> bool {
    return kernalJumper && rom;
}

auto Fastloader::readRomH( uint16_t addr ) -> uint8_t {

    return rom[ addr & 0x1fff ];
}

auto Fastloader::setJumper( bool state ) -> void {
    kernalJumper = state;
}

auto Fastloader::getJumper( ) -> bool {
    return kernalJumper;
}

auto Fastloader::serialize(Emulator::Serializer& s) -> void {
    s.integer(kernalJumper);
    s.integer( (uint8_t&)type );
    s.integer( mode );
    if (mode & FASTLOADER_PIA)
        pia.serialize(s);
    else if (mode & FASTLOADER_VIA)
        via.serialize(s);

    ExpansionPort::serialize( s );
}

auto Fastloader::reset(bool softReset) -> void {
    pia.reset();
    via.reset();

    uint8_t _type = 0;
    auto& group = system->interface->mediaGroups[Interface::MediaGroupIdExpansionFastloader];

    for(auto& _media : group.media) {
        if (&_media == group.selected)
            break;
        _type++;
    }

    type = (Type)_type;
    mode = FASTLOADER_VIA | FASTLOADER_PORTB | FASTLOADER_OUTPINB | FASTLOADER_IO2;

    if (type == PROLOGIC_DOS)
        mode = FASTLOADER_PIA | FASTLOADER_PORTA | FASTLOADER_OUTPINA | FASTLOADER_IO1;

    else if (type == TURBO_TRANS)
        mode = FASTLOADER_VIA | FASTLOADER_PORTA | FASTLOADER_OUTPINA | FASTLOADER_IO2;
}

auto Fastloader::customButton() -> void {

    if (type == TURBO_TRANS)
        system->iecBus.drives[0]->cpu.setNmi();
}

}
