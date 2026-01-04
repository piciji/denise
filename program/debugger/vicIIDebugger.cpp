
#include "vicIIDebugger.h"
#include "../program.h"
#include "../thread/emuThread.h"

VicIIDebugger::VicIIDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Mode::VICII ) {
    build();
}

VicIIDebugger::Video::Wraper::RegWrapper::Mode::Mode() {
    modeVal.setFont( GUIKIT::Font::system( "", true ) );
    modeVal.setEditable( false );
    modeLabel.setAlign( GUIKIT::Label::Align::Right );

    append(modeLabel, {90u, 0u}, 10);
    append(modeVal, {~0u, 0u});

    setAlignment( 0.5 );
}

VicIIDebugger::Video::Wraper::RegWrapper::Registers::Registers() {
    left.setAlign( GUIKIT::Label::Align::Right );
    right.setAlign( GUIKIT::Label::Align::Right );

    leftVal.setFont( GUIKIT::Font::system( "", true ) );
    rightVal.setFont( GUIKIT::Font::system( "", true ) );
    leftVal.setEditable( false );
    rightVal.setEditable( false );
    leftVal.setAlign( GUIKIT::LineEdit::Align::Right );
    rightVal.setAlign( GUIKIT::LineEdit::Align::Right );

    leftVal.setText( "0" );
    leftVal.setStore( 0 );
    rightVal.setText( "0" );
    rightVal.setStore( 0 );

    append(left, {90u, 0u}, 10);
    append(leftVal, {getWidth(4, true, false), 0u}, 10);
    append(right, {110u, 0u}, 10);
    append(rightVal, {getWidth(4, true, false), 0u});

    setAlignment( 0.5 );
}

VicIIDebugger::Video::Wraper::RegWrapper::RegWrapper() {
    setPadding( 10 );
    registers.resize( 7 );
    append( mode, {~0u, 0u}, 7 );

    for (auto& reg : registers) {
        reg = new Registers();
        append(*reg, {0u, 0u}, registers.back() == reg ? 0 : 7);
    }
}

VicIIDebugger::Video::Wraper::Flags::First::First() {
    den.setReadonly( false );
    badLine.setReadonly( false );
    append( den, {0u, 0u}, 5 );
    append( badLine, {0u, 0u} );

}

VicIIDebugger::Video::Wraper::Flags::Second::Second() {
    idle.setReadonly( false );
    vblank.setReadonly( false );
    append( idle, {0u, 0u}, 5 );
    append( vblank, {0u, 0u} );
}

VicIIDebugger::Video::Wraper::Flags::Third::Third() {
    hFlop.setReadonly( false );
    vFlop.setReadonly( false );
    append( hFlop, {0u, 0u}, 5 );
    append( vFlop, {0u, 0u} );
}

VicIIDebugger::Video::Wraper::Flags::Flags() {
    append(spacer, {~0u, 0u});
    append(first, {0u, 0u}, 30);
    append(second, {0u, 0u}, 30);
    append(third, {0u, 0u});

    setPadding( 10 );
    setAlignment( 0.5 );
}

VicIIDebugger::Video::Wraper::Lp::Lp() {
    valX.setFont( GUIKIT::Font::system("", true ) );
    valX.setText( "0" );
    valX.setStore( 0 );
    valX.setEditable( false );
    valX.setAlign( GUIKIT::LineEdit::Align::Right );

    valY.setFont( GUIKIT::Font::system("", true ) );
    valY.setText( "0" );
    valY.setStore( 0 );
    valY.setEditable( false );
    valY.setAlign( GUIKIT::LineEdit::Align::Right );

    line.setReadonly(  );
    latched.setReadonly(  );

    append( spacer, {~0u, 0u} );
    append(labelX, {0u, 0u}, 5u);
    append(valX, {getWidth(2, true, false), 0u}, 10u);
    append(labelY, {0u, 0u}, 5u);
    append(valY, {getWidth(2, true, false), 0u}, 10u);
    append(line, {0u, 0u}, 10u);
    append(latched, {0u, 0u});

    setAlignment( 0.5 );
    setPadding( 10 );
}

