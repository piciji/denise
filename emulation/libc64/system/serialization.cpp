
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
    // in this context there are two kinds of gui elements:
    // 1. switchable during runtime (some features)
    // 2. not switchable during runtime (grayed out)
    // for first kind the actual values will be used and the feature
    // gui is updated from the values of the state file.
    // for second kind the actual values will be used too but the relevant
    // gui elements (grayed out) will be not updated according to the
    // state file.
    
    // states depends on:
    // region, connected devices
    // features
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
    
    serialize( s );
    cia1->serialize( s );
    cia2->serialize( s );
    vicII->serialize( s );
    sid->serialize( s );
    cart->serialize( s );
    tape->serialize( s ); 
    iecBus->serialize( s );
    input->serialize( s );
    
    events.serialize( s );
}    
    
auto System::serialize(Emulator::Serializer& s) -> void {
    
    s.array( ram, 64 * 1024 );
    s.array( colorRam, 1 * 1024 );    
    s.integer( serializationSize );
    s.integer( mode );
    s.integer( vicBank );    
    s.integer( irqIncomming );
    s.integer( nmiIncomming );    
    s.integer( ntsc );
    s.integer( kernalBootComplete );    
    keyBuffer->serialize( s );
    prg->serialize( s );
    glueLogic->serialize( s );
    powerSupply->serialize( s );
    
    serialize6502( s, cpuCtx );    
    //serialize6502( s, cpu-> );    
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
    s.integer( cpuCtx->db );
    s.integer( cpuCtx->addrBus );
    s.integer( cpuCtx->irqLine );
    s.integer( cpuCtx->nmiLine );
    s.integer( cpuCtx->nmiDetect );
    s.integer( cpuCtx->irqPending );
    s.integer( cpuCtx->nmiPending );
    s.integer( cpuCtx->interruptSampled );
    s.integer( cpuCtx->rdyLine );
    s.integer( cpuCtx->killed );
    s.integer( cpuCtx->magicAne );
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
    s.integer( cpuCtx->dataH );
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
}

}
