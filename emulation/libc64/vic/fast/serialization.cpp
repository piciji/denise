
#include "vicIIFast.h"

namespace LIBC64  {

auto VicIIFast::serialize(Emulator::Serializer& s) -> void {
    
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
    s.integer( rev65 );
    s.integer( lastReadPhi1 );
    
    s.array( colorReg );    
    s.integer( lastColorReg );
    s.integer( cycle );
    s.integer( vCounter );
    s.integer( xCounter );
    s.integer( vStart );
    s.integer( vHeight );
    s.integer( hWidth );
    s.integer( lineCycles );
    s.integer( firstVisiblePixel );
    s.array( spriteBa );
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
    s.integer(lpxBefore);
    s.integer(lpyBefore);
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
    s.integer( controlReg1 );
    s.integer( controlReg2 );
    s.integer( linePos );
    s.integer( lineVCounter );
    s.integer( visibleLine );
    s.integer( hFlipFlop );
    s.integer( vFlipFlop );
    s.integer( vFlipFlopShadow );
    s.integer( idleMode );
	s.integer( idleModeTemp );
    s.integer( initVCounter );
    s.integer( refreshCounter );
    s.integer( ntsc );
    s.integer( modeEcmBmm );
    s.integer( modeMcm );    
    s.integer( dmaDelay );
    
    for( unsigned i = 0; i < 8; i++ ) {
        Sprite& spr = sprite[i];
        
        s.integer( spr.enabled );
        s.integer( spr.dma );
        s.integer( spr.halt );
        s.integer( spr.active );
        s.integer( spr.dataP );
        s.integer( spr.dataS );
        s.integer( spr.dataShiftReg );
        s.integer( spr.shiftOut );
        s.integer( spr.mcBase );
        s.integer( spr.mc );
        s.integer( spr.y );
        s.integer( spr.x );
        s.integer( spr.useX );
        s.integer( spr.prioMD );
        s.integer( spr.usePrioMD );
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
    
    s.integer( spriteForegroundCollided );
    s.integer( spriteSpriteCollided );
    s.integer( cAccessArea );
    s.integer( leftLineAnomaly.mode );
	s.integer( leftLineAnomaly.permanent );
	s.integer( leftLineAnomaly.framePos );
    
    s.integer( borderLeft );
    s.integer( borderRight );
    
    s.integer( color );
    s.integer( dataC );
    s.integer( dataG );
    s.integer( ecmBmmMcm );
    s.integer( vcBase );
    s.integer( vc );
    s.integer( rc );
    
    s.array( cBuffer );        
    
    xLookupPtrPhi1 = ntsc ? &xLookUpNtscPhi1[0] : &xLookUpPalPhi1[0];
    xLookupPtrPhi2 = ntsc ? &xLookUpNtscPhi2[0] : &xLookUpPalPhi2[0];        

}

}