VicIIDebugger::Video::Wraper::Intr::Latch::Latch() {
    latchVal.setFont( GUIKIT::Font::system("", true ) );
    latchVal.setText( "0" );
    latchVal.setStore( 0 );
    latchVal.setEditable( false );

    intLine.setReadonly();
    lightPen.setReadonly();
    ssCollision.setReadonly();
    sfCollision.setReadonly();
    raster.setReadonly();

    append(latch, {35u, 0u}, 5);
    append(latchVal, {getWidth(2, true, false), 0u}, 10);
    append(intLine, {0u, 0u}, 5);
    append(lightPen, {0u, 0u}, 5);
    append(ssCollision, {0u, 0u}, 5);
    append(sfCollision, {0u, 0u}, 5);
    append(raster, {0u, 0u});

    setAlignment( 0.5 );
}

VicIIDebugger::Video::Wraper::Intr::Mask::Mask() {
    maskVal.setFont( GUIKIT::Font::system("", true ) );
    maskVal.setText( "0" );
    maskVal.setStore( 0 );
    maskVal.setEditable( false );

    intLine.setEnabled( false );
    lightPen.setReadonly();
    ssCollision.setReadonly();
    sfCollision.setReadonly();
    raster.setReadonly();

    append(mask, {35u, 0u}, 5);
    append(maskVal, {getWidth(2, true, false), 0u}, 10);
    append(intLine, {0u, 0u}, 5);
    append(lightPen, {0u, 0u}, 5);
    append(ssCollision, {0u, 0u}, 5);
    append(sfCollision, {0u, 0u}, 5);
    append(raster, {0u, 0u});

    setAlignment( 0.5 );
}

VicIIDebugger::Video::Wraper::Intr::Intr() {
    append( latch, {0u, 0u}, 7 );
    append( mask, {0u, 0u} );

    setPadding( 10 );
}

VicIIDebugger::Video::Wraper::Wraper() {

    append( regWrapper, {~0u, 0u}, 7 );
    append( flags, {~0u, 0u}, 7 );
    append( intr, {~0u, 0u}, 7 );
    append( lp, {~0u, 0u} );
}

VicIIDebugger::Video::Sprites::Selector::Selector() {
    append(label, {0u, 0u}, 10);

    std::vector<GUIKIT::RadioBox*> boxes;
    for ( int i = 0; i < 8; i++ ) {
        spr[i].setText( std::to_string( i ) );
        spr[i].setStore( 0 );
        append(spr[i], {0u, 0u}, 5);

        boxes.push_back( &spr[i] );
    }
    GUIKIT::RadioBox::setGroup( boxes );
}

VicIIDebugger::Video::Sprites::Viewer::Viewer() {
    canvas.setPadding( 2 );
    append(canvas, {360u, 300u});
}

VicIIDebugger::Video::Sprites::Props::Props() {
    append( priority, {0u, 0u}, 10 );
    append( multiColor, {0u, 0u}, 10 );
    append( sfCollision, {0u, 0u}, 10 );
    append( ssCollision, {0u, 0u} );

    setAlignment( 0.5 );
}

VicIIDebugger::Video::Sprites::Position::First::First() {
    valX.setFont( GUIKIT::Font::system( "", true ) );
    valX.setEditable( false );
    valX.setAlign( GUIKIT::LineEdit::Align::Right );
    locationVal.setFont( GUIKIT::Font::system( "", true ) );
    locationVal.setEditable( false );
    locationVal.setAlign( GUIKIT::LineEdit::Align::Right );
    expandX.setReadonly( true );

    append( labelX, {0u, 0u}, 10 );
    append( valX, {getWidth(3, true), 0u}, 10 );
    append( expandX, {0u, 0u}, 20 );
    append( location, {0u, 0u}, 10 );
    append( locationVal, {getWidth(4, true), 0u} );

    setAlignment( 0.5 );
}

