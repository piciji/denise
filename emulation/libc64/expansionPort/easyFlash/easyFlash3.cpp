
#include "../../system/system.h"
#include "easyFlash3.h"

namespace LIBC64 {  
    
EasyFlash3* easyFlash3 = nullptr;  

#include "eapi-mx29640b.h"


EasyFlash3::EasyFlash3() : Cart(false, true) {
    
    this->media = nullptr;    

    ram = new uint8_t[ 32 * 1024 ];

    dataFlash = new uint8_t[ 8 * 1024 * 1024 ];

    activeSlot = &slots[0];

    flash.setData( dataFlash );

    flash.setEvents(&sysTimer);

    flash.written = []() {
        system->serializationSize += 8 * 1024 * 1024;
    };
    
    setId( Interface::ExpansionIdEasyFlash );
}

EasyFlash3::~EasyFlash3() {

    delete[] dataFlash;

    delete[] ram;
}

auto EasyFlash3::useEF1Slots() -> bool {

    return media->pcbLayout && (media->pcbLayout->id == Interface::CartridgeIdEasyFlash);
}

auto EasyFlash3::unsetRom(Emulator::Interface::Media* media) -> void {

    Slot* slot = &slots[media->id & 7];
    bool slot0 = media->id == 0;

    if ( slot->rom == nullptr )
        return;

    if(slot0 || useEF1Slots() )
        write( slot );

    if (slot0)
        this->media = media;

    slot->media = media;
    slot->rom = nullptr;
    slot->romSize = 0;
}

auto EasyFlash3::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {

    bool slot0 = media->id == 0;

    Slot* slot = &slots[media->id & 7];

    if ( (activeSlot->rom == nullptr) && (rom == nullptr) )
        return;

    if (slot0)
        this->media = media;
    else if (!useEF1Slots())
        // don't use EF Slots 1 - 7
        return;

    slot->media = media;
    slot->rom = rom;
    slot->romSize = romSize;

    this->rom = rom;
    this->romSize = romSize;
    readHeader();
    slot->binFormat = binFormat;

    this->cartridgeId = Interface::CartridgeIdEasyFlash;
        
    if ( !readChips() )
        assumeChips();

    slot->chips = chips;

    if (slot0)
        std::memset(dataFlash, 0xff, 8 * 1024 * 1024 );

    unsigned slotOffset = media->id * 1024 * 1024;
    unsigned offset = 0;
    unsigned bank;

    for(auto& chip : slot->chips) {

        bank = chip.bank;

        if (!slot0) {
            // EF1 slots
            if (bank >= 64)
                break;
        } else {
            if (bank >= 512)
                break;

            slotOffset = (bank >> 6) * 1024 * 1024;

            bank &= 63;
        }

        offset = slotOffset + (((((bank >> 3) & 7) << 1) | (bank & 7)) << 13);

        if (!chip.ptrHi) {                        
                        
            if (chip.addr & 0x2000)
                std::memcpy( dataFlash + (offset | (1 << 16)), chip.ptr, chip.size );
            else
                std::memcpy( dataFlash + offset, chip.ptr, chip.size );
            
        } else {
            // when ptrHi exists, chip size is more than 8192 bytes            
            std::memcpy( dataFlash + offset, chip.ptr, 0x2000 );
            std::memcpy( dataFlash + (offset | (1 << 16)), chip.ptrHi, std::min(chip.size - 0x2000, 0x2000) );
        }            
    }
    
    if (std::memcmp(&dataFlash[0x1800 | (1 << 16)], "eapi", 4) == 0) {
        std::memcpy(dataFlash + (0x1800 | (1 << 16)), eapi, 768);
    }
}

auto EasyFlash3::assumeChips( ) -> void {
    Cart::assumeChips( );
    
    bool toggle = false;
    for(auto& chip : chips) {
                
        chip.addr = toggle ? 0xa000 : 0x8000;
        chip.bank = chip.id >> 1;
        
        toggle ^= 1;
    }
}

auto EasyFlash3::write( Slot* slot ) -> void {

    bool writeEf1Slots = useEF1Slots();

	bool _dirty = flash.dirty;

    flash.dirty = false;

    if (!slot->media || !slot->media->guid || !_dirty || slot->writeProtect )
        return;
        
    if (!system->interface->questionToWrite(slot->media))
        return;    
    
    system->interface->truncateMedia( slot->media );
    
    unsigned offset = 0;
    
    if (!slot->binFormat) {
        uint8_t header[64];
        buildHeader(&header[0], 0x20, false, true, "EasyFlash Cartridge" );
        system->interface->writeMedia(slot->media, &header[0], 0x40, 0);
        offset += 0x40;
    }    
    
    if (!slot->chips.size()) {
        Chip chip;
        chip.size = 0x2000;
        chip.type = Chip::Type::FlashRom;
        slot->chips.push_back( chip );
    }

    Chip* chip = &slot->chips[0];
    uint8_t cheader[16];

    bool crt8k = chip->size == 0x2000;
    
    for (unsigned b = 0; b < 64; b++ ) {

        if (slot->binFormat) {
            system->interface->writeMedia(slot->media, slot->dataLo + b * 0x2000, 0x2000, offset);
            offset += 0x2000;

            system->interface->writeMedia(slot->media, slot->dataHi + b * 0x2000, 0x2000, offset);
            offset += 0x2000;

            continue;
        }
        // crt format 
        chip->bank = b;
        
        bool writeBankLo = !checkForEmptyFlashBank(slot->dataLo + b * 0x2000);
        bool writeBankHi = !checkForEmptyFlashBank(slot->dataHi + b * 0x2000);
                
        if ( writeBankLo || (!crt8k && writeBankHi) ) {            
            chip->addr = 0x8000;
            buildChipHeader( &cheader[0], *chip );
            system->interface->writeMedia(slot->media, &cheader[0], 16, offset);
            offset += 16;
            system->interface->writeMedia(slot->media, slot->dataLo + b * 0x2000, 0x2000, offset);
            offset += 0x2000;   
        }                                
        
        if (crt8k && writeBankHi) {
            chip->addr = 0xa000;
            buildChipHeader(&cheader[0], *chip);
            system->interface->writeMedia(slot->media, &cheader[0], 16, offset);
            offset += 16;
        }                

        if (writeBankHi || (!crt8k && writeBankLo) ) {
            system->interface->writeMedia(slot->media, slot->dataHi + b * 0x2000, 0x2000, offset);
            offset += 0x2000;
        }
    }    
}

auto EasyFlash3::assign( Cart* cart ) -> void {
    // don't rebuild
}

auto EasyFlash3::create( Interface::CartridgeId cartridgeId ) -> Cart* {
    // don't rebuild
    return easyFlash3;
}

auto EasyFlash3::reset(bool softReset) -> void {

    bank = 0;
    ef3Boot = true;
    game = !ef3Boot;
    exRom = true;
    LED = false;
    enableMenu = true;

    if (!softReset)
        activeSlot = &slots[0];

    std::memset(ram, 0, 32 * 1024);


    disableUlimaxForVICInFirstHalfCycle = false;

    for(auto& slot : slots) {
        slot.flashLo.reset();
        slot.flashHi.reset();
    }
}

auto EasyFlash3::customButton() -> void { // menu button

    activeSlot = &slots[0];

    bank = 0;

    ef3Boot = true;

    enableMenu = true;
}

auto EasyFlash3::updateDeviceState() -> void {
    
    system->interface->updateDeviceState( media, false, 0, LED, true );
}

auto EasyFlash3::isBootable( ) -> bool {
    return true;
}

auto EasyFlash3::readIo1( uint16_t addr ) -> uint8_t {

    addr &= 0xf;

    if ((mode != Mode::EF1) && (addr == 1) )
        return activeSlot->media->id;

    return ExpansionPort::readIo1(addr);
}

auto EasyFlash3::writeIo1( uint16_t addr, uint8_t value ) -> void {

    if (mode == Mode::EF1) {
        if ((addr & 2) == 0)
            bank = value & 0x3f;
        else
            control( value );

        // in EF1 you can not reach the modes, like AR, SS or Kernal
        return;
    }
    // ef3 modes
    addr &= 0xf;

    if (addr == 0)
        bank = value & 0x3f;
    else if (addr == 1)
        activeSlot = &slots[value & 7];
    else if (addr == 2)
        control( value );
    else if (enableMenu && (addr == 0xf) ) { // cart mode
        enableMenu = false;

        switch( value & 0xf ) {
            case 0:
                mode = Mode::EF3;   // this is not to switch back from EF1 but AR, SS or Kernal
                reset();
                break;
            case 1:
                mode = Mode::EF3;
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                break;

        }
    }
}

auto EasyFlash3::control( uint8_t value ) -> void {
    bool LEDNew = value & 0x80;

    if (LED != LEDNew) {
        LED = LEDNew;
        updateDeviceState();
    }

    disableUlimaxForVICInFirstHalfCycle = (mode != Mode::EF1) && !!(value & 8);

    value = ~value;

    exRom = (value >> 1) & 1;
    game = value & 1;

    if (value & 4)
        game = !ef3Boot;

    system->changeExpansionPortMemoryMode( exRom, game );
}

auto EasyFlash3::writeIo2( uint16_t addr, uint8_t value ) -> void {

    addr &= 0x1fff;

    ram[ addr ] = value;
}

auto EasyFlash3::readIo2( uint16_t addr ) -> uint8_t {

    addr &= 0x1fff;

    return ram[ addr ];
}

auto EasyFlash3::readRomL( uint16_t addr ) -> uint8_t {
    
    return activeSlot->flashLo.read( bank * 0x2000 + (addr & 0x1fff) );
}

auto EasyFlash3::writeRomL( uint16_t addr, uint8_t data ) -> void {

    activeSlot->flashLo.write( bank * 0x2000 + (addr & 0x1fff), data );
    
    ExpansionPort::writeRomL( addr, data );
}

auto EasyFlash3::readRomH( uint16_t addr ) -> uint8_t {
    
    return activeSlot->flashHi.read( bank * 0x2000 + (addr & 0x1fff) );
}

auto EasyFlash3::writeRomH( uint16_t addr, uint8_t data ) -> void {

    activeSlot->flashHi.write( bank * 0x2000 + (addr & 0x1fff), data );
    
    ExpansionPort::writeRomH( addr, data );
}

auto EasyFlash3::writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {

    activeSlot->flashLo.write( bank * 0x2000 + (addr & 0x1fff), data );

    ExpansionPort::writeUltimaxRomL( addr, data );
}

auto EasyFlash3::writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void {

    activeSlot->flashHi.write( bank * 0x2000 + (addr & 0x1fff), data );

    ExpansionPort::writeUltimaxRomH( addr, data );
}

auto EasyFlash3::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( (uint16_t&)cartridgeId );    
    
