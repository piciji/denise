
#pragma once

#include "../../../tools/socket.h"
#include "../expansionPort.h"

namespace LIBC64 {

struct Acia : ExpansionPort {

    Acia();
    ~Acia();

    enum Status {  ParityError = 1, FramingError = 2, OverrunError = 4, ReceiveDataFull = 8,
                    TransmitterEmpty = 16, DataCarrierDetect = 32, DataSetReady = 64, Interrupt = 128 };

    std::function<void ()> receiver;
    std::function<void ()> transmitter;

    std::string address;
    std::string port;
    bool ip232; // for tcpser
    Interface::CartridgeId cartridgeId;
    Emulator::Interface::Media* media;

    Emulator::Socket socket;

    uint8_t status;
    uint8_t control;
    uint8_t enhancedControl;
    uint8_t command;
    uint8_t rxData;
    uint8_t txData;

    bool redoTx;
    unsigned bitCycles;

    bool useNmi;
    bool useIrq;
    bool useDE00;

    bool dcd;
    bool dtr;

    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    auto readIo1( uint16_t addr ) -> uint8_t;
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto writeIo( uint16_t addr, uint8_t value ) -> void;
    auto readIo( uint16_t addr ) -> uint8_t;
    auto updateBaudGenerator() -> void;
    auto handShake() -> void;
    auto receiveData(uint8_t* data) -> bool;
    auto sendData() -> void;
    auto close() -> void;
    auto reset() -> void;

    auto setInt( bool state ) -> void;
    auto prepareSocket( Emulator::Interface::Media* media, std::string address, std::string port ) -> void;
    auto setJumper( unsigned jumperId, bool state ) -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    static const double bpsLookup[16];
    static const double t232BpsLookup[4];
};

extern Acia* acia;

}
