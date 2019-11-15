
/**
 * all the following findings are the work of the VICE team
 * 
 */

#include "input.h"
#include "../vic/vicII.h"

namespace LIBC64 {       

Input::Input() {
    controlPort1 = new ControlPort;
    controlPort2 = new ControlPort;
}
    
auto Input::readCiaPortA( CIA::Base::Lines* lines ) -> uint8_t {
    
    this->lines = lines;
    
    uint8_t val = 0xff; // default high: no activity 
    // [ reading game port 2 ]
	// cia1 port A reads the state of game port 2
	// direct game port input or cia in output mode can be used to pull
	// down the bits ( read '0' bits or called active low )
	// cia1 port A controls the rows of the keyboard matrix too
	uint8_t rowMask = lines->ioa & controlPort2->read( );
    
    for( unsigned row = 0; row < 8; row++ ) {
		// all 0-bits activate the appropriate row
        if ( rowMask & (1 << row) )
            continue;
		
		// the trigger rows pull down the bits always and inform about
		// the current input state described above, that is expected behaviour.
		// if the keyboard is pressed the same time it gets more complicated.
		// each activated row will be scanned for pressed keys. The column which
		// belongs to a pressed key in this row is scanned for pressed keys too.
		// The appropriate row will be activated too, if it isn't already.
		// In the end more bits could be pulled down.		
        val &= ~keyboard.matrixGetActiveRowsByRow( row );
    }
	
    // [ side effect: game port 1 + keyboard ]
    // the state of game port 1 can indirectly change the result of cia1 port A
    // but only in combination with pressed keyboard at the same time
	// ok, like described above we scan the activity of cia1 port B
	// cia1 port B controls the columns of the keyboard matrix too
    uint8_t colMask = lines->iob & controlPort1->read( );
    
    for( unsigned col = 0; col < 8; col++ ) {
        // all 0-bits activate the appropriate column
        if ( colMask & (1 << col) )
            continue;
        // like described above we scan for pressed keys on this column
		// to get the appropriate rows and scan the rows for more pressed keys
		// to get the columns, which will be activated too
        uint8_t tmp = keyboard.matrixGetActiveColumnsByColumn( col );
                
		// if port B bits are in output mode and pulled up and their appropriate
		// columns activated then all pressed keys for this column will pull down
		// the bits. This eliminates ghost keys. 
        if ( tmp & lines->prb & lines->ddrb )
            // no ghostkeys
            val &= ~keyboard.cols[col];

        else
            // ghostkeys can happen in input mode, because all rows
			// which are connected about this column will be activated
            val &= ~keyboard.matrixGetActiveRowsByColumn( col );
    }    
    
    return val;
}

auto Input::moreThanTwoRowsActivated( uint8_t row ) -> bool {
    
    uint8_t out = keyboard.matrixGetActiveRowsByRow( row ) 
        & ( lines->ddra & ~lines->pra );
         
    // at least 2 bits are logical 1, bit position doesn't matter.
    if ( ( out & (out - 1) ) != 0 )
        return 1;
        
    return 0;
}

auto Input::readCiaPortB( CIA::Base::Lines* lines ) -> uint8_t {
    
    this->lines = lines;    
    
    uint8_t val = 0xff;
    // [ reading game port 1 ]
	// cia1 port B reads the state of game port 1
	// direct game port input or cia in output mode can be used to pull
	// down the bits ( read '0' bits )
	// cia1 port B controls the columns of the keyboard matrix too
	uint8_t colMask = lines->iob & controlPort1->read( );
    
    for( unsigned col = 0; col < 8; col++ ) {
		// all 0-bits activate the appropriate columns
        if ( colMask & (1 << col) )
            continue;
		
		// the trigger columns pull down the bits always and inform about
		// the current input state described above, that is expected behaviour.
		// if the keyboard is pressed the same time it gets more complicated.
		// each activated column will be scanned for pressed keys. The row which
		// belongs to a pressed key in this column is scanned for pressed keys too.
		// The appropriate column will be activated too, if it isn't already.
		// In the end more bits could be pulled down.		
        val &= ~keyboard.matrixGetActiveColumnsByColumn( col );
    }
	
    // [ reading keyboard ]
	// the state of game port 2 can indirectly change the result of cia1 port B
    // but only in combination with pressed keyboard at the same time
	// ok, like described above we scan the activity of cia1 port A
	// cia1 port A controls the rows of the keyboard matrix too
	uint8_t rowMask = lines->ioa & controlPort2->read( );
        
    uint8_t outHi = lines->ddrb & lines->prb;
	
	for( unsigned row = 0; row < 8; row++ ) {
		// all 0-bits activate the appropriate rows
        if ( rowMask & (1 << row) )
            continue;
        
		// the keyboard matrix will be scanned for pressed keys in each
        // activated row and the appropriate columns will be activated
		uint8_t tmp = keyboard.matrixGetActiveColumnsByRow( row );
        val &= ~tmp;
        
        // [ special case when both ports in output mode ]
        if  ( ( lines->ddra & ~lines->pra & (1 << row) ) && 
              ( lines->ddrb & lines->prb & tmp ) ) {
            			
            // this way you can distinguish between left shift and shift lock
            // when both ports in output mode a single left shift can not drive port B
            // low, you need at least another key in the same column. But a shift lock
            // driven left shift can drive port B low without any other key press needed
            if ( ( (row == 1) && keyboard.shiftLock ) 
                || moreThanTwoRowsActivated( row ) ) {
                
                outHi &= ~tmp;
            }            
        }
	}
	
    // bits where port A is output low and port B output hi are considered active by special conditions
    // if these conditions (directly above) are not met, they will pulled up again
	val |= outHi;
    
    // normally active game port 1 bits were pulled down in the beginning of this function
    // but because of the pull up (the code line before) we have to override this.
    // a game port input pull down the appropriate bits, doesn't matter what the keyboard does :-)
    // this multi usage is annoying, luckily the amiga controls the keyboard on cia serial function
    return val & controlPort1->read( );
}

auto Input::writeCiaPortA( CIA::Base::Lines* lines ) -> void {
    this->lines = lines;
	// read below why a port A write can trigger lightpen
	updateLightpen( lines->ioa, lines->iob );
    // controls which pots are transfered to Sid DA
    potMask = (lines->ioa >> 6) & 3;
    
    controlPort2->write( lines->ioa );
}

auto Input::writeCiaPortB( CIA::Base::Lines* lines ) -> void {
    this->lines = lines;
    
	updateLightpen( lines->ioa, lines->iob );
    
    controlPort1->write( lines->iob );
}

auto Input::poll() -> void {
	
	keyboard.poll(); 
    
    controlPort1->poll();
    controlPort2->poll();
    
	// changed keyboard or joyport state can trigger lightpen    
	updateLightpen( !lines ? 0xff : lines->ioa, !lines ? 0xff : lines->iob );
}

auto Input::drawCursor(bool midScreen) -> void {
    controlPort1->draw( midScreen );
    controlPort2->draw( midScreen );
}

auto Input::restore() -> bool {
    
    return keyboard.restore();
}

auto Input::updateLightpen( uint8_t ioa, uint8_t iob ) -> void {
	// basically lightpen is controlled by cia1 port B.
	// and indirectly by port A, because of the keyboard matrix port B rows
	// are connected to port A columns by pressed keys.
	// and that isn't all.
	// the state of game port 2 over cia1 port A influence 
	// involved keyboard rows.
	// normal behaviour without external influence: iob = prb | ~ddrb
	// in input mode, when ddrb pins are unset, cia chips pull pins up.
	// we are only interested in pin 4, thats the joypad fire button or
	// lightpen trigger.
	// if pressed it reads a zero and can pull down iob but never pull it up
	// so we can trigger the VicII lightpen latch by cia itsself or external
	// joypad, lightgun and so on.
	uint8_t out = iob & controlPort1->read( );
	// now we consider the complicated side effects mentioned above
	// we do the same for port A, which connects devices in port 2
	// by the way, if there is no device connected the button state is 0xff (unpressed)
	// the result doesn't influence port A directly but the involved rows of the
	// keyboard matrix
	uint8_t rowMask = ioa & controlPort2->read( );
	// default: no influence by port A
	uint8_t portAInfluence = 0xff; 
	
	for (unsigned i = 0; i < 8; i++)
		if ( !( rowMask & ( 1 << i ) ) )
			// all involved rows will be checked for pressed keyboard keys 
			// each key is a combination of a row and a column
			// if column bit 4 of this row belongs to a pressed key,
			// lightpen trigger is active, even when port B doesn't trigger
			portAInfluence &= ~keyboard.rows[i];
	
	// the influence can only switch a non-trigger to trigger but not vice versa because of the 
	// "and" concatenation
	out &= portAInfluence;
	// isolate bit 4
	vicII->triggerLightPen( (out >> 4) & 1 );		
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

auto Input::clock() -> void {
    
    controlPort1->tick();
    controlPort2->tick();
}

auto Input::reset() -> void {
    lines = nullptr;
    potMask = 1;
    keyboard.reset();
    controlPort1->reset();
    controlPort2->reset();
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
    
    (*controlPort)->reset();
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
    
    this->lines = &system->cia1->lines;
    
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
}

}

