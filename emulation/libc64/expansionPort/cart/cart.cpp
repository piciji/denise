
#include "cart.h"
#include "../../../tools/buffer.h"
#include "../../system/system.h"

namespace LIBC64 {
    
Cart::Cart(System* system, bool game, bool exrom) : ExpansionPort(system) {
    
    this->game = game;
    this->exRom = exrom;
    
    cRomH = nullptr;
    cRomL = nullptr;
    
    rom = nullptr;
    media = nullptr;
    romSize = 0;
    cartridgeId = Interface::CartridgeIdNoRom;
    
    data = nullptr;
    size = 0;
    binFormat = false;
    version = 0;
    cartridgeRevision = 0;
}

auto Cart::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
    if ( (this->rom == nullptr) && (rom == nullptr) )
        return;

    this->media = media;
    this->rom = rom;
    this->romSize = romSize;

    readHeader();

    if ( !readChips() )
        assumeChips();
}

auto Cart::readHeader( ) -> bool {
    
    data = rom;
    size = romSize;
    binFormat = true;    
    std::memset(&cartName[0], 0, sizeof cartName);
    
    if (!rom)
        return false;
    
    uint8_t header[64];
    
    if (size < (sizeof header))
        return false;

    std::memcpy(header, data, sizeof header);

    if (std::memcmp(header, "C64 CARTRIDGE   ", 16))
        return false;

    uint32_t headerLength = Emulator::copyBufferToIntBigEndian<uint32_t>(&header[0x10]);

    if (headerLength < (sizeof header))
        return false;

    if (size < headerLength)
        return false;
    
    data += headerLength;
    size -= headerLength;        
    
    cartridgeId = (Interface::CartridgeId)Emulator::copyBufferToIntBigEndian<uint16_t>(&header[0x16]);
    
    std::memcpy(&cartName[0], &header[0x20], sizeof cartName);
    
    version = Emulator::copyBufferToIntBigEndian<uint16_t>(&header[0x14]);
    cartridgeRevision = header[0x1A];
    exRom = header[0x18] & 1;
    game = header[0x19] & 1;
    binFormat = false;
    return true;
}

auto Cart::readChips() -> bool {
    chips.clear();
    
    if (!data || (size == 0) )
        return false;
        
    uint8_t* ptr = data;
    unsigned offset = 0;
    unsigned id = 0;
    
    uint8_t cheader[16]; //chip header
    
    while(true) {
                        
        offset += sizeof cheader;
        
        if ( offset >= size )
            break;
        
        std::memcpy(cheader, ptr, sizeof cheader);
        
        if (std::memcmp(cheader, "CHIP", 4))
            break;
        
        ptr += sizeof cheader;
        
        Chip chip;
        chip.id = id++;
        chip.type = (Chip::Type)Emulator::copyBufferToIntBigEndian<uint16_t>(&cheader[0x8]);
        chip.bank = Emulator::copyBufferToIntBigEndian<uint16_t>(&cheader[0xa]);
        chip.addr = Emulator::copyBufferToIntBigEndian<uint16_t>(&cheader[0xc]);
        chip.size = Emulator::copyBufferToIntBigEndian<uint16_t>(&cheader[0xe]);
        chip.offset = offset;
        chip.ptr = ptr;        
		
        offset += chip.size;
        
        if (offset > size)
            chip.size -= offset - size;
        
        if (chip.size == 0)
            break;
        
        chip.ptrHi = chip.size > 8192 ? chip.ptr + 8192 : nullptr;
        
        chips.push_back( chip );

        if (offset >= size)
            break;
        
        ptr += chip.size;
    }
    
    if ( chips.size() == 0 )
        return false;
    
    return true;
}

auto Cart::assumeChips( ) -> void {
    
    assumeChips( {8192} );
}

