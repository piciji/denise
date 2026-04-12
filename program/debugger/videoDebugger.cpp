
#include "videoDebugger.h"
#include "../program.h"
#include "../thread/emuThread.h"

VideoDebugger::VideoDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Debugger::Mode::Video ) {
    build();
}

VideoDebugger::Video::Wraper::RegWrapper::Mode::Mode() {
    val.setFont( GUIKIT::Font::monospace() );
    val.setEditable( false );
    label.setAlign( GUIKIT::Label::Align::Right );

    append(label, {90u, 0u}, 10);
    append(val, {~0u, 0u});

    setAlignment( 0.5 );
}

VideoDebugger::Video::Wraper::RegWrapper::Register::Register() {
    left.setAlign( GUIKIT::Label::Align::Right );
    right.setAlign( GUIKIT::Label::Align::Right );

    leftVal.setFont( GUIKIT::Font::monospace() );
    rightVal.setFont( GUIKIT::Font::monospace() );
    leftVal.setEditable( false );
    rightVal.setEditable( false );
    leftVal.setAlign( GUIKIT::LineEdit::Align::Right );
    rightVal.setAlign( GUIKIT::LineEdit::Align::Right );

    leftVal.setText( "0" );
    leftVal.setStore( 0 );
    rightVal.setText( "0" );
    rightVal.setStore( 0 );

    append(left, {90u, 0u}, 10);
    append(leftVal, {getWidth(4, true), 0u}, 10);
    append(right, {110u, 0u}, 10);
    append(rightVal, {getWidth(4, true), 0u});

    setAlignment( 0.5 );
}

VideoDebugger::Video::Wraper::RegWrapper::RegWrapper(Debugger* debugger) {
    setPadding( 10 );
    registers.resize( debugger->isAmiga() ? 9 : 7 );
    if (debugger->isC64())
        append( mode, {~0u, 0u}, 7 );

    for (auto& reg : registers) {
        reg = new Register();
        append(*reg, {0u, 0u}, registers.back() == reg ? 0 : 7);
    }
}

VideoDebugger::Video::Wraper::Flags::Block::Block() {
    flag1.setReadonly( );
    flag2.setReadonly( );
    append( flag1, {0u, 0u}, 7 );
    append( flag2, {0u, 0u} );
}

VideoDebugger::Video::Wraper::Flags::Flags(Debugger* debugger) {
    blocks.resize( debugger->isAmiga() ? 4 : 3 );
    append(spacer, {~0u, 0u});

    for (auto& block : blocks) {
        block = new Block();
        append(*block, {0u, 0u}, blocks.back() == block ? 0 : (debugger->isAmiga() ? 5 : 30));
    }

    setPadding( 10 );
    setAlignment( 0.5 );
}

VideoDebugger::Video::Wraper::Colors::Row::Row() {
    append(spacer, {~0u, 0u});
    int i = 0;

    for (auto& col : cols)
        append(col, {15u, 15u}, ++i == 16 ? 0 : 5);

    setAlignment( 0.5 );
}

VideoDebugger::Video::Wraper::Colors::Colors() {
    setPadding( 10 );
    int i = 0;

    for (auto& row : rows)
        append(row, {~0u, 0u}, ++i == 2 ? 0 : 5);
}

VideoDebugger::Video::Wraper::Intr::Row::Row(Debugger* debugger, bool isMask) {
    val.setFont( GUIKIT::Font::monospace() );
    val.setAlign( GUIKIT::LineEdit::Align::Right );
    val.setText( "0" );
    val.setStore( 0 );
    val.setEditable( false );
    isMask ? intLine.setEnabled( false ) : intLine.setReadonly();

    append(label, {35u, 0u}, 5);
    append(val, {getWidth(2, true), 0u}, 10);
    append(intLine, {0u, 0u}, 5);

    boxes.resize( 4 );
    for (auto& box : boxes) {
        box = new GUIKIT::CheckBox();
        box->setReadonly();
        append(*box, {0u, 0u}, boxes.back() == box ? 0 : 5);
    }

    setAlignment( 0.5 );
}

