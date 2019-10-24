
#include "gameCart.h"
#include "../../system/system.h"
#include "funplay.h"
#include "ocean.h"
#include "zaxxon.h"
#include "system3.h"
#include "supergames.h"
#include "cart16k.h"

namespace LIBC64 {

GameCart::GameCart(bool game, bool exrom) : ExpansionPort() {
    
    this->game = game;
    this->exRom = exrom;
    
    cRomH = nullptr;
    cRomL = nullptr;
    
    setId( Interface::ExpansionIdGame );
}

auto GameCart::setRom(uint8_t* rom, unsigned romSize) -> void {
        
    auto _cartridgeId = this->cartridgeId;
    
    auto newCart = GameCart::getInstance( _cartridgeId, rom, romSize );
    
    if (!newCart)
        return;
    
    delete this;
    
    system->expansionPort = newCart;
}
    
auto GameCart::getInstance( Interface::CartridgeId cartridgeId, uint8_t* rom, unsigned romSize ) -> ExpansionPort* {
    
    if (!rom || (romSize == 0) )
        return nullptr;
    
    GameCart* cart = create( cartridgeId );
    
    cart->ExpansionPort::setRom( rom, romSize );    
    
    if( cart->readHeader( ) ) {
        
        if (cart->cartridgeId != cartridgeId) {
            cartridgeId = cart->cartridgeId;
            // if user doesn't request a specific cart and analyzing header detects a specific cart
            delete cart;                        
            // lets recreate by detected type
            return getInstance( cartridgeId, rom, romSize );            
        }        
    } else
        cart->cartridgeId = cartridgeId;
    
    if ( !cart->readChips() ) {
        // no chip headers found, we assume it by user requested type
        cart->assumeChips();
    }
    
    cart->init();
    
    return cart;
}

auto GameCart::create( Interface::CartridgeId cartridgeId ) -> GameCart* {
    GameCart* cart = nullptr;
    
    switch(cartridgeId) {
        case Interface::CartridgeIdFunplay:
            cart = new Funplay;
            break;
        case Interface::CartridgeIdOcean:
            cart = new Ocean;
            break;
        case Interface::CartridgeIdSystem3:
            cart = new System3;
            break;
        case Interface::CartridgeIdSuperGames:
            cart = new SuperGames;            
            break;
        case Interface::CartridgeIdZaxxon:
            cart = new Zaxxon;
            break;
        case Interface::CartridgeIdDefault:
        case Interface::CartridgeIdDefault8k:
        default:
            cart = new GameCart(true, false);
            break;            
            
        case Interface::CartridgeIdDefault16k:
            cart = new Cart16k;
            break;            
            
        case Interface::CartridgeIdUltimax:
            cart = new GameCart(false, true);
            break;            
    }
    
    return cart;
}
        
auto GameCart::readHeader( ) -> bool {
    
    data = rom;
    size = romSize;
    
    if (size < (sizeof header))
        return false;

    std::memcpy(header, data, sizeof header);

    if (std::memcmp(header, "C64 CARTRIDGE   ", 16))
        return false;

    uint32_t headerLength = getDWord(&header[0x10]);

    if (headerLength < (sizeof header))
        return false;

    if (size <= headerLength)
        return false;
    
    data += headerLength;
    size -= headerLength;        
    
    cartridgeId = (Interface::CartridgeId)getWord(&header[0x16]);
    version = getWord(&header[0x14]);        
    exRom = header[0x18] & 1;
    game = header[0x19] & 1;

    return true;
}

auto GameCart::readChips() -> bool {
    chips.clear();
    uint8_t* ptr = data;
    unsigned offset = 0;
    unsigned id = 0;
    
    uint8_t cheader[16]; //chip header
    
    while(1) {
                        
        offset += sizeof cheader;
        
        if ( offset >= size )
            break;
        
        std::memcpy(cheader, ptr, sizeof cheader);
        
        if (std::memcmp(cheader, "CHIP", 4))
            break;
        
        ptr += sizeof cheader;
        
        Chip chip;
        chip.id = id++;
        chip.type = (Chip::Type)getWord(&cheader[0x8]);
        chip.bank = getWord(&cheader[0xa]);
        chip.addr = getWord(&cheader[0xc]);
        chip.size = getWord(&cheader[0xe]);
        chip.offset = offset;
        chip.ptr = ptr;
        chip.ptrHi = chip.size > 8192 ? chip.ptr + 8192 : nullptr;
		
        offset += chip.size;
        
        if (offset > size)
            chip.size -= offset - size;
        
        chips.push_back( chip );

        if (offset >= size)
            break;
        
        ptr += chip.size;
    }
    
    if ( chips.size() == 0 )
        return false;
    
    return true;
}

auto GameCart::assumeChips( ) -> void {
    
    assumeChips( {8192} );
}

auto GameCart::assumeChips( std::vector<unsigned> sizes ) -> void {
    
    // for standard game cards there are 2 banks at best.
    
    uint8_t* ptr = data;
    unsigned offset = 0;
    unsigned id = 0;
    unsigned lastChipSize = 8192;
    
    while(true) {
        
        lastChipSize = id < sizes.size() ? sizes[id] : lastChipSize;
        
        Chip chip;
        chip.size = lastChipSize;
        chip.ptr = ptr;
        chip.ptrHi = chip.size > 8192 ? chip.ptr + 8192 : nullptr;
        chip.bank = id++;
                
        offset += chip.size;
        
        if (offset > size)
            chip.size -= offset - size;
        
        chips.push_back( chip );    
        
        if (offset >= size)
            break;
        
        ptr += chip.size;
    }    
}

auto GameCart::readRomL(uint16_t addr) -> uint8_t {
	
    if (!cRomL)
        return ExpansionPort::readRomL( addr );
    
    addr %= cRomL->size;		
    
    return *(cRomL->ptr + addr);
}
    
auto GameCart::readRomH(uint16_t addr) -> uint8_t {
    
    if (!cRomH)
        return ExpansionPort::readRomH( addr );
    
    if (cRomH->ptrHi) {		
		
        addr %= cRomH->size - 8192;				
        
        return *( cRomH->ptrHi + addr);        
    }
	
    addr %= cRomH->size;		

    return *(cRomH->ptr + addr);
}

auto GameCart::init() -> void {
    
    cRomL = &chips[0];
    
    cRomH = chips.size() > 1 ? &chips[1] : &chips[0];
}

auto GameCart::getDWord( uint8_t* ptr ) -> uint32_t {
    
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

auto GameCart::getWord( uint8_t* ptr ) -> uint16_t {
    
    return ptr[0] << 8 | ptr[1];
}

auto GameCart::serialize(Emulator::Serializer& s) -> void {
    
    int romLId = cRomL ? cRomL->id : -1;
    int romHId = cRomH ? cRomH->id : -1;

    s.integer(romLId);
    s.integer(romHId);

    if (s.mode() == Emulator::Serializer::Mode::Load) {

        cRomL = ((romLId >= 0) && (romLId < chips.size())) ? &chips[romLId] : nullptr;
        cRomH = ((romLId >= 0) && (romHId < chips.size())) ? &chips[romHId] : nullptr;
    }   
    
    ExpansionPort::serialize( s );
}

auto GameCart::isBootable( ) -> bool {
    
    return true;
}
    
}
