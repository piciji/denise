
#include "vicII.h"

namespace LIBC64 {

auto VicII::serialize(Emulator::Serializer& s) -> void {
        
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
    s.integer( lastSpriteShift ); 
    
    s.array( render );
    s.array( renderPipe );
    s.array( colorReg );
    s.array( colorUse );
    s.integer( lastColorReg );
    s.integer( cycle );
    s.integer( vCounter );
    s.integer( xCounter );
    s.integer( xCounterLatch );
    s.integer( xCounterSprites );
    s.integer( vStart );
    s.integer( vHeight );
    s.integer( hWidth );
    s.integer( lineCycles );
    s.integer( firstVisiblePixel );
    s.integer( baLow );
    s.integer( aecDelay ); 
    s.array( spriteBa );
    s.integer( allowBadlines );
    s.integer( badLine );
    s.integer( irqLine );
    s.integer( lineIrqMatched );
    s.integer( irqLatchPending );
    s.integer( den );
    s.integer( borderTop );
    s.integer( borderBottom );
    s.integer( yScroll );
    s.integer( lpx );
    s.integer( lpy );
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
    s.integer( modeEcmBmmDma );
    s.integer( modeMcmDma );
    s.integer( modeEcmBmmSequencer );
    s.integer( modeMcmSequencer );
    s.integer( display.color );
    s.integer( display.mcFlop );
    s.integer( display.dataC );
    s.integer( display.vcBase );
    s.integer( display.vc );
    s.integer( display.rc );
    s.integer( display.vmli );
    s.array( display.cBuffer );
    s.integer( display.cBufferPipe1 );
    s.integer( display.cBufferPipe2 );
    s.integer( display.xScroll );
    s.integer( display.gBuffer );
    s.integer( display.gBufferUse );
    s.integer( display.gBufferPipe1 );
    s.integer( display.gBufferPipe2 );
    s.integer( display.enable );
    s.integer( display.dmli );
    s.integer( display.gBufferShift );
    s.integer( display.gBits );
    
    uint8_t spriteOpenBusPos = 0xff;
    
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
        s.integer( spr.useExpandX );
        s.integer( spr.multiColor );
        s.integer( spr.useMultiColor );
        s.integer( spr.mcFlop );
        s.integer( spr.expandYFlop );
        s.integer( spr.expandXFlop );
        s.integer( spr.colorCode );    
        
        if (spriteOpenBus && (&spr == spriteOpenBus))
            spriteOpenBusPos = i;
    }
    
    s.integer( spriteTrigger );
    s.integer( spritePending );
    s.integer( spriteForegroundCollided );
    s.integer( spriteForegroundCollidedRead );
    s.integer( spriteSpriteCollided );
    s.integer( spriteSpriteCollidedRead );
    s.integer( spriteDmaCycle1 );
    s.integer( spriteDmaCycle2 );
    s.integer( clearCollision );
    s.integer( canSpriteSpriteCollisionIrq );
    s.integer( canSpriteForegroundCollisionIrq );
    s.integer( updateMc );
    s.integer( updatePrioExpand );
    s.integer( cAccessArea );
    s.integer( sprite0DmaLateBA );
    s.integer( disableEcmBmmTogether );
    s.integer( leftLineAnomaly.mode );
	s.integer( leftLineAnomaly.permanent );
	s.integer( leftLineAnomaly.framePos );
    s.integer( spriteOpenBusPos );
    
    xLookupPtrPhi1 = ntsc ? &xLookUpNtscPhi1[0] : &xLookUpPalPhi1[0];
    xLookupPtrPhi2 = ntsc ? &xLookUpNtscPhi2[0] : &xLookUpPalPhi2[0];    
    
    if (s.mode() == Emulator::Serializer::Mode::Load) {
        switch(cycle) {
            case 15: onHalfCycle = [this]() { borderLeft<true>(); }; break;
            case 16: onHalfCycle = [this]() { borderLeft<false>(); }; break;
            case 53: if(!ntsc) onHalfCycle = [this]() { spriteDmaCheck(); }; break;
            case 54: onHalfCycle = [this]() { borderRight<true>(); spriteDmaCheck(); }; break;
            case 55: onHalfCycle = [this]() { borderRight<false>(); if(ntsc) spriteDmaCheck(); }; break;
            default: onHalfCycle = nullptr; break;
        }
        
        spriteOpenBus = spriteOpenBusPos == 0xff ? nullptr : &sprite[spriteOpenBusPos];
    }
}

}