VicIIDebugger::Video::Sprites::Position::Second::Second() {
    valY.setFont( GUIKIT::Font::system( "", true ) );
    valY.setEditable( false );
    valY.setAlign( GUIKIT::LineEdit::Align::Right );
    mcBaseVal.setFont( GUIKIT::Font::system( "", true ) );
    mcBaseVal.setEditable( false );
    mcBaseVal.setAlign( GUIKIT::LineEdit::Align::Right );
    expandY.setReadonly( true );

    append( labelY, {0u, 0u}, 10 );
    append( valY, {getWidth(3, true), 0u}, 10 );
    append( expandY, {0u, 0u}, 20 );
    append( mcBase, {0u, 0u}, 10 );
    append( mcBaseVal, {getWidth(4, true), 0u} );

    setAlignment( 0.5 );
}

VicIIDebugger::Video::Sprites::Position::Position() {
    append( first, {0u, 0u}, 10 );
    append( second, {0u, 0u} );
}

VicIIDebugger::Video::Sprites::Sprites() {
    append(viewer, {0u, 0u}, 10);
    append(selector, {0u, 0u}, 10);
    append(props, {0u, 0u}, 10);
    append(position, {0u, 0u});

    setPadding( 10 );
}

VicIIDebugger::Video::Video( Debugger* debugger ) {
    append(wraper, {0u, 0u}, 20);
    append(sprites, {0u, 0u});
}

auto VicIIDebugger::screenIdent() -> std::string {
    return "debugger_vicII";
}

auto VicIIDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger VIC-II";
}

auto VicIIDebugger::buildTheme() -> GUIKIT::Layout* {
    video = new Video( this );

    control->remove( control->searchEdit );
    control->remove( control->search );

    for (auto& spr : video->sprites.selector.spr) {
        spr.onActivate = [this]() {
            if (isPaused())
                updateTheme();
        };
    }

    return video;
}

auto VicIIDebugger::updateTheme() -> void {
    bool locked = emuThread->lock();
    snapshot->theme = Emulator::Interface::DebuggerSnapshot::Theme::Video;

    if (isC64()) {
        auto* c64Emu = dynamic_cast<LIBC64::Interface*>(emulator);
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);

        c64Emu->getDebuggerSnapshot(snap);

        updateView(snap);
    }

    if (locked)
        emuThread->unlock();
}

auto VicIIDebugger::initTheme() -> void {
    emulator->debuggerEnable( Emulator::Interface::DebuggerChip::Video, Emulator::Interface::DebuggerAction::None, 0);
}

auto VicIIDebugger::closeTheme() -> void {
    emulator->debuggerDisable( Emulator::Interface::DebuggerChip::Video, Emulator::Interface::DebuggerAction::None, 0);
}