VideoDebugger::Video::Wraper::Intr::Intr(Debugger* debugger)
: latch( debugger, false ), mask( debugger, true ) {
    if (debugger->isAmiga())
        return;
    append( latch, {0u, 0u}, 7 );
    append( mask, {0u, 0u} );

    setPadding( 10 );
}

VideoDebugger::Video::Wraper::Wraper(Debugger* debugger)
: regWrapper( debugger), flags( debugger ), intr( debugger ) {
    append( regWrapper, {~0u, 0u}, 7 );
    append( flags, {~0u, 0u}, 7 );
    if (debugger->isC64()) {
        append( intr, {~0u, 0u}, 7 );
    } else {
        append( colors, {~0u, 0u} );
    }
}

VideoDebugger::Video::WraperRight::Lightpen::Top::Top() {
    valX.setFont( GUIKIT::Font::monospace() );
    valX.setText( "0" );
    valX.setStore( 0 );
    valX.setEditable( false );
    valX.setAlign( GUIKIT::LineEdit::Align::Right );
    line.setReadonly( );

    append(labelX, {0u, 0u}, 5u);
    append(valX, {getWidth(2, true), 0u}, 10u);
    append(line, {0u, 0u}, 10u);

    setAlignment( 0.5 );
}

VideoDebugger::Video::WraperRight::Lightpen::Bottom::Bottom() {
    valY.setFont( GUIKIT::Font::monospace() );
    valY.setText( "0" );
    valY.setStore( 0 );
    valY.setEditable( false );
    valY.setAlign( GUIKIT::LineEdit::Align::Right );
    latched.setReadonly( );

    append(labelY, {0u, 0u}, 5u);
    append(valY, {getWidth(2, true), 0u}, 10u);
    append(latched, {0u, 0u});

    setAlignment( 0.5 );
}

VideoDebugger::Video::WraperRight::Lightpen::Lightpen(Debugger* debugger) {
    append( top, {0u, 0u}, 10 );
    append( bottom, {0u, 0u} );

    setPadding( 10 );
}

VideoDebugger::Video::WraperRight::WraperRight(Debugger* debugger)
: lightpen( debugger ) {
    append( lightpen, {~0u, 0u} );
}

VideoDebugger::Video::Sprites::Viewer::Viewer(Debugger* debugger) {
    canvas.setPadding( 2 );
    append(canvas, { debugger->isAmiga() ? 300u : 360u, 300u});
}

