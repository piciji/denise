
#include "retroReplay.h"
#include "../../system/system.h"

namespace LIBC64 {
    
auto RetroReplay::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
    if ((this->rom == nullptr) && (rom == nullptr))
        return;

    if (this->rom && (rom == nullptr))
        // unset
        write();        

    this->media = media;
    this->rom = rom;
    this->romSize = romSize;

    readHeader();
    
    // it seems there is no separate cartridge id defined for Nordic Replay, so don't rely on header here
    // edit: identify nordic replay by minor version
    if (media->pcbLayout && (media->pcbLayout->id != 0) )
        cartridgeId = (Interface::CartridgeId)media->pcbLayout->id;
    else if (version == 0x101 || (cartridgeId == Interface::CartridgeIdAtomicPower) ) // revision 1
        cartridgeId = Interface::CartridgeIdNordicReplay;
    else
        cartridgeId = Interface::CartridgeIdRetroReplay;
    
    if (!readChips())
        assumeChips();  
    
    std::memset(flashData, 0xff, 128 * 1024 );

    for (auto& chip : chips) {

        if (chip.bank >= 16)
            break;

        // logical bank 1 (64k) is saved first
        unsigned _b = chip.bank < 8 ? (chip.bank + 8) : (chip.bank - 8);
        
        std::memcpy(flashData + (_b << 13), chip.ptr, chip.size);
    }
}

auto RetroReplay::write() -> void {
	
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
        buildHeader(&header[0], 0x24, true, false, "RetroReplay Cartridge" );
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
    
    for (unsigned b = 0; b < 16; b++ ) {

        // logical bank 1 (64k) is saved first
        unsigned _b = b < 8 ? (b + 8) : (b - 8);
        
        if (binFormat) {
            system->interface->writeMedia(media, flashData + _b * 0x2000, 0x2000, offset);
            offset += 0x2000;

            continue;
        }
        // crt format 
        chip->bank = b;
        
        bool writeBank = !checkForEmptyFlashBank(flashData + _b * 0x2000);
                
        if ( writeBank ) {            
            chip->addr = 0x8000;
            buildChipHeader( &cheader[0], *chip );
            system->interface->writeMedia(media, &cheader[0], 16, offset);
            offset += 16;
            system->interface->writeMedia(media, flashData + _b * 0x2000, 0x2000, offset);
            offset += 0x2000;   
        }                                
    }        
}

auto RetroReplay::createImage(Emulator::Interface::Media* media, unsigned& imageSize) -> uint8_t* {
    imageSize = 64 + 16 + 8 * 1024;
    
    uint8_t* buffer = new uint8_t[ imageSize ];
    std::memset(buffer, 0xff, imageSize);
    
    uint8_t header[64];
    unsigned id = media->pcbLayout->id == Interface::CartridgeIdNordicReplay ? 1 : 0;
    buildHeader(&header[0], 0x24, true, false, (id == 1) ? "NordicReplay Cartridge" : "RetroReplay Cartridge", 0x100 + id );
    
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
    
}