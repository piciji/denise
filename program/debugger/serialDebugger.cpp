
#include "serialDebugger.h"
#include "../../emulation/libami/system/debuggerSnapshot.h"

SerialDebugger::Serial::Top::Uart::Transmit::Transmit() {
    edit.setEditable( false );
    editR.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace( ) );
    editR.setFont( GUIKIT::Font::monospace( ) );
    editB.setFont( GUIKIT::Font::monospace( ) );
    label.setAlign( GUIKIT::Label::Align::Right );

    append( label, {0u, 0u}, 10 );
    append( edit, {getWidth( 4, true ), 0u}, 10 );
    append( imageView, {0u, 0u}, 10 );
    append( labelR, {0u, 0u}, 10 );
    append( editR, {getWidth( 4, true ), 0u}, 10 );
    append( labelB, {0u, 0u}, 10 );
    append( editB, {getWidth( 6, true ), 0u} );

    setAlignment( 0.5 );
}

SerialDebugger::Serial::Top::Uart::Receive::Receive() {
    edit.setEditable( false );
    editR.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace( ) );
    editR.setFont( GUIKIT::Font::monospace( ) );
    label.setAlign( GUIKIT::Label::Align::Right );
    radio8Bit.setReadonly(  );
    radio9Bit.setReadonly(  );

    append( label, {0u, 0u}, 10 );
    append( edit, {getWidth( 4, true ), 0u}, 10 );
    append( imageView, {0u, 0u}, 10 );
    append( labelR, {0u, 0u}, 10 );
    append( editR, {getWidth( 4, true ), 0u}, 10 );
    append( radio8Bit, {0u, 0u}, 10 );
    append( radio9Bit, {0u, 0u}, 10 );

    GUIKIT::RadioBox::setGroup( radio8Bit, radio9Bit );

    setAlignment( 0.5 );
}

SerialDebugger::Serial::Top::Uart::SerdatR::SerdatR() {
    edit.setEditable( false );
    edit.setFont( GUIKIT::Font::monospace( ) );
    overrun.setReadonly(  );
    rbf.setReadonly(  );
    tbe.setReadonly(  );
    tsre.setReadonly(  );
    rxd.setReadonly(  );
    label.setAlign( GUIKIT::Label::Align::Right );

    append( label, {0u, 0u}, 10 );
    append( edit, {getWidth( 4, true ), 0u}, 10 );
    append( overrun, {0u, 0u}, 10 );
    append( rbf, {0u, 0u}, 10 );
    append( tbe, {0u, 0u}, 10 );
    append( tsre, {0u, 0u}, 10 );
    append( rxd, {0u, 0u}, 10 );

    setAlignment( 0.5 );
}

SerialDebugger::Serial::Top::Uart::Uart() {
    append( transmit, {0u, 0u}, 10 );
    append( receive, {0u, 0u}, 10 );
    append( serdatR, {0u, 0u} );

    setPadding( 10 );
}

SerialDebugger::Serial::Top::Pins::Flags::Flags() {
    check1.setReadonly(  );
    check2.setReadonly(  );
    check3.setReadonly(  );
    check4.setReadonly(  );

    append(check1, {0u, 0u}, 10 );
    append(check2, {0u, 0u}, 10 );
    append(check3, {0u, 0u}, 10 );
    append(check4, {0u, 0u} );

    setAlignment( 0.5 );
}

SerialDebugger::Serial::Top::Pins::Pins() {
    append( line1, {0u, 0u}, 10 );
    append( line2, {0u, 0u} );

    setPadding( 10 );
}

SerialDebugger::Serial::Top::Top() {
    append( uart, {0u, 0u}, 15 );
    append( pins, {0u, 0u} );
}

SerialDebugger::Serial::Bottom::Data::Data() {
    edit.setEditable( false );
    edit.scrollToEndWhenUpdating();
    edit.setFont( GUIKIT::Font::monospace( ) );

    append( edit, {~0u, ~0u} );

    setPadding( 10 );
}

SerialDebugger::Serial::Bottom::Bottom() {
    append( outgoing, {~0u, ~0u}, 10 );
    append( incoming, {~0u, ~0u} );
}

SerialDebugger::Serial::Serial() {
    append( top, {0u, 0u}, 10 );
    append( bottom, {~0u, ~0u} );
}

SerialDebugger::SerialDebugger( Emulator::Interface* emulator )
: Debugger( emulator ) {
}

auto SerialDebugger::buildTheme() -> GUIKIT::Layout* {
    serial = new Serial( );
    serial->top.uart.transmit.imageView.setImage( &arrowRightImg );
    serial->top.uart.receive.imageView.setImage( &arrowLeftImg );

    return serial;
}

