
#include "reu.h"
#include "../../system/system.h"

namespace LIBC64 {
 
Reu::Reu(unsigned size, uint8_t* rom, unsigned romSize) {
    this->active = true;
    this->size = 128;
    this->romSize = romSize;
    this->rom = rom;
    
    if (rom && romSize) 
        exRom = false;
    
    auto& reuCart = system->interface->cartridges[1];
    
    for( auto& memory : reuCart.memory ) {        
        if (memory.size == size) {
            this->size = size;            
            break;
        }
    }
    
    size = this->size;
    
    data = new uint8_t[size * 1024];
    
    // when lower 19 bits of REU address reach this value, address wraps around to zero
    wrapAround = size == 128 ? 0x20000 : 0x80000;
    dramWrapAround = 0xffffff;
    if (size == 128)
        dramWrapAround = 0x1ffff;
    else if (size == 256 || size == 512)
        dramWrapAround = 0x7ffff;
    
    status = size == 128 ? 0 : 0x10;
    
    setIrq = [this]() {
        
        irqCall( true ); // enable expansion port irq line
    };
    
    unsetIrq = [this]() {
        
        irqCall( false ); // disable expansion port irq line
    };
    
    setDma = [this]() {
        dma = true;
        swapRead = false;
        steal = false;
        dmaCall( true );
        
        hostAddr = reg.hostAddr;
        reuAddr = reg.reuAddr;
        transferLength = reg.transferLength;
    };
    
    finish = [this]() {
        
        if (!(command & 0x20)) { // not autoload
            reg.hostAddr = hostAddr;
            reg.reuAddr = reuAddr;
            reg.transferLength = transferLength;                        
        }
        
        if (allowIrq()) {
            status |= 0x80;
            irqCall( true );          
        }
        
        // disable execute bit, disable FF00 trigger 
        command = (command & ~0x80) | 0x10;
        dma = false;
        dmaCall( false );
    };

    system->events.registerCallback( { {&setIrq, 1}, {&unsetIrq, 1}, {&setDma, 1}, {&finish, 1} } );    
    
    reset();
}    

Reu::~Reu() {
    if (data)
        delete[] data;
}

auto Reu::reset() -> void {
    
    status = size == 128 ? 0 : 0x10;
    
    command = 0x10; // FF00 trigger disabled    
    intMask = 0x1f;    
    control = 0x3f;
    
    reg.transferLength = 0xffff;    
    reg.reuAddr = 0;
    reg.hostAddr = 0;
    
    waitForStart = false;
    vicBaLow = 0;
    steal = false;
    swapRead = false;
}

inline auto Reu::readReu() -> uint8_t {

    reuAddr &= dramWrapAround;
    
    if (reuAddr < size)        
        return data[reuAddr];    
    
    return 0xff;
}

inline auto Reu::writeReu(uint8_t value) -> void {

    reuAddr &= dramWrapAround;
    
    if (reuAddr < size)        
        data[reuAddr] = value;
}

inline auto Reu::incrementAddresses() -> void {
    
    if ((control & 0x80) == 0)
        hostAddr = (hostAddr + 1) & 0xffff;

    if (control & 0x40)
        return;    

    uint32_t incremented = (reuAddr & 0x07ffff) + 1;

    if (incremented == wrapAround)
        incremented = 0;

    reuAddr = (reuAddr & 0xf80000) | incremented;
}

auto Reu::cycleLo() -> void {
    
    if (waitForStart && !dma) {        
        // listen CPU bus usage        
        if (system->cpu->isWriteCycle()) {            
            if ( system->cpu->addressBus() == 0xff00 ) {
                waitForStart = false;
                setDma();
            }
        }
    }  
}

auto Reu::cycleHi() -> void {
    vicBaLow <<= 1;
    vicBaLow |= vicBA();
    
    if (!dma)
        return;
    
    switch ( command & 3 ) {
        case 0: stash(); break;
        case 1: fetch(); break;
        case 2: swap(); break;
        case 3: verify(); break;
    }      
}

inline auto Reu::verify() -> void {
    if (vicBaLow & 1)
        return;

    value = readReu();
    value2 = system->memoryCpu.read(hostAddr);

    if (steal) {
        steal = false;

        if (transferLength == 1) {
            if (value == value2) {
                status |= 0x40; // end of block
            }
        }

        system->events.add(&finish, 1, Emulator::Events::UpdateExisting);
        
        return;
    } 
    
    incrementAddresses();

    if (value != value2) {
        status |= 0x20; // verify error

        if (transferLength > 1)
            steal = true;
    }
        
    decrementTransferLength(); 
}

inline auto Reu::swap() -> void {
    
    if (swapRead) {
        if (vicBaLow & 1)
            return;
        value = readReu();
        value2 = system->memoryCpu.read(hostAddr);
        
    } else {

        if ((vicBaLow & 3) == 3) {
            steal = true;
            return;
        }

        if (steal) {
            steal = false;
            if (transferLength == 1)
                return;
        }
        
        writeReu(value2);
        system->memoryCpu.write(hostAddr, value);
        incrementAddresses();      
        decrementTransferLength(); 
    }
        
    swapRead ^= 1;  
}

inline auto Reu::fetch() -> void {
    if ((vicBaLow & 3) == 3) { // first BA cycle is usable for REU
        steal = true;
        return;
    }
    
    if (steal) {
        steal = false;
        if (transferLength == 1) // when BA happened in last cylce, there is an extra cycle.
            return;
    }
    
    value = readReu();
    system->memoryCpu.write( hostAddr, value );
    incrementAddresses();      
    decrementTransferLength();
}

inline auto Reu::stash() -> void {
    if (vicBaLow & 1) // VIC needs this cycle
        return;
    
    value = system->memoryCpu.read( hostAddr );
    writeReu( value );
    incrementAddresses();      
    decrementTransferLength();
}

inline auto Reu::decrementTransferLength() -> void {
    if (--transferLength == 0) {
        transferLength = 1;
        status |= 0x40;
        system->events.add( &finish, 1, Emulator::Events::UpdateExisting );
    }
}

auto Reu::readIo2( uint16_t addr ) -> uint8_t {
    
    if (dma)
        return ExpansionPort::readIo2( addr ); // open address space
    
    addr &= 0x1f;
    uint8_t val = 0xff;
    
    switch( addr ) {
        case 0:
            val = status;
            status &= ~0xe0;
            system->events.add( &unsetIrq, 1, Emulator::Events::UpdateExisting );
            break;
        case 1:
            return command;
        case 2:
            return reg.hostAddr & 0xff;
        case 3:
            return (reg.hostAddr >> 8) & 0xff;
        case 4:
            return reg.reuAddr & 0xff;
        case 5:
            return (reg.reuAddr >> 8) & 0xff;
        case 6:
            return (reg.reuAddr >> 16) | 0xf8;
        case 7:
            return reg.transferLength & 0xff;
        case 8:
            return (reg.transferLength >> 8) & 0xff;
        case 9:
            return intMask | 0x1f;
        case 0xa:
            return control | 0x3f;                        
    }
    
    return val;
}

auto Reu::writeIo2( uint16_t addr, uint8_t value ) -> void {
    
    if (dma)
        return;
    
    addr &= 0x1f;
    
    switch( addr ) {
        case 0: // read only
            break;
        case 1:
            command = value;
            if (command & 0x80) {
                if (command & 0x10) {                    
                    system->events.add( &setDma, 1, Emulator::Events::UpdateExisting );
                } else
                    waitForStart = true;
            }
            break;
        case 2:
            reg.hostAddr = (reg.hostAddr & 0xff00) | value;
            break;
        case 3:
            reg.hostAddr = (reg.hostAddr & 0xff) | (value << 8);
            break;
        case 4:
            reg.reuAddr = (reg.reuAddr & 0xffff00) | value;
            break;
        case 5:
            reg.reuAddr = (reg.reuAddr & 0xff00ff) | (value << 8);
            break;
        case 6:
            reg.reuAddr = (reg.reuAddr & 0xffff) | (value << 16);
            break;
        case 7:
            reg.transferLength = (reg.transferLength & 0xff00) | value;
            break;
        case 8:
            reg.transferLength = (reg.transferLength & 0xff) | (value << 8);
            break;
        case 9:
            intMask = value | 0x1f;
            
            if (allowIrq()) {
                // maybe intmask is activated after a finished transfer, but before reading status.
                status |= 0x80;
                system->events.add( &setIrq, 1, Emulator::Events::UpdateExisting );                
            }
            
            break;
        case 0xa:
            control = value | 0x3f;
            break;
    }
        
}

auto Reu::allowIrq() -> bool {
    
    if (intMask & 0x80) {                
        if ( ((intMask & 0x40) && (status & 0x40)) ||
             ((intMask & 0x20) && (status & 0x20)) ) {
            
            return true;
        }
    }
    
    return false;
}

auto Reu::readRomL(uint16_t addr) -> uint8_t {
	
    addr %= romSize;		
    
    return *(rom + addr);
}

auto Reu::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( status );
    s.integer( command );
    s.integer( intMask );
    s.integer( control );
    
    s.integer( reg.hostAddr );
    s.integer( reg.reuAddr );
    s.integer( reg.transferLength );
    s.integer( hostAddr );
    s.integer( reuAddr );
    s.integer( transferLength );
    
    s.integer( size );
    s.array( data, size * 1024 );
    
    s.integer( wrapAround );
    s.integer( dramWrapAround );
    
    s.integer( waitForStart );
    s.integer( vicBaLow );
    s.integer( steal );
    s.integer( value );
    s.integer( value2 );
    s.integer( swapRead );               
}

}