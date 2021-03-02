
#include "../../system/system.h"
#include "easyFlash.h"

namespace LIBC64 {  
    
EasyFlash* easyFlash = nullptr;  

#include "eapi-am29f040.h"

EasyFlash::EasyFlash() : Cart(false, true),
    flashLo(Emulator::Flash040::TypeB),
    flashHi(Emulator::Flash040::TypeB) {
    
    this->media = nullptr;
    
    this->writeProtect = true;
    
    this->flashJumper = false;
    
    init();
    
    setId( Interface::ExpansionIdEasyFlash );
}

EasyFlash::~EasyFlash() {
    delete[] dataLo;
    delete[] dataHi;
}

auto EasyFlash::init( ) -> void {
    
    dataLo = new uint8_t[ 512 * 1024 ];
    dataHi = new uint8_t[ 512 * 1024 ];
    
    flashLo.setData( dataLo );
    flashHi.setData( dataHi );
    
    flashLo.setEvents( &sysTimer );
    flashHi.setEvents( &sysTimer );   
    
    flashLo.written = []() {
        system->serializationSize += 512 * 1024;
    };

    flashHi.written = []() {
        system->serializationSize += 512 * 1024;
    };
}

auto EasyFlash::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
    
    if ( (this->rom == nullptr) && (rom == nullptr) )
        return;
      
    if (this->rom && (rom == nullptr))
        // unset
        write();
    
    this->media = media;
    this->rom = rom;
    this->romSize = romSize;
    
    readHeader();
    
    this->cartridgeId = Interface::CartridgeIdEasyFlash;
        
    if ( !readChips() )
        assumeChips();  
                   
    std::memset(dataLo, 0xff, 512 * 1024 );
    std::memset(dataHi, 0xff, 512 * 1024 );
    unsigned offset = 0;
    
    for(auto& chip : chips) {

        if (chip.bank >= 64)
            break;
        
        offset = chip.bank << 13;
        
        if (!chip.ptrHi) {                        
                        
            if (chip.addr & 0x2000)
                std::memcpy( dataHi + offset, chip.ptr, chip.size );   
            else
                std::memcpy( dataLo + offset, chip.ptr, chip.size );
            
        } else {
            // when ptrHi exists, chip size is more than 8192 bytes            
            std::memcpy( dataLo + offset, chip.ptr, 0x2000 );            
            std::memcpy( dataHi + offset, chip.ptrHi, std::min(chip.size - 0x2000, 0x2000) );
        }            
    }
    
    if (std::memcmp(&dataHi[0x1800], "eapi", 4) == 0) {
        std::memcpy(dataHi + 0x1800, eapi, 768);
    }
}

auto EasyFlash::assumeChips( ) -> void {
    Cart::assumeChips( );
    
    bool toggle = false;
    for(auto& chip : chips) {
                
        chip.addr = toggle ? 0xa000 : 0x8000;
        chip.bank = chip.id >> 1;
        
        toggle ^= 1;
    }
}

auto EasyFlash::write() -> void {
    
	bool _dirty = flashLo.dirty || flashHi.dirty;
	
	flashLo.dirty = false;
    flashHi.dirty = false;

    if (!media || !media->guid || !_dirty || writeProtect )
        return;
        
    if (!system->interface->questionToWrite(media))
        return;    
    
    system->interface->truncateMedia( media );
    
    unsigned offset = 0;
    
    if (!binFormat) {
        uint8_t header[64];
        buildHeader(&header[0], 0x20, false, true, "EasyFlash Cartridge" );
        system->interface->writeMedia(media, &header[0], 0x40, 0);
        offset += 0x40;
    }    
    
    if (!chips.size()) {
        Chip chip;
        chip.size = 0x2000;
        chip.type = Chip::Type::FlashRom;
        chips.push_back( chip );
    }

    Chip* chip = &chips[0];
    uint8_t cheader[16];

    bool crt8k = chip->size == 0x2000;
    
    for (unsigned b = 0; b < 64; b++ ) {

        if (binFormat) {
            system->interface->writeMedia(media, dataLo + b * 0x2000, 0x2000, offset);
            offset += 0x2000;

            system->interface->writeMedia(media, dataHi + b * 0x2000, 0x2000, offset);
            offset += 0x2000;

            continue;
        }
        // crt format 
        chip->bank = b;
        
        bool writeBankLo = !checkForEmptyFlashBank(dataLo + b * 0x2000);
        bool writeBankHi = !checkForEmptyFlashBank(dataHi + b * 0x2000);
                
        if ( writeBankLo || (!crt8k && writeBankHi) ) {            
            chip->addr = 0x8000;
            buildChipHeader( &cheader[0], *chip );
            system->interface->writeMedia(media, &cheader[0], 16, offset);
            offset += 16;
            system->interface->writeMedia(media, dataLo + b * 0x2000, 0x2000, offset);
            offset += 0x2000;   
        }                                
        
        if (crt8k && writeBankHi) {
            chip->addr = 0xa000;
            buildChipHeader(&cheader[0], *chip);
            system->interface->writeMedia(media, &cheader[0], 16, offset);
            offset += 16;
        }                

        if (writeBankHi || (!crt8k && writeBankLo) ) {
            system->interface->writeMedia(media, dataHi + b * 0x2000, 0x2000, offset);
            offset += 0x2000;
        }
    }    
}