auto SerialDebugger::updateTheme() -> void {
    LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    auto& s = snap.serial;

    auto& transmit = serial->top.uart.transmit;
    updateReg( transmit.edit, s.transmit );
    updateReg( transmit.editR, s.transmitShifter );
    updateRegDec( transmit.editB, s.baudRate );

    auto& receive = serial->top.uart.receive;
    updateReg( receive.edit, s.serDatR & 0x3ff );
    updateReg( receive.editR, s.receiveShifter );
    s.LONG ? updateReg( receive.radio9Bit ) : updateReg( receive.radio8Bit );

    auto& serDatR = serial->top.uart.serdatR;
    updateReg(serDatR.edit, s.serDatR);
    updateReg( serDatR.overrun, s.serDatR & 0x8000 );
    updateReg( serDatR.rbf, s.serDatR & 0x4000 );
    updateReg( serDatR.tbe, s.serDatR & 0x2000 );
    updateReg( serDatR.tsre, s.serDatR & 0x1000 );
    updateReg( serDatR.rxd, s.serDatR & 0x800 );

    auto& pins = serial->top.pins;
    updateReg( pins.line1.check1, s.port & (1 << 2) );
    updateReg( pins.line1.check2, s.port & (1 << 3) );
    updateReg( pins.line1.check3, s.port & (1 << 4) );
    updateReg( pins.line1.check4, s.port & (1 << 5) );

    updateReg( pins.line2.check1, s.port & (1 << 6) );
    updateReg( pins.line2.check2, s.port & (1 << 8) );
    updateReg( pins.line2.check3, s.port & (1 << 20) );
    updateReg( pins.line2.check4, s.port & (1 << 22) );

    auto& incoming = serial->bottom.incoming;
    if (!GUIKIT::String::equalFromEnd(s.incoming, incoming.edit.textRef())) {
        incoming.edit.setText( s.incoming );
    }

    auto& outgoing = serial->bottom.outgoing;
    if (!GUIKIT::String::equalFromEnd(s.outgoing, outgoing.edit.textRef())) {
        outgoing.edit.setText( s.outgoing );
    }

    updateControl( snap.vPos, snap.hPos );
}

auto SerialDebugger::translateTheme() -> void {
    auto& uart = serial->top.uart;
    uart.setText( "UART" );

    auto& transmit = uart.transmit;
    transmit.label.setText( "Transmit" );
    transmit.labelR.setText( "Shift Register" );
    transmit.labelB.setText( "Baud Rate" );

    auto& receive = uart.receive;
    receive.label.setText( "Receive" );
    receive.labelR.setText( "Shift Register" );
    receive.radio8Bit.setText( "8 Bit" );
    receive.radio9Bit.setText( "9 Bit" );

    auto& serdatR = uart.serdatR;
    serdatR.label.setText( "SERDATR" );
    serdatR.overrun.setText( "OVERRUN" );
    serdatR.rbf.setText( "RBF" );
    serdatR.tbe.setText( "TBE" );
    serdatR.tsre.setText( "TSRE" );
    serdatR.rxd.setText( "RXD" );

    auto& pins = serial->top.pins;
    pins.setText( "Serial Pins" );

    pins.line1.check1.setText( "TXD" );
    pins.line1.check2.setText( "RXD" );
    pins.line1.check3.setText( "RTS" );
    pins.line1.check4.setText( "CTS" );

    pins.line2.check1.setText( "DSR" );
    pins.line2.check2.setText( "CD" );
    pins.line2.check3.setText( "DTR" );
    pins.line2.check4.setText( "RI" );

    auto& outgoing = serial->bottom.outgoing;
    outgoing.setText( "Outgoing" );

    auto& incoming = serial->bottom.incoming;
    incoming.setText( "Incoming" );

    std::vector<GUIKIT::Layout*> entries;
    entries.push_back( &transmit );
    entries.push_back( &receive );
    entries.push_back( &serdatR );
    GUIKIT::Layout::alignChildWidth( entries );

    entries.clear();
    entries.push_back( &pins.line1 );
    entries.push_back( &pins.line2 );
    GUIKIT::Layout::alignChildWidth( entries, 0 );
    GUIKIT::Layout::alignChildWidth( entries, 1 );
    GUIKIT::Layout::alignChildWidth( entries, 2 );
}

auto SerialDebugger::initTheme() -> void {
    emulator->debuggerAdd( getTheme(), DebuggerAction::None, 0);
}

auto SerialDebugger::closeTheme() -> void {
    emulator->debuggerRemove( getTheme(), DebuggerAction::None);
}

auto SerialDebugger::saveIdent() -> std::string {
    return "debugger_serial";
}

auto SerialDebugger::titleIdent() -> std::string {
    return emulator->ident + " Debugger Serial Port";
}
