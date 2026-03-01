
#include "vicIIFast.h"

namespace LIBC64  {

auto VicIIFast::serialize(Emulator::Serializer& s) -> void {
    s.integer( borderLeft );
    s.integer( borderRight );
	
    s.integer( dataC );
    s.integer( dataG );
	s.integer( ecmBmmMcm );
	s.integer( dmaDelay );
	
    s.integer( crop.rSel ); 
    s.integer( crop.cSel );
    s.integer( crop.top );
    s.integer( crop.bottom );
    s.integer( crop.left );
    s.integer( crop.right );
    s.integer( crop.topOverscan );
    s.integer( crop.bottomOverscan );
    s.integer( crop.leftOverscan );
    s.integer( crop.rightOverscan );

    s.array( colorReg );    
	s.integer( flags );
	s.integer( color );
	s.integer( vcBase );
    s.integer( vc );
    s.integer( rc );
	s.array( cBuffer );
		
    s.integer( cycle );
    s.integer( vCounter );
    s.integer( xCounterLatch );
	s.integer( xCounterLatchBefore );
    s.integer( vStart );
    s.integer( vHeight );
    s.integer( hWidth );
    s.integer( firstVisiblePixel );
	   
	s.integer( baLow );	
    s.integer( allowBadlines );
    s.integer( badLine );
    s.integer( irqLine );
    s.integer( lineIrqMatched );
    s.integer( irqLatchPending );
	
    s.integer( den );
    s.integer( borderTop );
    s.integer( borderBottom );
    s.integer( xScroll );
    s.integer( yScroll );
    s.integer( lpx );
    s.integer( lpy );
    s.integer( lpxBefore );
    s.integer( lpyBefore );
    s.integer( vm );
    s.integer( cb );
    s.integer( irqLatch );
    s.integer( irqEnable );
    s.integer( lpLatched );
    s.integer( lpPin );
    s.integer( lpTrigger );
    s.integer( lpTriggerDelay );
    s.integer( lpPhi1 );
    s.integer( rSel );
    s.integer( cSel );
	s.integer( modeEcmBmm );
    s.integer( modeMcm );  
    s.integer( controlReg1 );
    s.integer( controlReg2 );
    s.integer( linePos );
    s.integer( lineVCounter );
    s.integer( visibleLine );
    s.integer( hFlipFlop );
    s.integer( vFlipFlop );
    
    s.integer( idleMode );	
    s.integer( initVCounter );      
    
    for( unsigned i = 0; i < 8; i++ ) {
        Sprite& spr = sprite[i];
        
        s.integer( spr.enabled );                       
        s.integer( spr.dataP );
        s.integer( spr.dataS );
        s.integer( spr.dataShiftReg );
        s.integer( spr.shiftOut );
        s.integer( spr.mcBase );
        s.integer( spr.mc );
        s.integer( spr.y );
        s.integer( spr.x );
        
        s.integer( spr.prioMD );
        
        s.integer( spr.expandY );
        s.integer( spr.expandX );
        s.integer( spr.multiColor );
        s.integer( spr.mcFlop );
        s.integer( spr.expandYFlop );
        s.integer( spr.expandXFlop );
        s.integer( spr.colorCode );  
		
		s.integer( spr.xPos );
        s.integer( spr.mask );  
    }
	
	s.integer( spriteDma );
	s.integer( spriteActive );
    
    s.integer( spriteForegroundCollided );
    s.integer( spriteSpriteCollided );
    
    s.integer( canSpriteSpriteCollisionIrq );
    s.integer( canSpriteForegroundCollisionIrq );

    s.integer( ultimaxPhi1 );
    s.integer( ultimaxPhi2 );
    
    s.integer( reg2mhz );
    s.integer( vicBank );
}

}
