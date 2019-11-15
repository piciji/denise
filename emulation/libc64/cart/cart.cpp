
#include "cart.h"
#include "../vic/vicII.h"
#include <cstring>
#include "ocean.h"
#include "system3.h"
#include "supergames.h"
#include "funplay.h"
#include "zaxxon.h"

namespace LIBC64 {
    
Cart* cart = nullptr;
    
Cart::Cart() {
	unload();
}

auto Cart::init() -> unsigned {
	
	if(!cartFound)
		return 3;
	
	//setMapper();
    mapper->init();
	return (header.exRom << 1) | header.game;
}

auto Cart::load( uint8_t* data, unsigned size ) -> bool {
    unload();
    
    this->data = data;
    this->size = size;
    
    if (!data)
        return false;
    
    if ( !readHeader() )
        return false;

    if ( !readChips() )
        return false;
    
    setMapper();
    
    cartFound = true;
    
    return true;
}

auto Cart::unload() -> void {
    
    data = nullptr;
    size = 0;
    cartFound = false;
    cRomH = nullptr;
    cRomL = nullptr;
}

auto Cart::romL(unsigned addr) -> uint8_t {
    if (!cartFound || !cRomL)
        return vicII->getLastReadedValue();
    
	mapper->readRomL(addr);
	
    addr %= cRomL->size;		
    
    return *(cRomL->ptr + addr);
}
    
auto Cart::romH(unsigned addr) -> uint8_t {
    if (!cartFound || !cRomH)
        return vicII->getLastReadedValue();

    if (cRomH->ptrHi) {
		mapper->readRomH(addr);
		
        addr %= cRomH->size - 8192;				
        
        return *( cRomH->ptrHi + addr);        
    }
	mapper->readRomH(addr);
	
    addr %= cRomH->size;		

    return *(cRomH->ptr + addr);
}
    
auto Cart::setMapper() -> void {
    if (mapper)
        delete mapper;    
    
    switch( header.type ) {
        case Header::Type::Ocean:
            mapper = new OceanMapper;
            break;
        case Header::Type::System3:
            mapper = new System3Mapper;
            break;
        case Header::Type::SuperGames:
            mapper = new SuperGamesMapper;
            break;
		case Header::Type::Funplay:
            mapper = new FunplayMapper;
            break;
		case Header::Type::Zaxxon:
            mapper = new ZaxxonMapper;
            break;
        default:
            mapper = new Mapper;
            break;
    }    
}
    
auto Cart::readHeader() -> bool {
    if (size < (sizeof header.data))
        return false;

    std::memcpy(header.data, data, sizeof header.data);

    if (std::memcmp(header.data, "C64 CARTRIDGE   ", 16))
        return false;

    uint32_t headerLength = getDWord(&header.data[0x10]);

    if (headerLength < (sizeof header.data))
        return false;

    if (size <= headerLength)
        return false;
    
    data += headerLength;
    size -= headerLength;
    
    header.version = getWord(&header.data[0x14]);
    header.type = (Header::Type)getWord(&header.data[0x16]);
    header.exRom = header.data[0x18] & 1;
    header.game = header.data[0x19] & 1;

    return true;
}

auto Cart::readChips() -> bool {
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

auto Cart::getDWord( uint8_t* ptr ) -> uint32_t {
    
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

auto Cart::getWord( uint8_t* ptr ) -> uint16_t {
    
    return ptr[0] << 8 | ptr[1];
}

auto Cart::writeIo(bool io1, uint16_t addr, uint8_t value) -> void {
    
    mapper->write(io1, addr, value);
}

auto Cart::readIo(bool io1, uint16_t addr) -> uint8_t {
	
	return mapper->read(io1, addr );
}

auto Mapper::read(bool io1, uint16_t addr) -> uint8_t {
	
	return vicII->getLastReadedValue();
}

auto Mapper::init() -> void {
    cart->cRomL = &cart->chips[0];
    
    cart->cRomH = cart->chips.size() > 1 ? &cart->chips[1] : &cart->chips[0];
}

auto Cart::serialize(Emulator::Serializer& s) -> void {
    
    // dont overwrite 'cartFound' by state
    bool stateIncludeCart = cartFound;
    
    s.integer( stateIncludeCart );
    
    if (!stateIncludeCart)
        return;
    
    // Note: state could be loaded without inserted cart, hence we use 'cartFound'
    // from state file and not from cart load action. it's important, otherwise the
    // serializer would be in disorder
    
    int romLId = cRomL ? cRomL->id : -1;
    int romHId = cRomH ? cRomH->id : -1;
    
    s.integer( romLId );
    s.integer( romHId );
    
    if ( s.mode() == Emulator::Serializer::Mode::Load ) {
        
        cRomL = ( (romLId >= 0) && (romLId < chips.size()) ) ? &chips[romLId] : nullptr;
        cRomH = ( (romLId >= 0) && (romHId < chips.size()) ) ? &chips[romHId] : nullptr;
    }    
    
    mapper->serialize( s );    
}

}
