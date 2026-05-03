
#include "paulaDebugger.h"

#include "../../emulation/libami/system/debuggerSnapshot.h"

PaulaDebugger::Paula::Intr::Header::Header() {
    append( spacer, {~0u, 0u} );
    append( label, {0u, 0u}, 10 );
    append( labelR, {0u, 0u} );
    append( spacerR, {~0u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::Intr::Body::Body() {
    edit.setEditable( false );
    edit.setAlign( GUIKIT::LineEdit::Align::Right );
    editR.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace( ) );
    editR.setFont( GUIKIT::Font::monospace( ) );

    append( spacer, {~0u, 0u} );
    append( edit, {60u, 0u}, 10 );
    append( editR, {60u, 0u} );
    append( spacerR, {~0u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::Intr::Entry::Entry() {
    check.setReadonly(  );
    checkR.setReadonly(  );
    label.setAlign( GUIKIT::Label::Align::Right );

    append( label, {0u, 0u}, 4 );
    append( check, {0u, 0u}, 10 );
    append( checkR, {0u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::Intr::Intr() {
    const char* inters[] = { "INTEN", "EXTER", "DSKSYN", "RBF", "AUD3", "AUD2", "AUD1", "AUD0",
        "BLIT", "VERTB", "COPPER", "PORTS", "SOFT", "DSKBLK", "TBE"  };

    header.label.setText( "INTENA" );
    header.labelR.setText( "INTREQ" );
    append( header, {~0u, 0u}, 5 );
    append( body, {~0u, 0u}, 5 );

    auto elements = std::size(inters);
    entries.reserve( elements );

    int i = 0;
    for (auto& inter : inters) {
        auto* entry = new Entry();
        entry->label.setText( inter );
        entry->checkR.setText( inter );
        entries.push_back( entry );
        append( *entry, {0u, 0u}, ++i < elements ? 7 : 0 );
    }
    setPadding( 10 );
}

PaulaDebugger::Paula::Aud::Cha::Line1::Line1() {
    editLen.setEditable( false );
    editCurLen.setEditable( false );
    editLen.setFont( GUIKIT::Font::monospace( ) );
    editCurLen.setFont( GUIKIT::Font::monospace( ) );
    editPer.setEditable( false );
    editCurPer.setEditable( false );
    editPer.setFont( GUIKIT::Font::monospace( ) );
    editCurPer.setFont( GUIKIT::Font::monospace( ) );

    append( labelLen, {0u, 0u}, 10 );
    append( editLen, {60u, 0u}, 5 );
    append( imageLen, {0u, 0u}, 5 );
    append( editCurLen, {60u, 0u}, 10 );

    append( labelPer, {0u, 0u}, 10 );
    append( editPer, {60u, 0u}, 5 );
    append( imagePer, {0u, 0u}, 5 );
    append( editCurPer, {60u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::Aud::Cha::Line2::Line2() {
    editDat.setEditable( false );
    editCurDat.setEditable( false );
    editDat.setFont( GUIKIT::Font::monospace( ) );
    editCurDat.setFont( GUIKIT::Font::monospace( ) );
    editVol.setEditable( false );
    editCurVol.setEditable( false );
    editVol.setFont( GUIKIT::Font::monospace( ) );
    editCurVol.setFont( GUIKIT::Font::monospace( ) );

    append( labelDat, {0u, 0u}, 10 );
    append( editDat, {60u, 0u}, 5 );
    append( imageDat, {0u, 0u}, 5 );
    append( editCurDat, {60u, 0u}, 10 );

    append( labelVol, {0u, 0u}, 10 );
    append( editVol, {60u, 0u}, 5 );
    append( imageVol, {0u, 0u}, 5 );
    append( editCurVol, {60u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::Aud::Cha::Line3::Line3() {
    useV.setReadonly(  );
    useP.setReadonly(  );

    append( useV, {0u, 0u}, 10 );
    append( useP, {0u, 0u}, 20 );
    append( state, {0u, 0u}, 10 );

    std::vector<GUIKIT::RadioBox*> boxes;
    for (auto& check : radios) {
        check.setReadonly(  );
        append( check, {0u, 0u}, 10 );
        boxes.push_back( &check );
    }
    GUIKIT::RadioBox::setGroup( boxes );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::Aud::Cha::Cha() {
    append( line1, {0u, 0u}, 10 );
    append( line2, {0u, 0u}, 10 );
    append( line3, {0u, 0u} );

    setPadding( 10 );
}

PaulaDebugger::Paula::Aud::Aud() {
    int i = 0;
    auto s = std::size( chas );
    for (auto& cha : chas) {
        append( cha, {0u, 0u}, ++i < s ? 5 : 0 );
    }
}

PaulaDebugger::Paula::FdcAndPot::Fdc::Entry::Entry() {
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace( ) );
    editR.setEditable( false );
    editR.setFont( GUIKIT::Font::monospace( ) );

    append( label, {0u, 0u}, 10 );
    append( edit, {60u, 0u}, 10 );
    append( labelR, {0u, 0u}, 10 );
    append( editR, {80u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::FdcAndPot::Fdc::Flags::Flags() {
    check1.setReadonly( );
    check2.setReadonly( );
    check3.setReadonly( );

    append( check1, {0u, 0u}, 10 );
    append( check2, {0u, 0u}, 10 );
    append( check3, {0u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::FdcAndPot::Fdc::Selected::Selected() {
    append( label, {0u, 0u}, 10 );

    int i = 0;
    auto s = std::size(checks);
    for (auto& check : checks) {
        check.setReadonly(  );
        check.setText( std::to_string( i++ ) );
        append( check, {0u, 0u}, i < s ? 10 : 0 );
    }
    setAlignment( 0.5 );
}

PaulaDebugger::Paula::FdcAndPot::Fdc::Fifo::Fifo() {
    edit1.setEditable( false );
    edit2.setEditable( false );
    edit3.setEditable( false );
    edit1.setFont( GUIKIT::Font::monospace( ) );
    edit2.setFont( GUIKIT::Font::monospace( ) );
    edit3.setFont( GUIKIT::Font::monospace( ) );

    append( imgIn, {0u, 0u}, 5 );
    append( edit1, {60u, 0u}, 10 );
    append( edit2, {60u, 0u}, 10 );
    append( edit3, {60u, 0u}, 5 );
    append( imgOut, {0u, 0u} );

    setPadding( 10 );
    setAlignment( 0.5 );
}

PaulaDebugger::Paula::FdcAndPot::Fdc::Fdc() {
    append(entry1, {0u, 0u}, 10 );
    append(entry2, {0u, 0u}, 10 );
    append(entry3, {0u, 0u}, 10 );

    append(selected, {0u, 0u}, 10 );
    append(flags1, {0u, 0u}, 10 );
    append(flags2, {0u, 0u}, 10 );
    append(fifo, {0u, 0u} );

    setPadding( 10 );
}

PaulaDebugger::Paula::FdcAndPot::Pot::PotGo::PotGo() {
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace( ) );

    append( label, {0u, 0u}, 10 );
    append( edit, {getWidth( 4, true ), 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::FdcAndPot::Pot::Flags::Flags() {
    check1.setReadonly( );
    check2.setReadonly( );
    check3.setReadonly( );
    check4.setReadonly( );

    append( check1, {0u, 0u}, 10 );
    append( check2, {0u, 0u}, 10 );
    append( check3, {0u, 0u}, 10 );
    append( check4, {0u, 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::FdcAndPot::Pot::PotDat::PotDat() {
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace( ) );
    editR.setEditable( false );
    editR.setFont( GUIKIT::Font::monospace( ) );

    append( label, {0u, 0u}, 10 );
    append( edit, {getWidth( 4, true ), 0u}, 10 );
    append( labelR, {0u, 0u}, 10 );
    append( editR, {getWidth( 4, true ), 0u} );

    setAlignment( 0.5 );
}

PaulaDebugger::Paula::FdcAndPot::Pot::Pot() {
    append( potGo, {0u, 0u}, 10 );
    append( flagsPotGoDir, {0u, 0u}, 10 );
    append( flagsPotGo, {0u, 0u}, 10 );
    append( potGoR, {0u, 0u}, 10 );
    append( flagsPotGoR, {0u, 0u}, 10 );
    append( potDat, {0u, 0u} );

    setPadding( 10 );
}

PaulaDebugger::Paula::FdcAndPot::FdcAndPot() {
    append( fdc, {0u, 0u}, 5 );
    append( pot, {0u, 0u} );
}

PaulaDebugger::Paula::Paula() {
    append( intr, {0u, 0u}, 15 );
    append( aud, {0u, 0u}, 15 );
    append( fdcAndPot, {0u, 0u} );
}

PaulaDebugger::PaulaDebugger( Emulator::Interface* emulator )
: Debugger( emulator ) {
}

auto PaulaDebugger::updateTheme() -> void {
    LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    auto& s = snap.paula;

    auto& intr = paula->intr;

    updateReg( intr.body.edit, s.intena );
    updateReg( intr.body.editR, s.intreq );

    int i = 0;
    for (auto* entry : intr.entries) {
        updateReg( entry->check, s.intena & (0x4000 >> i)  );
        updateReg( entry->checkR, s.intreq & (0x4000 >> i++)  );
    }

    auto& aud = paula->aud;
    i = 0;
    for (auto& audCha : aud.chas) {
        auto& cha = s.chas[i];
        updateReg(audCha.line1.editLen, cha.lenLatch);
        updateReg(audCha.line1.editCurLen, cha.len);

        updateReg(audCha.line1.editPer, cha.perLatch);
        updateReg(audCha.line1.editCurPer, cha.per);

        updateReg(audCha.line2.editDat, cha.datLatch);
        updateReg(audCha.line2.editCurDat, cha.dat);

        updateReg(audCha.line2.editVol, cha.volLatch);
        updateReg(audCha.line2.editCurVol, cha.vol);

        int _state = std::min((int)cha.state, 5);
        updateReg(audCha.line3.radios[_state]);

        updateReg(audCha.line3.useV, s.adkcon & (1 << i) );
        updateReg(audCha.line3.useP, s.adkcon & (0x10 << i) );

        i++;
    }

    auto& fdc = paula->fdcAndPot.fdc;
    updateReg( fdc.entry1.edit, s.dskLen );
    updateReg( fdc.entry1.editR, s.dskTransferLength );
    updateReg( fdc.entry2.edit, s.adkcon );
    updateReg( fdc.entry2.editR, s.dskSync );
    updateReg( fdc.entry3.edit, s.dskBytr );
    updateReg( fdc.entry3.editR, LIBAMI::DebuggerSnapshot::diskState[s.diskState], s.diskState );

    updateReg(fdc.selected.checks[0], s.selectedDrive & 1);
    updateReg(fdc.selected.checks[1], s.selectedDrive & 2);
    updateReg(fdc.selected.checks[2], s.selectedDrive & 4);
    updateReg(fdc.selected.checks[3], s.selectedDrive & 8);

    updateReg( fdc.flags1.check1, s.adkcon & 0x400 );
    updateReg( fdc.flags1.check2, s.adkcon & 0x200 );
    updateReg( fdc.flags1.check3, s.adkcon & 0x100 );

    updateReg( fdc.flags2.check1, s.wordEqual );
    updateReg( fdc.flags2.check2, s.dskBytr & 0x8000 );
    updateReg( fdc.flags2.check3, s.dskBytr & 0x4000 );

    if (s.fifoPos > 0) {
        updateReg( fdc.fifo.edit1, GUIKIT::String::convertToHex( s.fifo & 0xffff ), s.fifo & 0xffff );
        if (s.fifoPos > 1) {
            updateReg( fdc.fifo.edit2, GUIKIT::String::convertToHex( (s.fifo >> 16) & 0xffff ), (s.fifo >> 16) & 0xffff );
            if (s.fifoPos > 2)
                updateReg( fdc.fifo.edit3, GUIKIT::String::convertToHex( (s.fifo >> 32) & 0xffff ), (s.fifo >> 32) & 0xffff );
            else
                updateReg( fdc.fifo.edit3, "", ~0 );
        } else {
            updateReg( fdc.fifo.edit2, "", ~0 );
            updateReg( fdc.fifo.edit3, "", ~0 );
        }
    } else {
        updateReg( fdc.fifo.edit1, "", ~0 );
        updateReg( fdc.fifo.edit2, "", ~0 );
        updateReg( fdc.fifo.edit3, "", ~0 );
    }

    auto& pot = paula->fdcAndPot.pot;
    updateReg( pot.potGo.edit, s.potgo);
    updateReg( pot.potGoR.edit, s.potgoR);
    updateReg( pot.potDat.edit, s.pot0Dat);
    updateReg( pot.potDat.editR, s.pot1Dat);

    updateReg( pot.flagsPotGoDir.check1, s.potgo & 0x8000 );
    updateReg( pot.flagsPotGoDir.check2, s.potgo & 0x2000 );
    updateReg( pot.flagsPotGoDir.check3, s.potgo & 0x800 );
    updateReg( pot.flagsPotGoDir.check4, s.potgo & 0x200 );

    updateReg( pot.flagsPotGo.check1, s.potgo & 0x4000 );
    updateReg( pot.flagsPotGo.check2, s.potgo & 0x1000 );
    updateReg( pot.flagsPotGo.check3, s.potgo & 0x400 );
    updateReg( pot.flagsPotGo.check4, s.potgo & 0x100 );

    updateReg( pot.flagsPotGoR.check1, s.potgoR & 0x4000 );
    updateReg( pot.flagsPotGoR.check2, s.potgoR & 0x1000 );
    updateReg( pot.flagsPotGoR.check3, s.potgoR & 0x400 );
    updateReg( pot.flagsPotGoR.check4, s.potgoR & 0x100 );

    updateControl( snap.vPos, snap.hPos );
}

auto PaulaDebugger::saveIdent() -> std::string {
    return "debugger_paula";
}

auto PaulaDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger Paula";
}

auto PaulaDebugger::buildTheme() -> GUIKIT::Layout* {
    paula = new Paula( );
    for (auto& cha : paula->aud.chas) {
        cha.line1.imageLen.setImage( &arrowRightImg );
        cha.line1.imagePer.setImage( &arrowRightImg );
        cha.line2.imageDat.setImage( &arrowRightImg );
        cha.line2.imageVol.setImage( &arrowRightImg );
    }
    auto& fdc = paula->fdcAndPot.fdc;
    fdc.fifo.imgIn.setImage(&arrowRightImg);
    fdc.fifo.imgOut.setImage(&arrowRightImg);
    return paula;
}

auto PaulaDebugger::translateTheme() -> void {
    paula->intr.setText( "Interrupts" );
    paula->fdcAndPot.fdc.setText( "FDC" );
    paula->fdcAndPot.fdc.fifo.setText( "FIFO" );

    paula->fdcAndPot.pot.setText( "POT" );
    paula->fdcAndPot.pot.potGo.label.setText( "POTGO" );
    paula->fdcAndPot.pot.potGoR.label.setText( "POTGOR" );

    paula->fdcAndPot.pot.flagsPotGoDir.check1.setText( "OUTRY" );
    paula->fdcAndPot.pot.flagsPotGoDir.check2.setText( "OUTRX" );
    paula->fdcAndPot.pot.flagsPotGoDir.check3.setText( "OUTLY" );
    paula->fdcAndPot.pot.flagsPotGoDir.check4.setText( "OUTLX" );

    paula->fdcAndPot.pot.flagsPotGo.check1.setText( "DATRY" );
    paula->fdcAndPot.pot.flagsPotGo.check2.setText( "DATRX" );
    paula->fdcAndPot.pot.flagsPotGo.check3.setText( "DATLY" );
    paula->fdcAndPot.pot.flagsPotGo.check4.setText( "DATLX" );

    paula->fdcAndPot.pot.flagsPotGoR.check1.setText( "DATRY" );
    paula->fdcAndPot.pot.flagsPotGoR.check2.setText( "DATRX" );
    paula->fdcAndPot.pot.flagsPotGoR.check3.setText( "DATLY" );
    paula->fdcAndPot.pot.flagsPotGoR.check4.setText( "DATLX" );

    paula->fdcAndPot.pot.potDat.label.setText( "POT0DAT" );
    paula->fdcAndPot.pot.potDat.labelR.setText( "POT1DAT" );

    std::vector<GUIKIT::Layout*> entries;
    int i = 0;
    for (auto& cha : paula->aud.chas) {
        cha.setText( "AUD " + std::to_string( i ) );

        cha.line1.labelLen.setText( "AUD" + std::to_string( i ) + "LEN" );
        cha.line1.labelPer.setText( "AUD" + std::to_string( i ) + "PER" );
        cha.line2.labelDat.setText( "AUD" + std::to_string( i ) + "DAT" );
        cha.line2.labelVol.setText( "AUD" + std::to_string( i ) + "VOL" );

        if (i == 3) {
            cha.line3.useV.setText( "USE" + std::to_string( i ) + "VN");
            cha.line3.useP.setText( "USE" + std::to_string( i ) + "PN");
        } else {
            cha.line3.useV.setText( "USE" + std::to_string( i ) + "V" + std::to_string( i + 1 ));
            cha.line3.useP.setText( "USE" + std::to_string( i ) + "P" + std::to_string( i + 1 ));
        }

        cha.line3.state.setText( "State:" );
        int state = 0;
        for (auto& check : cha.line3.radios) {
            check.setText( std::to_string( state++ ) );
        }

        entries.push_back( &cha.line1 );
        entries.push_back( &cha.line2 );

        i++;
    }
    GUIKIT::Layout::alignChildWidth( entries );
    GUIKIT::Layout::alignChildWidth( entries, 4 );

    auto& fdc = paula->fdcAndPot.fdc;
    auto& pot = paula->fdcAndPot.pot;
    fdc.entry1.label.setText("DSKLEN");
    fdc.entry1.labelR.setText("Transfer");
    fdc.entry2.label.setText("ADKCON");
    fdc.entry2.labelR.setText("DSKSYNC");
    fdc.entry3.label.setText("DSKBYTR");
    fdc.entry3.labelR.setText("State");

    fdc.selected.label.setText("Drive");

    fdc.flags1.check1.setText("Word Sync");
    fdc.flags1.check2.setText("Msb Sync");
    fdc.flags1.check3.setText("Fast");

    fdc.flags2.check1.setText("Word Equal");
    fdc.flags2.check2.setText("Byte Ready");
    fdc.flags2.check3.setText("Dma On");

    entries.clear();
    entries.push_back( &fdc.entry1 );
    entries.push_back( &fdc.entry2 );
    entries.push_back( &fdc.entry3 );
    entries.push_back( &fdc.selected );
    GUIKIT::Layout::alignChildWidth( entries );
    entries.pop_back();
    GUIKIT::Layout::alignChildWidth( entries, 2 );

    entries.clear();
    entries.push_back( &fdc.flags1 );
    entries.push_back( &fdc.flags2 );
    GUIKIT::Layout::alignChildWidth( entries, 0 );
    GUIKIT::Layout::alignChildWidth( entries, 1 );
    GUIKIT::Layout::alignChildWidth( entries, 2 );

    entries.clear();
    for (auto* entry : paula->intr.entries)
        entries.push_back( entry );
    GUIKIT::Layout::alignChildWidth( entries );

    entries.clear();
    entries.push_back( &pot.potGo );
    entries.push_back( &pot.potGoR );
    entries.push_back( &pot.potDat );
    GUIKIT::Layout::alignChildWidth( entries );

    entries.clear();
    entries.push_back( &pot.flagsPotGo );
    entries.push_back( &pot.flagsPotGoR );
    entries.push_back( &pot.flagsPotGoDir );
    GUIKIT::Layout::alignChildWidth( entries, 0 );
    GUIKIT::Layout::alignChildWidth( entries, 1 );
    GUIKIT::Layout::alignChildWidth( entries, 2 );
}

auto PaulaDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Paula, DebuggerAction::None, 0);
}

auto PaulaDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Paula, DebuggerAction::None);
}
