
#include "deniseDebugger.h"
#include "../program.h"
#include "../thread/emuThread.h"

DeniseDebugger::DeniseDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Mode::DENISE ) {
    build();
}

DeniseDebugger::Video::Wraper::Colors::Row::Row() {
    append(spacer, {~0u, 0u});
    int i = 0;

    for (auto& col : cols) {
        append(col, {15u, 15u}, ++i == 8 ? 0 : 5);
    }
    setAlignment( 0.5 );
}

DeniseDebugger::Video::Wraper::Colors::Colors() {

    for (auto& row : rows) {
        append(row, {~0u, 0u}, 5);
    }
}

DeniseDebugger::Video::Wraper::Registers::Registers() {
    left.setAlign( GUIKIT::Label::Align::Right );
    right.setAlign( GUIKIT::Label::Align::Right );

    leftVal.setFont( GUIKIT::Font::system( 11, "", true ) );
    rightVal.setFont( GUIKIT::Font::system( 11, "", true ) );
    leftVal.setEditable( false );
    rightVal.setEditable( false );

    leftVal.setText( "0" );
    leftVal.setStore( 0 );
    rightVal.setText( "0" );
    rightVal.setStore( 0 );

    append(left, {90u, 0u}, 10);
    append(leftVal, {50u, 0u}, 10);
    append(right, {90u, 0u}, 10);
    append(rightVal, {50u, 0u});

    setAlignment( 0.5 );
}

DeniseDebugger::Video::Wraper::Flags::Flags() {
    append(spacer, {~0u, 0u});
    append(hires, {0u, 0u}, 10);
    append(shres, {0u, 0u}, 10);
    append(ham, {0u, 0u}, 10);
    append(dual, {0u, 0u}, 10);
    append(pf2OverPf1, {0u, 0u});

    setAlignment( 0.5 );
}

DeniseDebugger::Video::Wraper::FlagsECS::FlagsECS() {
    append(spacer, {~0u, 0u});
    append(ecsena, {0u, 0u}, 10);
    append(brdblnk, {0u, 0u}, 10);
    append(extblken, {0u, 0u});

    setAlignment( 0.5 );
}

DeniseDebugger::Video::Wraper::Wraper() {
    int i = 0;
    registers.resize( 9 );

    for (auto& reg : registers) {
        reg = new Registers();
        i++;
        append(*reg, {0u, 0u}, 10);
    }

    append( flags, {~0u, 0u}, 10 );
    append( flagsECS, {~0u, 0u}, 20 );
    append( colors, {~0u, 0u} );
}

DeniseDebugger::Video::Sprites::Selector::Selector() {
    label.setText( "Sprite:" );

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

DeniseDebugger::Video::Sprites::Viewer::Viewer() {
    canvas.setPadding( 2 );
    append(canvas, {300u, 300u});
}

DeniseDebugger::Video::Sprites::Position::Position() {
    valVStart.setEditable( false );
    valVStop.setEditable( false );
    valH.setEditable( false );
    attached.setReadonly( true );
    valVStart.setText( "0" );
    valVStop.setText( "0" );
    valH.setText( "0" );
    valVStart.setStore( 0 );
    valVStop.setStore( 0 );
    valH.setStore( 0 );

    append(labelVStart, {0u, 0u}, 10);
    append(valVStart, {40u, 0u}, 5);
    append(labelVStop, {0u, 0u}, 5);
    append(valVStop, {40u, 0u}, 10);
    append(labelH, {0u, 0u}, 10);
    append(valH, {40u, 0u}, 10);
    append(attached, {0u, 0u});

    setAlignment( 0.5 );
}

DeniseDebugger::Video::Sprites::Dat::Dat() {
    valDatA.setEditable( false );
    valDatB.setEditable( false );
    valDatA.setText( "0" );
    valDatB.setText( "0" );
    valDatA.setStore( 0 );
    valDatB.setStore( 0 );

    append(labelDatA, {0u, 0u}, 10);
    append(valDatA, {40u, 0u}, 5);
    append(labelDatB, {0u, 0u}, 5);
    append(valDatB, {40u, 0u});

    setAlignment( 0.5 );
}

DeniseDebugger::Video::Sprites::Sprites() {
    append(viewer, {0u, 0u}, 10);
    append(selector, {0u, 0u}, 10);
    append(position, {0u, 0u}, 10);
    append(dat, {0u, 0u});
}

DeniseDebugger::Video::Video( Debugger* debugger ) {
    append(wraper, {0u, 0u}, 20);
    append(sprites, {0u, 0u});
}


auto DeniseDebugger::screenIdent() -> std::string {
    return "debugger_denise";
}

auto DeniseDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger Denise";
}

auto DeniseDebugger::buildTheme() -> GUIKIT::Layout* {
    video = new Video( this );

    control->remove( control->searchEdit );
    control->remove( control->search );

    for (auto& spr : video->sprites.selector.spr) {
        spr.onActivate = [this]() {
            if (isPaused())
                updateTheme();
        };
    }

    emulator->debuggerAdd( Emulator::Interface::DebuggerChip::Video, Emulator::Interface::DebuggerAction::None, 0);

    return video;
}