auto Cart::assumeChips( std::vector<unsigned> sizes ) -> void {
    
    if (!data || (size == 0) )
        return;
    
    uint8_t* ptr = data;
    unsigned offset = 0;
    unsigned id = 0;
    unsigned lastChipSize = 8192;
    
    while(true) {
        
        lastChipSize = id < sizes.size() ? sizes[id] : lastChipSize;
        
        Chip chip;
        chip.size = lastChipSize;
        chip.ptr = ptr;        
        chip.id = id;
        chip.bank = id++;
                
        offset += chip.size;
        
        if (offset > size)
            chip.size -= offset - size;
        
        if (chip.size == 0)
            break;
        
        chip.ptrHi = chip.size > 8192 ? chip.ptr + 8192 : nullptr;
        
        chips.push_back( chip );    
        
        if (offset >= size)
            break;
        
        ptr += chip.size;
    }        
}

auto Cart::peekRomL(uint16_t addr) -> uint8_t {
    return Cart::readRomL(addr);
}

auto Cart::readRomL(uint16_t addr) -> uint8_t {
	
    if (!cRomL)
        return ExpansionPort::readRomL( addr );

    addr %= cRomL->size;
    
    return *(cRomL->ptr + addr);
}

auto Cart::peekRomH(uint16_t addr) -> uint8_t {
    return Cart::readRomH(addr);
}

auto Cart::readRomH(uint16_t addr) -> uint8_t {

    if (!cRomH)
        return ExpansionPort::readRomH( addr );
    
    if (cRomH->ptrHi) {		
		
        addr %= cRomH->size - 8192;				
        
        return *( cRomH->ptrHi + addr);        
    }
	
    addr %= cRomH->size;		

    return *(cRomH->ptr + addr);
}

auto Cart::reset(bool softReset) -> void {
    
    cRomL = getChip(0);
    
    cRomH = chips.size() > 1 ? getChip(1) : getChip(0);
}

auto Cart::serialize(Emulator::Serializer& s) -> void {
    
    int romLId = cRomL ? (int)cRomL->id : -1;
    int romHId = cRomH ? (int)cRomH->id : -1;

    s.integer(romLId);
    s.integer(romHId);

    if (s.mode() == Emulator::Serializer::Mode::Load) {

        cRomL = ((romLId >= 0) && (romLId < chips.size())) ? &chips[romLId] : nullptr;
        cRomH = ((romHId >= 0) && (romHId < chips.size())) ? &chips[romHId] : nullptr;
    }

    ExpansionPort::serialize(s);
}

auto Cart::buildHeader(uint8_t* header, uint16_t _type, bool _game, bool _exrom, std::string _name, uint16_t _version ) -> void {
    
    std::memset( header, 0, 64 );
    
    std::memcpy( header, "C64 CARTRIDGE   ", 16 );
    
    Emulator::copyIntToBufferBigEndian<uint32_t>( header + 0x10, 0x40 );
    
    Emulator::copyIntToBufferBigEndian<uint16_t>( header + 0x14, _version );
    
    Emulator::copyIntToBufferBigEndian<uint16_t>( header + 0x16, _type );
    
    header[0x18] = _exrom;
    
    header[0x19] = _game;    
    
    std::memcpy( header + 0x20, _name.c_str(), _name.size() );        
}

auto Cart::buildChipHeader(uint8_t* header, Chip& chip) -> void {    
    
    std::memcpy( header, "CHIP", 4 );
    
    Emulator::copyIntToBufferBigEndian<uint32_t>( header + 0x4, chip.size + 0x10 );
    
    Emulator::copyIntToBufferBigEndian<uint16_t>( header + 0x8, chip.type );
    
    Emulator::copyIntToBufferBigEndian<uint16_t>( header + 0xa, chip.bank );
    
    Emulator::copyIntToBufferBigEndian<uint16_t>( header + 0xc, chip.addr );
    
    Emulator::copyIntToBufferBigEndian<uint16_t>( header + 0xe, chip.size );
}
    
auto Cart::checkForEmptyFlashBank(uint8_t* ptr) -> bool {
    
    for(unsigned i = 0; i < 0x2000; i++) {
        if (ptr[i] != 0xff)
            return false;
    }
    return true;
}

