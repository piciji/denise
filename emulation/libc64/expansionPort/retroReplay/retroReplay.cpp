
#include "retroReplay.h"
#include "../../system/system.h"
#include "memoryHandler.cpp"

namespace LIBC64 {

RetroReplay* retroReplay = nullptr;
    
RetroReplay::RetroReplay(Emulator::Events* events) : Freezer( true, false ), flash(Emulator::Flash040::Type010) {   
    
    unbeatable = true;
    flashJumper = false;
    bankJumper = false;
    
    ram = new uint8_t[ 32 * 1024 ];
    
    this->events = events;
    
    this->media = nullptr;
    
    this->writeProtect = true;        
    
    init();
    
    setId( Interface::ExpansionIdRetroReplay );
}

RetroReplay::~RetroReplay() {
    delete[] ram;
    delete[] flashData;
}

auto RetroReplay::init( ) -> void {
    flashData = new uint8_t[ 128 * 1024 ];
    
    flash.setData( flashData );
    flash.setEvents( events );
    
    flash.written = []() {
        system->serializationSize += 128 * 1024;
    };
}

auto RetroReplay::assign( Cart* cart ) -> void {
    // don't rebuild
}

auto RetroReplay::create( Interface::CartridgeId cartridgeId ) -> Cart* {
    // don't rebuild
    return retroReplay;
}

auto RetroReplay::writeIo1( uint16_t addr, uint8_t value ) -> void {
    
    if (!enabled)
        return;    
        
    switch(addr & 0xff) {
        case 0: {           
            bool disableFreeze = !!(value & 0x40);       
            if (disableFreeze) {
                frozen = false; 
                nmiCall(false);
            }
            
            if (value & 4) { // disable cart
                enabled = false;
                exRom = true;
                game = true;
            } else if (frozen) {
                // keep Ultimax
            } else {
                exRom = (value >> 1) & 1;
                game = (value & 1) ^ 1;                
            }
            
            bank = ((value >> 3) & 3) | ((value >> 5) & 4);
            ramMode = !!(value & 0x20);
                                
            // nordic replay + ram mode + 16k config
            alternateRam = (cartridgeId == Interface::CartridgeIdNordicReplay) && !disableFreeze && ramMode && enabled && !game && !exRom;                      
            
            system->changeExpansionPortMemoryMode(exRom, game);            
        } break;
            
        case 1:
            bank = ((value >> 3) & 3) | ((value >> 5) & 4);
            
            if (flashJumper && bankJumper)                                
                bank |= ((value >> 2) & 8) ^ 8;                                                      
                        
            if (flashJumper || !writeOnce) {
                writeOnce = true;
                allowBank = !!(value & 2);
                noFreeze = !!(value & 4);
                reuMapping = flashJumper ? false : !!(value & 0x40);
            }
            
            break;
            
        default:
            if (reuMapping && !frozen && ramMode)            
                ram[ getRamAddr( 0x1e00 | (addr & 0xff)) ] = value; 
            
            break;
    }
}

auto RetroReplay::writeIo2( uint16_t addr, uint8_t value ) -> void {

    if (!enabled)
        return;  

    if (!reuMapping && !frozen && ramMode)
        ram[ getRamAddr( 0x1f00 | (addr & 0xff)) ] = value; 
}

auto RetroReplay::readIo1( uint16_t addr ) -> uint8_t {
    if (!enabled)
        return 0; 
    
    addr &= 0xff;
    
    if (addr == 0 || addr == 1)        
        return ((bank & 4) << 5) | (reuMapping << 6) | ((bank & 8) << 2) | ((bank & 3) << 3) | (freezeArmed << 2) | (allowBank << 1) | flashJumper;        
    
    if (reuMapping && !frozen) {
        if (ramMode)
            return ram[ getRamAddr( 0x1e00 | (addr & 0xff)) ];
    
        if (game)                   
            return flash.read(  getFlashAddr( (0xde00 | addr) & 0x1fff ) );
    }
    
    return 0;
}

auto RetroReplay::readIo2( uint16_t addr ) -> uint8_t {
    if (!enabled)
        return 0; 
    
    addr &= 0xff;
    
    if (!reuMapping && !frozen) {
        if (ramMode)
            return ram[ getRamAddr( 0x1f00 | (addr & 0xff)) ];
    
        if (game)        
            return flash.read(  getFlashAddr( (0xdf00 | addr) & 0x1fff ) );
    }
    
    return 0;
}


inline auto RetroReplay::getRamAddr( uint16_t addr ) -> uint16_t {
    
    return (allowBank ? ((bank & 3) << 13) : 0) | addr;
}

template<bool specialCase> inline auto RetroReplay::getFlashAddr( uint32_t addr ) -> uint32_t {
    
    uint8_t _bank = bank;
    
    if (specialCase)
        _bank = bank & ~3;
    
    if (!bankJumper)  // access only upper 64 k
        return 0x10000 | addr | ((_bank & 7) << 13);
    
    return addr | (_bank << 13); // access full 128 k
}

auto RetroReplay::setJumper( unsigned jumperId, bool state ) -> void {
    
    if (jumperId == 0) { // bank jumper
        bankJumper = state;
            
        
    } if (jumperId == 1) { // flash jumper
        
        if (flashJumper && !state)
            writeOnce = false;

        flashJumper = state;        
    }
}

// 80 - 9f [8k, 16k, ultimax]
auto RetroReplay::readRomL( uint16_t addr ) -> uint8_t {
    
    if (frozen)
        return ExpansionPort::readRomL(addr);
    
    if (alternateRam)
        return flash.read( getFlashAddr( addr & 0x1fff ) );
    
    if (ramMode)
        return ram[ getRamAddr(addr & 0x1fff) ];
    
    if (!game && !exRom) // 16 k config
        return ExpansionPort::readRomL(addr);
    
    return flash.read( getFlashAddr( addr & 0x1fff ) );
}

auto RetroReplay::writeRomL( uint16_t addr, uint8_t data ) -> void {
    
    if (flashJumper) {
        if (ramMode)
            ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
        else
            flash.write(getFlashAddr(addr & 0x1fff), data); 
    } else {
        
        if ( (cartridgeId == Interface::CartridgeIdNordicReplay) && ramMode )
            ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
    }
    
    ExpansionPort::writeRomL( addr, data );
}

auto RetroReplay::writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {
    
    if (ramMode)
        ram[ ((bank & 3) << 13) | (addr & 0x1fff) ] = data;
    else if (flashJumper)        
        flash.write( getFlashAddr( addr & 0x1fff ), data );        
}

// a0 - bf [16k] , e0 - ff [ultimax]
auto RetroReplay::readRomH( uint16_t addr ) -> uint8_t {
    
    if (flashJumper)
        return ExpansionPort::readRomH(addr);
        
    if (alternateRam) {
        if (frozen) {
            if (allowBank)
                return flash.read( getFlashAddr( addr & 0x1fff ) );
            
            return flash.read( getFlashAddr<true>( addr & 0x1fff ) );
        }
        
        return ram[ getRamAddr(addr & 0x1fff) ];
    }
    
    if (allowBank || !ramMode)
        return flash.read( getFlashAddr( addr & 0x1fff ) );    
    
    return flash.read( getFlashAddr<true>( addr & 0x1fff ) );
}

auto RetroReplay::writeRomH( uint16_t addr, uint8_t data ) -> void {    
    
    if (flashJumper || !alternateRam || frozen)
        return;
    
    ram[ getRamAddr(addr & 0x1fff) ] = data;
    
    ExpansionPort::writeRomH( addr, data );
}

// a0 - bf [ultimax] [NordicReplay only]
auto RetroReplay::readUltimaxA0( uint16_t addr ) -> uint8_t {
    
    if (flashJumper || !alternateRam || !frozen)
        return ExpansionPort::readUltimaxA0(addr);
    
    return ram[ getRamAddr(addr & 0x1fff) ];
}

auto RetroReplay::writeUltimaxA0( uint16_t addr, uint8_t data ) -> void {
    if (flashJumper || !alternateRam || !frozen)
        return;
    
    ram[ getRamAddr(addr & 0x1fff) ] = data;
}

// writeUltimaxRomH at $e0 is not mapped by retroReplay

auto RetroReplay::didFreeze() -> void {
    if (noFreeze)
        return;
    
    frozen = true;
    enabled = true;
    bank = 0;
    ramMode = false;
    alternateRam = false;
}

auto RetroReplay::blockFreeze() -> bool {
    return flashJumper;
}

auto RetroReplay::reset() -> void {
    
    enabled = true;
    frozen = false;
    writeOnce = false;
    exRom = false;
    game = true;
    bank = 0;
    ramMode = false;
    alternateRam = false;
    allowBank = false;
    noFreeze = false;
    reuMapping = false;
    
    if (flashJumper)
        exRom = true;  

    std::memset(ram, 0, 32 * 1024);
    flash.reset();
}

auto RetroReplay::setWriteProtect(bool state) -> void {
    
    writeProtect = state;
}

auto RetroReplay::isBootable( ) -> bool {
    return !flashJumper;
} 

auto RetroReplay::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( (uint16_t&)cartridgeId );    
    
    s.integer( bank );
    
    s.array( ram, 32 * 1024 );
    
    flash.serialize(s);
    
    if (flash.dirty)
        s.array(flashData, 128 * 1024);   
    
    s.integer( flashJumper );
    s.integer( bankJumper );
    s.integer( enabled );
    s.integer( frozen );
    s.integer( ramMode );
    s.integer( alternateRam );
    s.integer( allowBank );
    s.integer( noFreeze );
    s.integer( reuMapping );
    s.integer( writeOnce );
    
    ExpansionPort::serialize(s);        
}

}