VideoDebugger::Video::Sprites::Selector::Selector() {
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

VideoDebugger::Video::Sprites::Flags::Flags(Debugger* debugger) {
    boxes.resize( 4 );
    for (auto& box : boxes) {
        box = new GUIKIT::CheckBox();
        box->setReadonly();
        append(*box, {0u, 0u}, boxes.back() == box ? 0 : 10);
    }

    setAlignment( 0.5 );
}

VideoDebugger::Video::Sprites::Position::Direction::Direction(Debugger* debugger) {
    val.setFont( GUIKIT::Font::monospace() );
    val.setEditable( false );
    val.setAlign( GUIKIT::LineEdit::Align::Right );
    memoryVal.setFont( GUIKIT::Font::monospace() );
    memoryVal.setEditable( false );
    memoryVal.setAlign( GUIKIT::LineEdit::Align::Right );
    flag.setReadonly();

    if (debugger->isAmiga()) {
        valTo.setFont( GUIKIT::Font::monospace() );
        valTo.setEditable( false );
        valTo.setAlign( GUIKIT::LineEdit::Align::Right );
    }

    append( label, {0u, 0u}, 10 );
    append( val, {getWidth(3, true), 0u}, 10 );
    if (debugger->isAmiga()) {
        append( labelTo, {0u, 0u}, 10 );
        append( valTo, {getWidth(3, true), 0u}, 10 );
    } else
        append( flag, {0u, 0u}, 20 );

    append( labelMemory, {0u, 0u}, 10 );
    append( memoryVal, {getWidth(4, true), 0u} );

    setAlignment( 0.5 );
}

VideoDebugger::Video::Sprites::Position::Position(Debugger* debugger)
: vertical( debugger ), horizontal( debugger ) {
    append( vertical, {0u, 0u}, 10 );
    append( horizontal, {0u, 0u} );
}

VideoDebugger::Video::Sprites::Sprites(Debugger* debugger)
: viewer( debugger ), flags( debugger ), position( debugger ) {
    append(viewer, {0u, 0u}, 10);
    append(selector, {0u, 0u}, 10);
    append(flags, {0u, 0u}, 10);
    append(position, {0u, 0u});

    setPadding( 10 );
}

VideoDebugger::Video::Video( Debugger* debugger )
: wraper( debugger ), wraperRight( debugger ), sprites( debugger ) {
    append(wraper, {0u, 0u}, 20);

    if (debugger->isC64()) {
        append(sprites, {0u, 0u}, 20);
        append(wraperRight, {0u, 0u});
    } else {
        append(sprites, {0u, 0u});
    }
}

auto VideoDebugger::buildTheme() -> GUIKIT::Layout* {
    video = new Video( this );

    for (auto& spr : video->sprites.selector.spr) {
        spr.onActivate = [this]() {
            if (isPaused())
                updateTheme();
        };
    }

    return video;
}

auto VideoDebugger::getSelectedSprite() -> unsigned {
    int i = 0;
    for (auto& spr : video->sprites.selector.spr) {
        if (spr.checked())
            return i;
        i++;
    }
    return 0;
}

auto VideoDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isC64()) {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
        updateView(snap);
    } else {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        updateView(snap);
    }
}

auto VideoDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Video, DebuggerAction::None, 0);
}

auto VideoDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Video, DebuggerAction::None);
}

auto VideoDebugger::updateView(LIBC64::DebuggerSnapshot& s) -> void {
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

    auto& sprFlags = video->sprites.flags;
    updateReg( *sprFlags.boxes[0], spr.prioMD );
    updateReg( *sprFlags.boxes[1], spr.multiColor );
    updateReg( *sprFlags.boxes[2], snap.spriteForegroundCollided & (1 << nr) );
    updateReg( *sprFlags.boxes[3], snap.spriteSpriteCollided & (1 << nr) );

    auto& sprPos = video->sprites.position;
    updateReg( sprPos.vertical.val, spr.y );
    updateReg( sprPos.vertical.flag, spr.expandY );
    updateReg( sprPos.vertical.memoryVal, spr.addr );
    updateReg( sprPos.horizontal.val, spr.x );
    updateReg( sprPos.horizontal.flag, spr.expandX );
    updateReg( sprPos.horizontal.memoryVal, spr.mcBase );

    auto& mode = video->wraper.regWrapper.mode.val;
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
    updateReg( flags.blocks[0]->flag1, snap.den );
    updateReg( flags.blocks[0]->flag2, snap.badLine );
    updateReg( flags.blocks[1]->flag1, snap.idleMode );
    updateReg( flags.blocks[1]->flag2, snap.visibleLine );
    updateReg( flags.blocks[2]->flag1, snap.hFlipFlip );
    updateReg( flags.blocks[2]->flag2, snap.vFlipFlip );

    auto& intLatch = video->wraper.intr.latch;
    updateReg( intLatch.val, snap.irqLatch );
    updateReg( intLatch.intLine, !!(snap.irqLatch & 0x80) );
    updateReg( *intLatch.boxes[0], !!(snap.irqLatch & 8) );
    updateReg( *intLatch.boxes[1], !!(snap.irqLatch & 4) );
    updateReg( *intLatch.boxes[2], !!(snap.irqLatch & 2) );
    updateReg( *intLatch.boxes[3], !!(snap.irqLatch & 1) );

    auto& intMask = video->wraper.intr.mask;
    updateReg( intMask.val, snap.irqEnable );
    updateReg( *intMask.boxes[0], !!(snap.irqEnable & 8) );
    updateReg( *intMask.boxes[1], !!(snap.irqEnable & 4) );
    updateReg( *intMask.boxes[2], !!(snap.irqEnable & 2) );
    updateReg( *intMask.boxes[3], !!(snap.irqEnable & 1) );

    auto& lp = video->wraperRight.lightpen;
    updateReg( lp.top.valX, snap.lpx );
    updateReg( lp.bottom.valY, snap.lpy );
    updateReg( lp.top.line, snap.lpPin );
    updateReg( lp.bottom.latched, snap.lpLatched );

    updateControl( s.vPos, s.hPos );
}

