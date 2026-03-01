
#include "../../system/system.h"
#include "reu.h"
#include "../retroReplay/retroReplay.h"
#include "../superCpu/superCpu.h"

namespace LIBC64 {  

Reu::Reu(System* system) : ExpansionPort(system), sysTimer(system->sysTimer) {
    setId( Interface::ExpansionIdReu );
    prepareRam( 128 );

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
        //system->interface->log( "start" );
        //system->interface->log( vicII->getVcounter(), 0 );
        //system->interface->log( vicII->getCycle(), 0 );
    };
    
    finish = [this]() {
        
        if (command & 0x20) { // autoload
            hostAddr = reg.hostAddr;
            reuAddr = reg.reuAddr;
            transferLength = reg.transferLength; 
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

    sysTimer.registerCallback( { {&setIrq, 1}, {&unsetIrq, 1}, {&setDma, 1}, {&finish, 1} } );        
}    

Reu::~Reu() {
    if (data)
        delete[] data;
}

auto Reu::isExrom( ) -> bool {
	if (expander)
		return expander->isExrom();
	
	return exRom;
}

auto Reu::isGame( ) -> bool {
	if (expander)
		return expander->isGame();
		
	return game;
}

auto Reu::setExpander( ExpansionPort* expander ) -> void {
	
	this->expander = expander;
	
	if (expander == system->retroReplay)
		setId( Interface::ExpansionIdReuRetroReplay );
	else 
		setId( Interface::ExpansionIdReu );
}

auto Reu::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
    
    if (rom && (romSize & 2) ) {
        // simple check if prg header is attached
        romSize -= 2;
        rom += 2;
    }
    
    this->rom = rom;
    this->romSize = romSize;
    
    exRom = (rom && romSize) ? false : true;
}

auto Reu::isBootable( ) -> bool {
    
	if (expander)
		return expander->isBootable();
	
    return !exRom;
}

auto Reu::hasFreezeButton() -> bool {
    if (expander)
        return expander->hasFreezeButton();

    return false;
}

auto Reu::freeze() -> void {
    if (expander)
        expander->freeze();
}

auto Reu::setRamSize(int id) -> void {
    switch (id) {
        default:
        case 0: prepareRam( 128 ); break;
        case 1: prepareRam( 256 ); break;
        case 2: prepareRam( 512 ); break;
        case 3: prepareRam( 1024 ); break;
        case 4: prepareRam( 2048 ); break;
        case 5: prepareRam( 4096 ); break;
        case 6: prepareRam( 8192 ); break;
        case 7: prepareRam( 16384 ); break;
    }
}

auto Reu::getRamSize() -> int {
    switch (size) {
        case 128 * 1024: return 0;
        case 256 * 1024: return 1;
        case 512 * 1024: return 2;
        case 1024 * 1024: return 3;
        case 2048 * 1024: return 4;
        case 4096 * 1024: return 5;
        case 8192 * 1024: return 6;
        case 16384 * 1024: return 7;
    }
    return 0;
}

auto Reu::prepareRam(unsigned size) -> void {    
    unsigned sizeInKb = size;

    size <<= 10;
    
    if (data && (size == this->size))
        return;
    
    if(data)
        delete[] data;
        
    this->size = size;
        
    data = new uint8_t[size];     
    
    // when lower 19 bits of REU address reach this value, address wraps around to zero
    wrapAround = sizeInKb == 128 ? 0x20000 : 0x80000;
	dramWrapAround = size - 1;
	
    if (sizeInKb == 128)
        dramWrapAround = 0x1ffff;
    else if (sizeInKb == 256 || sizeInKb == 512)
        dramWrapAround = 0x7ffff;
    
    status = sizeInKb == 128 ? 0 : 0x10;    
}

auto Reu::setRam( uint8_t* dump, unsigned dumpSize ) -> void {
    this->dump = dump;
    this->dumpSize = dumpSize;       
}

auto Reu::unsetRam() -> void {
    this->dump = nullptr;
    this->dumpSize = 0;
}

auto Reu::injectRam( ) -> void {

    if (!dump || dumpSize == 0)
        return;
    
    std::memcpy(data, dump, std::min(dumpSize, size) );
}

auto Reu::reset(bool softReset) -> void {
    
	if (expander)
		expander->reset(softReset);
	
    status &= ~0xe0;    
	
	uint8_t value = 0;
	uint8_t inverter = 0;
	
	for(unsigned i = 0; i < size; i++) {
				
		if (++inverter == 2) {
			inverter = 0;
			
			value ^= 0xff;
		}		
		
		if (i != 0x20000) {	
			if ( (i & 0xff) == 0) {
				inverter = 1;
				value ^= 0xff;
			}
		}
		
		data[i] = value;
	}	
	
    injectRam( );
    
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
    dma = false;
	
	busValue = 0xff;
	busFloating = 0xff;

    memChange = nullptr;
}

inline auto Reu::readReu() -> uint8_t {

    reuAddr &= dramWrapAround;
    
    if (reuAddr < size)        
        return data[reuAddr];    
	
    return busFloating;
}

inline auto Reu::writeReu(uint8_t value) -> void {

    reuAddr &= dramWrapAround;
    
    if (reuAddr < size) {
        if (memChange)
            memChange->remember(reuAddr);

        data[reuAddr] = value;
    }
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

auto Reu::clock() -> void {

	if (expander)
		expander->clock();
	
    if (waitForStart) {
        // listen CPU bus usage        
        if (system->cpu.isWriteCycle()) {
            if (system->cpu.addressBus() == 0xff00) {
                waitForStart = false;
                sysTimer.add(&setDma, 1, Emulator::SystemTimer::Action::UpdateExisting);
            }
        }
    }  

    vicBaLow <<= 1;
    vicBaLow |= vicBA();

    if (!dma)
        return;

    switch (command & 3) {
        case 0: stash(); break;
        case 1: fetch(); break;
        case 2: swap(); break;
        case 3: verify(); break;
    }   
}

auto Reu::clockSCPU() -> void {
    vicBaLow <<= 1;
    vicBaLow |= vicBA();

    if (!dma)
        return;

    switch (command & 3) {
        case 0: stash<true>(); break;
        case 1: fetch<true>(); break;
        case 2: swap<true>(); break;
        case 3: verify<true>(); break;
    }
}

template<bool fromSCPU> inline auto Reu::verify() -> void {
    if (vicBaLow & 1)
        return;

    busValue = readReu();

    if constexpr (fromSCPU)
        busValue2 = system->superCpu->readByteReu(bus.addr = hostAddr);
    else
        busValue2 = system->memoryCpu.read( bus.addr = hostAddr);

    if (steal) {
        steal = false;

        if (transferLength == 1) {
            if (busValue == busValue2) {
                status |= 0x40; // end of block
            }
        }

        sysTimer.add(&finish, 1, Emulator::SystemTimer::Action::UpdateExisting);
        
        return;
    } 
    
    incrementAddresses();

    if (busValue != busValue2) {
        status |= 0x20; // verify error

        if (transferLength > 1)
            steal = true;
    }
        
    decrementTransferLength(); 
}

template<bool fromSCPU> auto Reu::swap() -> void {
    if (!swapRead) {
        if ((vicBaLow & 3) == 1) {
            if (transferLength == reg.transferLength)
                return;
        }

        if ((vicBaLow & 15) != 15) {
            busValue = readReu();

            if constexpr (fromSCPU) {
                busValue2 = system->superCpu->readByteReu(bus.addr = hostAddr);
            } else {
                if (((vicBaLow & 1) != 0) && system->isC64C())
                    busValue2 = system->ram[bus.addr = hostAddr];
                else
                    busValue2 = system->memoryCpu.read(bus.addr = hostAddr);
            }
        }

    } else {
        if (((vicBaLow & 1) == 0) || (transferLength == 1) ) {
            writeReu( busValue2 );
            if constexpr (fromSCPU)
                system->superCpu->writeByteReu(bus.addr = hostAddr, busValue);
            else
                system->memoryCpu.write( bus.addr = hostAddr, busValue);
            incrementAddresses();
            decrementTransferLength();
        }
    }

    swapRead ^= 1;
}

template<bool fromSCPU> inline auto Reu::fetch() -> void {
    if ((vicBaLow & 3) == 3) { // first BA cycle is usable for REU
        return;
    }

    if (vicBaLow & 2) { // BA off
        if (transferLength == (reg.transferLength-1))
            return;

    } else if (vicBaLow & 1) { // BA on
        if (transferLength == 1)
            return;
    }
    
	busFloating = busValue = readReu();
    
    if constexpr (fromSCPU)
        system->superCpu->writeByteReu( bus.addr = hostAddr, busValue );
    else
        system->memoryCpu.write( bus.addr = hostAddr, busValue );
    incrementAddresses();

	if (transferLength == 1)
		busFloating = readReu(); // dummy read

    decrementTransferLength();	
}

template<bool fromSCPU> inline auto Reu::stash() -> void {
    if (vicBaLow & 1) // VIC needs this cycle
        return;

    if constexpr (fromSCPU) {
        busFloating = busValue = system->superCpu->readByteReu(bus.addr = hostAddr);
    } else {
        if (vicII->reuSprite0() && system->isC64C())
            busFloating = busValue = system->ram[bus.addr = hostAddr];
        else
            busFloating = busValue = system->memoryCpu.read( bus.addr = hostAddr );
    }
    writeReu( busValue );
    incrementAddresses();
    decrementTransferLength();
}

inline auto Reu::decrementTransferLength() -> void {
    if (--transferLength == 0) {
        transferLength = 1;
        status |= 0x40;
        sysTimer.add( &finish, 1, Emulator::SystemTimer::Action::UpdateExisting );
    }
}

auto Reu::peekIo1( uint16_t addr ) -> uint8_t {
    if (expander)
        return expander->peekIo1( addr );

    return ExpansionPort::peekIo1( addr );
}

auto Reu::readIo1( uint16_t addr ) -> uint8_t {
	
	if (expander)
		return expander->readIo1( addr );
	
	// REU don't use it
	return ExpansionPort::readIo1( addr );
}

auto Reu::writeIo1( uint16_t addr, uint8_t value ) -> void {
	
	if (expander)
		expander->writeIo1( addr, value );
	
	// REU don't use it
}

auto Reu::peekIo2( uint16_t addr ) -> uint8_t {
    if (dma)
        return ExpansionPort::peekIo2( addr ); // open address space

    addr &= 0x1f;
    uint8_t val = 0xff;

    switch( addr ) {
        case 0:
            val = status; break;
        case 1:
            val = command; break;
        case 2:
            val = hostAddr & 0xff; break;
        case 3:
            val = (hostAddr >> 8) & 0xff; break;
        case 4:
            val = reuAddr & 0xff; break;
        case 5:
            val = (reuAddr >> 8) & 0xff; break;
        case 6:
            val = (reuAddr >> 16) | 0xf8; break;
        case 7:
            val = transferLength & 0xff; break;
        case 8:
            val = (transferLength >> 8) & 0xff; break;
        case 9:
            val = intMask | 0x1f; break;
        case 0xa:
            val = control | 0x3f; break;
    }

    if (expander)
        return val | expander->peekIo2( addr );

    return val;
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
            sysTimer.add( &unsetIrq, 1, Emulator::SystemTimer::Action::UpdateExisting );
            break;
        case 1:
            val = command; break;
        case 2:
            val = hostAddr & 0xff; break;
        case 3:
            val = (hostAddr >> 8) & 0xff; break;
        case 4:
            val = reuAddr & 0xff; break;
        case 5:
            val = (reuAddr >> 8) & 0xff; break;
        case 6:
            val = (reuAddr >> 16) | 0xf8; break;
        case 7:
            val = transferLength & 0xff; break;
        case 8:
            val = (transferLength >> 8) & 0xff; break;
        case 9:
            val = intMask | 0x1f; break;
        case 0xa:
            val = control | 0x3f; break;                 
    }
    
	if (expander)
		return val | expander->readIo2( addr );
		
    return val;
}

auto Reu::writeIo2( uint16_t addr, uint8_t value ) -> void {
    
	if (expander)
		expander->writeIo2( addr, value );
	
    if (dma)
        return;
    
    addr &= 0x1f;
    
    switch( addr ) {
        case 0: // read only
            break;
        case 1:
            command = value;
            if (command & 0x80) {
                waitForStart = true;
                
                if (command & 0x10) {                    
                    sysTimer.add( &setDma, 1, Emulator::SystemTimer::Action::UpdateExisting );
                    waitForStart = false;
					
					//system->interface->log( "start ", 1 );
					//system->interface->log( vicII->getVcounter(), 0 );
					//system->interface->log( vicII->getCycle(), 0 );
                }                    
            }
            break;
        case 2:
            hostAddr = reg.hostAddr = (reg.hostAddr & 0xff00) | value;
            break;
        case 3:
            hostAddr = reg.hostAddr = (reg.hostAddr & 0xff) | (value << 8);
            break;
        case 4:
            reuAddr = reg.reuAddr = (reg.reuAddr & 0xffff00) | value;
            break;
        case 5:
            reuAddr = reg.reuAddr = (reg.reuAddr & 0xff00ff) | (value << 8);
            break;
        case 6:
            reg.reuAddr = (reg.reuAddr & 0xffff) | (value << 16);
            reuAddr = (reuAddr & 0xffff) | (value << 16);
            break;
        case 7:
            //system->interface->log(dma,1);
            //system->interface->log("tl",0);
            //system->interface->log(reg.transferLength,0);
            transferLength = reg.transferLength = (reg.transferLength & 0xff00) | value;
            break;
        case 8:
            //system->interface->log(dma,1);
            //system->interface->log("th",0);
            //system->interface->log(reg.transferLength,0);
            transferLength = reg.transferLength = (reg.transferLength & 0xff) | (value << 8);
            break;
        case 9:
            intMask = value | 0x1f;
            
            if (allowIrq()) {
                // maybe intmask is activated after a finished transfer, but before reading status.
                status |= 0x80;
                sysTimer.add( &setIrq, 1, Emulator::SystemTimer::Action::UpdateExisting );                
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

auto Reu::peekRomL(uint16_t addr) -> uint8_t {

    if (expander)
        return expander->peekRomL(addr);

    if (!rom)
        return ExpansionPort::peekRomL( addr );

    addr %= romSize;

    return *(rom + addr);
}

auto Reu::readRomL(uint16_t addr) -> uint8_t {
	
	if (expander)
		return expander->readRomL(addr);
	
    if (!rom)
        return ExpansionPort::readRomL( addr );
    
    addr %= romSize;		
    
    return *(rom + addr);
}

// not used by REU itself, but possible expander
auto Reu::writeRomL( uint16_t addr, uint8_t data ) -> void {
	if (expander)
		expander->writeRomL(addr, data);
	else
		ExpansionPort::writeRomL( addr, data );
}

auto Reu::listenToWritesAt80To9F(uint16_t addr, uint8_t data ) -> void {
	if (expander)
		expander->listenToWritesAt80To9F(addr, data);
}

auto Reu::writeUltimaxRomL( uint16_t addr, uint8_t data ) -> void {
	if (expander)
		expander->writeUltimaxRomL(addr, data);
	else
		ExpansionPort::writeUltimaxRomL( addr, data );
}

auto Reu::peekRomH( uint16_t addr ) -> uint8_t {
    if (expander)
        return expander->peekRomH(addr);

    return ExpansionPort::peekRomH( addr );
}

auto Reu::readRomH( uint16_t addr ) -> uint8_t {
	if (expander)
		return expander->readRomH(addr);
	
	return ExpansionPort::readRomH( addr );
}

auto Reu::readUltimaxA0( uint16_t addr ) -> uint8_t {
	if (expander)
		return expander->readUltimaxA0(addr);
	
	return ExpansionPort::readUltimaxA0( addr );
}

auto Reu::peekUltimaxA0( uint16_t addr ) -> uint8_t {
    if (expander)
        return expander->peekUltimaxA0(addr);

    return ExpansionPort::peekUltimaxA0( addr );
}

auto Reu::writeRomH( uint16_t addr, uint8_t data ) -> void {
	if (expander)
		expander->writeRomH(addr, data);
	else
		ExpansionPort::writeRomH( addr, data );
}

auto Reu::writeUltimaxA0( uint16_t addr, uint8_t data ) -> void {
	if (expander)
		expander->writeUltimaxA0(addr, data);
	else
		ExpansionPort::writeUltimaxA0( addr, data );
}

auto Reu::serialize(Emulator::Serializer& s) -> void {
    unsigned _size = size;
    bool memUsage = s.memUsage();
    
    s.integer( _size );
    if ( s.mode() == Emulator::Serializer::Mode::Load ) {
        if (memUsage && memChange) {
            memChange->apply();
            memChange = nullptr;
        } else
            prepareRam( _size >> 10 ); // when size mismatches, recreate

    } else {
        if (memUsage && memChange)
            memChange->reset(data);
    }

    if (!memUsage)
        s.array( data, size );
    
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
    
    s.integer( wrapAround );
    s.integer( dramWrapAround );
    
    s.integer( waitForStart );
    s.integer( vicBaLow );
    s.integer( steal );
    s.integer( busValue );
    s.integer( busValue2 );
	s.integer( busFloating );
    s.integer( swapRead );     
    
    ExpansionPort::serialize( s );
	
	if (expander)
		expander->serialize( s );
}

auto Reu::getSizeNotConsideredForMemorySerialization() -> unsigned {
    if (expander)
        return size + expander->getSizeNotConsideredForMemorySerialization();
    return size;
}

}