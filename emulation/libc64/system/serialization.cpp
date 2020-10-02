
#include "system.h"

namespace LIBC64 {

auto System::calcSerializationSize() -> void {
        
    Emulator::Serializer s;
    
    unsigned signature = 0;
    char version[16] = {};
    char reserved[256] = {};

    s.integer(signature);
    s.array(version);
    s.array(reserved);
  
    serializeAll(s);
    
    serializationSize = s.size();
}

auto System::serialize(unsigned& size) -> uint8_t* {   
    
    if (keyBuffer->isPrgInjectionInQueue())
        return nullptr;
    
    Emulator::Serializer s( serializationSize );
    
    unsigned signature = 0x433634; // always constant for each emulation core
    char version[16] = {}; 
    char reserved[256] = {}; // reserved, e.g. state description
    
    auto str = Interface::Version;    
    str.copy( version, str.size() );

    s.integer(signature);
    s.array(version);
    s.array(reserved);
    
    serializeAll(s);
    
    // avoid extra copy of state memory
    serializer = std::move( s );
    
    size = serializer.size();
    
    return serializer.data();
}

auto System::checkSerialization(uint8_t* data, unsigned size) -> bool {
    
    if (size < 20)
        return false;
    
    Emulator::Serializer s( data, 20 );

    unsigned signature = 0;
    char version[16] = {};
    
    s.integer(signature);
    s.array(version);

    if (signature != 0x433634)
        return false;

    if (std::string(version) != Interface::Version)
        return false;   
    
    return true;
}

auto System::unserialize(uint8_t* data, unsigned size) -> bool {
    
    // states depends on:
    // region, connected devices
    // models
    // chipset, cpu, memory
    // enabled drives               
    // image paths
    
    Emulator::Serializer s( data, size );
    
    unsigned signature = 0;
    char version[16] = {};
    char reserved[256] = {};

    s.integer(signature);
    s.array(version);
    s.array(reserved);

    if(signature != 0x433634)
        return false;
    
    if( std::string(version) != Interface::Version)
        return false;          
    
    serializeAll(s);
    
    remapVic();
    remapCpu();  
    
    return true;
}    
    
auto System::serializeAll(Emulator::Serializer& s) -> void {

	uint8_t _vicModel = vicII->getModel();
	s.integer( _vicModel );
	
    bool useCycleRenderer = vicII == vicIICycle;
    s.integer(useCycleRenderer);

    if (s.mode() == Emulator::Serializer::Mode::Load) {
        setCycleRenderer( useCycleRenderer );    
		
		if (_vicModel != vicII->getModel())
			system->interface->setModel( LIBC64::Interface::ModelIdVicIIModel, _vicModel );		
	}
    
    serialize( s );
    cia1->serialize( s );
    cia2->serialize( s );
    vicII->serialize( s );
    Sid::searializeActiveSids( s );
    tape->serialize( s ); 
    iecBus->serialize( s );
    input->serialize( s );
    serializeExpansion( s );
    
    sysTimer.serialize( s );
        
    if (s.mode() == Emulator::Serializer::Mode::Load) {        
        cpu->setContext( cpuCtx );
        dispatcha();
    }
}    
    
inline auto System::serializeDiskIdle(Emulator::Serializer& s) -> void {
    s.integer( diskSilence.idle );
    s.integer( diskSilence.idleFrames );

    if (!diskSilence.active && (s.mode() == Emulator::Serializer::Mode::Load))
        diskSilence.idle = false;    
}

auto System::serialize(Emulator::Serializer& s) -> void {
    
    serializeDiskIdle( s );
    
    s.array( ram, 64 * 1024 );
    s.array( colorRam, 1 * 1024 );    
    s.integer( serializationSize );
    s.integer( mode );
    s.integer( vicBank );    
    s.integer( irqIncomming );
    s.integer( nmiIncomming );    
    s.integer( rdyIncomming );   
    s.integer( kernalBootComplete );    
    s.integer( requestedSids );
    keyBuffer->serialize( s );    
    prgInUse->serialize( s );
    glueLogic->serialize( s );
    powerSupply->serialize( s );    
    
    serialize6502( s, cpuCtx );   
}

auto System::serialize6502( Emulator::Serializer& s, MOS65Context* cpuCtx ) -> void {

    s.integer( cpuCtx->c );
    s.integer( cpuCtx->z );
    s.integer( cpuCtx->i );
    s.integer( cpuCtx->d );
    s.integer( cpuCtx->v );
    s.integer( cpuCtx->n );
    s.integer( cpuCtx->IR );
    s.integer( cpuCtx->a );
    s.integer( cpuCtx->x );
    s.integer( cpuCtx->y );
    s.integer( cpuCtx->s );
    s.integer( cpuCtx->pc );
    s.integer( cpuCtx->dataBus );
    s.integer( cpuCtx->addrBus );
    s.integer( cpuCtx->writeCycle );
    s.integer( cpuCtx->irqLine );
    s.integer( cpuCtx->nmiLine );
    s.integer( cpuCtx->nmiDetect );
    s.integer( cpuCtx->irqPending );
    s.integer( cpuCtx->nmiPending );
    s.integer( cpuCtx->interruptSampled );
    s.integer( cpuCtx->rdyLine );
    s.integer( cpuCtx->killed );
    s.integer( cpuCtx->magicAne );
	s.integer( cpuCtx->magicLax );
    s.integer( cpuCtx->soLine );
    s.integer( cpuCtx->soDetect );
    s.integer( cpuCtx->soSampled );
    s.integer( cpuCtx->resetCompleted );
    s.integer( cpuCtx->ddr );
    s.integer( cpuCtx->por );
    s.integer( cpuCtx->ioLines );
    s.integer( cpuCtx->pullup );
    s.integer( cpuCtx->pulldown );
    s.integer( cpuCtx->bit6.cycles );
    s.integer( cpuCtx->bit6.charge );
    s.integer( cpuCtx->bit7.cycles );
    s.integer( cpuCtx->bit7.charge );

    s.integer( cpuCtx->absolute );
    s.integer( cpuCtx->absIndexed );
    s.integer( cpuCtx->zeroPage );
    s.integer( cpuCtx->data );
    s.integer( cpuCtx->data2 );
    s.integer( cpuCtx->dataW );
    s.integer( cpuCtx->vector );
    s.integer( cpuCtx->displacement );
        
    s.integer( cpuCtx->boundaryCrossing );
    s.integer( cpuCtx->rdyLastCycle );
    s.integer( cpuCtx->xaa );
    s.integer( cpuCtx->cli );
    s.integer( cpuCtx->sei );
    s.integer( cpuCtx->storeFlags );
    s.integer( cpuCtx->soBlock );
    
    s.integer( cpuCtx->useDummy );
    s.integer( cpuCtx->resumeCycle );    
}

// for runahead
auto System::serializeLight() -> void {   
      
    auto& s = runAhead.serializer;
    
    s.setData( serializationSize );
    s.setMode( Emulator::Serializer::Mode::Save );
    
    serialize(s);
    cia1->serialize(s);
    cia2->serialize(s);
    vicII->serialize(s);
	Sid::searializeActiveSids( s, runAhead.frames > 1 );
    tape->serializeLight(s);
    iecBus->serializeLight(s);
    input->serialize(s);
    serializeExpansion(s);

    sysTimer.serialize(s);
}

auto System::unserializeLight() -> void {   
      
    auto& s = runAhead.serializer;
    
    s.setMode( Emulator::Serializer::Mode::Load );
    
    serialize(s);
    cia1->serialize(s);
    cia2->serialize(s);
    vicII->serialize(s);
    Sid::searializeActiveSids( s, runAhead.frames > 1 );
    tape->serializeLight(s);
    iecBus->serializeLight(s);
    input->serialize(s);
    serializeExpansion(s);

    sysTimer.serialize(s);    
    cpu->setContext(cpuCtx);        
    
    remapVic();
    remapCpu();  
}

}