auto DeniseDebugger::updateTheme() -> void {
    bool locked = emuThread->lock();
    snapshot->theme = Emulator::Interface::DebuggerSnapshot::Theme::Video;

    if (isAmiga()) {
        auto* amiEmu = dynamic_cast<LIBAMI::Interface*>(emulator);
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);

        amiEmu->getDebuggerSnapshot(snap);

        updateDenise(snap);
    }

    if (locked)
        emuThread->unlock();
}

auto DeniseDebugger::closeTheme() -> void {
    emulator->debuggerRemove( Emulator::Interface::DebuggerChip::Video, Emulator::Interface::DebuggerAction::None, 0);
}

auto DeniseDebugger::updateDenise(LIBAMI::DebuggerSnapshot& s) -> void {
    auto& snap = s.denise;
    auto& spr = snap.spr[getSelectedSprite()];
    auto vManager = VideoManager::getInstance( emulator );
    auto& canvas = video->sprites.viewer.canvas;
    auto& pos = video->sprites.position;
    auto& dat = video->sprites.dat;
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

    pos.attached.setChecked( spr.attached );
    updateReg(pos.valVStart, spr.vStart);
    updateReg(pos.valVStop, spr.vStop);
    updateReg(pos.valH, spr.x);

    updateReg(dat.valDatA, spr.datA);
    updateReg(dat.valDatB, spr.datB);

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
    for (auto& row : video->wraper.colors.rows) {
        for (auto& col : row.cols) {
            uint16_t colIndex = snap.colors[i++];
            uint32_t rgb = vManager->colorTable[ colIndex ];
            std::string tooltip = "Index: " + hex(colIndex) + " RGB: " + hex(rgb & 0xffffff);
            col.setTooltip( tooltip );
            col.setBackgroundColor( rgb );
        }
    }

    auto& regs = video->wraper.registers;

    i = 0;
    for (auto& reg : video->wraper.registers) {
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
    flags.hires.setChecked( snap.bplCon0 & 0x8000 );
    flags.shres.setChecked( snap.bplCon0 & 0x40 );
    flags.ham.setChecked( snap.bplCon0 & 0x800 );
    flags.dual.setChecked( snap.bplCon0 & 0x400 );
    flags.pf2OverPf1.setChecked( snap.bplCon2 & 0x40 );

    auto& flagsECS = video->wraper.flagsECS;
    flagsECS.ecsena.setChecked( snap.bplCon0 & 1 );
    flagsECS.brdblnk.setChecked( snap.bplCon3 & 0x20 );
    flagsECS.extblken.setChecked( snap.bplCon3 & 1 );

    control->position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
}

auto DeniseDebugger::getSelectedSprite() -> unsigned {
    int i = 0;
    for (auto& spr : video->sprites.selector.spr) {
        if (spr.checked())
            return i;
        i++;
    }
    return 0;
}

auto DeniseDebugger::translateTheme() -> void {
    auto& pos = video->sprites.position;
    pos.labelVStart.setText( "Vpos" );
    pos.labelVStop.setText( "-" );
    pos.labelH.setText( "Hpos" );
    pos.attached.setText( "Attach" );

    auto& dat = video->sprites.dat;
    dat.labelDatA.setText( "DatA" );
    dat.labelDatB.setText( "DatB" );

    int i = 0;
    for (auto& reg : video->wraper.registers) {
        switch (i++) {
            case 0:
                reg->left.setText("BPLCON0" );
                reg->right.setText("BPLCON2" );
                break;
            case 1:
                reg->left.setText("BPLCON1" );
                reg->right.setText("BPLCON3" );
                break;
            case 2:
                reg->left.setText("Bitplanes" );
                reg->right.setText("CLXDAT" );
                break;
            case 3:
                reg->left.setText("Delay PF1" );
                reg->right.setText("Delay PF2" );
                break;
            case 4:
                reg->left.setText("PF1P" );
                reg->right.setText("PF2P" );
                break;
            case 5:
                reg->left.setText("HSTART" );
                reg->right.setText("HSTOP" );
                break;
            case 6:
                reg->left.setText("BPL1DAT" );
                reg->right.setText("BPL2DAT" );
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

    auto& flags = video->wraper.flags;
    flags.hires.setText( "HIRES" );
    flags.shres.setText( "SHRES" );
    flags.ham.setText( "HAM" );
    flags.dual.setText( "DUAL" );
    flags.pf2OverPf1.setText( "PF2Pri" );

    auto& flagsEcs = video->wraper.flagsECS;
    flagsEcs.ecsena.setText( "ECS Enable" );
    flagsEcs.brdblnk.setText( "Border Blank" );
    flagsEcs.extblken.setText( "External Blank" );
}
