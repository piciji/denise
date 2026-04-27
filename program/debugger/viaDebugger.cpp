
#include "viaDebugger.h"
#include "../thread/emuThread.h"
#include "../program.h"

ViaDebugger::ViaDebugger( Emulator::Interface* emulator )
: Debugger( emulator ) {
}

ViaDebugger::VIA::Chip::Port::Port() {
    prVal.setFont( GUIKIT::Font::monospace() );
    ddrVal.setFont( GUIKIT::Font::monospace() );
    prVal.setText("0");
    ddrVal.setText("0");
    prVal.setStore( 0 );
    ddrVal.setStore( 0 );
    prVal.setEditable( false );
    ddrVal.setEditable( false );
    useLatch.setReadonly();

    append(pr, {0u, 0u}, 5);
    append(prVal, {getWidth(2, true), 0u}, 10);
    append(ddr, {40u, 0u}, 5);
    append(ddrVal, {getWidth(2, true), 0u}, 10);
    append(useLatch, {0u, 0u});

    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::PortIO::PortIO(uint8_t chipNr, uint8_t portNr, Debugger* debugger) {
    portLabel.setFont( GUIKIT::Font::system( "bold" ) );

    append( portLabel, {0u, 0u}, 5 );
    const Emulator::Interface::DebuggerIdent* idents = &LIBC64::DebuggerSnapshot::ViaPorts[chipNr][portNr][0];

    for (unsigned i = 0; i < 8; i++) {
        const Emulator::Interface::DebuggerIdent& port = idents[i];
        auto* l = new GUIKIT::Label;
        l->setText( port.ident );
        l->setStore( 1 );
        l->setFont( GUIKIT::Font::system( "bold", true ) );
        l->setEnabled( true );
        line.push_back(l);
        append( *l, {l->minimumSize().width, 0u}, 10 );
    }
    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::Timer::Timer() {
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
    pb6Pulses.setReadonly();

    append(label, {0u, 0u}, 5);
    append(val, {getWidth(4, true), 0u}, 10);
    append(latch, {0u, 0u}, 5);
    append(latchVal, {getWidth(4, true), 0u}, 10);

    append(oneShot, {0u, 0u}, 5);
    append(pbOut, {0u, 0u}, 5);
    append(toggleOut, {0u, 0u}, 5);
    append(pb6Pulses, {0u, 0u});

    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::Intr::Intr(bool isMask) {
    val.setFont( GUIKIT::Font::monospace() );
    val.setEditable( false );

    val.setText( "0" );
    val.setStore( 0 );
    isMask ? ir.setEnabled( false ) : ir.setReadonly();
    cb1.setReadonly();
    cb2.setReadonly();
    shift.setReadonly();
    ca1.setReadonly();
    ca2.setReadonly();
    ta.setReadonly();
    tb.setReadonly();

    append(label, {0u, 0u}, 5);
    append(val, {getWidth(2, true), 0u}, 10);
    append(ir, {0u, 0u}, 5);
    append(ta, {0u, 0u}, 5);
    append(tb, {0u, 0u}, 5);
    append(cb1, {0u, 0u}, 5);
    append(cb2, {0u, 0u}, 5);
    append(shift, {0u, 0u}, 5);
    append(ca1, {0u, 0u}, 5);
    append(ca2, {0u, 0u});

    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::CX1::CX1() {
    checkPositiveA.setReadonly( );
    checkPositiveB.setReadonly();

    append( labelA, {0u, 0u}, 5 );
    append( checkPositiveA, {0u, 0u}, 15 );
    append( labelB, {0u, 0u}, 5 );
    append( checkPositiveB, {0u, 0u} );

    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::CX2Out::CX2Out() {
    checkOutput.setReadonly(  );
    radioHandshake.setReadonly(  );
    radioPulse.setReadonly(  );
    radioLow.setReadonly(  );
    radioHigh.setReadonly(  );

    append( label, {0u, 0u}, 5 );
    append( checkOutput, {0u, 0u}, 5 );
    append( radioHandshake, {0u, 0u}, 5 );
    append( radioPulse, {0u, 0u}, 5 );
    append( radioLow, {0u, 0u}, 5 );
    append( radioHigh, {0u, 0u}, 5 );

    GUIKIT::RadioBox::setGroup( radioHandshake, radioPulse, radioLow, radioHigh );

    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::CX2In::CX2In() {
    checkInput.setReadonly(  );
    checkPositive.setReadonly(  );
    checkIndependent.setReadonly(  );

    append( label, {0u, 0u}, 5 );
    append( checkInput, {0u, 0u}, 5 );
    append( checkPositive, {0u, 0u}, 5 );
    append( checkIndependent, {0u, 0u} );

    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::Shifter::Shifter() {
    editSdr.setFont( GUIKIT::Font::monospace() );
    editShiftCount.setFont( GUIKIT::Font::monospace() );
    editSdr.setEditable( false );
    editShiftCount.setEditable( false );
    radioDisabled.setReadonly(  );
    radioTimer2.setReadonly(  );
    radioPhi2.setReadonly(  );
    radioExt.setReadonly(  );

    append(label, {0u, 0u}, 5);
    append(editSdr, {getWidth(2, true), 0u}, 10);
    append(labelShiftCount, {0u, 0u}, 5);
    append(editShiftCount, {getWidth(2, true), 0u}, 10);
    append( radioDisabled, {0u, 0u}, 5 );
    append( radioTimer2, {0u, 0u}, 5 );
    append( radioPhi2, {0u, 0u}, 5 );
    append( radioExt, {0u, 0u} );

    GUIKIT::RadioBox::setGroup( radioDisabled, radioTimer2, radioPhi2, radioExt );

    setAlignment( 0.5 );
}

ViaDebugger::VIA::Chip::Chip(uint8_t chipNr, Debugger* debugger)
: portIO{ {chipNr, 0, debugger }, { chipNr, 1, debugger }},
    ifr(false), ier( true ) {

    append( port[0], {0u, 0u}, 10 );
    append( portIO[0], {0u, 0u}, 10 );
    append( port[1], {0u, 0u}, 10 );
    append( portIO[1], {0u, 0u}, 10 );

    append( timer[0], {0u, 0u}, 10 );
    append( timer[1], {0u, 0u}, 10 );

    append( ifr, {0u, 0u}, 10 );
    append( ier, {0u, 0u}, 10 );

    append( cx1, {0u, 0u}, 10 );
    append( ca2Out, {0u, 0u}, 5 );
    append( cb2Out, {0u, 0u}, 10 );
    append( ca2In, {0u, 0u}, 5 );
    append( cb2In, {0u, 0u}, 10 );
    append( shifter, {0u, 0u} );

    setPadding( 10 );
}

ViaDebugger::VIA::VIA( Debugger* debugger )
: chip{{0, debugger }, { 1, debugger }} {

    append(chip[0], {~0u, 0u}, 10);
    append(chip[1], {~0u, 0u});
}

template<typename T> auto ViaDebugger::updateVia(T& s) -> void {
    for (int i = 0; i < 2; i++) {
        auto& c = via->chip[i];
        auto& sc = s.via[i];

        for (int j = 0; j < 2; j++) {
            auto& p = c.port[j];
            auto& sp = sc.port[j];
            updateReg(p.prVal, sp.pr );
            updateReg(p.ddrVal, sp.ddr );
            updateReg(p.useLatch, sc.acr & (1 << j));

            auto& io = c.portIO[j];
            int ioPos = io.line.size();
            auto& portIdents = T::ViaPorts[i][j];

            for (auto& l : io.line) {
                ioPos--;
                bool state = sp.io & (1 << ioPos);

                if ((bool)l->getStore() != state) {
                    l->setStore( state );
                    std::string _t{portIdents[(~ioPos) & 7].ident};

                    if (state) {
                        l->setEnabled(  );
                        l->setFont( GUIKIT::Font::system( "bold", true ) );
                    } else {
                        l->setEnabled( false );
                        l->setFont( GUIKIT::Font::system( "", true ) );
                        GUIKIT::String::toLowerCase( _t );
                    }

                    l->setText( _t );
                }
            }

            auto& t = c.timer[j];

            updateReg( t.val, sp.timer );
            updateReg( t.latchVal, sp.timerLatch );

            if (j == 0) {
                updateReg(t.oneShot, !(sc.acr & 0x40));
                if (t.toggleOut.enabled())
                    t.toggleOut.setEnabled( false );
                if (t.pbOut.enabled())
                    t.pbOut.setEnabled( false );
                if (t.pb6Pulses.enabled())
                    t.pb6Pulses.setEnabled( false );

            } else {
                updateReg(t.oneShot, true);
                updateReg(t.pbOut, sc.acr & 0x80);
                updateReg(t.toggleOut, sp.toggleOut);
                updateReg(t.pb6Pulses, sc.acr & 0x20);
            }
        }

        auto& id = c.ifr;
        updateReg( id.val, sc.ifr );

        updateReg(id.ir,!!(sc.ifr & 0x80) );
        updateReg(id.ta,!!(sc.ifr & 0x40) );
        updateReg(id.tb,!!(sc.ifr & 0x20) );
        updateReg(id.cb1,!!(sc.ifr & 0x10) );
        updateReg(id.cb2,!!(sc.ifr & 0x08) );
        updateReg(id.shift,!!(sc.ifr & 0x04) );
        updateReg(id.ca1,!!(sc.ifr & 0x02) );
        updateReg(id.ca2,!!(sc.ifr & 0x01) );

        auto& im = c.ier;
        updateReg( im.val, sc.ier );
        updateReg(im.ta,!!(sc.ier & 0x40) );
        updateReg(im.tb,!!(sc.ier & 0x20) );
        updateReg(im.cb1,!!(sc.ier & 0x10) );
        updateReg(im.cb2,!!(sc.ier & 0x08) );
        updateReg(im.shift,!!(sc.ier & 0x04) );
        updateReg(im.ca1,!!(sc.ier & 0x02) );
        updateReg(im.ca2,!!(sc.ier & 0x01) );

        auto& cx1 = c.cx1;
        updateReg(cx1.checkPositiveA, sc.pcr & 1);
        updateReg(cx1.checkPositiveB, sc.pcr & 0x10);

        auto& ca2Out = c.ca2Out;
        auto& cb2Out = c.cb2Out;
        auto& ca2In = c.ca2In;
        auto& cb2In = c.cb2In;
        bool _out = (sc.pcr & 8) != 0;
        unsigned _mode;

        updateReg( ca2Out.checkOutput, _out );
        updateReg( ca2In.checkInput, !_out );

        if (_out) {
            _mode = (sc.pcr >> 1) & 3;
            switch (_mode) {
                default:
                case 0: updateReg( ca2Out.radioHandshake ); break;
                case 1: updateReg( ca2Out.radioPulse ); break;
                case 2: updateReg( ca2Out.radioLow ); break;
                case 3: updateReg( ca2Out.radioHigh ); break;
            }
        } else {
            updateReg( ca2In.checkPositive, sc.pcr & 4 );
            updateReg( ca2In.checkIndependent, sc.pcr & 2 );
        }

        if (ca2Out.checkOutput.enabled() != _out) ca2Out.checkOutput.setEnabled( _out );
        if (ca2Out.radioHandshake.enabled() != _out) ca2Out.radioHandshake.setEnabled( _out );
        if (ca2Out.radioPulse.enabled() != _out) ca2Out.radioPulse.setEnabled( _out );
        if (ca2Out.radioLow.enabled() != _out) ca2Out.radioLow.setEnabled( _out );
        if (ca2Out.radioHigh.enabled() != _out) ca2Out.radioHigh.setEnabled( _out );

        if (ca2In.checkInput.enabled() != !_out) ca2In.checkInput.setEnabled( !_out );
        if (ca2In.checkPositive.enabled() != !_out) ca2In.checkPositive.setEnabled( !_out );
        if (ca2In.checkIndependent.enabled() != !_out) ca2In.checkIndependent.setEnabled( !_out );

        _out = (sc.pcr & 0x80) != 0;
        updateReg( cb2Out.checkOutput, _out );
        updateReg( cb2In.checkInput, !_out );

        if (_out) {
            _mode = (sc.pcr >> 5) & 3;
            switch (_mode) {
                default:
                case 0: updateReg( cb2Out.radioHandshake ); break;
                case 1: updateReg( cb2Out.radioPulse ); break;
                case 2: updateReg( cb2Out.radioLow ); break;
                case 3: updateReg( cb2Out.radioHigh ); break;
            }
        } else {
            updateReg( cb2In.checkPositive, sc.pcr & 0x40 );
            updateReg( cb2In.checkIndependent, sc.pcr & 0x20 );
        }

        if (cb2Out.checkOutput.enabled() != _out) cb2Out.checkOutput.setEnabled( _out );
        if (cb2Out.radioHandshake.enabled() != _out) cb2Out.radioHandshake.setEnabled( _out );
        if (cb2Out.radioPulse.enabled() != _out) cb2Out.radioPulse.setEnabled( _out );
        if (cb2Out.radioLow.enabled() != _out) cb2Out.radioLow.setEnabled( _out );
        if (cb2Out.radioHigh.enabled() != _out) cb2Out.radioHigh.setEnabled( _out );

        if (cb2In.checkInput.enabled() != !_out) cb2In.checkInput.setEnabled( !_out );
        if (cb2In.checkPositive.enabled() != !_out) cb2In.checkPositive.setEnabled( !_out );
        if (cb2In.checkIndependent.enabled() != !_out) cb2In.checkIndependent.setEnabled( !_out );


        auto& sh = c.shifter;
        updateReg( sh.editSdr, sc.sdr );
        updateReg( sh.editShiftCount, sc.shiftCount );
        updateReg( sh.checkOutput, sc.acr & 0x10 );

        _mode = (sc.acr >> 2) & 3;
        switch (_mode) {
            default:
            case 0: updateReg( sh.radioDisabled ); break;
            case 1: updateReg( sh.radioTimer2 ); break;
            case 2: updateReg( sh.radioPhi2 ); break;
            case 3: updateReg( sh.radioExt ); break;
        }
    }

    updateControl( s.vPos, s.hPos );
}

auto ViaDebugger::buildTheme() -> GUIKIT::Layout* {
    via = new VIA(this);

    return via;
}

auto ViaDebugger::translateTheme() -> void {
    for (int i = 0; i < 2; i++) {
        auto& c = via->chip[i];
        c.setText( i == 0 ? "VIA A" : "VIA B" );

        for (int j = 0; j < 2; j++) {
            auto& p = c.port[j];
            p.pr.setText( j == 0 ? "PRA:" : "PRB:" );
            p.ddr.setText( j == 0 ? "DDRA" : "DDRB" );
            p.useLatch.setText( "Use Latch" );


            auto& pio = c.portIO[j];
            pio.portLabel.setText( j == 0 ? "Port A:" : "Port B:" );

            auto& t = c.timer[j];
            t.label.setText( j == 0 ? "Timer 1:" : "Timer 2:" );
            t.latch.setText( "Latch" );

            t.oneShot.setText( "oneshot" );
            t.toggleOut.setText("toggle" );
            t.pbOut.setText( "PB6" );
            t.pb6Pulses.setText( "PB6Pulse" );
        }

        auto& ier = c.ier;
        ier.label.setText( "IER:" );
        ier.ir.setText( "IR" );
        ier.cb1.setText( "CB1" );
        ier.cb2.setText( "CB2" );
        ier.shift.setText( "Shift" );
        ier.ca1.setText( "CA1" );
        ier.ca2.setText( "CA2" );
        ier.tb.setText( "TB" );
        ier.ta.setText( "TA" );

        auto& ifr = c.ifr;
        ifr.label.setText( "IFR:" );
        ifr.ir.setText( "IR" );
        ifr.cb1.setText( "CB1" );
        ifr.cb2.setText( "CB2" );
        ifr.shift.setText( "Shift" );
        ifr.ca1.setText( "CA1" );
        ifr.ca2.setText( "CA2" );
        ifr.tb.setText( "TB" );
        ifr.ta.setText( "TA" );

        auto& cx1 = c.cx1;
        cx1.labelA.setText( "CA1" );
        cx1.checkPositiveA.setText( "Positive" );
        cx1.labelB.setText( "CB1" );
        cx1.checkPositiveB.setText( "Positive" );

        auto& ca2Out = c.ca2Out;
        ca2Out.label.setText( "CA2" );
        ca2Out.checkOutput.setText( "Output" );
        ca2Out.radioHandshake.setText( "Handshake" );
        ca2Out.radioPulse.setText( "Pulse" );
        ca2Out.radioLow.setText( "Low" );
        ca2Out.radioHigh.setText( "High" );

        auto& cb2Out = c.cb2Out;
        cb2Out.label.setText( "CB2" );
        cb2Out.checkOutput.setText( "Output" );
        cb2Out.radioHandshake.setText( "Handshake" );
        cb2Out.radioPulse.setText( "Pulse" );
        cb2Out.radioLow.setText( "Low" );
        cb2Out.radioHigh.setText( "High" );

        auto& ca2In = c.ca2In;
        ca2In.label.setText( "CA2" );
        ca2In.checkInput.setText( "Input" );
        ca2In.checkPositive.setText( "Positive" );
        ca2In.checkIndependent.setText( "Independent" );

        auto& cb2In = c.cb2In;
        cb2In.label.setText( "CB2" );
        cb2In.checkInput.setText( "Input" );
        cb2In.checkPositive.setText( "Positive" );
        cb2In.checkIndependent.setText( "Independent" );

        c.shifter.label.setText( "SDR:" );
        c.shifter.labelShiftCount.setText("Shifter" );
        c.shifter.checkOutput.setText("Output");
        c.shifter.radioDisabled.setText("Free/Off");
        c.shifter.radioTimer2.setText("T2");
        c.shifter.radioPhi2.setText("PHI2");
        c.shifter.radioExt.setText("EXT");

        GUIKIT::Layout::alignChildWidth( {&c.port[0], &c.port[1], &c.portIO[0], &c.portIO[1], &c.timer[0], &c.timer[1], &c.ifr, &c.ier, &c.cx1, &c.ca2Out, &c.cb2Out, &c.ca2In, &c.cb2In, &c.shifter} );
    }
}

auto ViaDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
    updateVia<LIBC64::DebuggerSnapshot>( snap);
}

auto ViaDebugger::initTheme() -> void {
    emulator->debuggerAdd( getTheme(), DebuggerAction::None, 0 );
}

auto ViaDebugger::closeTheme() -> void {
    emulator->debuggerRemove( getTheme(), DebuggerAction::None );
}

auto ViaDebugger::saveIdent() -> std::string {
    return "debugger_via";
}

auto ViaDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger VIA";
}
