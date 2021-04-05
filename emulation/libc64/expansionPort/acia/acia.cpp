
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

        if (socket.connected()) {
       //     system->interface->log("try receive", 1);
            if (receiveData( &receiveByte ) ) {
system->interface->log("received", 1);
                if (!(status & Status::ReceiveDataFull))
                    rxData = receiveByte;
                else
                    status |= Status::OverrunError;

                status |= Status::ReceiveDataFull;

                if (!(command & 2)) {
                    setInt(true);
                    status |= Status::Interrupt;
                }
            }
        }

        if (command & 1)
            sysTimer.add( &receiver, bitCycles );
    };

    transmitter = [this]() {

        if (socket.connected()) {
            system->interface->log("try send", 1);
            sendData();

            status |= Status::TransmitterEmpty;

            if ((command & 0xc) == 4) {
                setInt(true);
                status |= Status::Interrupt;
            }
        }

        if (redoTx) {
            sysTimer.add( &transmitter, bitCycles );
            redoTx = false;
        }
    };

    sysTimer.registerCallback( {{&receiver, 1}, {&transmitter, 1}} );
}

auto Acia::reset() -> void {
    socket.close();
    status = Status::TransmitterEmpty;
    control = 0;
    command = 0;
    enhancedControl = 0;
    dcd = false;
    dtr = false;
    redoTx = false;
    rxData = false;
    txData = false;
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

    bitCycles = (unsigned)((double)vicII->frequency() / useBPS * (double(bits) + (useHalfBit ? 0.5 : 0.0) ) );

    bitCycles = (unsigned)((float)bitCycles * 5.0 / 4.0);

    system->interface->log(bitCycles , 0);

    if (command & 1)
        sysTimer.add( &receiver, bitCycles, Emulator::SystemTimer::UpdateExisting );
}

auto Acia::setInt( bool state ) -> void {

    if (useIrq)
        irqCall(state);

    if (useNmi)
        nmiCall(state);
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

auto Acia::writeIo( uint16_t addr, uint8_t value ) -> void {

    addr &= (cartridgeId == Interface::CartridgeIdTurbo232) ? 7 : 3;

    switch(addr) {
        case 0:
            txData = value;
            system->interface->log("write tx",1);

            if (command & 1) {

                if (sysTimer.has(&transmitter))
                    redoTx = true;
                else
                    sysTimer.add( &transmitter, bitCycles, Emulator::SystemTimer::Action::UpdateExisting );

                status &= ~Status::TransmitterEmpty;
            }
            break;

        case 1: // write status resets chip
            system->interface->log("write status",1);
            if (socket.connected())
                close();
            status &= ~Status::OverrunError;
            command &= 0xe0;
            redoTx = false;
            sysTimer.remove(&transmitter);
            setInt(false);
            break;
        case 2: {
            system->interface->log("write command", 1);
            bool dtrToggle = (command ^ value) & 1;
            command = value;

            if (socket.connected()) {
                if ((command & 1) != dtr) {

                    handShake();

                    dtr = command & 1;
                }
            }

            if (command & 1) {
                if (!socket.connected()) {

                    system->interface->log(address.c_str(), 1);
                    system->interface->log(port.c_str(), 0);

                    int res = socket.establish(address, port);

                    system->interface->log(res, 0);

                    if (socket.connected())
                        system->interface->log("drin", 1);

                    updateBaudGenerator();

            //        if (dtrToggle)
              //          handShake();
                }

            } else {
        //        if (dtrToggle)
          //          handShake();

                if (socket.connected())
                    close();
                sysTimer.remove(&transmitter);
                redoTx = false;
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
}

auto Acia::readIo( uint16_t addr ) -> uint8_t {

    addr &= (cartridgeId == Interface::CartridgeIdTurbo232) ? 7 : 3;

    switch(addr) {
        case 0:
            system->interface->log("read rx");
            status &= ~(Status::OverrunError | Status::ParityError | Status::FramingError | Status::ReceiveDataFull);
            return rxData;

        case 1: {
            system->interface->log("read status");
            status &= ~(Status::DataCarrierDetect | Status::DataSetReady);

            if (!socket.connected())
                status |= Status::DataCarrierDetect | Status::DataSetReady;
            else {
                status |= (dcd ? Status::DataCarrierDetect : 0) | Status::DataSetReady;
            }

            uint8_t out = status;

            status &= ~Status::Interrupt;

            return out;
        }
        case 2:
            system->interface->log("read command");
            return command;
        case 3:
            system->interface->log("read control");
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

auto Acia::setJumper( unsigned jumperId, bool state ) -> void {

    system->interface->log("jumper",1);
    system->interface->log(jumperId,0);
    system->interface->log(state,0);

    switch(jumperId) {
        case 0: // use irq
            useIrq = state;
            break;
        case 1: // use nmi
            useNmi = state;
            break;
        case 2: // use $DE00
            useDE00 = state;
            break;
        case 3: // use $DE00
            ip232 = state;
            break;
    }
}

auto Acia::receiveData(uint8_t* data) -> bool {

    while(1) {

        system->interface->log("poll", 1);
        auto res = socket.poll();

        system->interface->log(res, 0);

return false;

        if ( socket.poll() || !socket.receiveData( (char*)data, 1 ))
            return false;

        if (!ip232)
            break;

        if (*data != 0xff)
            break;

        if (!socket.poll() || !socket.receiveData( (char*)data, 1 ))
            return false;

        switch( *data ) {
            case 0: dcd = false; break;
            case 1: dcd = true; break;
            default:
                return true;
        }
    }

    return true;
}

auto Acia::sendData() -> void {

    if (ip232) {
        if (txData == 0xff)
            if (!socket.sendData( (char*)&txData, 1 ))
                return;
    }

    if (socket.sendData( (char*)&txData, 1 ))
        system->interface->log("sended", 1);
}

auto Acia::close() -> void {

    if (ip232) {
        uint8_t byte = 0xff;
        socket.sendData( (char*)&byte, 1 );
        byte = 0;
        socket.sendData( (char*)&byte, 1 );
    }

    socket.close();
}

auto Acia::handShake() -> void {
    system->interface->log("handshake", 1);
    bool DTR = command & 1;
    uint8_t byte = 0xff;

    if ((command & 0xc) == 0) {
        sysTimer.remove(&receiver);
    } else {
        updateBaudGenerator();
    }


    if (ip232) {

        if (socket.sendData( (char*)&byte, 1 ))
            system->interface->log("success", 1);

        byte = DTR ? 1 : 0;
        if (socket.sendData( (char*)&byte, 1 ))
            system->interface->log("success", 0);
    }
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

    if ( !s.lightUsage() && (s.mode() == Emulator::Serializer::Mode::Load ) ) {
        socket.close();

        if (command & 1)
            socket.establish( address, port );
    }

    ExpansionPort::serialize( s );
}

}
