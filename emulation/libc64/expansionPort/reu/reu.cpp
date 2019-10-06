
#include "reu.h"

namespace LIBC64 {
 
Reu::Reu(unsigned size) {
    this->size = size;
    
    data = new uint8_t[size];
    
    setIrq = [this]() {
        
        irqCall( true ); // enable expansion port irq line
    };
    
    unsetIrq = [this]() {
        
        irqCall( false ); // disable expansion port irq line
    };
    
    setDma = [this]() {
        dma = true;
        dmaCall( true );
    };

    events->registerCallback( { {&setIrq, 1}, {&unsetIrq, 1}, {&setDma, 1} } );
}    

Reu::~Reu() {
    if (data)
        delete[] data;
}
    
auto Reu::cycle() -> void {
       
    if (waitForStart) {
        bool writeCycle = false;
        // check bus usage of last cycle.
        // and we are in the beginning of this cycle, before cpu puts next address on bus
        uint16_t addr = listenAddrBus( writeCycle );
        if (writeCycle && (addr == 0xff00) ) {
            waitForStart = false;
            dma = true;
            dmaCall(true);
        }
    }
    
    if (!dma || vicBA())
        return;
    
    switch ( command & 3 ) {
        case 0: // stash: C64 to Reu
            break;
        case 1: // fetch: Reu to C64
            break;
        case 2: // swap: Reu <-> C64
            break;
        case 3: // verify Reu ??? C64
            break;
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
            events->add( &unsetIrq, 1, Emulator::Events::UpdateExisting );
            break;
        case 1:
            return command;
        case 2:
            return hostAddr & 0xff;
        case 3:
            return (hostAddr >> 8) & 0xff;
        case 4:
            return reuAddr & 0xff;
        case 5:
            return (reuAddr >> 8) & 0xff;
        case 6:
            return reuBank | 0xf8;
        case 7:
            return transferLength & 0xff;
        case 8:
            return (transferLength >> 8) & 0xff;
        case 9:
            return intMask | 0x1f;
        case 0xa:
            return addrControl | 0x3f;                        
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
                    events->add( &setDma, 1, Emulator::Events::UpdateExisting );
                } else
                    waitForStart = true;
            }
            break;
        case 2:
            hostAddr = (hostAddr & 0xff00) | value;
            break;
        case 3:
            hostAddr = (hostAddr & 0xff) | (value << 8);
            break;
        case 4:
            reuAddr = (reuAddr & 0xff00) | value;
            break;
        case 5:
            reuAddr = (reuAddr & 0xff) | (value << 8);
            break;
        case 6:
            reuBank = value;
            break;
        case 7:
            transferLength = (transferLength & 0xff00) | value;
            break;
        case 8:
            transferLength = (transferLength & 0xff) | (value << 8);
            break;
        case 9:
            intMask = value | 0x1f;
            
            if (intMask & 0x80) {                
                if ( ((intMask & 0x40) && (status & 0x40)) ||
                     ((intMask & 0x20) && (status & 0x20)) ) {
                    // maybe intmask is activated after a finished transfer, but before reading status.
                    status |= 0x80;
                    events->add( &setIrq, 1, Emulator::Events::UpdateExisting );
                }
            }
            
            break;
        case 0xa:
            addrControl = value | 0x3f;
            break;
    }
        
}

}