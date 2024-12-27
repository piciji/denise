
#include "fastloader.h"
#include "../../system/system.h"
#include "../../disk/iec.h"

namespace LIBC64 {

Fastloader::Fastloader(System* system) : via(3), Cart(system, true, true) {
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
    if (rom && (romSize & 2) ) {
        // simple check if prg header is attached
        romSize -= 2;
        rom += 2;
    }

    Cart::setRom(media, rom, romSize);

    exRom = game = true;
}

auto Fastloader::writeIo1( uint16_t addr, uint8_t value ) -> void {
    addr &= 0xff;
    if ( (mode & FASTLOADER_PIA_IO1) == FASTLOADER_PIA_IO1) {
        if (addr == 0x5c || addr == 0x5d) {
            pia.write(addr & 3, value);
        }
    } else if ( (mode & FASTLOADER_CHARGE_IO1) == FASTLOADER_CHARGE_IO1) {
        charge();
    }
}

auto Fastloader::readIo1( uint16_t addr ) -> uint8_t {
    addr &= 0xff;
    if ( (mode & FASTLOADER_PIA_IO1) == FASTLOADER_PIA_IO1) {
        if (addr == 0x5c || addr == 0x5d) {
            return pia.read(addr & 3);
        }
    } else if ( (mode & FASTLOADER_CHARGE_IO1) == FASTLOADER_CHARGE_IO1) {
        charge();
        return 0;
    }

    return ExpansionPort::readIo1(addr);
}

auto Fastloader::writeIo2( uint16_t addr, uint8_t value ) -> void {
    if ( (mode & FASTLOADER_VIA_IO2) == FASTLOADER_VIA_IO2) {
        via.write( addr & 0xff, value );
    } else if ( (mode & FASTLOADER_CHARGE_IO2) == FASTLOADER_CHARGE_IO2) {
        discharge();
    }
}

auto Fastloader::readIo2( uint16_t addr ) -> uint8_t {
    if ( (mode & FASTLOADER_VIA_IO2) == FASTLOADER_VIA_IO2) {
        return via.read( addr & 0xff );
    } else if ( (mode & FASTLOADER_CHARGE_IO2) == FASTLOADER_CHARGE_IO2) {
        discharge();
        return 0;
    }

    return ExpansionPort::readIo1(addr);
}

auto Fastloader::charge() -> void {
    autoCharge();

    voltage += 78125;
    if (voltage > 5000000) // 5V
        voltage = 5000000;

    if (voltage > 2700000) { // 2.7V
        exRom = false;
        system->changeExpansionPortMemoryMode(exRom, game);
    }
}

auto Fastloader::discharge() -> void {
    autoCharge();

    voltage -= std::min(voltage, 78125U);
    if (voltage < 1400000) { // 1.4V
        exRom = true;
        system->changeExpansionPortMemoryMode(exRom, game);
    }
}

auto Fastloader::autoCharge() -> void {
    unsigned cyclesElapsed = system->sysTimer.fallBackCycles( chargeClock );

    voltage += std::min(2000000 - voltage, cyclesElapsed * 2);

    chargeClock = system->sysTimer.clock;
}

auto Fastloader::hasHiramCableConnected() -> bool {
    return kernalJumper && rom;
}

auto Fastloader::readRomL( uint16_t addr ) -> uint8_t {
    if (cRomL)
        return *(cRomL->ptr + addr);

    return ExpansionPort::readRomL( addr );
}

auto Fastloader::readRomH( uint16_t addr ) -> uint8_t {

    return *(cRomH->ptr + addr );
}

auto Fastloader::setJumper( unsigned jumperId, bool state ) -> void {
    kernalJumper = state;
}

auto Fastloader::getJumper(unsigned jumperId) -> bool {
    return kernalJumper;
}

auto Fastloader::serialize(Emulator::Serializer& s) -> void {
    unsigned _cartridgeId = cartridgeId;
    s.integer(_cartridgeId);
    Cart::serializeStep2( s );

    s.integer(kernalJumper);
    s.integer( mode );

    if (mode & FASTLOADER_PIA)
        pia.serialize(s);
    else if (mode & FASTLOADER_VIA)
        via.serialize(s);
    else if (mode & FASTLOADER_CHARGE) {
        s.integer(voltage);
        s.integer(chargeClock);
    }
}

auto Fastloader::reset(bool softReset) -> void {
    Cart::reset();
    pia.reset();
    via.reset();

    chargeClock = system->sysTimer.clock;
    // voltage = 0;

    mode = 0;
    if (cartridgeId == Interface::CartridgeIdProfDos)
        mode = FASTLOADER_VIA | FASTLOADER_PORTB | FASTLOADER_OUTPINB | FASTLOADER_IO2;
    else if (cartridgeId == Interface::CartridgeIdPrologicDos)
        mode = FASTLOADER_PIA | FASTLOADER_PORTA | FASTLOADER_OUTPINA | FASTLOADER_IO1;
    else if (cartridgeId == Interface::CartridgeIdTurboTrans)
        mode = FASTLOADER_VIA | FASTLOADER_PORTA | FASTLOADER_OUTPINA | FASTLOADER_IO2;
    else if (cartridgeId == Interface::CartridgeIdStarDos)
        mode = FASTLOADER_CHARGE | FASTLOADER_IO1 | FASTLOADER_IO2;
}

auto Fastloader::customButton() -> void {

    if (cartridgeId == Interface::CartridgeIdTurboTrans)
        system->iecBus.drives[0]->cpu.setNmi();
}

auto Fastloader::create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart* {
    // don't rebuild
    return this;
}

}
