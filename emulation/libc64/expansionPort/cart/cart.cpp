
#include "cart.h"
#include "../../../tools/buffer.h"
#include "../../system/system.h"

namespace LIBC64 {
    
Cart::Cart(bool game, bool exrom) : ExpansionPort() {
    
    this->game = game;
    this->exRom = exrom;
    
    cRomH = nullptr;
    cRomL = nullptr;
    
    rom = nullptr;
    romSize = 0;
    cartridgeId = Interface::CartridgeIdNoRom;
    
    data = nullptr;
    size = 0;
    binFormat = false;
    version = 0;
}

auto Cart::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
        
    if ( (this->rom == nullptr) && (rom == nullptr) )
        return;
    
	if (this->rom && (rom == nullptr))
        write(); // unset
	
    auto _cartridgeId = media->pcbLayout ? media->pcbLayout->id : 0;
    
    auto newCart = rebuild( (Interface::CartridgeId)_cartridgeId, rom, romSize );
	
	newCart->media = media;
	newCart->prepare();
  
    assign( newCart );
}
    
auto Cart::rebuild( Interface::CartridgeId cartridgeId, uint8_t* _rom, unsigned _romSize ) -> Cart* {
    
    if (!_rom || (_romSize == 0) )
        cartridgeId = Interface::CartridgeIdNoRom;
    
    Cart* cart = create( cartridgeId );
    
    cart->rom = _rom;
    cart->romSize = _romSize;
    
    if( cart->readHeader( ) ) {
        
        if (cart->cartridgeId != cartridgeId) {
            cartridgeId = cart->cartridgeId;
            // if user doesn't request a specific cart and analyzing header detects a specific cart
			
			if (!cart->protectFromDeletion())
				delete cart;                        
            // lets recreate by detected type
            return rebuild( cartridgeId, _rom, _romSize );            
        }        
    } else
        cart->cartridgeId = cartridgeId;
    
    if ( !cart->readChips() ) {
        // no chip headers found, we assume it by user requested type
        cart->assumeChips();
    }    
    
    return cart;
}
        
auto Cart::readHeader( ) -> bool {
    
    data = rom;
    size = romSize;
    binFormat = true;
    
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
    
    version = Emulator::copyBufferToIntBigEndian<uint16_t>(&header[0x14]);
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

auto Cart::readRomL(uint16_t addr) -> uint8_t {
	
    if (!cRomL)
        return ExpansionPort::readRomL( addr );

    addr %= cRomL->size;
    
    return *(cRomL->ptr + addr);
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

auto Cart::reset() -> void {  
    
    cRomL = getChip(0);
    
    cRomH = chips.size() > 1 ? getChip(1) : getChip(0);
}

auto Cart::serialize(Emulator::Serializer& s) -> void {
    
    unsigned _cartridgeId = cartridgeId;
    s.integer(_cartridgeId);
    
    if (s.mode() == Emulator::Serializer::Mode::Load) {
        
        if ( (_cartridgeId == Interface::CartridgeIdDefault) && 
                (cartridgeId == Interface::CartridgeIdDefault8k || cartridgeId == Interface::CartridgeIdDefault16k || cartridgeId == Interface::CartridgeIdUltimax ));
        // state file of a standard cartridge was created from a CRT and reloaded from a BIN.
        // don't recreate because standard CRT cart id is same for 8k, 16k and ultimax.
        // serialization frame is identical for these cartridges, so no problem
        else if (cartridgeId != _cartridgeId) { // oh kacke
            // cartridge id of state file mismatches with loaded one.
            // it seems the cart which was loaded while creating this save state isn't present anymore.
            // we need to recreate the expected cartridge in order to unserialize the right data.
            // probably the loaded state file is unusable but we don't want to crash the emulation
            // on top of that when data is unserialized in wrong order.
            
            auto cart = create( (Interface::CartridgeId)_cartridgeId );
            
            if (_cartridgeId != Interface::CartridgeIdNoRom) {
                cart->rom = rom;
                cart->romSize = romSize;
                cart->readHeader();
            }
            
            // force cart id from state.
            cart->cartridgeId = (Interface::CartridgeId)_cartridgeId;
            if (!cart->readChips())
                cart->assumeChips();            
            			
			cart->prepare();
            assign( cart );            
            cart->serializeStep2( s );
            
            return;
        }
    }
    
    serializeStep2( s );
}

auto Cart::serializeStep2(Emulator::Serializer& s) -> void {
    
    int romLId = cRomL ? cRomL->id : -1;
    int romHId = cRomH ? cRomH->id : -1;

    s.integer(romLId);
    s.integer(romHId);

    if (s.mode() == Emulator::Serializer::Mode::Load) {

        cRomL = ((romLId >= 0) && (romLId < chips.size())) ? &chips[romLId] : nullptr;
        cRomH = ((romLId >= 0) && (romHId < chips.size())) ? &chips[romHId] : nullptr;
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

}