auto VicIIDebugger::updateView(LIBC64::DebuggerSnapshot& s) -> void {
    auto& snap = s.vicII;
    uint8_t nr = getSelectedSprite();
    auto& spr = snap.spr[nr];
    auto vManager = VideoManager::getInstance( emulator );
    auto& canvas = video->sprites.viewer.canvas;
    auto& sel = video->sprites.selector;

    if (spr.pos) {
        canvas.setGrid( 5, spr.pos / 48, 48);
        auto* ptr = canvas.getDotPtr();
        uint8_t _d;

        for (unsigned i = 0; i < spr.pos; i++) {
            _d = spr.data[i];
            *ptr++ = _d & 0x80 ? 0 : vManager->colorTable[ _d ];
        }
    } else
        canvas.setGrid( 5, 0, 0);

    canvas.update();

    for (unsigned i = 0; i < 8; i++) {
        auto& spr = sel.spr[i];
        auto& _spr = snap.spr[i];

        if (spr.getStore() != !!_spr.pos) {
            if (_spr.pos) {
                spr.setFont( GUIKIT::Font::system( "bold" ) );
                spr.setForegroundColor(SUCCESS_COLOR );
            } else {
                spr.setFont( GUIKIT::Font::system( "" ) );
                spr.resetForegroundColor();
            }
            spr.setStore( _spr.pos != 0 );
        }
    }

    auto& regs = video->wraper.regWrapper.registers;
    int i = 0;
    for (auto& reg : regs) {
        switch (i++) {
            case 0:
                updateReg(reg->leftVal, s.vPos);
                updateReg(reg->rightVal, snap.xPos);
                break;
            case 1:
                updateReg(reg->leftVal, snap.yScroll);
                updateReg(reg->rightVal, snap.xScroll);
                break;
            case 2:
                updateReg(reg->leftVal, snap.vc);
                updateReg(reg->rightVal, snap.vcBase);
                break;
            case 3:
                updateReg(reg->leftVal, snap.rc);
                updateReg(reg->rightVal, snap.vmli);
                break;
            case 4:
                updateReg(reg->leftVal, snap.irqLine);
                updateReg(reg->rightVal, snap.vicBank);
                break;
            case 5:
                updateReg(reg->leftVal, snap.screenMemory);
                updateReg(reg->rightVal, snap.charMemory);
                break;
            case 6:
                updateReg(reg->leftVal, snap.controlReg1);
                updateReg(reg->rightVal, snap.controlReg2);
                break;
        }
    }

    auto& sprProps = video->sprites.props;
    updateReg( sprProps.priority, spr.prioMD );
    updateReg( sprProps.multiColor, spr.multiColor );
    updateReg( sprProps.sfCollision, snap.spriteForegroundCollided & (1 << nr) );
    updateReg( sprProps.ssCollision, snap.spriteSpriteCollided & (1 << nr) );

    auto& sprPos = video->sprites.position;
    updateReg( sprPos.first.valX, spr.x );
    updateReg( sprPos.first.expandX, spr.expandX );
    updateReg( sprPos.first.locationVal, spr.addr );
    updateReg( sprPos.second.valY, spr.y );
    updateReg( sprPos.second.expandY, spr.expandY );
    updateReg( sprPos.second.mcBaseVal, spr.mcBase );

    auto& mode = video->wraper.regWrapper.mode.modeVal;
    switch (snap.mode & 7) {
        case 0: updateReg( mode, "Standard Character", 0 ); break;
        case 1: updateReg( mode, "Multicolor Character", 1 ); break;
        case 2: updateReg( mode, "Standard Bitmap", 2 ); break;
        case 3: updateReg( mode, "Multicolor Bitmap", 3 ); break;
        case 4: updateReg( mode, "Extended Background Color", 4 ); break;
        case 5: updateReg( mode, "Invalid Multicolor Character", 5 ); break;
        case 6: updateReg( mode, "Invalid Standard Bitmap", 6 ); break;
        case 7: updateReg( mode, "Invalid Multicolor Bitmap", 7 ); break;
    }

    auto& flags = video->wraper.flags;
    updateReg( flags.first.den, snap.den );
    updateReg( flags.first.badLine, snap.badLine );
    updateReg( flags.second.idle, snap.idleMode );
    updateReg( flags.second.vblank, snap.visibleLine );
    updateReg( flags.third.hFlop, snap.hFlipFlip );
    updateReg( flags.third.vFlop, snap.vFlipFlip );

    auto& intLatch = video->wraper.intr.latch;
    updateReg( intLatch.latchVal, snap.irqLatch );
    updateReg( intLatch.intLine, !!(snap.irqLatch & 0x80) );
    updateReg( intLatch.lightPen, !!(snap.irqLatch & 8) );
    updateReg( intLatch.ssCollision, !!(snap.irqLatch & 4) );
    updateReg( intLatch.sfCollision, !!(snap.irqLatch & 2) );
    updateReg( intLatch.raster, !!(snap.irqLatch & 1) );

    auto& intMask = video->wraper.intr.mask;
    updateReg( intMask.maskVal, snap.irqEnable );
    updateReg( intMask.lightPen, !!(snap.irqEnable & 8) );
    updateReg( intMask.ssCollision, !!(snap.irqEnable & 4) );
    updateReg( intMask.sfCollision, !!(snap.irqEnable & 2) );
    updateReg( intMask.raster, !!(snap.irqEnable & 1) );

    auto& lp = video->wraper.lp;
    updateReg( lp.valX, snap.lpx );
    updateReg( lp.valY, snap.lpy );
    updateReg( lp.line, snap.lpPin );
    updateReg( lp.latched, snap.lpLatched );
}

