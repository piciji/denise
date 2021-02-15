
#include "gmod2.h"
#include <algorithm>

namespace LIBC64 {
	
Gmod2* gmod2 = nullptr;  


Gmod2::Gmod2() : GameCart(true, false), flash(Emulator::Flash040::TypeNormal) {
    init();
}

Gmod2::~Gmod2() {
	delete[] flashData;
	delete[] eepromData;
}

auto Gmod2::init() -> void {
	flashData = new uint8_t[ 512 * 1024 ];	
	eepromData = new uint8_t[ 2 * 1024 ];

    flash.setData( flashData );
    flash.setEvents( &sysTimer );

    flash.written = []() {
        system->serializationSize += 512 * 1024;
    };

    eeprom.setData( eepromData );
    eeprom.setEvents( &sysTimer );
    eeprom.orgSelect( true );

    eeprom.written = []() {
        system->serializationSize += 2 * 1024;
    };
}

auto Gmod2::prepare() -> void {
	
    std::memset(flashData, 0xff, 512 * 1024 );
    unsigned offset;

    for(auto& chip : chips) {

        if (chip.bank >= 64)
            break;

        offset = chip.bank << 13;

        std::memcpy( flashData + offset, chip.ptr, 0x2000 );
    }
}

auto Gmod2::write() -> void {

	bool _dirty = flash.dirty;
	flash.dirty = false;
	
    if (!media || !media->guid || !_dirty || writeProtect)
        return;

    if (!system->interface->questionToWrite(media))
        return;

    system->interface->truncateMedia( media );

    unsigned offset = 0;

    if (!binFormat) {
        uint8_t header[64];
        buildHeader(&header[0], 0x3c, true, false, "Gmod2 Cartridge" );
        system->interface->writeMedia(media, &header[0], 0x40, 0);
        offset += 0x40;
    }

    if (!chips.size()) {
        Chip chip;
        chip.size = 0x2000;
        chip.addr = 0x8000;
        chip.type = Chip::Type::FlashRom;
        chips.push_back( chip );
    }

    Chip* chip = &chips[0];
    uint8_t cheader[16];

    for (unsigned b = 0; b < 64; b++ ) {

        if (binFormat) {
            system->interface->writeMedia(media, flashData + b * 0x2000, 0x2000, offset);
            offset += 0x2000;

            continue;
        }
        // crt format
        chip->bank = b;

        if (checkForEmptyFlashBank(flashData + b * 0x2000))
            continue;

        buildChipHeader( &cheader[0], *chip );
        system->interface->writeMedia(media, &cheader[0], 16, offset);
        offset += 16;
        system->interface->writeMedia(media, flashData + b * 0x2000, 0x2000, offset);
        offset += 0x2000;
    }   
}

// eeprom
auto Gmod2::setSecondaryRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {

    if ( (this->romSecondary == nullptr) && (rom == nullptr) )
        return;

    if (this->romSecondary && (rom == nullptr))
        // unset
        writeEeprom();

    this->romSecondary = rom;
    mediaSecondary = media;
    
	std::memset( eepromData, 0xff, 2 * 1024 );

	if (rom)
		std::memcpy( eepromData, rom, std::min( (unsigned)(2 * 1024), romSize ) );
}

auto Gmod2::writeEeprom() -> void {
	bool _dirty = eeprom.dirty;
	eeprom.dirty = false;		
	
    if (!mediaSecondary || !mediaSecondary->guid || !_dirty || writeProtectEeprom)
        return;

    if (!system->interface->questionToWrite(mediaSecondary))
        return;

    system->interface->truncateMedia( mediaSecondary );

    system->interface->writeMedia(mediaSecondary, eepromData, 2 * 1024, 0);		
}

auto Gmod2::readIo1( uint16_t addr ) -> uint8_t {

    return (eeprom.read() << 7) | (ExpansionPort::readIo1( addr ) & 0x7f);
}

auto Gmod2::writeIo1( uint16_t addr, uint8_t value ) -> void {	
    bank = value & 0x3f;

    eeprom.chipSelect( value & 0x40 );
    eeprom.write( value & 0x10 );
    eeprom.clock( value & 0x20 );

    value >>= 6;

    exRom = value & 1;

    system->changeExpansionPortMemoryMode(exRom, true); // 8K mode or cart disabled

    // listen on bus activity for CPU places address to write (first half cycle)
    // if "Write" >= 0x8000 is recognized, expansion switches to Ultimax Mode, just before the write happens.
    // otherwise it switches back to requested mode (8K or Cart disabled)
    writeEnable = value & 2; // write enable
}

auto Gmod2::clock() -> void {

    if (writeEnable) {
        uint16_t _addr = cpu->addressBus();
        bool _write = cpu->isWriteCycle();

        if ( _write && (_addr >= 0x8000) ) {// ultimax
            system->changeExpansionPortMemoryMode(true, false);
            vicII->setUltimaxPhi1( false );
        } else
            system->changeExpansionPortMemoryMode( exRom, true);
    }
}

auto Gmod2::readRomL( uint16_t addr ) -> uint8_t {

    return flash.read((addr & 0x1fff) | (bank << 13) );
}

auto Gmod2::writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void {

    flash.write( (addr & 0x1fff) | (bank << 13), data );
}

auto Gmod2::serializeStep2(Emulator::Serializer& s) -> void {

    s.integer( bank );

    s.integer( writeEnable );

    s.integer( writeProtect );

    s.integer( writeProtectEeprom );

    flash.serialize(s);

    eeprom.serialize(s );

    if (!s.lightUsage()) {

        if (flash.dirty)
            s.array(flashData, 512 * 1024);

        if (eeprom.dirty)
            s.array(eepromData, 2 * 1024);
    }

    ExpansionPort::serialize(s);
}

auto Gmod2::reset() -> void {

    bank = 0;
    writeEnable = false;
    flash.reset();
    eeprom.reset();
}

auto Gmod2::createImage(unsigned& imageSize) -> uint8_t* {
    imageSize = 64 + 16 + 8 * 1024;

    uint8_t* buffer = new uint8_t[ imageSize ];
    std::memset(buffer, 0xff, imageSize);

    uint8_t header[64];
    buildHeader(&header[0], 0x3c, true, false, "Gmod2 Cartridge" );

    std::memcpy(buffer, &header, 64);

    Chip chip;
    chip.bank = 0;
    chip.size = 0x2000;
    chip.type = Chip::Type::FlashRom;
    chip.addr = 0x8000;

    uint8_t cheader[16];
    buildChipHeader( &cheader[0], chip );

    std::memcpy(buffer + 64, &cheader, 16);

    return buffer;
}

auto Gmod2::createSecondaryImage(unsigned& imageSize) -> uint8_t* {
    imageSize = 2 * 1024;
    uint8_t* buffer = new uint8_t[ imageSize ];
    std::memset(buffer, 0xff, imageSize);

    return buffer;
}

auto Gmod2::setWriteProtect(bool state) -> void {
    writeProtect = state;
}

auto Gmod2::isWriteProtected() -> bool {
    return writeProtect;
}

auto Gmod2::setSecondaryWriteProtect(bool state) -> void {
    writeProtectEeprom = state;
}

auto Gmod2::isSecondaryWriteProtected() -> bool {
    return writeProtectEeprom;
}

}
