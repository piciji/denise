
#include "agnusDebugger.h"

#include "../../emulation/libami/system/debuggerSnapshot.h"

AgnusDebugger::AgnusDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Debugger::Mode::Agnus ) {
    build();
}

AgnusDebugger::Agnus::Entry::Entry(bool useCheck) {
    if (useCheck) {
        check.setReadonly(  );
        append( check, {0u, 0u}, 10 );
    } else {
        label.setAlign( GUIKIT::Label::Align::Right );
        append( label, {0u, 0u}, 10 );
    }

    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace(  ) );
    append( edit, {70u, 0u} );

    setAlignment( 0.5 );
};

AgnusDebugger::Agnus::ContainerBplRef::Bpl::Bpl() : mod1(false), mod2(false) {
    for (int i = 1; i <= 6; i++) {
        auto* entry = new Entry();
        entry->check.setText( "BPL" + std::to_string(i) + "PT" );
        append( *entry, {0u, 0u}, 10 );
        entries.push_back( entry );
    }
    append( mod1, {0u, 0u}, 10 );
    append( mod2, {0u, 0u} );
    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerBplRef::Ref::Ref() : entry( false ) {
    entry.label.setText( "REFPTR" );
    append( entry, {0u, 0u} );
    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerBplRef::ContainerBplRef() {
    append( bpl, {0u, 0u}, 10 );
    append( ref, {0u, 0u} );
}

AgnusDebugger::Agnus::ContainerSprDsk::Spr::Spr() {
    for (int i = 0; i < 8; i++) {
        auto* entry = new Entry();
        entry->check.setText( "SPR" + std::to_string(i) + "PT" );
        append( *entry, {0u, 0u}, i < 7 ? 10 : 0 );
        entries.push_back( entry );
    }
    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerSprDsk::Dsk::Dsk() {
    entry.check.setText( "DSKPT" );
    append( entry, {0u, 0u} );
    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerSprDsk::ContainerSprDsk() {
    append( spr, {0u, 0u}, 10 );
    append( dsk, {0u, 0u} );
}

AgnusDebugger::Agnus::ContainerBltCop::Blt::Blt() {
    for (auto& cha : {'A', 'B', 'C', 'D'}) {
        auto* entry = new Entry();
        std::string s(1, cha);
        entry->check.setText( "BLT" + s +  "PT" );
        append( *entry, {0u, 0u}, 10 );
        entries.push_back( entry );
    }
    for (auto& cha : {'A', 'B', 'C', 'D'}) {
        auto* entry = new Entry(false);
        std::string s(1, cha);
        entry->label.setText( "BLT" + s +  "MOD" );
        append( *entry, {0u, 0u}, cha != 'D' ? 10 : 0 );
        mods.push_back( entry );
    }
    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerBltCop::Cop::Cop() {
    entry.check.setText( "COPPC" );
    append( entry, {0u, 0u} );
    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerBltCop::ContainerBltCop() {
    append( blt, {0u, 0u}, 10 );
    append( cop, {0u, 0u} );
}

AgnusDebugger::Agnus::ContainerAudReg::Aud::Entry::Entry( ) {
    check.setReadonly(  );
    edit.setEditable( false );
    edit.setAlign( GUIKIT::LineEdit::Align::Right );
    edit.setFont( GUIKIT::Font::monospace(  ) );
    editLatch.setEditable( false );
    editLatch.setFont( GUIKIT::Font::monospace(  ) );

    append( check, {0u, 0u}, 10 );
    append( edit, {70u, 0u}, 10 );
    append( img, {0u, 0u}, 10 );
    append( editLatch, {70u, 0u}, 10 );
    append( labelLatch, {0u, 0u} );

    setAlignment( 0.5 );
}

AgnusDebugger::Agnus::ContainerAudReg::Aud::Aud() {
    for (int i = 0; i < 4; i++) {
        auto* entry = new Entry();
        entry->check.setText( "AUD" + std::to_string(i) + "PT" );
        entry->labelLatch.setText( "AUD" + std::to_string(i) + "LC" );
        append( *entry, {0u, 0u}, i < 3 ? 10 : 0 );
        entries.push_back( entry );
    }
    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerAudReg::Reg::Entry::Entry( ) {
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace(  ) );
    editR.setEditable( false );
    editR.setFont( GUIKIT::Font::monospace(  ) );

    append( label, {0u, 0u}, 10 );
    append( edit, {70u, 0u}, 10 );
    append( labelR, {0u, 0u}, 10 );
    append( editR, {70u, 0u} );

    setAlignment( 0.5 );
}

AgnusDebugger::Agnus::ContainerAudReg::Reg::Reg() {
    line1.label.setText( "DMACON" );
    line1.labelR.setText( "BPL0CON" );
    append( line1, {0u, 0u}, 10 );

    line2.label.setText( "DDFSTRT" );
    line2.labelR.setText( "DDFSTOP" );
    append( line2, {0u, 0u}, 10 );

    line3.label.setText( "VSTRT" );
    line3.labelR.setText( "VSTOP" );
    append( line3, {0u, 0u}, 10 );

    line4.label.setText( "BEAMCON0" );
    line4.remove( line4.editR );
    line4.remove( line4.labelR );
    bltPri.setText( "BLTPRI" );
    bltPri.setReadonly(  );
    line4.append( bltPri, {0u, 0u} );
    append( line4, {0u, 0u} );

    setPadding( 10 );
}

AgnusDebugger::Agnus::ContainerAudReg::ContainerAudReg() {
    append( aud, {0u, 0u}, 10 );
    append( reg, {~0u, 0u} );
}

AgnusDebugger::Agnus::Agnus() {
    append(containerBplRef, {0u, 0u}, 10);
    append(containerSprDsk, {0u, 0u}, 10);
    append(containerBltCop, {0u, 0u}, 10);
    append(containerAudReg, {0u, 0u});
}

auto AgnusDebugger::buildTheme() -> GUIKIT::Layout* {
    agnus = new Agnus();
    for (auto& entry : agnus->containerAudReg.aud.entries)
        entry->img.setImage( &arrowLeftImg );
    return agnus;
}

auto AgnusDebugger::updateTheme() -> void {
    LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    auto& s = snap.agnus;
    unsigned mask = 0x220;
    auto& sprEntries = agnus->containerSprDsk.spr.entries;
    int i = 0;
    for (auto& entry : sprEntries) {
        updateReg( entry->check, ((s.dmaCon & mask) == mask) && s.sprEnable[i]);
        updateReg( entry->edit, s.sprPtr[i++]);
    }

    auto& bpl = agnus->containerBplRef.bpl;
    i = 0;
    mask = 0x300;
    unsigned bplInUse = (s.bplCon0 >> 12) & 7;
    for (auto& entry : bpl.entries) {
        updateReg( entry->check, ((s.dmaCon & mask) == mask) && (i < bplInUse));
        updateReg( entry->edit, s.bplPtr[i++]);
    }
    updateReg( bpl.mod1.edit, s.bpl1Mod );
    updateReg( bpl.mod2.edit, s.bpl2Mod );
    updateReg( agnus->containerBplRef.ref.entry.edit, s.refPtr );

    auto& blt = agnus->containerBltCop.blt;
    i = 0;
    mask = 0x240;
    for (auto& entry : blt.entries) {
        updateReg( entry->check, ((s.dmaCon & mask) == mask) && (s.bltCon0 & (0x800 >> i)) );
        updateReg( entry->edit, s.bltPtr[i++]);
    }
    i = 0;
    for (auto& entry : blt.mods) {
        updateReg( entry->edit, s.bltMod[i++]);
    }

    i = 0;
    auto& aud = agnus->containerAudReg.aud;
    for (auto& entry : aud.entries) {
        mask = (1 << i) | 0x200;
        updateReg( entry->check, (s.dmaCon & mask) == mask );
        updateReg( entry->edit, s.audPtr[i]);
        updateReg( entry->editLatch, s.audLcPtr[i++]);
    }

    auto& reg = agnus->containerAudReg.reg;
    updateReg( reg.line1.edit, s.dmaCon);
    updateReg( reg.line1.editR, s.bltCon0);
    updateReg( reg.line2.edit, s.ddfStrt);
    updateReg( reg.line2.editR, s.ddfStop);
    updateReg( reg.line3.edit, s.diwStrt);
    updateReg( reg.line3.editR, s.diwStop);
    updateReg( reg.line4.edit, s.beamCon0);
    updateReg( reg.bltPri, s.dmaCon & 0x400);

    mask = 0x210;
    auto& dsk = agnus->containerSprDsk.dsk;
    updateReg( dsk.entry.check, (s.dmaCon & mask) == mask );
    updateReg( dsk.entry.edit, s.dskPtr);
    mask = 0x280;
    auto& cop = agnus->containerBltCop.cop;
    updateReg( cop.entry.check, (s.dmaCon & mask) == mask );
    updateReg( cop.entry.edit, s.copPtr);

    updateControl( snap.vPos, snap.hPos );
}

auto AgnusDebugger::translateTheme() -> void {
    //bool showTips = showTipsItem.checked();

    agnus->containerBplRef.bpl.setText( "Bitplanes" );
    agnus->containerBplRef.ref.setText( "Refresh" );
    agnus->containerSprDsk.spr.setText( "Sprites" );
    agnus->containerSprDsk.dsk.setText( "Disk" );
    agnus->containerBltCop.blt.setText( "Blitter" );
    agnus->containerBltCop.cop.setText( "Copper" );
    agnus->containerAudReg.aud.setText( "Audio" );
    agnus->containerAudReg.reg.setText( "Register" );
    agnus->containerBplRef.bpl.mod1.label.setText( "BPL1MOD" );
    agnus->containerBplRef.bpl.mod2.label.setText( "BPL2MOD" );

    std::vector<GUIKIT::Layout*> entries;
    for (auto& entry : agnus->containerBplRef.bpl.entries)
        entries.push_back( entry );
    entries.push_back( &agnus->containerBplRef.bpl.mod1 );
    entries.push_back( &agnus->containerBplRef.bpl.mod2 );
    entries.push_back( &agnus->containerBplRef.ref.entry );
    GUIKIT::Layout::alignChildWidth( entries );

    entries.clear();
    for (auto& entry : agnus->containerBltCop.blt.entries)
        entries.push_back( entry );
    for (auto& entry : agnus->containerBltCop.blt.mods)
        entries.push_back( entry );
    entries.push_back( &agnus->containerBltCop.cop.entry );
    GUIKIT::Layout::alignChildWidth( entries );

    entries.clear();
    for (auto& entry : agnus->containerSprDsk.spr.entries)
        entries.push_back( entry );
    entries.push_back( &agnus->containerSprDsk.dsk.entry );
    GUIKIT::Layout::alignChildWidth( entries );

    entries.clear();
    for (auto& entry : agnus->containerAudReg.aud.entries)
        entries.push_back( entry );
    GUIKIT::Layout::alignChildWidth( entries );

    entries.clear();
    entries.push_back( &agnus->containerAudReg.reg.line1 );
    entries.push_back( &agnus->containerAudReg.reg.line2 );
    entries.push_back( &agnus->containerAudReg.reg.line3 );
    entries.push_back( &agnus->containerAudReg.reg.line4 );
    GUIKIT::Layout::alignChildWidth( entries, 0 );
    GUIKIT::Layout::alignChildWidth( entries, 2 );
}

auto AgnusDebugger::saveIdent() -> std::string {
    return "debugger_agnus";
}

auto AgnusDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger Agnus";
}


auto AgnusDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Agnus, DebuggerAction::None, 0);
}

auto AgnusDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Agnus, DebuggerAction::None);
}