    s.integer( bank );

    s.integer( LED );

    if ( mode == Mode::EF1) {
        serializeSlot(activeSlot, s);

    } else {
        for(auto& slot : slots)
            serializeSlot( &slot, s );

        s.array( ram, 32 * 1024 );
    }

    if (!s.lightUsage() && s.mode() == (Emulator::Serializer::Mode::Load) )
        updateDeviceState();

    ExpansionPort::serialize(s);        
}

auto EasyFlash3::serializeSlot(Slot* slot, Emulator::Serializer& s) -> void {
    s.integer( slot->writeProtect );

    slot->flashLo.serialize(s);
    slot->flashHi.serialize(s);

    if (!s.lightUsage()) {
        if (slot->flashLo.dirty)
            s.array(slot->dataLo, 512 * 1024);

        if (slot->flashHi.dirty)
            s.array(slot->dataHi, 512 * 1024);
    }
}

auto EasyFlash3::createImage(unsigned& imageSize) -> uint8_t* {
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

auto EasyFlash3::setWriteProtect( Emulator::Interface::Media* media, bool state ) -> void {
    Slot* slot = &slots[media->id & 7];

    slot->writeProtect = state;
}

auto EasyFlash3::isWriteProtected( Emulator::Interface::Media* media ) -> bool {
    Slot* slot = &slots[media->id & 7];

    return slot->writeProtect;
}


}
