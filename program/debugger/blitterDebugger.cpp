
#include "blitterDebugger.h"
#include "../../emulation/libami/interface.h"
#include "../thread/emuThread.h"
#include "../program.h"

BlitterDebugger::Blitter::ColLeft::Control::BltCon::BltCon() {
    editShift.setEditable( false );
    editChannel.setEditable( false );
    editControl.setEditable( false );

    append( label, {0u, 0u}, 10 );
    append( editShift, {30u, 0u}, 10 );
    append( editChannel, {30u, 0u}, 10 );
    append( editControl, {40u, 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColLeft::Control::BltSize::BltSize() {
    edit.setEditable( false );
    editCur.setEditable( false );

    append( label, {0u, 0u}, 10 );
    append( edit, {50u, 0u}, 10 );
    append( labelCur, {0u, 0u}, 10 );
    append( editCur, {50u, 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColLeft::Control::Control() {
    append( bltCon0, {0u, 0u}, 10 );
    append( bltCon1, {0u, 0u}, 10 );
    append( bltSizeW, {0u, 0u}, 10 );
    append( bltSizeH, {0u, 0u} );

    setPadding( 10 );
}

BlitterDebugger::Blitter::ColLeft::Flags::Block::Block() {
    flag1.setReadonly( );
    flag2.setReadonly( );
    append( flag1, {0u, 0u}, 7 );
    append( flag2, {0u, 0u} );
}

BlitterDebugger::Blitter::ColLeft::Flags::Flags() {
    blocks.resize( 4 );
    append(spacer, {~0u, 0u});

    for (auto& block : blocks) {
        block = new Block();
        append(*block, {0u, 0u}, blocks.back() == block ? 0 : 10);
    }

    setPadding( 10 );
    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColLeft::BltD::Data::Data() {
    check.setReadonly( );
    edit.setEditable( false );

    append( check, {0u, 0u}, 10 );
    append( edit, {60u, 0u} );
    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColLeft::BltD::Fill::Fill() {
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace(  ) );

    append( label, {0u, 0u}, 10 );
    append( edit, {binaryLength(), 0u} );
    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColLeft::BltD::BltD() {
    append( data, {0u, 0u}, 10 );
    append( fillIn, {0u, 0u}, 10 );
    append( fillOut, {0u, 0u} );

    setPadding( 10 );
}

BlitterDebugger::Blitter::ColLeft::ColLeft() {
    append( control, {~0u, 0u}, 10 );
    append( flags, {~0u, 0u}, 10 );
    append( bltD, {~0u, 0u} );
}

BlitterDebugger::Blitter::ColCenter::BltA::Data::Data() {
    check.setReadonly( );
    edit.setEditable( false );
    editOld.setEditable( false );

    append( check, {0u, 0u}, 10 );
    append( edit, {60u, 0u}, 10 );
    append( labelOld, {0u, 0u}, 10 );
    append( editOld, {60u, 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColCenter::BltA::BltWM::BltWM() {
    check.setReadonly(  );
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace(  ) );

    append( check, {0u, 0u}, 10 );
    append( edit, {binaryLength(), 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColCenter::BltA::Barrel::Barrel() {
    edit.setEditable( false );

    append( label, {0u, 0u}, 10 );
    append( edit, {60u, 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColCenter::BltA::BltA() {
    append( data, {0u, 0u}, 10 );
    append( first, {0u, 0u}, 10 );
    append( last, {0u, 0u}, 10 );
    append( barrel, {0u, 0u} );

    setPadding( 10 );
}

BlitterDebugger::Blitter::ColCenter::BltB::Data::Data() {
    check.setReadonly( );
    edit.setEditable( false );
    editOld.setEditable( false );

    append( check, {0u, 0u}, 10 );
    append( edit, {60u, 0u}, 10 );
    append( labelOld, {0u, 0u}, 10 );
    append( editOld, {60u, 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColCenter::BltB::Barrel::Barrel() {
    edit.setEditable( false );

    append( label, {0u, 0u}, 10 );
    append( edit, {60u, 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::ColCenter::BltB::BltB() {
    append( data, {0u, 0u}, 10 );
    append( barrel, {0u, 0u} );

    setPadding( 10 );
}

BlitterDebugger::Blitter::ColCenter::BltC::BltC() {
    check.setReadonly( );
    edit.setEditable( false );

    append( check, {0u, 0u}, 10 );
    append( edit, {60u, 0u} );

    setAlignment( 0.5 );
    setPadding( 10 );
}

BlitterDebugger::Blitter::ColCenter::ColCenter() {
    append( bltA, {~0u, 0u}, 10 );
    append( bltB, {~0u, 0u}, 10 );
    append( bltC, {~0u, 0u} );
}

BlitterDebugger::Blitter::Minterm::Entry::Entry(bool useCheck) {
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace(  ) );

    if (useCheck) {
        check.setReadonly( );
        check.setFont( GUIKIT::Font::monospace(  ) );
        append(check, {0u, 0u}, 10 );
    } else {
        label.setFont( GUIKIT::Font::monospace(  ) );
        label.setAlign( GUIKIT::Label::Align::Right );
        append(label, {0u, 0u}, 10 );
    }

    append( edit, {binaryLength(), 0u} );

    setAlignment( 0.5 );
}

BlitterDebugger::Blitter::Minterm::Minterm() {
    Entry* entry;

    for (auto& e : {"A", "B", "C"}) {
        entry = new Entry(false);
        entry->label.setText( e );
        entries.push_back( entry );
        append( *entry, {0u, 0u}, 7 );
    }

    for (auto& e : {"~a ∧ ~b ∧ ~c", "~a ∧ ~b ∧  C", "~a ∧  B ∧ ~c", "~a ∧  B ∧  C", " a ∧ ~b ∧ ~c", " a ∧ ~b ∧  C", " a ∧  B ∧ ~c", " a ∧  B ∧  C"}) {
        entry = new Entry(true);
        entry->check.setText( e );
        entries.push_back( entry );
        append( *entry, {0u, 0u}, 7 );
    }

    entry = new Entry(false);
    entry->label.setText( "ORed" );
    entries.push_back( entry );
    append( *entry, {0u, 0u} );

    setPadding( 10 );
}

BlitterDebugger::Blitter::Blitter() {
    append( colLeft, {~0u, 0u}, 10 );
    append( colCenter, {~0u, 0u}, 10 );
    append( minterm, {~0u, 0u} );
}

BlitterDebugger::BlitterControl::BlitterControl() {
    append( softStopButton, {0u, 0u} );
    append( spacer, {~0u, 0u} );
    setAlignment( 0.5 );
}

BlitterDebugger::BlitterDebugger( Emulator::Interface* emulator )
: Debugger( emulator ) {
}

auto BlitterDebugger::saveIdent() -> std::string {
    return "debugger_blitter";
}

auto BlitterDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger Blitter";
}

auto BlitterDebugger::buildTheme() -> GUIKIT::Layout* {
    blitter = new Blitter( );
    return blitter;
}

auto BlitterDebugger::translateTheme() -> void {
    bool showTips = showTipsItem.checked();

    auto& control = blitter->colLeft.control;
    control.setText( "Register" );
    control.bltCon0.label.setText( "Blt Con 0" );
    control.bltCon1.label.setText( "Blt Con 1" );
    control.bltSizeW.label.setText( "Blt Size W" );
    control.bltSizeH.label.setText( "Blt Size H" );
    control.bltSizeW.labelCur.setText( "cur." );
    control.bltSizeH.labelCur.setText( "cur." );

    auto& flags = blitter->colLeft.flags;
    flags.setText( "Flags" );
    flags.blocks[0]->flag1.setText( "Busy" );
    flags.blocks[0]->flag2.setText( "Zero" );
    flags.blocks[1]->flag1.setText( "Desc" );
    flags.blocks[1]->flag2.setText( "Line" );
    flags.blocks[2]->flag1.setText( "Exc Fill" );
    flags.blocks[2]->flag2.setText( "Inc Fill" );
    flags.blocks[3]->flag1.setText( "Cary Fill" );
    flags.blocks[3]->flag2.setText( "Doff" );

    auto& bltD = blitter->colLeft.bltD;
    bltD.setText( "Channel D" );
    bltD.data.check.setText( "DMA" );
    bltD.fillIn.label.setText( "Fill In" );
    bltD.fillOut.label.setText( "Fill Out" );

    auto& bltA = blitter->colCenter.bltA;
    bltA.setText( "Channel A" );
    bltA.data.check.setText( "DMA" );
    bltA.data.labelOld.setText( "old" );
    bltA.first.check.setText( "First Word" );
    bltA.last.check.setText( "Last Word" );
    bltA.barrel.label.setText( "Barrel" );

    auto& bltB = blitter->colCenter.bltB;
    bltB.setText( "Channel B" );
    bltB.data.check.setText( "DMA" );
    bltB.data.labelOld.setText( "old" );
    bltB.barrel.label.setText( "Barrel" );

    auto& bltC = blitter->colCenter.bltC;
    bltC.setText( "Channel C" );
    bltC.check.setText( "DMA" );

    auto& minterm = blitter->minterm;
    minterm.setText( "Minterm" );

    blitterControl->softStopButton.setTooltip( showTips ? trans->getA( "step next blitter" ) : "" );

    GUIKIT::Layout::alignChildWidth({&control.bltCon0, &control.bltCon1, &control.bltSizeW, &control.bltSizeH});
    GUIKIT::Layout::alignChildWidth({&control.bltSizeW, &control.bltSizeH}, 2);
    GUIKIT::Layout::alignChildWidth({&bltD.data, &bltD.fillIn, &bltD.fillOut});

    GUIKIT::Layout::alignChildWidth({&bltA.data, &bltA.first, &bltA.last, &bltA.barrel, &bltB.data, &bltB.barrel, &bltC});

    std::vector<GUIKIT::Layout*> layouts;
    for (auto* p : minterm.entries) {
        layouts.push_back( p );
    }

    GUIKIT::Layout::alignChildWidth(layouts);
}

auto BlitterDebugger::updateTheme() -> void {
    LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    auto& s = snap.blitter;


    auto& bltCon0 = blitter->colLeft.control.bltCon0;
    updateReg(bltCon0.editShift, s.bltCon0 >> 12);
    updateReg(bltCon0.editChannel, (s.bltCon0 >> 8) & 0xf );
    updateReg(bltCon0.editControl, s.bltCon0 & 0xff);

    auto& bltCon1 = blitter->colLeft.control.bltCon1;
    updateReg(bltCon1.editShift, s.bltCon1 >> 12);
    updateReg(bltCon1.editChannel, (s.bltCon1 >> 8) & 0xf );
    updateReg(bltCon1.editControl, s.bltCon1 & 0xff);

    auto& bltSizeW = blitter->colLeft.control.bltSizeW;
    updateRegDec(bltSizeW.edit, s.bltSizeW);
    updateRegDec(bltSizeW.editCur, s.curW);

    auto& bltSizeH = blitter->colLeft.control.bltSizeH;
    updateRegDec(bltSizeH.edit, s.bltSizeH);
    updateRegDec(bltSizeH.editCur, s.curH);

    auto& flags = blitter->colLeft.flags;
    bool isLine = s.bltCon1 & 1;
    if (isLine != flags.blocks[1]->flag2.checked()) {
        flags.blocks[1]->flag1.setText( isLine ? "Sing" : "Desc" );
        flags.blocks[2]->flag1.setText( isLine ? "Sud" : "Exc Fill" );
        flags.blocks[2]->flag2.setText( isLine ? "Sul" : "Inc Fill" );
        flags.blocks[3]->flag1.setText( isLine ? "Aul" : "Cary Fill" );
        flags.blocks[3]->flag2.setText( isLine ? "Sign" : "Doff" );
    }

    updateReg( flags.blocks[0]->flag1, s.busy );
    updateReg( flags.blocks[0]->flag2, s.zero );
    updateReg( flags.blocks[1]->flag1, s.bltCon1 & 2 );
    updateReg( flags.blocks[1]->flag2, isLine );
    updateReg( flags.blocks[2]->flag1, s.bltCon1 & 0x10 );
    updateReg( flags.blocks[2]->flag2, s.bltCon1 & 8 );
    updateReg( flags.blocks[3]->flag1, s.bltCon1 & 4 );
    updateReg( flags.blocks[3]->flag2, s.bltCon1 & (isLine ? 0x40 : 0x80) );

    auto& bltD = blitter->colLeft.bltD;
    hilight( bltD.data.check, snap.busUsage == 60 );
    updateReg( bltD.data.check, s.bltCon0 & 0x100 );
    updateReg( bltD.data.edit, s.bltDdat );
    if (!isLine && (s.bltCon1 & 0x18)) {
        updateRegBin<16>( bltD.fillIn.edit, s.fillIn );
        updateRegBin<16>( bltD.fillOut.edit, s.bltDdat );
    } else {
        updateReg( bltD.fillIn.edit, "", ~0 );
        updateReg( bltD.fillOut.edit, "", ~0 );
    }

    auto& bltA = blitter->colCenter.bltA;
    hilight( bltA.data.check, snap.busUsage == 57 );
    updateReg( bltA.data.check, s.bltCon0 & 0x800 );
    updateReg( bltA.data.edit, s.bltAdat );
    updateReg( bltA.data.editOld, s.bltADatOld );

    updateReg( bltA.first.check, s.curW == s.bltSizeW );
    updateRegBin<16>( bltA.first.edit, s.bltAfwm  );
    updateReg( bltA.last.check, s.curW == 1 );
    updateRegBin<16>( bltA.last.edit, s.bltAlwm );

    updateReg( bltA.barrel.edit, s.bltADatShifted );

    auto& bltB = blitter->colCenter.bltB;
    hilight( bltB.data.check, snap.busUsage == 58 );
    updateReg( bltB.data.check, s.bltCon0 & 0x400 );
    updateReg( bltB.data.edit, s.bltBdat );
    updateReg( bltB.data.editOld, s.bltBDatOld );
    updateReg( bltB.barrel.edit, s.bltBDatShifted );

    auto& bltC = blitter->colCenter.bltC;
    hilight( bltC.check, snap.busUsage == 59 );
    updateReg( bltC.check, s.bltCon0 & 0x200 );
    updateReg( bltC.edit, s.bltCdat );

    auto& mEntries = blitter->minterm.entries;

    uint16_t a = s.bltADatShifted;
    uint16_t b = s.bltBDatShifted;
    uint16_t c = s.bltCdat;

    updateRegBin<16>(mEntries[0]->edit, a );
    updateRegBin<16>(mEntries[1]->edit, b );
    updateRegBin<16>(mEntries[2]->edit, c );

    updateReg(mEntries[3]->check, s.bltCon0 & 1 );
    updateRegBin<16>(mEntries[3]->edit, ~a & ~b & ~c );
    updateReg(mEntries[4]->check, s.bltCon0 & 2 );
    updateRegBin<16>(mEntries[4]->edit, ~a & ~b & c );
    updateReg(mEntries[5]->check, s.bltCon0 & 4 );
    updateRegBin<16>(mEntries[5]->edit, ~a & b & ~c );
    updateReg(mEntries[6]->check, s.bltCon0 & 8 );
    updateRegBin<16>(mEntries[6]->edit, ~a & b & c );
    updateReg(mEntries[7]->check, s.bltCon0 & 0x10 );
    updateRegBin<16>(mEntries[7]->edit, a & ~b & ~c );
    updateReg(mEntries[8]->check, s.bltCon0 & 0x20 );
    updateRegBin<16>(mEntries[8]->edit, a & ~b & c );
    updateReg(mEntries[9]->check, s.bltCon0 & 0x40 );
    updateRegBin<16>(mEntries[9]->edit, a & b & ~c );
    updateReg(mEntries[10]->check, s.bltCon0 & 0x80 );
    updateRegBin<16>(mEntries[10]->edit, a & b & c );

    updateRegBin<16>(mEntries[11]->edit, s.minterm );

    updateControl( snap.vPos, snap.hPos );
}

auto BlitterDebugger::buildControl() -> GUIKIT::Layout* {
    blitterControl = new BlitterControl();

    blitterControl->softStopButton.setImage( &nextImg );
    blitterControl->softStopButton.onActivate = [this]() {
        if (emulator != activeEmulator)
            return;
        emuThread->lock();
        timerVisibility->setEnabled();
        emulator->debuggerAdd( DebuggerTheme::Blitter, DebuggerAction::SoftstopBlitter, 0 );
        emuThread->unlockDebugger();
        emuThread->unlock();
    };

    return blitterControl;
}

auto BlitterDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Blitter, DebuggerAction::None, 0);
}

auto BlitterDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Blitter, DebuggerAction::None);
}

auto BlitterDebugger::binaryLength() -> unsigned {
    static unsigned _w = 0;
    if (_w == 0) {
        GUIKIT::LineEdit l;
        l.setFont( GUIKIT::Font::monospace(  ) );
        l.setText( "0000000000000000" );
        _w = l.minimumSize().width;
    }
    return _w;
}
