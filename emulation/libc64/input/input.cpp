
#include "input.h"
#include "../vicII/vicII.h"
#include "../../tools/bits.h"

#define portAOutputLo (lines->ddra & ~lines->pra)
#define portAOutputHi (lines->ddra & lines->pra)
#define portBOutputLo (lines->ddrb & ~lines->prb)
#define portBOutputHi (lines->ddrb & lines->prb)

namespace LIBC64 {       

Input::Input() {
    controlPort1 = new ControlPort;
    controlPort2 = new ControlPort;
}
    
auto Input::readCiaPortA( CIA::Base::Lines* lines ) -> uint8_t {
    this->lines = lines;
    
    jitPoll();
    
    uint8_t val = 0xff; // default high: no activity
	uint8_t rowMask = lines->ioa & controlPort2->read();
    uint8_t linkedRows = 0;
    uint8_t linkedCols = 0;
    
    for( unsigned row = 0; row < 8; row++ ) {
        if ( rowMask & (1 << row) )
            continue;

        keyboard.matrixResolveRow( row, linkedRows, linkedCols);
    }
    val &= ~linkedRows;

    uint8_t colMask = lines->iob & controlPort1->read();
    
    for( unsigned col = 0; col < 8; col++ ) {
        if ( colMask & (1 << col) )
            continue;

        linkedRows = 0, linkedCols = 0;
        keyboard.matrixResolveCol( col, linkedRows, linkedCols);

        if ( linkedCols & portBOutputHi )
            val &= ~keyboard.cols[col]; // eliminate ghosting
        else
            val &= ~linkedRows;
    }    
    
    return val & rowMask;
}

auto Input::readCiaPortB( CIA::Base::Lines* lines ) -> uint8_t {
    this->lines = lines;    
    
    jitPoll();

    uint8_t val = 0xff;
    uint8_t valControlPort1 = controlPort1->read();
	uint8_t colMask = lines->iob & valControlPort1;
    uint8_t linkedRows = 0;
    uint8_t linkedCols = 0;
    
    for( unsigned col = 0; col < 8; col++ ) {
        if ( colMask & (1 << col) )
            continue;

        keyboard.matrixResolveCol( col, linkedRows, linkedCols);
    }
    val &= ~linkedCols;

	uint8_t rowMask = lines->ioa & controlPort2->read();
        
    uint8_t pullUp = portBOutputHi;
	
	for( unsigned row = 0; row < 8; row++ ) {
        if ( rowMask & (1 << row) )
            continue;

        linkedRows = 0, linkedCols = 0;
        keyboard.matrixResolveRow( row, linkedRows, linkedCols);
        val &= ~linkedCols;

        if  ( ( portAOutputLo & (1 << row) ) && ( portBOutputHi & linkedCols ) ) {
            if ( ( (row == 1) && keyboard.shiftLock ) || Emulator::atLeastTwoBitsSeted( linkedRows & portAOutputLo ) ) {
                pullUp &= ~linkedCols;
            }            
        }
	}

    val &= lines->iob;
	val |= pullUp;

    return val & valControlPort1;
}

auto Input::writeCiaPortA( CIA::Base::Lines* lines ) -> void {
    this->lines = lines;
	updateLightpen( lines->ioa, lines->iob );
    potMask = (lines->ioa >> 6) & 3;
    
    controlPort2->write( lines->ioa );
}

auto Input::writeCiaPortB( CIA::Base::Lines* lines ) -> void {
    this->lines = lines;
    
	updateLightpen( lines->ioa, lines->iob );
    
    controlPort1->write( lines->iob );
}

inline auto Input::jitPoll() -> void {
    if (sampling.allow && ((sampling.mode == Dynamic_Sampling) || (sampling.midscreen < 2))) {
        if (system->interface->jitPoll(sampling.mode == Restricted_Dynamic_Sampling ? 5 : -1)) {
            keyboard.poll();
            updateLightpen(!lines ? 0xff : lines->ioa, !lines ? 0xff : lines->iob);
            sampling.midscreen++;
            //system->interface->log(vicII->getVcounter(), false);
        }
    }
}

auto Input::poll() -> void {
	
    bool jitDisable = !sampling.allow || (sampling.midscreen == 0);

    if ( jitDisable )
        keyboard.poll(); 
    
    controlPort1->poll();
    controlPort2->poll();
    
    if (jitDisable)
        // changed keyboard or joyport state can trigger lightpen    
        updateLightpen( !lines ? 0xff : lines->ioa, !lines ? 0xff : lines->iob );

    sampling.midscreen = 0;
}

auto Input::drawCursor(bool midScreen) -> void {
    controlPort1->draw( midScreen );
    controlPort2->draw( midScreen );
}

auto Input::restore() -> bool {
    
    return keyboard.restore();
}

auto Input::updateLightpen( uint8_t ioa, uint8_t iob ) -> void {
	uint8_t out = iob & controlPort1->read( );
	uint8_t rowMask = ioa & controlPort2->read( );
	uint8_t portAInfluence = 0xff; 
	
	for (unsigned i = 0; i < 8; i++) {
        if (!(rowMask & (1 << i)))
            portAInfluence &= ~keyboard.rows[i];
    }

	out &= portAInfluence;
	vicII->triggerLightPen( (out >> 4) & 1 ); // isolate bit 4
}

auto Input::readPotX() -> uint8_t {    
    
    switch(potMask) {
        case 1:
            return controlPort1->getPotX();
        case 2:
            return controlPort2->getPotX();
        case 3:
            return controlPort1->getPotX() & controlPort2->getPotX();
        default:
            return 0xff;
    }
}

auto Input::readPotY() -> uint8_t {    
    
    switch(potMask) {
        case 1:
            return controlPort1->getPotY();
        case 2:
            return controlPort2->getPotY();
        case 3:
            return controlPort1->getPotY() & controlPort2->getPotY();
        default:
            return 0xff;
    }
}

auto Input::reset() -> void {
    lines = nullptr;
    potMask = 1;
    keyboard.reset();
    controlPort1->reset();
    controlPort2->reset();
    sampling.midscreen = 0;
}

auto Input::connectControlport( Interface::Connector* connector, Interface::Device* device ) -> void {
    
    if (!connector)
        return;
    
    ControlPort** controlPort = connector->isPort1() ? &controlPort1 : &controlPort2;

    if ((*controlPort)->device == device)
        return;
    
    if (*controlPort)
        delete *controlPort;
    
    *controlPort = ControlPort::create( device );

    updateSampling();
    
    (*controlPort)->reset();
}

auto Input::setSampling(uint8_t mode) -> void {
    sampling.mode = (SamplingMode)mode;
    updateSampling();
}

auto Input::updateSampling() -> void {
    if (system->runAhead.preventJit && system->runAhead.frames)
        sampling.allow = false;
    else
        sampling.allow = (sampling.mode != 0) && !system->enabledDebugCart() && controlPort1->useJitPolling() && controlPort2->useJitPolling();
}

auto Input::getConnectedDevice( Interface::Connector* connector ) -> Interface::Device* {
    
    ControlPort* controlPort = connector->isPort1() ? controlPort1 : controlPort2;
    
    return controlPort->device;
}

auto Input::getCursorPosition( Interface::Device* device, int16_t& x, int16_t& y ) -> bool {
    
    if (controlPort1->device == device)
        return controlPort1->getCursorPosition( x, y );
    
    if (controlPort2->device == device)
        return controlPort2->getCursorPosition( x, y );
    
    return false;
}

auto Input::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( potMask );
    
    this->lines = &cia1->lines;
    
    keyboard.serialize( s );
    
    for( auto& connector : system->interface->connectors ) {
        
        Interface::Device* device = getConnectedDevice( &connector );
        
        if (!device)
            device = system->interface->getUnplugDevice();
        
        unsigned deviceId = device->id;
        
        s.integer( deviceId );
        
        if ( s.mode() == Emulator::Serializer::Mode::Load ) {
            
            if (deviceId != device->id) {
                // state was generated with another connected device.
                // we need to connect the requested device.
                device = system->interface->getDevice( deviceId );
                
                connectControlport( &connector, device );
            }
        }
        
        ControlPort* controlPort = connector.isPort1() ? controlPort1 : controlPort2;
        
        controlPort->serialize( s );
    }
    
    if ( s.mode() == Emulator::Serializer::Mode::Load )
        sampling.midscreen = 0;
}

}