auto EasyFlash::assign( Cart* cart ) -> void {
    // don't rebuild
}

auto EasyFlash::create( Interface::CartridgeId cartridgeId ) -> Cart* {
    // don't rebuild
    return easyFlash;
}

auto EasyFlash::reset(bool softReset) -> void {
    
    std::memset(ram, 0xff, 256);
   
    bank = 0;
    game = flashJumper;
    exRom = true;
    LED = false;
    
    flashLo.reset();
    flashHi.reset();
}

auto EasyFlash::updateDeviceState() -> void {
    
    system->interface->updateDeviceState( media, false, 0, LED, true );
}

auto EasyFlash::isBootable( ) -> bool {
    return !flashJumper;
}  

auto EasyFlash::writeIo1( uint16_t addr, uint8_t value ) -> void {
    
    if ((addr & 2) == 0) {
        bank = value & 0x3f;
        
    } else {
        // LED
        bool LEDNew = value & 0x80;
        
        if (LED != LEDNew) {
            LED = LEDNew;
            updateDeviceState();
        }
        
        bool mode = value & 4;
        
        value = ~value;
        
        exRom = (value >> 1) & 1;
        game = value & 1;
        
        if (game && !mode)
            game = flashJumper;                 
                  
        system->changeExpansionPortMemoryMode( exRom, game );            
    }   
}

auto EasyFlash::writeIo2( uint16_t addr, uint8_t value ) -> void {
    
    ram[ addr & 0xff ] = value;
}

auto EasyFlash::readIo2( uint16_t addr ) -> uint8_t {
    
    return ram[ addr & 0xff ];
}

auto EasyFlash::readRomL( uint16_t addr ) -> uint8_t {
    
    return flashLo.read( bank * 0x2000 + (addr & 0x1fff) );
}

auto EasyFlash::writeRomL( uint16_t addr, uint8_t data ) -> void {
    
    flashLo.write( bank * 0x2000 + (addr & 0x1fff), data );
    
    ExpansionPort::writeRomL( addr, data );
}

auto EasyFlash::readRomH( uint16_t addr ) -> uint8_t {
    
    return flashHi.read( bank * 0x2000 + (addr & 0x1fff) );
}

auto EasyFlash::writeRomH( uint16_t addr, uint8_t data ) -> void {
    
    flashHi.write( bank * 0x2000 + (addr & 0x1fff), data );
    
    ExpansionPort::writeRomH( addr, data );
}

auto EasyFlash::writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {

    flashLo.write( bank * 0x2000 + (addr & 0x1fff), data );

    ExpansionPort::writeUltimaxRomL( addr, data );
}

auto EasyFlash::writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void {

    flashHi.write( bank * 0x2000 + (addr & 0x1fff), data );

    ExpansionPort::writeUltimaxRomH( addr, data );
}

auto EasyFlash::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( (uint16_t&)cartridgeId );    
    
    s.integer( bank );
    
    s.integer( flashJumper );
    
    s.integer( writeProtect );
    
    s.integer( LED );
    
    s.array( ram );
    
    flashLo.serialize(s);
    flashHi.serialize(s);

    if (!s.lightUsage()) {
        if (flashLo.dirty)
            s.array(dataLo, 512 * 1024);

        if (flashHi.dirty)
            s.array(dataHi, 512 * 1024);

        if (s.mode() == Emulator::Serializer::Mode::Load)
            updateDeviceState();
    }
    
    ExpansionPort::serialize(s);        
}

auto EasyFlash::createImage(unsigned& imageSize) -> uint8_t* {
    imageSize = 64 + 16 + 16 + 16 * 1024;
    
    uint8_t* buffer = new uint8_t[ imageSize ];
    std::memset(buffer, 0xff, imageSize);
    
    uint8_t header[64];
    buildHeader(&header[0], 0x20, false, true, "EasyFlash Cartridge" );
    
    std::memcpy(buffer, &header, 64);
    
    Chip chip;
    chip.bank = 0;
    chip.size = 0x2000;
    chip.type = Chip::Type::FlashRom;
    chip.addr = 0x8000;
    
    uint8_t cheader[16];
    buildChipHeader( &cheader[0], chip );
    
    std::memcpy(buffer + 64, &cheader, 16);
    
    chip.addr = 0xa000;
    buildChipHeader( &cheader[0], chip );
    
    std::memcpy(buffer + 64 + 16 + 0x2000, &cheader, 16);
    
    return buffer;
}

auto EasyFlash::setWriteProtect(bool state) -> void {
    
    writeProtect = state;
}

auto EasyFlash::isWriteProtected() -> bool {
    return writeProtect;
}

auto EasyFlash::setJumper( unsigned jumperId, bool state ) -> void {
    
    flashJumper = state;
}

auto EasyFlash::getJumper( unsigned jumperId ) -> bool {
    
    return flashJumper;
}

}