auto VideoDebugger::updateView(LIBAMI::DebuggerSnapshot& s) -> void {
    auto& snap = s.denise;
    uint8_t nr = getSelectedSprite();
    auto& spr = snap.spr[nr];
    auto vManager = VideoManager::getInstance( emulator );
    auto& canvas = video->sprites.viewer.canvas;
    auto& pos = video->sprites.position;
    auto& sel = video->sprites.selector;

    if (spr.pos) {
        canvas.setGrid( 15, spr.pos / 16, 16 );
        auto* ptr = canvas.getDotPtr();
        uint16_t _d;

        for (unsigned i = 0; i < spr.pos; i++) {
            _d = spr.data[i];
            *ptr++ = _d & 0x8000 ? 0 : vManager->colorTable[ _d ];
        }
    } else
        canvas.setGrid( 15, 0, 0);

    canvas.update();

    auto& sprFlags = video->sprites.flags;
    updateReg( *sprFlags.boxes[0], spr.attached );

    uint8_t sprGroup = nr & ~1;
    switch (sprGroup) {
        case 0: // group 0/1
            updateReg( *sprFlags.boxes[1], snap.clxDat & 0b111000000000 );
            updateReg( *sprFlags.boxes[2], snap.clxDat & 0x20 );
            updateReg( *sprFlags.boxes[3], snap.clxDat & 2 );
            break;
        case 2: // group 2/3
            updateReg( *sprFlags.boxes[1], snap.clxDat & 0b11001000000000 );
            updateReg( *sprFlags.boxes[2], snap.clxDat & 0x40 );
            updateReg( *sprFlags.boxes[3], snap.clxDat & 4 );
            break;
        case 4: // group 4/5
            updateReg( *sprFlags.boxes[1], snap.clxDat & 0b101010000000000 );
            updateReg( *sprFlags.boxes[2], snap.clxDat & 0x80 );
            updateReg( *sprFlags.boxes[3], snap.clxDat & 8 );
            break;
        case 6: // group 6/7
            updateReg( *sprFlags.boxes[1], snap.clxDat & 0b110100000000000 );
            updateReg( *sprFlags.boxes[2], snap.clxDat & 0x100 );
            updateReg( *sprFlags.boxes[3], snap.clxDat & 0x10 );
            break;
    }

    updateReg(pos.vertical.val, spr.vStart);
    updateReg(pos.vertical.valTo, spr.vStop);
    updateReg(pos.vertical.memoryVal, spr.datA);

    updateReg(pos.horizontal.val, spr.x);
    updateReg(pos.horizontal.valTo, spr.x + 16);
    updateReg(pos.horizontal.memoryVal, spr.datB);

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

    int i = 0;
    unsigned colMask = (1 << vManager->countColorBits) - 1;
    for (auto& row : video->wraper.colors.rows) {
        for (auto& col : row.cols) {
            uint16_t colIndex = snap.colors[i++] & colMask;
            uint32_t rgb = vManager->colorTable[ colIndex ];
            std::string tooltip = "Index: " + GUIKIT::String::convertToHex(colIndex) + ", RGB: " + GUIKIT::String::convertToHex(rgb & 0xffffff);
            col.setTooltip( tooltip );
            col.setBackgroundColor( rgb );
        }
    }

    auto& regs = video->wraper.regWrapper.registers;
    i = 0;
    for (auto& reg : regs) {
        switch (i++) {
            case 0:
                updateReg(reg->leftVal, snap.bplCon0);
                updateReg(reg->rightVal, snap.bplCon2);
                break;
            case 1:
                updateReg(reg->leftVal, snap.bplCon1);
                updateReg(reg->rightVal, snap.bplCon3);
                break;
            case 2:
                updateReg(reg->leftVal, (snap.bplCon0 >> 12) & 7 );
                updateReg(reg->leftVal, snap.clxDat );
                break;
            case 3:
                updateReg(reg->leftVal, snap.delayPf1);
                updateReg(reg->rightVal, snap.delayPf2);
                break;
            case 4:
                updateReg(reg->leftVal, snap.bplCon2 & 7);
                updateReg(reg->rightVal, (snap.bplCon2 >> 3) & 7);
                break;
            case 5:
                updateReg(reg->leftVal, snap.hStart);
                updateReg(reg->rightVal, snap.hStop);
                break;
            case 6:
                updateReg(reg->leftVal, snap.bpl1dat);
                updateReg(reg->rightVal, snap.bpl2dat);
                break;
            case 7:
                updateReg(reg->leftVal, snap.bpl3dat);
                updateReg(reg->rightVal, snap.bpl4dat);
                break;
            case 8:
                updateReg(reg->leftVal, snap.bpl5dat);
                updateReg(reg->rightVal, snap.bpl6dat);
                break;
        }
    }

    auto& flags = video->wraper.flags;
    updateReg(flags.blocks[0]->flag1, snap.bplCon0 & 0x8000); // hires
    updateReg(flags.blocks[0]->flag2, snap.bplCon0 & 0x40); // shres

    updateReg(flags.blocks[1]->flag1, snap.bplCon0 & 0x400); // dual
    updateReg(flags.blocks[1]->flag2, snap.bplCon0 & 0x800); // ham

    updateReg(flags.blocks[2]->flag1, snap.bplCon2 & 0x40); // pf2pri
    updateReg(flags.blocks[2]->flag2, snap.bplCon0 & 1); // ecs enable

    updateReg(flags.blocks[3]->flag1, snap.bplCon3 & 1); // extblken
    updateReg(flags.blocks[3]->flag2, snap.bplCon3 & 0x20); // brdblnk

    updateControl( s.vPos, s.hPos );
}

