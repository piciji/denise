
// todo: echo mode, handle frame & parity error

#pragma once

#include "../../../tools/socket.h"
#include "../expansionPort.h"

namespace LIBC64 {

struct Acia : ExpansionPort {

    Acia(System* system);
    ~Acia();

    enum Status : uint8_t {  ParityError = 1, FramingError = 2, OverrunError = 4, ReceiveDataFull = 8,
                    TransmitterEmpty = 16, DataCarrierDetect = 32, DataSetReady = 64, Interrupt = 128 };

    std::function<void ()> receiver;
    std::function<void ()> transmitter;

    Emulator::SystemTimer& sysTimer;
    std::string address;
    std::string port;

    Interface::CartridgeId cartridgeId;
    Emulator::Interface::Media* media;

    Emulator::Socket socket;
    uint8_t connectionLock;

    uint8_t status;
    uint8_t control;
    uint8_t enhancedControl;
    uint8_t command;

    uint8_t redoTx;
    unsigned bitCycles;
    unsigned bitCyclesReceive;

    bool useNmi;
    bool useIrq;
    bool useDE00;
    bool ip232; // for tcpser

    // only first state change of dcd and dsr is visible in status register.
    // subsequent changes are only visible when status register is interrogated.
    bool dsrLock;
    bool dcdLock;

    /**
     * the 9 pins of RS232 serial com port
     */
    // direction from acia point of view
    // out = dte(acia) -> dce(modem)
    //  in = dce(modem) -> dte(acia)

    // data flow
    uint8_t rxData; //[in] Received Data            serial shit-in data
    uint8_t txData; //[out] Transmitted Data        serial shift-out data

    // signals that emulation must handle
    bool dtr; //[out] Data Terminal Ready           dte signals operational readiness
    bool dsr; //[in] Data Set Ready                 dce signals operational readiness
    bool dcd; //[in] Data Carrier Detect            dce signals that a carrier is incomming from remote

    // next two signals are handled by tcpser when transmitting data
    // rts  -> [out] Request to Send                dte signals prepare to transmit data (now wait for cts)

    // if high, transmitter will be disabled (indicates a modem problem not ready to receive data)
    // cts  -> [in] Clear to Send                   dce accepts data from dte (not seen by software)

    // ri   -> [in] Ring Indicator                  not connected
    // ground

    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    auto readIo1( uint16_t addr ) -> uint8_t;
    auto writeIo2( uint16_t addr, uint8_t value ) -> void;
    auto readIo2( uint16_t addr ) -> uint8_t;
    auto writeIo( uint16_t addr, uint8_t value ) -> void;
    auto readIo( uint16_t addr ) -> uint8_t;
    auto updateBaudGenerator() -> void;
    auto receiveData(uint8_t* data) -> bool;
    auto receiveFromSocket( uint8_t* data ) -> bool;
    auto sendData() -> bool;
    auto reset(bool softReset = false) -> void;
    auto handleReceiverInterrupt() -> void;
    auto updateDSR( bool newDsr ) -> void;
    auto updateDCD( bool newDCD ) -> void;
    auto updateDTR( bool newDTR ) -> void;
    auto transmitterEnabled() -> bool;
    auto transmitterContinous() -> bool;
    auto transmitterIntEnabled() -> bool;

    auto setInt( bool state ) -> void;
    auto prepareSocket( Emulator::Interface::Media* media, std::string address, std::string port ) -> void;
    auto setJumper( unsigned jumperId, bool state ) -> void;
    auto getJumper( unsigned jumperId ) -> bool;
    auto serialize(Emulator::Serializer& s) -> void;

    static const double bpsLookup[16];
    static const double t232BpsLookup[4];
};

}
