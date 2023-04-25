
#include "system.h"
#include "../traps/traps.h"

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
    
    if (keyBuffer->isPrgInjectionInQueue() || traps.installed)
        return nullptr;

    iecBus.updateSerializationSize();
    
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

    remapCpu();

    updateDriveSounds();
    
    return true;
}    
    
auto System::serializeAll(Emulator::Serializer& s) -> void {

	uint8_t _vicModel = vicII->getModel();
	s.integer( _vicModel );
	
    bool useCycleRenderer = vicII == &vicIICycle;
    s.integer(useCycleRenderer);

    if (s.mode() == Emulator::Serializer::Mode::Load) {
        setCycleRenderer( useCycleRenderer );    
		
		if (_vicModel != vicII->getModel())
			interface->setModelValue( LIBC64::Interface::ModelIdVicIIModel, _vicModel );
	}
    
    serialize( s );
    cia1.serialize( s );
    cia2.serialize( s );
    vicII->serialize( s );
    sidManager.searializeActiveSids( s );
    tape.serialize( s );
    iecBus.serialize( s );
    input.serialize( s );
    serializeExpansion( s );
    
    sysTimer.serialize( s );        
}    
    
inline auto System::serializeDiskIdle(Emulator::Serializer& s) -> void {
    s.integer( diskSilence.idle );
    s.integer( diskSilence.idleFrames );

    if (!diskSilence.active && (s.mode() == Emulator::Serializer::Mode::Load)) {
        diskIdleOff();
    }
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
    s.integer( secondDriveCable.burstUse );
    s.integer( secondDriveCable.parallelUse );
    s.integer( secondDriveCable.burstPossible );
    s.integer( secondDriveCable.parallelPossible );
    s.integer( secondDriveCable.burstRequested );
    s.integer( secondDriveCable.parallelRequested );
    s.integer( secondDriveCable.parallelUserport );
    s.integer( secondDriveCable.parallelExpansion );
    s.integer( secondDriveCable.cycleSyncing );

    s.integer( observer.memoryAccesses );
    s.integer( observer.enterRom );
    s.integer( observer.stateChange );
    s.integer( observer.motor );
    s.integer( observer.inputFetches );
    s.integer( observer.inputLock );
    keyBuffer->serialize( s );    
    prgInUse->serialize( s );
    glueLogic.serialize( s );
    powerSupply->serialize( s );    
    cpu.serialize( s );
}

// for runahead
auto System::serializeLight() -> void {   
      
    auto& s = runAhead.serializer;
    
    s.setData( serializationSize );
    s.setMode( Emulator::Serializer::Mode::Save );
    
    serialize(s);
    cia1.serialize(s);
    cia2.serialize(s);
    vicII->serialize(s);
    sidManager.searializeActiveSids( s, runAhead.frames > 1 );
    tape.serialize(s, true);
    iecBus.serializeLight(s);
    input.serialize(s);
    expansionPort->serialize( s );

    sysTimer.serialize(s);
}

auto System::unserializeLight() -> void {   
      
    auto& s = runAhead.serializer;
    
    s.setMode( Emulator::Serializer::Mode::Load );
    uint8_t _mode = mode;
    
    serialize(s);
    cia1.serialize(s);
    cia2.serialize(s);
    vicII->serialize(s);
    sidManager.searializeActiveSids( s, runAhead.frames > 1 );
    tape.serialize(s, true);
    iecBus.serializeLight(s);
    input.serialize(s);
    expansionPort->serialize( s );

    sysTimer.serialize(s);         

    if (mode != _mode)
        remapCpu();
}

}
