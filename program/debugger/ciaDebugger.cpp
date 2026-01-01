
#include "ciaDebugger.h"
#include "../thread/emuThread.h"
#include "../program.h"

CiaDebugger::CiaDebugger( Emulator::Interface* emulator, Mode mode )
: Debugger( emulator, mode ) {
    build();
}

CiaDebugger::CIA::Chip::Port::Port(uint8_t port) {
    pr.setText( port == 0 ? "PRA:" : "PRB:" );
    ddr.setText( port == 0 ? "DDRA:" : "DDRB:" );
    prVal.setText("0");
    ddrVal.setText("0");
    prVal.setStore( 0 );
    ddrVal.setStore( 0 );
    prVal.setEditable( false );
    ddrVal.setEditable( false );

    append(pr, {50u, 0u}, 5);
    append(prVal, {40u, 0u}, 10);
    append(ddr, {40u, 0u}, 5);
    append(ddrVal, {40u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::PortIO::PortIO(uint8_t chipNr, uint8_t portNr, Debugger* debugger) {
    portLabel.setText( portNr == 0 ? "Port A:" : "Port B:" );
    portLabel.setFont( GUIKIT::Font::system( "bold" ) );

    append( portLabel, {50u, 0u}, 5 );
    const Emulator::Interface::DebuggerIdent* idents = debugger->isAmiga() ? &LIBAMI::DebuggerSnapshot::CiaPorts[chipNr][portNr][0]
    : &LIBC64::DebuggerSnapshot::CiaPorts[chipNr][portNr][0];

    //for (const Emulator::Interface::DebuggerIdent& port : idents) {
    for (unsigned i = 0; i < 8; i++) {
        const Emulator::Interface::DebuggerIdent& port = idents[i];
        auto* l = new GUIKIT::Label;
        l->setText( port.ident );
        l->setStore( 1 );
        l->setFont( GUIKIT::Font::system( "bold" ) );
        l->setEnabled( true );
        line.push_back(l);
        append( *l, {l->minimumSize().width, 0u}, 10 );
    }
    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Timer::Timer(uint8_t portNr) {
    label.setText( portNr == 0 ? "Timer A:" : "Timer B:" );
    label.setStore( 0 );
    latch.setText( "Latch:" );
    val.setText("0");
    latchVal.setText("0");
    val.setStore( 0 );
    latchVal.setStore( 0 );
    val.setEditable( false );
    latchVal.setEditable( false );

    oneShot.setText( "oneshot" );
    pbOut.setText( portNr == 0 ? "PB6" : "PB7" );
    toggleOut.setText( "toggle" );

    oneShot.setReadonly();
    pbOut.setReadonly();
    toggleOut.setReadonly();

    append(label, {50u, 0u}, 5);
    append(val, {40u, 0u}, 10);
    append(latch, {40u, 0u}, 5);
    append(latchVal, {40u, 0u}, 10);

    append(oneShot, {0u, 0u}, 5);
    append(pbOut, {0u, 0u}, 5);
    append(toggleOut, {0u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::IntData::IntData() {
    icr.setText( "ICR:" );
    ir.setText( "IR" );
    flag.setText( "Flag" );
    sp.setText( "SP" );
    alarm.setText( "Alarm" );
    tb.setText( "TB" );
    ta.setText( "TA" );
    icrVal.setText( "0" );
    icrVal.setStore( 0 );
    ir.setReadonly();
    flag.setReadonly();
    sp.setReadonly();
    alarm.setReadonly();
    ta.setReadonly();
    tb.setReadonly();
    icrVal.setEditable( false );

    append(icr, {50u, 0u}, 5);
    append(icrVal, {40u, 0u}, 10);
    append(ir, {0u, 0u}, 5);
    append(flag, {0u, 0u}, 5);
    append(sp, {0u, 0u}, 5);
    append(alarm, {0u, 0u}, 5);
    append(tb, {0u, 0u}, 5);
    append(ta, {0u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::IntMask::IntMask() {
    mask.setText( "Mask:" );
    ir.setText( "IR" );
    flag.setText( "Flag" );
    sp.setText( "SP" );
    alarm.setText( "Alarm" );
    tb.setText( "TB" );
    ta.setText( "TA" );
    maskVal.setText( "0" );
    maskVal.setStore( 0 );
    maskVal.setEditable( false );

    ir.setEnabled( false );
    flag.setReadonly();
    sp.setReadonly();
    alarm.setReadonly();
    ta.setReadonly();
    tb.setReadonly();

    append(mask, {50u, 0u}, 5);
    append(maskVal, {40u, 0u}, 10);
    append(ir, {0u, 0u}, 5);
    append(flag, {0u, 0u}, 5);
    append(sp, {0u, 0u}, 5);
    append(alarm, {0u, 0u}, 5);
    append(tb, {0u, 0u}, 5);
    append(ta, {0u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Tod24bit::Tod24bit() {
    counter.setStore( 0 );
    counter.setText( "0" );
    counterAlarm.setStore( 0 );
    counterAlarm.setText( "0" );
    counter.setEditable( false );
    counterAlarm.setEditable( false );
    label.setText( "TOD:" );

    append(label, {50u, 0u}, 5);
    append(counter, {60u, 0u}, 10);
    append(labelAlarm, {0u, 0u}, 5);
    append(counterAlarm, {60u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Shifter::Shifter() {
    sdr.setStore( 0 );
    sdr.setText( "0" );
    shiftCount.setStore( 0 );
    shiftCount.setText( "0" );
    sdr.setEditable( false );
    shiftCount.setEditable( false );
    label.setText( "SDR:" );

    append(label, {50u, 0u}, 5);
    append(sdr, {40u, 0u}, 10);
    append(labelShiftCount, {0u, 0u}, 5);
    append(shiftCount, {40u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Chip(uint8_t chipNr, Debugger* debugger)
: portIO{ {chipNr, 0, debugger }, { chipNr, 1, debugger }},
    port{ {0}, { 1 }}, timer {{0}, { 1 }} {

    append( port[0], {0u, 0u}, 10 );
    append( portIO[0], {0u, 0u}, 10 );
    append( port[1], {0u, 0u}, 10 );
    append( portIO[1], {0u, 0u}, 10 );

    append( timer[0], {0u, 0u}, 10 );
    append( timer[1], {0u, 0u}, 10 );

    append( intData, {0u, 0u}, 10 );
    append( intMask, {0u, 0u}, 10 );

    append( tod24bit, {0u, 0u}, 10 );
    append( shifter, {0u, 0u} );

    setPadding( 10 );
}

CiaDebugger::CIA::CIA( Debugger* debugger )
: chip{{0, debugger }, { 1, debugger }} {
    chip[0].setText( "CIA A" );
    chip[1].setText( "CIA B" );

    append(chip[0], {~0u, 0u}, 10);
    append(chip[1], {~0u, 0u});
}

template<typename T> auto CiaDebugger::updateCia(T& s) -> void {
    for (int i = 0; i < 2; i++) {
        auto& c = cia->chip[i];
        auto& sc = s.cia[i];

        for (int j = 0; j < 2; j++) {
            auto& p = c.port[j];
            auto& sp = sc.port[j];
            updateReg( p.prVal, sp.pr );
            updateReg( p.ddrVal, sp.ddr );

            auto& io = c.portIO[j];
            int ioPos = io.line.size();
            auto& portIdents = T::CiaPorts[i][j];

            for (auto& l : io.line) {
                ioPos--;
                bool state = sp.io & (1 << ioPos);

                if ((bool)l->getStore() != state) {
                    l->setStore( state );
                    std::string _t{portIdents[(~ioPos) & 7].ident};

                    if (state) {
                        l->setEnabled(  );
                        l->setFont( GUIKIT::Font::system( "bold" ) );
                    } else {
                        l->setEnabled( false );
                        l->setFont( GUIKIT::Font::system( "" ) );
                        GUIKIT::String::toLowerCase( _t );
                    }

                    l->setText( _t );
                }
            }

            auto& t = c.timer[j];
            if (t.label.getStore() != sp.timerRunning) {
                if (sp.timerRunning)
                    t.label.setForegroundColor( DEBUG_COLOR );
                else
                    t.label.resetForegroundColor();
                t.label.setStore( sp.timerRunning );
            }

            updateReg( t.val, sp.timer );
            updateReg( t.latchVal, sp.timerLatch );

            if (t.oneShot.checked() != sp.oneshot)
                t.oneShot.setChecked( sp.oneshot );
            if (t.pbOut.checked() != sp.pbOut)
                t.pbOut.setChecked( sp.pbOut );
            if (t.toggleOut.checked() != sp.toggleOut)
                t.toggleOut.setChecked( sp.toggleOut );
        }

        auto& id = c.intData;
        updateReg( id.icrVal, sc.icr );

        if (id.ir.checked() != !!(sc.icr & 0x80) ) id.ir.setChecked( !!(sc.icr & 0x80) );
        if (id.flag.checked() != !!(sc.icr & 0x10) ) id.flag.setChecked( !!(sc.icr & 0x10) );
        if (id.sp.checked() != !!(sc.icr & 0x08) ) id.sp.setChecked( !!(sc.icr & 0x08) );
        if (id.alarm.checked() != !!(sc.icr & 0x04) ) id.alarm.setChecked( !!(sc.icr & 0x04) );
        if (id.tb.checked() != !!(sc.icr & 0x02) ) id.tb.setChecked( !!(sc.icr & 0x02) );
        if (id.ta.checked() != !!(sc.icr & 0x01) ) id.ta.setChecked( !!(sc.icr & 0x01) );

        auto& im = c.intMask;
        updateReg( im.maskVal, sc.icrMask );

        if (im.flag.checked() != !!(sc.icrMask & 0x10) ) im.flag.setChecked( !!(sc.icrMask & 0x10) );
        if (im.sp.checked() != !!(sc.icrMask & 0x08) ) im.sp.setChecked( !!(sc.icrMask & 0x08) );
        if (im.alarm.checked() != !!(sc.icrMask & 0x04) ) im.alarm.setChecked( !!(sc.icrMask & 0x04) );
        if (im.tb.checked() != !!(sc.icrMask & 0x02) ) im.tb.setChecked( !!(sc.icrMask & 0x02) );
        if (im.ta.checked() != !!(sc.icrMask & 0x01) ) im.ta.setChecked( !!(sc.icrMask & 0x01) );

        auto& tod = c.tod24bit;
        updateReg( tod.counter, sc.tod );
        updateReg( tod.counterAlarm, sc.todAlarm );

        auto& sh = c.shifter;
        updateReg( sh.sdr, sc.sdr );
        updateReg( sh.shiftCount, sc.shiftCount );
    }
    control->position.setText("V: " + hex( s.vPos, 3 ) + " H: " + hex( s.hPos, 2 ) );
}

auto CiaDebugger::buildTheme() -> GUIKIT::Layout* {
    cia = new CIA(this);

    control->remove( control->searchEdit );
    control->remove( control->search );

    return cia;
}

auto CiaDebugger::translateTheme() -> void {
    for (int i = 0; i < 2; i++) {
        auto& c = cia->chip[i];
        // for (int j = 0; j < 2; j++) {
        //     auto& t = c.timer[j];
        //     t.oneShot.setText( trans->getA( "oneshot" ) );
        //     t.toggleOut.setText( trans->getA( "toggle" ) );
        // }

        c.tod24bit.labelAlarm.setText( trans->getA( "Alarm" ) );
        c.shifter.labelShiftCount.setText( trans->getA( "Shifter" ) );
    }
}

auto CiaDebugger::updateTheme() -> void {
    bool locked = emuThread->lock();
    snapshot->theme = Emulator::Interface::DebuggerSnapshot::Theme::CIA;

    if (isAmiga()) {
        auto* amiEmu = dynamic_cast<LIBAMI::Interface*>(emulator);
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);

        amiEmu->getDebuggerSnapshot(snap);
        updateCia<LIBAMI::DebuggerSnapshot>( snap);
    } else {
        auto* c64Emu = dynamic_cast<LIBC64::Interface*>(emulator);
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);

        c64Emu->getDebuggerSnapshot(snap);
        updateCia<LIBC64::DebuggerSnapshot>( snap);
    }

    if (locked)
        emuThread->unlock();
}

auto CiaDebugger::screenIdent() -> std::string {
    return "debugger_cia";
}

auto CiaDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger CIA";
}