auto VideoDebugger::translateTheme() -> void {
    auto& flags = video->wraper.flags;
    flags.setText( "Flags" );

    if (isC64()) {
        flags.blocks[0]->flag1.setText( "DEN" );
        flags.blocks[0]->flag2.setText( "Badline" );
        flags.blocks[1]->flag1.setText( "Idle" );
        flags.blocks[1]->flag2.setText( "Vblank" );
        flags.blocks[2]->flag1.setText( "H-Flop" );
        flags.blocks[2]->flag2.setText( "V-Flop" );
    } else {
        flags.blocks[0]->flag1.setText( "HIRES" );
        flags.blocks[0]->flag2.setText( "SHRES" );
        flags.blocks[1]->flag1.setText( "DUAL" );
        flags.blocks[1]->flag2.setText( "HAM" );
        flags.blocks[2]->flag1.setText( "PF2Pri" );
        flags.blocks[2]->flag2.setText( "ECS Enable" );
        flags.blocks[3]->flag1.setText( "External Blank" );
        flags.blocks[3]->flag2.setText( "Border Blank" );
    }

    video->wraper.intr.setText( "Interrupts" );
    auto& intMask = video->wraper.intr.mask;
    intMask.label.setText( "Mask:" );
    intMask.intLine.setText( "IRQ" );
    intMask.boxes[0]->setText( "LP" );
    intMask.boxes[1]->setText( "SS Col" );
    intMask.boxes[2]->setText( "SF Col" );
    intMask.boxes[3]->setText( "Raster" );

    auto& intLatch = video->wraper.intr.latch;
    intLatch.label.setText( "Latch:" );
    intLatch.intLine.setText( "IRQ" );
    intLatch.boxes[0]->setText( "LP" );
    intLatch.boxes[1]->setText( "SS Col" );
    intLatch.boxes[2]->setText( "SF Col" );
    intLatch.boxes[3]->setText( "Raster" );

    auto& lp = video->wraperRight.lightpen;
    lp.setText( "Lightpen" );
    lp.top.labelX.setText( "X" );
    lp.bottom.labelY.setText( "Y" );
    lp.top.line.setText( "LP Line" );
    lp.bottom.latched.setText( "Latched" );

    video->wraper.regWrapper.setText( "Register" );

    if (isC64()) {
        auto& mode = video->wraper.regWrapper.mode;
        mode.label.setText( "Mode" );
    } else {
        video->wraper.colors.setText( "Colors" );
    }

    int i = 0;
    for (auto& reg : video->wraper.regWrapper.registers) {
        switch (i++) {
            case 0:
                reg->left.setText(isAmiga() ? "BPLCON0" : "V" );
                reg->right.setText(isAmiga() ? "BPLCON2" : "H" );
                break;
            case 1:
                reg->left.setText(isAmiga() ? "BPLCON1" : "Y-Scroll" );
                reg->right.setText(isAmiga() ? "BPLCON3" : "X-Scroll" );
                break;
            case 2:
                reg->left.setText(isAmiga() ? "Bitplanes" : "VC" );
                reg->right.setText(isAmiga() ? "CLXDAT" : "VC Base" );
                break;
            case 3:
                reg->left.setText(isAmiga() ? "Delay PF1" : "RC" );
                reg->right.setText(isAmiga() ? "Delay PF2" : "VMLI" );
                break;
            case 4:
                reg->left.setText(isAmiga() ? "PF1P" : "Raster IRQ" );
                reg->right.setText(isAmiga() ? "PF2P" : "VIC Bank" );
                break;
            case 5:
                reg->left.setText(isAmiga() ? "HSTART" : "Screen Mem" );
                reg->right.setText(isAmiga() ? "HSTOP" : "Char Mem" );
                break;
            case 6:
                reg->left.setText(isAmiga() ? "BPL1DAT" : "Reg 11" );
                reg->right.setText(isAmiga() ? "BPL2DAT" : "Reg 16" );
                break;
            case 7:
                reg->left.setText("BPL3DAT" );
                reg->right.setText("BPL4DAT" );
                break;
            case 8:
                reg->left.setText("BPL5DAT" );
                reg->right.setText("BPL6DAT" );
                break;
        }
    }

    auto& sprites = video->sprites;
    sprites.setText( "Sprites" );
    sprites.selector.label.setText( "Sprite:" );

    auto& pos = video->sprites.position;
    pos.vertical.label.setText( "Y" );
    pos.horizontal.label.setText( "X" );

    if (isAmiga()) {
        pos.vertical.labelTo.setText( "-" );
        pos.horizontal.labelTo.setText( "-" );
        pos.vertical.labelMemory.setText( "DatA" );
        pos.horizontal.labelMemory.setText( "DatB" );

        auto& flagsSpr = video->sprites.flags;
        flagsSpr.boxes[0]->setText( "Attach" );
        flagsSpr.boxes[1]->setText( "SS Col" );
        flagsSpr.boxes[2]->setText( "Even BPL Col" );
        flagsSpr.boxes[3]->setText( "Odd BPL Col" );
    } else {
        pos.vertical.flag.setText( "expandY" );
        pos.vertical.labelMemory.setText( "Mc Base" );
        pos.horizontal.flag.setText( "expandX" );
        pos.horizontal.labelMemory.setText( "Memory" );

        auto& flagsSpr = video->sprites.flags;
        flagsSpr.boxes[0]->setText( "Priority" );
        flagsSpr.boxes[1]->setText( "MultiColor" );
        flagsSpr.boxes[2]->setText( "SF Collision" );
        flagsSpr.boxes[3]->setText( "SS Collision" );
    }
}

auto VideoDebugger::saveIdent() -> std::string {
    return "debugger_video";
}

auto VideoDebugger::titleIdent() -> std::string {
    return emulator->ident + (isC64() ? " Debugger VIC-II" : " Debugger Denise");
}