auto VicIIDebugger::getSelectedSprite() -> unsigned {
    int i = 0;
    for (auto& spr : video->sprites.selector.spr) {
        if (spr.checked())
            return i;
        i++;
    }
    return 0;
}

auto VicIIDebugger::translateTheme() -> void {
    auto& pos = video->sprites.position;
    pos.first.labelX.setText( "X" );
    pos.first.expandX.setText( "expandX" );
    pos.first.location.setText( "Memory" );
    pos.second.labelY.setText( "Y" );
    pos.second.expandY.setText( "expandY" );
    pos.second.mcBase.setText( "McBase" );

    auto& props = video->sprites.props;
    props.priority.setText( "Priority" );
    props.multiColor.setText( "MultiColor" );
    props.sfCollision.setText( "SF Collision" );
    props.ssCollision.setText( "SS Collision" );

    auto& flags = video->wraper.flags;
    flags.setText( "Flags" );
    flags.first.den.setText( "DEN" );
    flags.first.badLine.setText( "Badline" );
    flags.second.idle.setText( "Idle" );
    flags.second.vblank.setText( "Vblank" );
    flags.third.hFlop.setText( "H-Flop" );
    flags.third.vFlop.setText( "V-Flop" );

    video->wraper.intr.setText( "Interrupts" );
    auto& intMask = video->wraper.intr.mask;
    intMask.mask.setText( "Mask:" );
    intMask.intLine.setText( "IRQ" );
    intMask.lightPen.setText( "LP" );
    intMask.ssCollision.setText( "SS Col" );
    intMask.sfCollision.setText( "SF Col" );
    intMask.raster.setText( "Raster" );

    auto& intLatch = video->wraper.intr.latch;
    intLatch.latch.setText( "Latch:" );
    intLatch.intLine.setText( "IRQ" );
    intLatch.lightPen.setText( "LP" );
    intLatch.ssCollision.setText( "SS Col" );
    intLatch.sfCollision.setText( "SF Col" );
    intLatch.raster.setText( "Raster" );

    auto& lp = video->wraper.lp;
    lp.setText( "Lightpen" );
    lp.labelX.setText( "X" );
    lp.labelY.setText( "Y" );
    lp.line.setText( "LP Line" );
    lp.latched.setText( "Latched" );

    video->wraper.regWrapper.setText( "Register" );
    auto& mode = video->wraper.regWrapper.mode;
    mode.modeLabel.setText( "Mode" );

    int i = 0;
    for (auto& reg : video->wraper.regWrapper.registers) {
        switch (i++) {
            case 0:
                reg->left.setText("V" );
                reg->right.setText("H" );
                break;
            case 1:
                reg->left.setText("Y-Scroll" );
                reg->right.setText("X-Scroll" );
                break;
            case 2:
                reg->left.setText("VC" );
                reg->right.setText("VC Base" );
                break;
            case 3:
                reg->left.setText("RC" );
                reg->right.setText("VMLI" );
                break;
            case 4:
                reg->left.setText("Raster IRQ" );
                reg->right.setText("VIC Bank" );
                break;
            case 5:
                reg->left.setText("Screen Mem" );
                reg->right.setText("Char Mem" );
                break;
            case 6:
                reg->left.setText("Reg 11" );
                reg->right.setText("Reg 16" );
                break;
        }
    }

    auto& sprites = video->sprites;
    sprites.setText( "Sprites" );
    sprites.selector.label.setText( "Sprite:" );
}