auto Cart::isSupported() -> bool {
    switch(cartridgeId) {
        case LIBC64::Interface::CartridgeIdDefault:
        case LIBC64::Interface::CartridgeIdOcean:
        case LIBC64::Interface::CartridgeIdFunplay:
        case LIBC64::Interface::CartridgeIdSuperGames:
        case LIBC64::Interface::CartridgeIdSystem3:
        case LIBC64::Interface::CartridgeIdZaxxon:
        case LIBC64::Interface::CartridgeIdActionReplayMK2:
        case LIBC64::Interface::CartridgeIdActionReplayMK3:
        case LIBC64::Interface::CartridgeIdActionReplayMK4:
        case LIBC64::Interface::CartridgeIdActionReplayV41AndHigher:
        case LIBC64::Interface::CartridgeIdEasyFlash:
        case LIBC64::Interface::CartridgeIdRetroReplay:
        case LIBC64::Interface::CartridgeIdNordicReplay:
        case LIBC64::Interface::CartridgeIdGmod2:
        case LIBC64::Interface::CartridgeIdMagicDesk:
        case LIBC64::Interface::CartridgeIdFinalCartridge:
        case LIBC64::Interface::CartridgeIdFinalCartridge3:
        case LIBC64::Interface::CartridgeIdFinalCartridgePlus:
        case LIBC64::Interface::CartridgeIdSimonsBasic:
        case LIBC64::Interface::CartridgeIdWarpSpeed:
        case LIBC64::Interface::CartridgeIdAtomicPower:
        case LIBC64::Interface::CartridgeIdMach5:
        case LIBC64::Interface::CartridgeIdRoss:        
        case LIBC64::Interface::CartridgeIdWestermann:
        case LIBC64::Interface::CartridgeIdPagefox:
        case LIBC64::Interface::CartridgeIdDinamic:
        case LIBC64::Interface::CartridgeIdDiashowMaker:
        case LIBC64::Interface::CartridgeIdSuperSnapshotV5:
        case LIBC64::Interface::CartridgeIdComal80:
        case LIBC64::Interface::CartridgeIdSilverrock:
        case LIBC64::Interface::CartridgeIdRGCD:
        case LIBC64::Interface::CartridgeIdStarDos:
        case LIBC64::Interface::CartridgeIdEasyCalc:
        case LIBC64::Interface::CartridgeIdHyperBasic:
        case LIBC64::Interface::CartridgeIdBusinessBasic:
        case LIBC64::Interface::CartridgeIdMagicDesk2:
        case LIBC64::Interface::CartridgeIdStructuredBasic:
        case LIBC64::Interface::CartridgeIdKcsPower:
        case LIBC64::Interface::CartridgeIdProphet64:
        case LIBC64::Interface::CartridgeIdExpert:
            return true;
        default:
            break;
    }
    return false;
}

