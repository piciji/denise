
#include "vicII.h"

namespace LIBC64 {

auto VicIICycle::serialize(Emulator::Serializer& s) -> void {
	
	s.integer( flags );
	
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

	s.integer(leftLineAnomaly.mode);
	s.integer(leftLineAnomaly.permanent);
	s.integer(leftLineAnomaly.framePos);
	
	s.integer( color );
    s.integer( mcFlop );
    s.integer( dataC );
    s.integer( vcBase );
    s.integer( vc );
    s.integer( rc );
    s.integer( vmli );
    s.array( cBuffer );
    s.integer( cBufferPipe1 );
    s.integer( cBufferPipe2 );
    s.integer( xScrollPipe );
    s.integer( gBuffer );
    s.integer( gBufferUse );
    s.integer( gBufferPipe1 );
    s.integer( gBufferPipe2 );
    s.integer( dmli );
    s.integer( gBufferShift );
    s.integer( gBits );
	
    s.integer( lastReadPhi1 );
	s.integer( lastBusPhi2 );
	s.integer( greyDotBugDisabled );
    
    s.array( render );
    s.array( renderPipe );
    s.array( colorReg );
    s.array( colorUse );
    s.integer( lastColorReg );
	
    s.integer( cycle );
    s.integer( vCounter );
    s.integer( xCounterLatch );
    s.integer( xCounterLatchBefore );
    s.integer( vStart );
    s.integer( vHeight );
    s.integer( hWidth );
    s.integer( firstVisiblePixel );
	
    s.integer( baLow );
    s.integer( aecDelay ); 
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
    s.integer( modeEcmBmm );
    s.integer( modeMcm );
    s.integer( modeEcmBmmDma );
    s.integer( modeEcmBmmSequencer );
    s.integer( modeMcmSequencer );
    
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
        s.integer( spr.useX );
        s.integer( spr.prioMD );
        s.integer( spr.usePrioMD );
        s.integer( spr.expandY );
        s.integer( spr.expandX );
        s.integer( spr.useExpandX );
        s.integer( spr.multiColor );
        s.integer( spr.useMultiColor );
        s.integer( spr.mcFlop );
        s.integer( spr.expandYFlop );
        s.integer( spr.expandXFlop );
        s.integer( spr.colorCode );    
    }
	
	s.integer( spriteDma );
	s.integer( spriteActive );
	s.integer( spriteHalt );
    s.integer( spriteTrigger );
    s.integer( spritePending );
    s.integer( spriteForegroundCollided );
    s.integer( spriteForegroundCollidedRead );
    s.integer( spriteSpriteCollided );
    s.integer( spriteSpriteCollidedRead );   
    s.integer( clearCollision );
    s.integer( canSpriteSpriteCollisionIrq );
    s.integer( canSpriteForegroundCollisionIrq );
    s.integer( updateMc );
    s.integer( updatePrioExpand );
    s.integer( sprite0DmaLateBA );
    s.integer( disableEcmBmmTogether );

    s.integer( ultimaxPhi1 );
    s.integer( ultimaxPhi2 );
    
    s.integer( reg2mhz );
    s.integer( vicBank );
}

}
