
#include "retroReplay.h"
#include "../../system/system.h"
#include "memoryHandler.cpp"

namespace LIBC64 {

RetroReplay* retroReplay = nullptr;
    
RetroReplay::RetroReplay() : Freezer( true, false ), flash(Emulator::Flash040::Type010) {   
    
    unbeatable = true;
    flashJumper = false;
    bankJumper = false;
    
    ram = new uint8_t[ 32 * 1024 ];
    
    this->media = nullptr;
    
    this->writeProtect = true;       
    
    flashModeReset = [this]() {
        game = true;
        exRom = true;
        system->changeExpansionPortMemoryMode(exRom, game);
    };
    
    sysTimer.registerCallback({&flashModeReset, 1});
    
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
    flash.setEvents( &sysTimer );
    
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

auto RetroReplay::clock() -> void {
    
    if (flashJumper) {
        uint16_t _addr = system->cpu->addressBus();
        
        if (_addr >= 0x8000 && _addr <= 0x9fff) {
            exRom = requestedExRom;
            game = requestedGame;
            system->changeExpansionPortMemoryMode(exRom, game);   
            sysTimer.add( &flashModeReset, 1, Emulator::SystemTimer::Action::UpdateExisting );
        } 
    }
    
    Freezer::clock();
}

auto RetroReplay::writeIo1( uint16_t addr, uint8_t value ) -> void {
    
    if (!enabled)
        return;    
        
    switch(addr & 0xff) {
        case 0: {    
            exRom = (value >> 1) & 1;
            game = (value & 1) ^ 1;
            bank = ((value >> 3) & 3) | ((value >> 5) & 4);
            ramMode = !!(value & 0x20);            
            bool disableFreeze = !!(value & 0x40);                            
                                
            // nordic replay in nordic power mode
            nordicPower = (cartridgeId == Interface::CartridgeIdNordicReplay) && !disableFreeze && ramMode && enabled && game && exRom;
                        
            if (nordicPower) { // switch to 16k config
                exRom = false;
                game = false;
            }

            if (disableFreeze) {
                frozen = false; 
                nmiCall(false);
                irqCall(false);
            }            
                         
            if (frozen) { // keep Ultimax
                exRom = true;
                game = false;
            }
 
            if (flashJumper) { 
                // memorize mapping bits 0 and 1 for later
                // disable cart mapping
                // apply the requested (memorized) mapping if an address between $8000 and $9fff hits the BUS in second half cycle
                // disable cart mapping again, so VIC reads in first half cycle are definitely not in possible Ultimax mode
                requestedGame = (value & 1) ^ 1;
                requestedExRom = (value >> 1) & 1;
                game = true;
                exRom = true;               
            }
                    
            if (value & 4) { // disable cart mapping and cart registers
                enabled = false;
                exRom = true;
                game = true;                
            }
            
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

auto RetroReplay::readIo1( uint16_t addr ) -> uint8_t {
    
    if (!enabled)
        return 0; 
    
    addr &= 0xff;
    
    if (addr == 0 || addr == 1)        
        return ((bank & 4) << 5) | (reuMapping << 6) | ((bank & 8) << 2) | ((bank & 3) << 3) | (freezeArmed << 2) | (allowBank << 1) | flashJumper;        
    
    if (!reuMapping || frozen)
        return 0;
        
    if (ramMode)
        return ram[ getRamAddr( 0x1e00 | addr) ];

    if (game)                         
        return flash.read( getFlashAddr( (0xde00 | addr) & 0x1fff ) );
    
    return 0;
}

auto RetroReplay::writeIo2( uint16_t addr, uint8_t value ) -> void {

    if (!enabled)
        return;  

    if (!reuMapping && !frozen && ramMode)
        ram[ getRamAddr( 0x1f00 | (addr & 0xff)) ] = value; 
}

auto RetroReplay::readIo2( uint16_t addr ) -> uint8_t {

    if (!enabled || reuMapping || frozen)
        return 0; 
    
    addr &= 0xff;
    
    if (ramMode)
        return ram[ getRamAddr( 0x1f00 | addr) ];

    if (game)        
        return flash.read( getFlashAddr( (0xdf00 | addr) & 0x1fff ) );
    
    return 0;
}

// 80 - 9f [8k, 16k, ultimax]
auto RetroReplay::readRomL( uint16_t addr ) -> uint8_t {

    if (frozen)
        return ExpansionPort::readRomL(addr);
    
    if (nordicPower)
        return flash.read( getFlashAddr( addr & 0x1fff ) );
    
    if (ramMode)
        return ram[ getRamAddr<true>(addr & 0x1fff) ];
    
    if (!game && !exRom) // 16 k config
        return ExpansionPort::readRomL(addr);
    
    return flash.read( getFlashAddr( addr & 0x1fff ) );
}
// 80 - 9f [8k, 16k]
auto RetroReplay::writeRomL( uint16_t addr, uint8_t data ) -> void {

    if (flashJumper) {
        if (ramMode) {
            ram[ getRamAddr<true>(addr & 0x1fff) ] = data;
            return;
        }
        flash.write(getFlashAddr(addr & 0x1fff), data); 
        
    } else {
        
        if ( (cartridgeId == Interface::CartridgeIdNordicReplay) && ramMode )
            ram[ getRamAddr<true>(addr & 0x1fff) ] = data;
    }
    
    ExpansionPort::writeRomL( addr, data );
}

// 80 - 9f accepts writes while there is no cartridge mapped in this area
auto RetroReplay::listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void {
    // C64 ram will be written in an external process, doesn't matter if 'writeRomL' allows it or not.
    // because in this mode (C64 config) cartridge can listen only but not prevent the C64 RAM write   
    writeRomL( addr, data );
}

// 80 - 9f [Ultimax]
auto RetroReplay::writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {

    if (ramMode)
        ram[ getRamAddr<true>(addr & 0x1fff) ] = data;
    else if (flashJumper)        
        flash.write( getFlashAddr( addr & 0x1fff ), data );     
}

// a0 - bf [16k], e0 - ff [ultimax]
// Note: following mappings are not accessible while flash jumper is set
auto RetroReplay::readRomH( uint16_t addr ) -> uint8_t {
        
    if (nordicPower) {
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
// a0 - bf [16k]
auto RetroReplay::writeRomH( uint16_t addr, uint8_t data ) -> void {    

    if (!nordicPower || frozen)
        return;
    
    ram[ getRamAddr(addr & 0x1fff) ] = data;
    
    ExpansionPort::writeRomH( addr, data );
}

// writeUltimaxRomH at $e0 is not mapped by retroReplay ?

// a0 - bf [ultimax] [NordicReplay only]
auto RetroReplay::readUltimaxA0( uint16_t addr ) -> uint8_t {

    if (!nordicPower || !frozen)
        return ExpansionPort::readUltimaxA0(addr);
    
    return ram[ getRamAddr(addr & 0x1fff) ];
}

// a0 - bf [ultimax] [NordicReplay only]
auto RetroReplay::writeUltimaxA0( uint16_t addr, uint8_t data ) -> void {

    if (!nordicPower || !frozen)
        return;
    
    ram[ getRamAddr(addr & 0x1fff) ] = data;
}

template<bool ignoreAllowBank> inline auto RetroReplay::getRamAddr( uint16_t addr ) -> uint16_t {
    
    if (ignoreAllowBank || allowBank)
        return ((bank & 3) << 13) | addr;
    
    return addr;
}

template<bool specialCase> inline auto RetroReplay::getFlashAddr( uint32_t addr ) -> uint32_t {
    
    uint8_t _bank = bank;
    
    if (specialCase)
        _bank = bank & ~3;
    
    if (!bankJumper)  // access only upper 64 k
        return 0x10000 | addr | ((_bank & 7) << 13);
    
    return addr | (_bank << 13); // access full 128 k
}

auto RetroReplay::didFreeze() -> void {
    if (noFreeze)
        return;
    
    frozen = true;
    enabled = true;
    bank = 0;
    ramMode = false;
    nordicPower = false;
}

auto RetroReplay::blockFreeze() -> bool {
    return flashJumper;
}

auto RetroReplay::reset() -> void {
    
    enabled = true;
    frozen = false;
    writeOnce = false;
    exRom = requestedExRom = false;
    game = requestedGame = true;
    bank = 0;
    ramMode = false;
    nordicPower = false;
    allowBank = false;
    noFreeze = false;
    reuMapping = false;
    
    if (flashJumper)
        exRom = requestedExRom = true;            

    std::memset(ram, 0, 32 * 1024);
    flash.reset();
}

auto RetroReplay::setWriteProtect(bool state) -> void {
    
    writeProtect = state;
}

auto RetroReplay::isWriteProtected() -> bool {
    return writeProtect;
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
    s.integer( nordicPower );
    s.integer( allowBank );
    s.integer( noFreeze );
    s.integer( reuMapping );
    s.integer( writeOnce );
    s.integer( requestedGame );
    s.integer( requestedExRom );
    s.integer( writeProtect );
    
    Freezer::serializeStep2( s );
    
    cRomL = cRomH = nullptr;
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

auto RetroReplay::getJumper( unsigned jumperId ) -> bool {
    
    if (jumperId == 0) // bank jumper
        return bankJumper;
                
    return flashJumper;
}


}