auto Cart::getListing() -> std::vector<Emulator::Interface::Listing> {
    listings.clear();
    Emulator::C64Listing listing;
    listing.convertToScreencode = system->convertToScreencode;

    std::vector<uint16_t> out;

    out.push_back(listing.decodeToScreencodeHi('I'));
    out.push_back(listing.decodeToScreencodeHi('D'));
    out.push_back(listing.decodeToScreencodeHi(':'));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));

    std::string ident = std::to_string(cartridgeId);
    for(auto& c : ident)
        out.push_back(listing.decodeToScreencodeHi(c));
    listings.push_back( {out} );

    out.clear();
    out.push_back(listing.decodeToScreencodeHi('V'));
    out.push_back(listing.decodeToScreencodeHi('E'));
    out.push_back(listing.decodeToScreencodeHi('R'));
    out.push_back(listing.decodeToScreencodeHi('S'));
    out.push_back(listing.decodeToScreencodeHi('I'));
    out.push_back(listing.decodeToScreencodeHi('O'));
    out.push_back(listing.decodeToScreencodeHi('N'));
    out.push_back(listing.decodeToScreencodeHi(':'));

    ident = std::to_string(version >> 8);
    for (auto& c : ident)
        out.push_back(listing.decodeToScreencodeHi(c));
    out.push_back(listing.decodeToScreencodeHi('.'));
    ident = std::to_string(version & 0xff);
    for (auto& c : ident)
        out.push_back(listing.decodeToScreencodeHi(c));

    listings.push_back( {out} );

    out.clear();
    out.push_back(listing.decodeToScreencodeHi('N'));
    out.push_back(listing.decodeToScreencodeHi('A'));
    out.push_back(listing.decodeToScreencodeHi('M'));
    out.push_back(listing.decodeToScreencodeHi('E'));
    out.push_back(listing.decodeToScreencodeHi(':'));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));

    std::string str((const char*)cartName);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    for (auto& c : str)
        out.push_back(listing.decodeToScreencodeHi(c));
    listings.push_back({ out });

    out.clear();
    out.push_back(listing.decodeToScreencodeHi('E'));
    out.push_back(listing.decodeToScreencodeHi('X'));
    out.push_back(listing.decodeToScreencodeHi('R'));
    out.push_back(listing.decodeToScreencodeHi('O'));
    out.push_back(listing.decodeToScreencodeHi('M'));
    out.push_back(listing.decodeToScreencodeHi(':'));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));

    if (exRom) {
        out.push_back(listing.decodeToScreencodeHi('H'));
        out.push_back(listing.decodeToScreencodeHi('I'));
    } else {
        out.push_back(listing.decodeToScreencodeHi('L'));
        out.push_back(listing.decodeToScreencodeHi('O'));
    }
    listings.push_back({ out });

    out.clear();
    out.push_back(listing.decodeToScreencodeHi('G'));
    out.push_back(listing.decodeToScreencodeHi('A'));
    out.push_back(listing.decodeToScreencodeHi('M'));
    out.push_back(listing.decodeToScreencodeHi('E'));
    out.push_back(listing.decodeToScreencodeHi(':'));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));
    out.push_back(listing.decodeToScreencodeHi(' '));

    if (game) {
        out.push_back(listing.decodeToScreencodeHi('H'));
        out.push_back(listing.decodeToScreencodeHi('I'));
    } else {
        out.push_back(listing.decodeToScreencodeHi('L'));
        out.push_back(listing.decodeToScreencodeHi('O'));
    }
    listings.push_back({ out });
    out.clear();
    listings.push_back({ out });
    
    for(auto& chip : chips) {
        out.clear();
        out.push_back(listing.decodeToScreencode('B'));
        out.push_back(listing.decodeToScreencode('A'));
        out.push_back(listing.decodeToScreencode('N'));
        out.push_back(listing.decodeToScreencode('K'));
        out.push_back(listing.decodeToScreencode(':'));

        ident = std::to_string(chip.bank);
        for (auto& c : ident)
            out.push_back(listing.decodeToScreencode(c));

        if (chip.bank < 100)
            out.push_back(listing.decodeToScreencode(' '));
        if (chip.bank < 10)
            out.push_back(listing.decodeToScreencode(' '));

        out.push_back(listing.decodeToScreencode(' '));
        out.push_back(listing.decodeToScreencode('S'));
        out.push_back(listing.decodeToScreencode('I'));
        out.push_back(listing.decodeToScreencode('Z'));
        out.push_back(listing.decodeToScreencode('E'));
        out.push_back(listing.decodeToScreencode(':'));
        ident = std::to_string(chip.size / 1024);
        for (auto& c : ident)
            out.push_back(listing.decodeToScreencode(c));

        out.push_back(listing.decodeToScreencode('K'));
        out.push_back(listing.decodeToScreencode('B'));
        out.push_back(listing.decodeToScreencode(' '));
        out.push_back(listing.decodeToScreencode('L'));
        out.push_back(listing.decodeToScreencode('O'));
        out.push_back(listing.decodeToScreencode('A'));
        out.push_back(listing.decodeToScreencode('D'));
        out.push_back(listing.decodeToScreencode(':'));
        out.push_back(listing.decodeToScreencode('$'));

        char hex[5] = { 0 };

        snprintf(hex, 5, "%X", chip.addr);

        for (auto& c : hex) {
            if (c)
                out.push_back(listing.decodeToScreencode(c));
        }

        listings.push_back({ out });
    }

    return listings;
}

}
