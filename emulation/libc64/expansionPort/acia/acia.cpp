
#include "../../../tools/systimer.h"
#include "acia.h"

namespace LIBC64 {

Acia* acia = nullptr;

const double Acia::bpsLookup[16] = {
    10, 50, 75, 109.92, 134.58, 150, 300, 600, 1200, 1800,
    2400, 3600, 4800, 7200, 9600, 19200
};

const double Acia::t232BpsLookup[4] = {
    230400, 115200, 57600, 28800
};

Acia::~Acia() {
    socket.clean();
}

Acia::Acia() : ExpansionPort() {

    setId( Interface::ExpansionIdRS232 );
    useNmi = true;
    useIrq = false;
    useDE00 = true;
    ip232 = true;
    cartridgeId = Interface::CartridgeIdDefault;
    media = nullptr;

    receiver = [this] () {

        uint8_t receiveByte;
        if (!transmitterContinous() && socket.connected()) {

            if (receiveData( &receiveByte ) ) {
                system->interface->log("receive ok", 1);

                if (!(status & Status::ReceiveDataFull)) {
                    rxData = receiveByte;
                    status |= Status::ReceiveDataFull;
                    handleReceiverInterrupt();
                } else
                    status |= Status::OverrunError;
            }
        }

        if (command & 1)
            sysTimer.add( &receiver, bitCyclesReceive );
    };

    transmitter = [this]() {

        if (transmitterContinous()) // continous
            redoTx = 2;

        if ( redoTx == 2 && socket.connected()) {
            system->interface->log("transmit", 1);

            if (!sendData()) {
                system->interface->log("error", 0);
                system->interface->log(socket.getLastError(), 0);
            } else
                system->interface->log("ok", 0);

            status |= Status::TransmitterEmpty;

            if (transmitterIntEnabled()) {
                setInt(true);
                status |= Status::Interrupt;
            }
        }

        if (redoTx)
            redoTx--;

        if (redoTx)
            sysTimer.add( &transmitter, bitCycles );
    };

    sysTimer.registerCallback( {{&receiver, 1}, {&transmitter, 1}} );
}

auto Acia::reset(bool softReset) -> void {
    system->interface->log("reset",1);
    socket.disconnect();
    status = Status::TransmitterEmpty | Status::DataCarrierDetect | Status::DataSetReady;
    control = 0;
    command = 0;
    enhancedControl = 0;
    bitCycles = 0;
    bitCyclesReceive = 0;
    dsr = true;
    dcd = true;
    dtr = false;
    redoTx = 0;
    rxData = 0;
    txData = 0;
    dsrLock = dcdLock = false;
    updateBaudGenerator();
}

auto Acia::updateBaudGenerator() -> void {

    uint8_t bits = 2; // start bit + stop bit
    bool parityBit = command & 0x20;
    bool secondStopBit = control & 0x80;
    bool useHalfBit = false;

    if (parityBit)
        bits++;

    switch( control & 0x60 ) {
        default:
        case 0:
            bits += 8;
            if (secondStopBit && !parityBit)
                bits++;
            break;
        case 0x20:
            bits += 7;
            if (secondStopBit)
                bits++;
            break;
        case 0x40:
            bits += 6;
            if (secondStopBit)
                bits++;
            break;
        case 0x60:
            bits += 5;
            if (secondStopBit) {
                if (parityBit)
                    bits++;
                else
                    useHalfBit = true;
            }
            break;
    }

    system->interface->log("Baud" , 1);
    system->interface->log(bits , 0);

    double useBPS = bpsLookup[ control & 0xf ];

    if (cartridgeId == Interface::CartridgeIdSwiftlink)
        useBPS *= 2.0;

    else if (cartridgeId == Interface::CartridgeIdTurbo232) {

        if ((control & 0xf) == 0)
            useBPS = t232BpsLookup[ enhancedControl & 3 ];
        else
            useBPS *= 2.0;
    }

    system->interface->log(useBPS , 0);

    bitCycles = (unsigned)((double)vicII->frequency() / useBPS * (double(bits) + (useHalfBit ? 0.5 : 0.0) ) + 0.5 );

    bitCyclesReceive = (unsigned)((float)bitCycles * 5.0 / 4.0);

    //bitCyclesReceive = bitCycles;

    system->interface->log(bitCycles , 0);

    if (command & 1)
        sysTimer.add( &receiver, bitCyclesReceive, Emulator::SystemTimer::UpdateExisting );
}

auto Acia::setInt( bool state ) -> void {

    if (useIrq)
        irqCall(state);

    if (useNmi) {
        if (state)
            system->interface->log("nmi",1);

        nmiCall(state);
    }
}

auto Acia::writeIo2( uint16_t addr, uint8_t value ) -> void {

    if (!useDE00)
        writeIo( addr, value );
}

auto Acia::readIo2( uint16_t addr ) -> uint8_t {

    if (!useDE00)
        return readIo( addr );

    return ExpansionPort::readIo2(addr);
}

auto Acia::writeIo1( uint16_t addr, uint8_t value ) -> void {

    if (useDE00)
        writeIo( addr, value );
}

auto Acia::readIo1( uint16_t addr ) -> uint8_t {

    if (useDE00)
        return readIo( addr );

    return ExpansionPort::readIo1(addr);
}

inline auto Acia::transmitterEnabled() -> bool {
    return (command & 0xc) != 0;
}

inline auto Acia::transmitterContinous() -> bool {
    return (command & 0xc) == 0xc;
}

inline auto Acia::transmitterIntEnabled() -> bool {
    return (command & 0xc) == 0x4;
}

auto Acia::writeIo( uint16_t addr, uint8_t value ) -> void {

    addr &= (cartridgeId == Interface::CartridgeIdTurbo232) ? 7 : 3;

    switch(addr) {
        case 0:
            txData = value;
            system->interface->log("write tx",1);

            if (command & 1) {
                redoTx = 2;
                if (transmitterEnabled() && !sysTimer.has(&transmitter))
                    sysTimer.add( &transmitter, 1, Emulator::SystemTimer::Action::UpdateExisting );

                status &= ~Status::TransmitterEmpty;
            }
            break;

        case 1: // write status resets chip
            system->interface->log("write status",1);
            updateDTR( false );
            socket.disconnect();
            dsrLock = dcdLock = false;
            status &= ~Status::OverrunError;
            command &= 0xe0;
            redoTx = 0;
            sysTimer.remove(&transmitter);
            setInt(false);
            break;
        case 2: {
            system->interface->log("write command", 1);
            command = value;

            if ( !transmitterEnabled() )
                sysTimer.remove(&transmitter);

            if (command & 1) {
                if (!socket.connected()) {

                    system->interface->log(address.c_str(), 1);
                    system->interface->log(port.c_str(), 0);

                    int res = socket.establish(address, port);

                    system->interface->log(res, 0);

                    if (socket.connected()) {
                        system->interface->log("connection established", 1);

                        updateDSR( false );
                    }

                    updateBaudGenerator();
                }

                updateDTR( true );

            } else {

                updateDTR(false );

                if (socket.connected()) {

                    socket.disconnect();

                    system->interface->log("raus", 1);
                }

                sysTimer.remove(&transmitter);
                redoTx = 0;
            }
        } break;
        case 3:
            system->interface->log("write control",1);
            control = value;
            updateBaudGenerator();
            break;
        case 7: // enhanced control (turbo232)
            system->interface->log("write enhanced control",1);
            if ((control & 0xf) == 0) {
                enhancedControl = value;
                updateBaudGenerator();
            }
            break;
    }

    system->interface->log(value,0, 1);
}

auto Acia::readIo( uint16_t addr ) -> uint8_t {

    addr &= (cartridgeId == Interface::CartridgeIdTurbo232) ? 7 : 3;

    switch(addr) {
        case 0:
            system->interface->log("read rx", 1);
            status &= ~(Status::OverrunError | Status::ParityError | Status::FramingError | Status::ReceiveDataFull);
            system->interface->log(rxData,0, 1);
            return rxData;

        case 1: {
            system->interface->log("read status", 1);

            system->interface->log(status,0, 1);

            uint8_t out = status;

            status &= ~(Status::DataCarrierDetect | Status::DataSetReady);

            if (dsr)
                status |= Status::DataSetReady;

            if (dcd)
                status |= Status::DataCarrierDetect;

            //todo multiple state changes of dsr or dcd cause second interrupt here

            setInt( false );

            dsrLock = dcdLock = false;

            status &= ~Status::Interrupt;

            return out;
        }
        case 2:
            system->interface->log("read command");
            system->interface->log(command,0, 1);
            return command;
        case 3:
            system->interface->log("read control");
            system->interface->log(control,0, 1);
            return control;
        case 7: // enhanced control (turbo232)
            return enhancedControl | (((control & 0xf) == 0) ? 4 : 0);
        default:
            return 0;
    }

    __builtin_unreachable();
}

auto Acia::prepareSocket( Emulator::Interface::Media* media, std::string address, std::string port ) -> void {

    this->media = media;
    this->cartridgeId = media ? (Interface::CartridgeId)media->pcbLayout->id : Interface::CartridgeIdDefault;
    this->address = address;
    this->port = port;
}

auto Acia::receiveData(uint8_t* data) -> bool {
    bool ok = false;

    // keep up with real time
    system->interface->audioFlush();

    while(1) {

        ok = receiveFromSocket( data );

        if (!ip232)
            break;

        if (*data != 0xff)
            break;

        if (!receiveFromSocket( data ))
            return false;

        switch( *data ) {
            case 0: updateDCD( false ); break;
            case 1: updateDCD( true ); break;
            default:
                return true;
        }
    }

    return ok;
}

auto Acia::receiveFromSocket( uint8_t* data ) -> bool {
    bool error = false;

    if ( !socket.poll(error) ) {
        if (error) {
            system->interface->log("poll error", 1);

            updateDSR( true );
            updateDCD( true );

            socket.disconnect();

        } else // else: nothing to poll
            system->interface->log("no new byte to receive", 1);


        return false;
    }
    system->interface->log("success", 1);
    if ( !socket.receiveData( (char*)data, 1 ) ) {
        system->interface->log("receive error", 1);

        updateDSR( true );
        updateDCD( true );

        socket.disconnect();

        return false;
    }

    return true;
}

auto Acia::updateDSR( bool newDSR ) -> void {

    if (!dsrLock && (dsr != newDSR)) {

        system->interface->log(newDSR ? "DSR 1" : "DSR 0" );

        if (newDSR)
            status |= Status::DataSetReady; // not ready;
        else
            status &= ~Status::DataSetReady; //ready;

        handleReceiverInterrupt();

        dsrLock = true; // reading status register clears the lock
    }

    // reflect the PIN state
    dsr = newDSR;
}

auto Acia::updateDCD( bool newDCD ) -> void {

    if (!dcdLock && (dcd != newDCD)) {

        system->interface->log(newDCD ? "DCD 1" : "DCD 0" );

        if (newDCD)
            status |= Status::DataCarrierDetect; // not detected;
        else
            status &= ~Status::DataCarrierDetect; // detected;

        handleReceiverInterrupt();

        dcdLock = true; // reading status register clears the lock
    }

    // reflect the PIN state
    dcd = newDCD;
}

auto Acia::handleReceiverInterrupt() -> void {

    if (!(command & 2)) {
        setInt(true);
        status |= Status::Interrupt;
    }
}

auto Acia::updateDTR( bool newDTR ) -> void {

    if (ip232) {

        if (dtr != newDTR) {
            uint8_t byte = 0xff;

            if (socket.sendData(&byte, 1))
                system->interface->log("DTR", 1);

            byte = newDTR ? 1 : 0;

            if (socket.sendData(&byte, 1))
                system->interface->log(newDTR ? "1" : "0", 0);
        }
    }
    // reflect the PIN state
    dtr = newDTR;
}

auto Acia::sendData() -> bool {

    system->interface->audioFlush();

    if (ip232) {
        if (txData == 0xff)
            if (!socket.sendData( &txData, 1 ))
                return false;
    }

    return socket.sendData( &txData, 1 );
}

auto Acia::serialize(Emulator::Serializer& s) -> void {

    s.integer( (uint16_t&)cartridgeId );
    s.integer( ip232 );
    s.integer( status );
    s.integer( control );
    s.integer( enhancedControl );
    s.integer( command );
    s.integer( rxData );
    s.integer( txData );
    s.integer( redoTx );
    s.integer( bitCycles );
    s.integer( useNmi );
    s.integer( useIrq );
    s.integer( useDE00 );
    s.integer( dcd );
    s.integer( dsr );
    s.integer( dtr );
    s.integer( dsrLock );
    s.integer( dcdLock );

    if ( !s.lightUsage() && (s.mode() == Emulator::Serializer::Mode::Load ) ) {
        socket.disconnect();

        if (command & 1)
            socket.establish( address, port );
    }

    ExpansionPort::serialize( s );
}

auto Acia::setJumper( unsigned jumperId, bool state ) -> void {

    system->interface->log("jumper",1);
    system->interface->log(jumperId,0);
    system->interface->log(state ? 1 : 0,0);

    switch(jumperId) {
        case 0:
            useIrq = state;
            break;
        case 1:
            useNmi = state;
            break;
        case 2:
            useDE00 = state;
            break;
        case 3:
            ip232 = state;
            break;
    }
}

auto Acia::getJumper( unsigned jumperId ) -> bool {
    switch (jumperId) {
        case 0:
            return useIrq;
        case 1:
            return useNmi;
        case 2:
            return useDE00;
        case 3:
            return ip232;
    }

    return false;
}

}
