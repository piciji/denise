
#include "ciaDebugger.h"
#include "../thread/emuThread.h"
#include "../program.h"

CiaDebugger::CiaDebugger( Emulator::Interface* emulator, Mode mode )
: Debugger( emulator, mode ) {
    build();
}

CiaDebugger::CIA::Chip::Port::Port() {
    prVal.setFont( GUIKIT::Font::monospace() );
    ddrVal.setFont( GUIKIT::Font::monospace() );
    prVal.setText("0");
    ddrVal.setText("0");
    prVal.setStore( 0 );
    ddrVal.setStore( 0 );
    prVal.setEditable( false );
    ddrVal.setEditable( false );

    append(pr, {50u, 0u}, 5);
    append(prVal, {getWidth(2, true), 0u}, 10);
    append(ddr, {40u, 0u}, 5);
    append(ddrVal, {getWidth(2, true), 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::PortIO::PortIO(uint8_t chipNr, uint8_t portNr, Debugger* debugger) {
    portLabel.setFont( GUIKIT::Font::system( "bold" ) );

    append( portLabel, {50u, 0u}, 5 );
    const Emulator::Interface::DebuggerIdent* idents = debugger->isAmiga() ? &LIBAMI::DebuggerSnapshot::CiaPorts[chipNr][portNr][0]
    : &LIBC64::DebuggerSnapshot::CiaPorts[chipNr][portNr][0];

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

CiaDebugger::CIA::Chip::Timer::Timer() {
    val.setFont( GUIKIT::Font::monospace() );
    latchVal.setFont( GUIKIT::Font::monospace() );

    val.setText("0");
    latchVal.setText("0");
    val.setStore( 0 );
    latchVal.setStore( 0 );
    val.setEditable( false );
    latchVal.setEditable( false );

    oneShot.setReadonly();
    pbOut.setReadonly();
    toggleOut.setReadonly();

    append(label, {50u, 0u}, 5);
    append(val, {getWidth(4, true), 0u}, 10);
    append(latch, {0u, 0u}, 5);
    append(latchVal, {getWidth(4, true), 0u}, 10);

    append(oneShot, {0u, 0u}, 5);
    append(pbOut, {0u, 0u}, 5);
    append(toggleOut, {0u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Intr::Intr(bool isMask) {
    val.setFont( GUIKIT::Font::monospace() );
    val.setEditable( false );

    val.setText( "0" );
    val.setStore( 0 );
    isMask ? ir.setEnabled( false ) : ir.setReadonly();
    flag.setReadonly();
    sp.setReadonly();
    alarm.setReadonly();
    ta.setReadonly();
    tb.setReadonly();

    append(label, {50u, 0u}, 5);
    append(val, {getWidth(2, true), 0u}, 10);
    append(ir, {0u, 0u}, 5);
    append(flag, {0u, 0u}, 5);
    append(sp, {0u, 0u}, 5);
    append(alarm, {0u, 0u}, 5);
    append(tb, {0u, 0u}, 5);
    append(ta, {0u, 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Tod24bit::Tod24bit(Debugger* debugger) {
    counter.setFont( GUIKIT::Font::monospace() );
    counterAlarm.setFont( GUIKIT::Font::monospace() );

    counter.setStore( 0 );
    counter.setText( "0" );
    counterAlarm.setStore( 0 );
    counterAlarm.setText( "0" );
    counter.setEditable( false );
    counterAlarm.setEditable( false );

    append(label, {50u, 0u}, 5);
    append(counter, {getWidth( debugger->isC64() ? 8 : 6, true), 0u}, 10);
    append(labelAlarm, {0u, 0u}, 5);
    append(counterAlarm, {getWidth(debugger->isC64() ? 8 : 6, true), 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Shifter::Shifter() {
    sdr.setFont( GUIKIT::Font::monospace() );
    shiftCount.setFont( GUIKIT::Font::monospace() );
    sdr.setStore( 0 );
    sdr.setText( "0" );
    shiftCount.setStore( 0 );
    shiftCount.setText( "0" );
    sdr.setEditable( false );
    shiftCount.setEditable( false );

    append(label, {50u, 0u}, 5);
    append(sdr, {getWidth(2, true), 0u}, 10);
    append(labelShiftCount, {0u, 0u}, 5);
    append(shiftCount, {getWidth(2, true), 0u});

    setAlignment( 0.5 );
}

CiaDebugger::CIA::Chip::Chip(uint8_t chipNr, Debugger* debugger)
: portIO{ {chipNr, 0, debugger }, { chipNr, 1, debugger }},
    icr(false), icrMask( true ), tod24bit( debugger ) {

    append( port[0], {0u, 0u}, 10 );
    append( portIO[0], {0u, 0u}, 10 );
    append( port[1], {0u, 0u}, 10 );
    append( portIO[1], {0u, 0u}, 10 );

    append( timer[0], {0u, 0u}, 10 );
    append( timer[1], {0u, 0u}, 10 );

    append( icr, {0u, 0u}, 10 );
    append( icrMask, {0u, 0u}, 10 );

    append( tod24bit, {0u, 0u}, 10 );
    append( shifter, {0u, 0u} );

    setPadding( 10 );
}

CiaDebugger::CIA::CIA( Debugger* debugger )
: chip{{0, debugger }, { 1, debugger }} {

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

            updateReg(t.oneShot, sp.oneshot);
            updateReg(t.pbOut, sp.pbOut);
            updateReg(t.toggleOut, sp.toggleOut);
        }

        auto& id = c.icr;
        updateReg( id.val, sc.icr );

        updateReg(id.ir,!!(sc.icr & 0x80) );
        updateReg(id.flag,!!(sc.icr & 0x10) );
        updateReg(id.sp,!!(sc.icr & 0x08) );
        updateReg(id.alarm,!!(sc.icr & 0x04) );
        updateReg(id.tb,!!(sc.icr & 0x02) );
        updateReg(id.ta,!!(sc.icr & 0x01) );

        auto& im = c.icrMask;
        updateReg( im.val, sc.icrMask );

        updateReg(im.flag,!!(sc.icrMask & 0x10) );
        updateReg(im.sp,!!(sc.icrMask & 0x08) );
        updateReg(im.alarm,!!(sc.icrMask & 0x04) );
        updateReg(im.tb,!!(sc.icrMask & 0x02) );
        updateReg(im.ta,!!(sc.icrMask & 0x01) );

        auto& tod = c.tod24bit;
        updateReg( tod.counter, sc.tod );
        updateReg( tod.counterAlarm, sc.todAlarm );

        auto& sh = c.shifter;
        updateReg( sh.sdr, sc.sdr );
        updateReg( sh.shiftCount, sc.shiftCount );
    }

    updateControl( s.vPos, s.hPos );
}

auto CiaDebugger::buildTheme() -> GUIKIT::Layout* {
    cia = new CIA(this);

    return cia;
}

auto CiaDebugger::translateTheme() -> void {
    for (int i = 0; i < 2; i++) {
        auto& c = cia->chip[i];
        c.setText( i == 0 ? "CIA A" : "CIA B" );

        for (int j = 0; j < 2; j++) {
            auto& p = c.port[j];
            p.pr.setText( j == 0 ? "PRA:" : "PRB:" );
            p.ddr.setText( j == 0 ? "DDRA" : "DDRB" );

            auto& pio = c.portIO[j];
            pio.portLabel.setText( j == 0 ? "Port A:" : "Port B:" );

            auto& t = c.timer[j];
            t.label.setText( j == 0 ? "Timer A:" : "Timer B:" );
            t.latch.setText( "Latch" );

            t.oneShot.setText( "oneshot" );
            t.toggleOut.setText("toggle" );
            t.pbOut.setText( j == 0 ? "PB6" : "PB7" );
        }

        auto& icrMask = c.icrMask;
        icrMask.label.setText( "Mask:" );
        icrMask.ir.setText( "IR" );
        icrMask.flag.setText( "Flag" );
        icrMask.sp.setText( "SP" );
        icrMask.alarm.setText( "Alarm" );
        icrMask.tb.setText( "TB" );
        icrMask.ta.setText( "TA" );

        auto& icr = c.icr;
        icr.label.setText( "ICR:" );
        icr.ir.setText( "IR" );
        icr.flag.setText( "Flag" );
        icr.sp.setText( "SP" );
        icr.alarm.setText( "Alarm" );
        icr.tb.setText( "TB" );
        icr.ta.setText( "TA" );

        c.tod24bit.label.setText("TOD:" );
        c.tod24bit.labelAlarm.setText("Alarm" );
        c.shifter.label.setText( "SDR:" );
        c.shifter.labelShiftCount.setText("Shifter" );
    }
}

auto CiaDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isAmiga()) {
        LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
        updateCia<LIBAMI::DebuggerSnapshot>( snap);
    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
        updateCia<LIBC64::DebuggerSnapshot>( snap);
    }
}

auto CiaDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::CIA, DebuggerAction::None, 0 );
}

auto CiaDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::CIA, DebuggerAction::None );
}

auto CiaDebugger::saveIdent() -> std::string {
    return "debugger_cia";
}

auto CiaDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger CIA";
}
