
#include "../../system/system.h"
#include "easyFlash3.h"
#include "../../../tools/petcii.h"

namespace LIBC64 {  
    
EasyFlash3* easyFlash3 = nullptr;  

#include "eapi-mx29640b.h"

EasyFlash3::EasyFlash3() : FreezeButton(false, true) {
    
    this->media = nullptr;    

    ram = new uint8_t[ 32 * 1024 ];

    dataFlash = new uint8_t[ 8 * 1024 * 1024 ];

    activeSlot = 0;
    
    unbeatable = 0xff;
	
	loadSplitted = true;

    flash.setData( dataFlash );

    flash.setEvents(&sysTimer);

    flash.written = [this]() {

        Slot* slot = &slots[activeSlot];
        
        if (slot->dirty)
            return;

        slot->dirty = true;

        system->serializationSize += 1 * 1024 * 1024;
    };
    
    setId( Interface::ExpansionIdEasyFlash3 );
}

EasyFlash3::~EasyFlash3() {

    delete[] dataFlash;

    delete[] ram;
}

auto EasyFlash3::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {

    bool slot0 = media->id == 0;
    
    Slot* slot = &slots[media->id & 7];

    if (slot0) {
        this->media = media;
        loadSplitted = media->pcbLayout->id == 0;
    }

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

    unsigned slotOffset = media->id * 1024 * 1024;

    if (slot0)
        std::memset(dataFlash, 0xff, 8 * 1024 * 1024 );

    if (!loadSplitted && !slot0)
        return;

    unsigned offset = 0;
    unsigned bank;

    for(auto& chip : slot->chips) {

        bank = chip.bank;

        if (!slot0 || loadSplitted) {
            // EF1 slots
            if (bank >= 64)
                continue;
        } else {
            if (bank >= 512)
                continue;

            slotOffset = (bank >> 6) * 1024 * 1024;

            bank &= 63;
        }

        offset = slotOffset + (((((bank >> 3) & 7) << 4) | (bank & 7)) << 13);

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
    
    if (loadSplitted && !slot0)
        updateSlotDisplayName( slot );
    
    // overwriting eapi of EF1 CRT's. for EF3 this shouldn't be needed anymore
    if (loadSplitted) {
        if (std::memcmp(&dataFlash[slotOffset | 0x1800 | (1 << 16)], "eapi", 4) == 0)
            std::memcpy(dataFlash + (slotOffset | 0x1800 | (1 << 16)), eapi, 768);        
    } else {
        for (unsigned i = 1; i < 8; i++) {
            slotOffset = i * 1024 * 1024;
            
            if (std::memcmp(&dataFlash[slotOffset | 0x1800 | (1 << 16)], "eapi", 4) == 0)
                std::memcpy(dataFlash + (slotOffset | 0x1800 | (1 << 16)), eapi, 768);  
        }
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

auto EasyFlash3::unsetRom(Emulator::Interface::Media* media) -> void {

    Slot* slot = &slots[media->id & 7];
    
    bool _dirty = slot->dirty;
    slot->dirty = false;
    
    if (!slot->rom)
        return;
    
    bool slot0 = media->id == 0;
    bool saveSplitted = !slots[0].media || (slots[0].media->pcbLayout->id == 0);

    if (loadSplitted != saveSplitted)
        _dirty = true;
    else if (!saveSplitted) {
        for(unsigned i = 1; i < 8; i++)
            _dirty |= slots[i].dirty;
    }

    if (saveSplitted || slot0) {
        if (slot->rom && slot->media && slot->media->guid && _dirty && !slot->writeProtect ) {
            if (system->interface->questionToWrite(slot->media))
                write( slot, saveSplitted );
        }
    }

    if (slot0)
        this->media = media;

    slot->media = media;
    slot->rom = nullptr;
    slot->romSize = 0;
}

auto EasyFlash3::write( Slot* slot, bool splitted ) -> void {
    
    system->interface->truncateMedia( slot->media );
    
    unsigned offset = 0;
    
    if (!slot->binFormat) {
        uint8_t header[64];
        
        if (slot->media->id == 0)
            buildHeader(&header[0], 0x20, false, true, "EasyFlash Cartridge" );
        else {
            buildHeader(&header[0], 0x20, false, true, "" );
            updateSlotHeaderName( slot, header + 0x20 );
        }
        
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
    unsigned slotOffset = slot->media->id * 1024 * 1024;

    unsigned maxBank = splitted ? 64 : (64 * 8);

    for (unsigned b = 0; b < maxBank; b++) {

        unsigned slotBank = b;

        if (!splitted) {
            slotOffset = (b >> 6) * 1024 * 1024;
            slotBank = b & 63;
        }

        unsigned bankLo = (((slotBank >> 3) & 7) << 4) | (slotBank & 7);
        unsigned bankHi = bankLo | (1 << 3);
        bankLo <<= 13;
        bankHi <<= 13;

        if (slot->binFormat) {
            system->interface->writeMedia(slot->media, dataFlash + slotOffset + bankLo, 0x2000, offset);
            offset += 0x2000;

            system->interface->writeMedia(slot->media, dataFlash + slotOffset + bankHi, 0x2000, offset);
            offset += 0x2000;

            continue;
        }
        // crt format
        chip->bank = b;

        bool writeBankLo = !checkForEmptyFlashBank(dataFlash + slotOffset + bankLo);
        bool writeBankHi = !checkForEmptyFlashBank(dataFlash + slotOffset + bankHi);

        if (writeBankLo || (!crt8k && writeBankHi)) {
            chip->addr = 0x8000;
            buildChipHeader(&cheader[0], *chip);
            system->interface->writeMedia(slot->media, &cheader[0], 16, offset);
            offset += 16;
            system->interface->writeMedia(slot->media, dataFlash + slotOffset + bankLo, 0x2000, offset);
            offset += 0x2000;
        }

        if (crt8k && writeBankHi) {
            chip->addr = 0xa000;
            buildChipHeader(&cheader[0], *chip);
            system->interface->writeMedia(slot->media, &cheader[0], 16, offset);
            offset += 16;
        }

        if (writeBankHi || (!crt8k && writeBankLo)) {
            system->interface->writeMedia(slot->media, dataFlash + slotOffset + bankHi, 0x2000, offset);
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
    
    if (!softReset) {
        mode = Mode::EF3;
        activeSlot = 0;
        bank = 0;
        ef3Boot = true;
        enableMenu = true;
        std::memset(ram, 0, 32 * 1024);
        flash.reset();
        LED = false;
		
		if (!slots[0].rom)
			std::memset(dataFlash, 0xff, 8 * 1024 * 1024 );
        
    } else if (LED) {
        LED = false;
        updateDeviceState();
    }

    exRom = true;
    
    if (mode == Mode::Kernal || mode == Mode::Disable)
        game = true;
    else if (mode == Mode::EF3)
        game = !ef3Boot;
    else if (mode == Mode::SS5)
        game = false;
    else if (mode == Mode::FC3) {
        game = false;
        exRom = false;
    } else if (mode == Mode::AR) {
        game = true;
        exRom = false;
    }
      
    cartKill = false;
    useRam = false;
    writeOnce = false;
    reuMapping = false;
    npMode = false;
    
    romHLine = false;
    
    portUpdated = false; 
    
    buildFlashBaseAdr();
}

auto EasyFlash3::resetButton() -> bool {
    
    if (mode == Mode::EF3) {        
        ef3Boot = true;      
        bank = 0;
        system->power(true);
        
    } else if (mode == Mode::Kernal) { 
        system->power(true);
        
    } else if (mode == Mode::SS5 || mode == Mode::AR || mode == Mode::FC3) {
        if (mode != Mode::FC3)
            bank &= 0x38; // keep hi bank
        else
            bank = 5 << 3;        
            
        system->power(true);
    }    
    
    return true; // override default reset
}

auto EasyFlash3::customButton() -> void { // menu button

    activeSlot = 0;

    bank = 0;

    ef3Boot = true;

    enableMenu = true;

    mode = Mode::EF3;

    system->power(true);
}

auto EasyFlash3::freeze() -> void { // special button in EF3

    if (mode == Mode::EF3) {
        ef3Boot = false;    
        system->power(true);
        
    } else if (mode == Mode::SS5 || mode == Mode::AR || mode == Mode::FC3) {
        FreezeButton::freeze();
    }
}

auto EasyFlash3::didFreeze() -> void {
    
    if (mode == Mode::SS5 || mode == Mode::AR || mode == Mode::FC3) {
        
        if (mode != Mode::FC3) {
            bank &= 0x38;
            buildFlashBaseAdr();
        }
                
        cartKill = false;
        useRam = false;
        
        if (!LED) {
            LED = true;
            updateDeviceState();
        }
    }
}

auto EasyFlash3::updateDeviceState() -> void {
    
    system->interface->updateDeviceState( media, false, 0, LED, true );
}

auto EasyFlash3::isBootable( ) -> bool {
    return true;
}

auto EasyFlash3::readIo1( uint16_t addr ) -> uint8_t {

    if (mode == Mode::EF3 || mode == Mode::Kernal) {
    
        if ((addr & 0xf0) == 0) {
            addr &= 0xf;
            
            if (mode == Mode::EF3) {                        
                if (addr == 1)
                    return activeSlot;        
            }

            if (addr == 8)
                return 0x51;    // CPLD version, Firmware
            if (addr == 9)      // USB Transfer not emulated
                return 0;       // Bit 7 -> RXR, Bit 6 -> TXR            
        }            
    } else if ( (mode == Mode::SS5) && !cartKill) {
        
        return flash.read( flashBaseAdr | (addr & 0x1fff) ); // 0xde00 & 0x1fff => 0x1e00
        
    } else if (mode == Mode::FC3) {
        
        return flash.read( flashBaseAdr | (addr & 0x1fff) ); // 0xde00 & 0x1fff => 0x1e00
        
    } else if ( (mode == Mode::AR) && !cartKill) {
        
        switch(addr & 0xff) {
            case 0:
            case 1:                
                return ((bank & 4) << 5) | (reuMapping << 6) | ((bank & 3) << 3);
                
            default:
                if (reuMapping) {
                    if (useRam)
                        return ram[addr & 0x1fff]; // 0xde00 & 0x1fff => 0x1e00
                    
                    return flash.read( flashBaseAdr | (1 << 16) | (addr & 0x1fff) );
                }
                break;
        }       
    }

    return ExpansionPort::readIo1(addr);
}

auto EasyFlash3::writeIo1( uint16_t addr, uint8_t value ) -> void {

    bool LEDNew = LED;
    bool _addr0x = (addr & 0xf0) == 0;
    uint8_t _addr = addr & 0xf;
    
    if (mode == Mode::EF3) {
               
        if (_addr0x) {            
            if (_addr == 0) {
                bank = value & 0x3f;
                buildFlashBaseAdr();
                
            } else if (_addr == 1) {
                activeSlot = value & 7;
                buildFlashBaseAdr();
                
            } else if (_addr == 2) {
                LEDNew = value & 0x80;

                bool disableUlimaxForVICInFirstHalfCycle = !!(value & 8);

                value = ~value;

                exRom = (value >> 1) & 1;
                game = value & 1;

                if (value & 4)
                    game = !ef3Boot;

                system->changeExpansionPortMemoryMode(exRom, game);

                if (disableUlimaxForVICInFirstHalfCycle)
                    vicII->setUltimaxPhi1(false);
                
            } else if (_addr == 0xf) {
                
                if (enableMenu) {
                    enableMenu = false;
                    mode = Mode::Disable;
                    unbeatable = 0xff;

                    switch (value & 0xf) {
                        case 0:
                            mode = Mode::EF3;
                            system->power(true);
                            break;
                        case 1:
                            mode = Mode::EF3;
                            break;
                        case 2:
                            mode = Mode::Kernal;
                            system->power(true);
                            break;
                        case 3:
                            mode = Mode::FC3;
                            unbeatable &= ~2; // unbeatable without IRQ
                            system->power(true);
                            break;
                        case 4:
                            mode = Mode::AR;
                            system->power(true);
                            break;
                        case 5:
                            mode = Mode::SS5;
                            system->power(true);
                            break;
                        case 6:
                            // for C128 only, behaves like case 7 for C64
                        case 7:
                            // disable EF3 cartridge
                            system->power(true);
                            break;
                    }                                            
                                            
                    return;
                }
            }
        }
    } else if (mode == Mode::Kernal) {
        LEDNew = false;
        
        if (_addr0x) {            
            if (_addr == 0xe) {
                bank = value & 7;
                buildFlashBaseAdr();
            }
        }
        
    } else if (mode == Mode::SS5 && !cartKill) {
        
        bank &= 0x38; // hi bank will be set by Easy Flash
        bank |= ((value >> 3) & 2) | ((value >> 2) & 1);
        game = value & 1;
        exRom = ((value >> 1) & 1) ^ 1;
        cartKill = (value >> 3) & 1;
        
        LEDNew = !cartKill;
        
        buildFlashBaseAdr();
        system->changeExpansionPortMemoryMode( exRom, game );
        
        if (game) {
            nmiCall(false);
            irqCall(false);
        }
        
    } else if (mode == Mode::AR && !cartKill) {
        
        switch(addr & 0xff) {
            case 0:
                cartKill = (value >> 2) & 1;
                game = (value & 1) ^ 1;
                exRom = (value >> 1) & 1;
                
                if (value & 0x40) {
                    nmiCall(false);
                    irqCall(false);
                }                                
                
                LEDNew = !cartKill;
                
                // fall through
            case 1:                
                useRam = !!(value & 0x20);
                bank &= 0x38; // hi bank will be set by Easy Flash
                bank |= ((value >> 3) & 3) | ((value >> 5) & 4);
                buildFlashBaseAdr();
                
                npMode = game && exRom && useRam;
                
                if (npMode)
                    game = exRom = false;               
                
                system->changeExpansionPortMemoryMode( exRom, game );

                if (addr & 1) {
                    if (!writeOnce) {
                        writeOnce = true;
                        reuMapping = !!(value & 0x40);
                        vicII->setUltimaxPhi1(false);
                    }
                }

                break;
                
            default:
                if (reuMapping) {
                    if (useRam)
                        ram[addr & 0x1fff] = value; // 0xde00 & 0x1fff => 0x1e00
                }
    
                break;
        }
        
    } else if (mode == Mode::FC3);
        
    else
        LEDNew = false;
    
    if (LED != LEDNew) {
        LED = LEDNew;
        updateDeviceState();
    }    
}

auto EasyFlash3::writeIo2( uint16_t addr, uint8_t value ) -> void {

    bool LEDNew = LED;
    
    if (mode == Mode::EF3 || mode == Mode::Kernal) {        
        // ram bank 0
        ram[addr & 0x1fff] = value;
        
    } else if ( (mode == Mode::AR) && !cartKill) {

        if (!reuMapping) {
            if (useRam)
                ram[addr & 0x1fff] = value; // 0xdf00 & 0x1fff => 0x1f00
        }
    } else if ( (mode == Mode::FC3) && !cartKill && ((addr & 0xff) == 0xff) ) {
        bank &= 0x20;
        bank |= value & 7;
        
        if (value & 8)
            bank |= 2 << 3;
        else
            bank |= 1 << 3;
        
        exRom = (value >> 4) & 1;
        game = (value >> 5) & 1;
        nmiCall( (value & 0x40) == 0 );
        cartKill = (value & 0x80) != 0;

        LEDNew = !cartKill;

        buildFlashBaseAdr();
        system->changeExpansionPortMemoryMode(exRom, game);        
    }

    if (LED != LEDNew) {
        LED = LEDNew;
        updateDeviceState();
    }   
}

auto EasyFlash3::readIo2( uint16_t addr ) -> uint8_t {

    if (mode == Mode::EF3 || mode == Mode::Kernal) {
        // ram bank 0
        return ram[addr & 0x1fff];
       
    } else if (mode == Mode::FC3) {
        
        return flash.read( flashBaseAdr | (addr & 0x1fff) ); // 0xdf00 & 0x1fff => 0x1f00
       
    } else if ( (mode == Mode::AR) && !cartKill) {

        if (!reuMapping) {
            if (useRam)
                return ram[addr & 0x1fff]; // 0xdf00 & 0x1fff => 0x1f00

            return flash.read( flashBaseAdr | (1 << 16) | (addr & 0x1fff) );
        }
    }
    
    return ExpansionPort::readIo2(addr);
}


auto EasyFlash3::buildFlashBaseAdr() -> void {

    flashBaseAdr = (activeSlot << 20) | (((((bank >> 3) & 7) << 4) | (bank & 7)) << 13);
}

auto EasyFlash3::readRomL( uint16_t addr ) -> uint8_t {

    if (mode == Mode::EF3) {
        return flash.read( flashBaseAdr | (addr & 0x1fff) );
        
    } else if (!cartKill && mode == Mode::SS5) {
        
        if (exRom)
            return ram[ ((bank & 3) << 13) | (addr & 0x1fff) ];
            
        return flash.read( flashBaseAdr | (addr & 0x1fff) );
        
    } else if (!cartKill && mode == Mode::AR) {
        
        if (useRam && !npMode)
            return ram[ ((bank & 3) << 13) | (addr & 0x1fff) ];
            
        return flash.read( flashBaseAdr | (1 << 16) | (addr & 0x1fff) );
        
    } else if (mode == Mode::FC3) {
            
        return flash.read( flashBaseAdr | (addr & 0x1fff) );
    }

    return ExpansionPort::readRomL( addr );
}

auto EasyFlash3::writeRomL( uint16_t addr, uint8_t data ) -> void {

    if (mode == Mode::EF3) {
        flash.write( flashBaseAdr | (addr & 0x1fff), data );
        
    } else if (!cartKill && mode == Mode::SS5) {
        
        if (exRom)
            ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
        
    } else if (!cartKill && mode == Mode::AR) {
        
        if (useRam && !npMode)
            ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
    }
    
    ExpansionPort::writeRomL( addr, data );
}

auto EasyFlash3::readRomH( uint16_t addr ) -> uint8_t {

    if (mode == Mode::EF3) {
        return flash.read( flashBaseAdr | (1 << 16) | (addr & 0x1fff) );
        
    } else if (mode == Mode::Kernal) {
        romHLine = true; // PLA signals
        
        if (!kernalHack || (system->mode & 2) ) // in hack mode we check directly for HIRAM (the real cart can't do this)
            return flash.read( flashBaseAdr | (addr & 0x1fff) );
        
        return system->readRam( addr ); // hack
        
    } else if (!cartKill && mode == Mode::SS5) {
        
        return flash.read( flashBaseAdr | (1 << 16) | (addr & 0x1fff) );
        
    } else if (!cartKill && mode == Mode::AR) {
        
        if (npMode)
            return ram[ addr & 0x1fff ];
        
        return flash.read( flashBaseAdr | (1 << 16) | (addr & 0x1fff) );
        
    } else if (mode == Mode::FC3) {
            
        return flash.read( flashBaseAdr | (1 << 16) | (addr & 0x1fff) );
    }

    return ExpansionPort::readRomL( addr );
}

auto EasyFlash3::writeRomH( uint16_t addr, uint8_t data ) -> void {

    if (mode == Mode::EF3) {
        flash.write( flashBaseAdr | (1 << 16) | (addr & 0x1fff), data );
        
    } else if (!cartKill && mode == Mode::AR) {
        if (npMode)
            ram[ addr & 0x1fff ] = data;
    }
    
    ExpansionPort::writeRomH( addr, data );
}

auto EasyFlash3::writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {

    if (mode == Mode::EF3) {
        flash.write( flashBaseAdr | (addr & 0x1fff), data );
        
    } else if (!cartKill && mode == Mode::SS5) {
        
        if (exRom)
            ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
        
    } else if (!cartKill && mode == Mode::AR) {
        
        if (useRam && !npMode)
            ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
    }

    ExpansionPort::writeUltimaxRomL( addr, data );
}

auto EasyFlash3::writeUltimaxRomH( uint16_t addr, uint8_t data ) -> void {

    if (mode == Mode::EF3) {
        flash.write( flashBaseAdr | (1 << 16) | (addr & 0x1fff), data );
        
    } else if (!cartKill && mode == Mode::AR) {
        if (npMode)
            ram[ addr & 0x1fff ] = data;
    }

    ExpansionPort::writeUltimaxRomH( addr, data );
}

auto EasyFlash3::listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void {
    
    if (!cartKill && mode == Mode::AR) {
        
        if (!npMode && useRam)
            ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
    }
}

auto EasyFlash3::listenToWritesAtA0ToBF(uint16_t addr, uint8_t data ) -> void {
    
    if (!cartKill && mode == Mode::AR) {
        
        if (npMode)
             ram[ addr & 0x1fff ] = data;
    }
}

auto EasyFlash3::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( (uint16_t&)cartridgeId );
    
    s.integer( (uint16_t&)mode );
    
    s.integer( bank );

    s.integer( LED );

    s.integer( ef3Boot );

    s.integer( enableMenu );

    s.integer( flashBaseAdr );
    
    s.integer( activeSlot );

    s.integer( loadSplitted );
    
    s.integer( romHLine );
    
    s.integer( portUpdated );
    
    s.integer( cartKill );
    
    s.integer( useRam );
    
    s.integer( writeOnce );
    
    s.integer( reuMapping );
    
    s.integer( npMode );

    s.array( ram, 32 * 1024 );

    flash.serialize(s);

    for(auto& slot : slots) {

        s.integer( slot.writeProtect );

        s.integer( slot.dirty );

        if (slot.dirty && !s.lightUsage())
            s.array(dataFlash + slot.media->id * 1024 * 1024, 1024 * 1024);
    }

    if (!s.lightUsage() && s.mode() == (Emulator::Serializer::Mode::Load) )
        updateDeviceState();

    FreezeButton::serializeCustom(s);        
}

auto EasyFlash3::setWriteProtect( Emulator::Interface::Media* media, bool state ) -> void {
    Slot* slot = &slots[media->id & 7];

    slot->writeProtect = state;
}

auto EasyFlash3::isWriteProtected( Emulator::Interface::Media* media ) -> bool {
    Slot* slot = &slots[media->id & 7];

    return slot->writeProtect;
}

auto EasyFlash3::memoryMapUpdated() -> void {
    
    if (kernalHack && (mode == Mode::Kernal) )
        system->memoryCpu.map(&system->readRomH, 0xe0, 0xff);
}

auto EasyFlash3::clock() -> void {
    // this is the real way to do a KERNAL replacement, but it could be slow if CPU port is updated too often, hence the hack (no compatibility loss)      
    
    if (!kernalHack && (mode == Mode::Kernal) && !vicBA()) {
        
        // we are between the half cycles here and VIC hasn't BUS in second half cycle
        // this emulates PHI2 (second half cycle ~ 500 ns)
        // CPU has putted address already on BUS
        
        bool _write = cpu->isWriteCycle();
        uint16_t _addr = cpu->addressBus();
        
        if ( !_write && ((_addr & 0xe000) == 0xe000) ) { // read goes to KERNAL area
            
            if (portUpdated) {  // CPU port was updated before, so re check HIRAM state
                
                // the HIRAM state distinguish between KERNAL and memory access
                // expansion port can't see state of HIRAM directly, so switch in 16k mode
                // in 16k mode a ROMH access in area 0xa000 - 0xbfff always represents HIRAM state.
                system->changeExpansionPortMemoryMode( exRom = false, game = false );

                romHLine = false;

                // expansion can modify address BUS, so pull down A14 -> 0
                // this changes addresses within 0xe000 - 0xffff to 0xa000 - 0xbfff
                // PLA is accessing modified address immediatly and expansion listens to ROMH line.                
                
                // ~325 ns
                // test if ROM-H line is changing
                system->memoryCpu.read( _addr & ~0x4000 ); 
                
                // now we know if a read in KERNAL area would access RAM or KERNAL data.
                
                // we only have to recheck, if CPU port is written again
                portUpdated = false;
                
                // [fast way] for speed reasons: go back to non cartridge mode and overmap only the KERNAL area (if needed)
                system->changeExpansionPortMemoryMode( exRom = true, game = true );
            }
            
            // at this point we know if we have to provide a KERNAL access or not.
            // in case of KERNAL access, we have to switch to Ultimax mode to provide KERNAL data from cartridge instead of built in KERNAL
            
            // ~360 ns
            // [right way]           
            // system->changeExpansionPortMemoryMode( exRom = true, game = !romHLine, true );
            
            // some explantions to last parameter 'true' in function call one line before
            // it's important to disable Ultimax at the end of PHI2 already, so VIC is not in Ultimax anymore in first half cycle
            // from a CPU point of view switching 'exRom' and 'Game' in special situations like accessing custom addresses is emulated hack free in Denise.
            // within these 'clock' functions (running between the half cycles) each expansion can update the PLA before CPU is accessing but after CPU has puted address on BUS.
            // from a VIC point of view it's harder because the VIC can access two times a cycle and it's possible to set Ultimax or not for each VIC (half)cycle separately.
            // thats why a hack, so you can permanently set Ultimax or not for VIC PHI1 and PHI2 independently without updating the memory map each half cycle.
            // so i.e. VIC runs not in Ultimax in PHI1 but memory map for CPU accesses runs in Ultimax in PHI2
            // of course in real system cartridge would switch Ultimax on/off each half cycle in this situation.
            
            if (romHLine)
                // [fast way] overmap only the KERNAL area in case of Ultimax.
                system->memoryCpu.map(&system->readRomH, 0xe0, 0xff);

        } else if ( _write && (_addr == 0 || _addr == 1)  ) {

            portUpdated = true;
        }
    } else if (mode == Mode::SS5 || mode == Mode::AR || mode == Mode::FC3)
        FreezeButton::clock();
}

auto EasyFlash3::updateSlotDisplayName(Slot* slot) -> void {
    
    Emulator::PetciiConversion petcii;
    
    unsigned bank = (1 << 5) << 13; // bank 16 Lo is DIR bank
    bank += 16 + (slot->media->id * 16);
       
    if (!slot->binFormat) {
        
        if (std::memcmp(cartName, "EasyFlash", 8) && std::memcmp(cartName, "EASYFLASH", 8) ) {
            
            if (std::memcmp(cartName, "VICE", 4)) {
                
                for(uint8_t& c : cartName) {
                    c = petcii.encode( c );
                    if (c == petcii.PETSCII_UNMAPPED)
                        c = 0;
                }
                
                // ok use cart name as EF3 menu slot display name
                std::memcpy(dataFlash + bank, &cartName[0], 16);
                return;
            }
        }
    }
    
    // use filename
    std::string fileName = system->interface->getFileNameFromMedia( slot->media );
        
    uint8_t out[16];
    std::memset(&out[0], 0x20, sizeof out);
    
    for (unsigned i = 0; i < std::min(16u, (unsigned)fileName.size()); i++)
        out[i] = petcii.encode( fileName[i] );
    
    std::memcpy(dataFlash + bank, &out[0], sizeof out);
}

auto EasyFlash3::updateSlotHeaderName(Slot* slot, uint8_t* header) -> void {
    
    Emulator::PetciiConversion petcii;
    
    if (slot->binFormat)
        return;

    unsigned bank = (1 << 5) << 13; // bank 16 Lo is DIR bank
    bank += 16 + (slot->media->id * 16);

    std::memcpy( &header[0], dataFlash + bank, 16 );

    for (uint8_t i = 0; i < 16; i++)
        header[i] = petcii.decode( header[i] );
}

}
