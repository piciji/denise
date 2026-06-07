
#include "binaryMonitor.h"

#include "../../emulation/libc64/system/debuggerSnapshot.h"
#include "../../guikit/api.h"
#include "../helper/miscHelper.h"
#include "../tools/buffer.h"
#include "../thread/emuThread.h"
#include "../program.h"
#include "../media/autoloader.h"
#include "../view/view.h"

BinaryMonitor::BinaryMonitor() {
    initRegs();
}

BinaryMonitor::~BinaryMonitor() {
    destroy(client);
    destroy(server);
}

auto BinaryMonitor::setServer(const std::string& uri) -> void {
    server = new Socket;
    if (!server->establishServer(uri)) {
        destroy(server);
        return;
    }

    fprintf(stdout, "Socket: server online at %s \n", uri.c_str());
    waitForClientToAccept();

    // for (unsigned i = 0; i < 10; i++) {
    //     GUIKIT::System::sleep(50);
    // }
    //  client = new Socket;
    //  client->handle = 1;
}

auto BinaryMonitor::waitForClientToAccept() -> void {
    bool error = false;
    if (server == nullptr)
        return;

    for (unsigned i = 0; i < 20; i++) {
        if ( !server->poll(error) ) {
            if (error) {
                destroy(server);
                break;
            }
            GUIKIT::System::sleep( 50 );
            continue;
        }

        if ((client = server->acceptClient()) == nullptr) {
            destroy(server);
            break;
        }
        fprintf(stdout, "Socket: client %i accepted\n", client->handle);
        break;
    }
}

auto BinaryMonitor::update() -> void {
    bool error = false;
    if (client == nullptr)
        return;

    while (client->poll(error )) {
        if (error) {
            destroy(client);
            break;
        }

        if (!fetchCommands()) {
            destroy(client);
            break;
        }
    }
}

auto BinaryMonitor::fetchCommands() -> bool {
    char buffer[0xff];
    int received;
    int todo;

    received = client->receiveData( buffer, 1 );

    if (received <= 0)
        return false;

    if (buffer[0] != ASC_STX)
        return true;

    todo = 5;
    while (todo > 0) {
        received = client->receiveData( &buffer[1 + 5 - todo], todo );
        if (received <= 0) {
            destroy(client);
            return false;
        }
        todo -= received;
    }

    char apiVersion = buffer[1];
    auto bodyLength = copyBufferToInt<uint32_t>(reinterpret_cast<const uint8_t*>(&buffer[2]));
    bodyLength += 5; // + request id + command type

    if (apiVersion != 0x01 && apiVersion != 0x02)
        return true;

    todo = bodyLength;
    while (todo > 0) {
        received = client->receiveData( &buffer[6 + bodyLength - todo], todo );
        if (received <= 0) {
            destroy(client);
            return false;
        }
        todo -= received;
    }

    fprintf(stdout, "received ");
    for (int i = 0; i < 6 + bodyLength; ++i) {
        fprintf(stdout, "%x ", (unsigned char)buffer[i]);
    }
    fprintf(stdout, "\n");

    handleCommand( reinterpret_cast<uint8_t*>(&buffer[0]) );
    fflush(stdout);

    return true;
}

auto BinaryMonitor::handleCommand(uint8_t* buffer) -> void {
    Command command{};
    command.type = (Type)buffer[10];
    command.requestId = copyBufferToInt<uint32_t>(&buffer[6]);
    command.length = copyBufferToInt<uint32_t>(&buffer[2]);
    command.body = &buffer[11];

    switch (command.type) {
        default:
            fprintf(stdout, "Unknown command type: %02x\n", command.type);
            break;
        case Type::Ping:
            sendResponse(0, Type::Ping, Error::OK, command.requestId, nullptr);
            break;
        case Type::REGISTERS_AVAILABLE:
            sendRegistersAvailable(command);
            break;
        case Type::REGISTER_INFO:
            sendRegisters( command );
            break;
        case Type::RESET:
            performReset( command );
            break;
        case Type::AUTOSTART:
            performAutostart( command );
            break;
        case Type::QUIT:
            performQuit( command );
            break;
    }
}

auto BinaryMonitor::performReset(Command& command) -> void {
    if (command.length < 1) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    uint8_t resetType = command.body[0];

    sendResponse(0, Type::RESET, Error::OK, command.requestId, nullptr);

    emuThread->lock(resetType < 2);
    switch (resetType) {
        default:
        case 0: program->power(activeEmulator); break;
        case 1: program->reset(activeEmulator); break;

        case 8: activeEmulator->resetDrive( activeEmulator->getDisk(0) ); break;
        case 9: activeEmulator->resetDrive( activeEmulator->getDisk(1) ); break;
        case 10: activeEmulator->resetDrive( activeEmulator->getDisk(2) ); break;
        case 11: activeEmulator->resetDrive( activeEmulator->getDisk(3) ); break;
    }

    emuThread->unlock();
}

auto BinaryMonitor::performQuit(Command& command) -> void {

    view->onClose();

    sendResponse(0, Type::QUIT, Error::OK, command.requestId, nullptr);
}

auto BinaryMonitor::performAutostart(Command& command) -> void {
    bool run = command.body[0] == 1;
    unsigned index = copyBufferToInt<uint16_t>(&command.body[1]);
    uint8_t length = command.body[3];
    const char* fileData = (const char*)(command.body + 4);
    std::string fn( fileData, length);

    if(command.length < 4 + length) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    autoloader->init( {fn}, run ? Autoloader::Mode::AutoStart : Autoloader::Mode::Open, index );

    autoloader->setErrorLevel(1);

    autoloader->loadFiles();

    if (!autoloader->hasLoaded()) {
        sendError( Error::CMD_FAILURE, command.requestId );
        return;
    }

    sendResponse(0, Type::AUTOSTART, Error::OK, command.requestId, nullptr);

    //activeEmulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::UIRequestedStop, 0 );
}

auto BinaryMonitor::sendResume() -> void {
    uint8_t body[2];
    copyIntToBuffer<uint16_t>(&body[0], snapshot->pc);

    sendResponse(2, Type::RESUMED, Error::OK, MON_EVENT, body);
}

auto BinaryMonitor::responseStopped(LIBC64::DebuggerSnapshot* snap) -> void {
    snapshot = snap;
    sendRegisters( MON_EVENT, 0 );

    uint8_t body[2];
    copyIntToBuffer<uint16_t>(&body[0], snap->pc);

    sendResponse(2, Type::STOPPED, Error::OK, MON_EVENT, body);
}

auto BinaryMonitor::sendResponse(uint32_t length, Type type, Error errorCode, uint32_t requestId, const uint8_t* body) -> void {
    uint8_t response[12];

    response[0] = ASC_STX;
    response[1] = API_VERSION;
    copyIntToBuffer<uint32_t>(&response[2], length);
    response[6] = (uint8_t)type;
    response[7] = (uint8_t)errorCode;
    copyIntToBuffer<uint32_t>(&response[8], requestId);

    fprintf(stdout, "sended ");
    for (int i = 0; i < sizeof(response); ++i) {
        fprintf(stdout, "%x ", (unsigned char)response[i]);
    }
    fprintf(stdout, "\n");
    int sended = client->sendData( reinterpret_cast<const char*>(response), sizeof(response) );
    //fprintf(stdout, "Sended: %i\n", sended);
    fflush(stdout);

    if (body)
        client->sendData( reinterpret_cast<const char*>(body), length );
}

auto BinaryMonitor::initRegs() -> void {
    regs6502 = {
        {"PC", rPC, 16},
        {"A", rA, 8},
        {"X", rX, 8},
        {"Y", rY, 8},
        {"SP", rSP, 8},
        {"Fl", rFlags, 8},
        {"LIN", rRaster, 16},
        {"CYC", rCycle, 16},
    };

    regs6510 = {
        {"PC", rPC, 16},
        {"A", rA, 8},
        {"X", rX, 8},
        {"Y", rY, 8},
        {"SP", rSP, 8},
        {"00", rZero, 8},
        {"01", rOne, 8},
        {"Fl", rFlags, 8},
        {"LIN", rRaster, 16},
        {"CYC", rCycle, 16},
    };

    regs65816 = {
        {"PBR", rPBR, 8},
        {"PC", rPC, 16},
        {"A", rA, 16},
        {"X", rX, 16},
        {"Y", rY, 16},
        {"SP", rSP, 16},
        {"D", rD, 16},
        {"DBR", rDBR, 8},
        {"Fl", rFlags, 8},
        {"E", rModeE, 1},
        {"LIN", rRaster, 16},
        {"CYC", rCycle, 16},
    };
}

auto BinaryMonitor::sendRegistersAvailable(Command& command) -> void {
    if (command.length < 1) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    uint8_t space = command.body[0];

    auto* regList = getRegList( space );

    if (!regList) {
        sendError( Error::INVALID_MEMSPACE, command.requestId );
        return;
    }

    unsigned bodySize = 2;
    for (auto& reg : *regList) {
        bodySize += reg.name.length() + 4;
    }

    auto* body = new uint8_t[bodySize];
    uint8_t* ptr = body;

    ptr = copyIntToBuffer<uint16_t>(ptr, regList->size());

    for (auto& reg : *regList) {
        auto nameSize = static_cast<uint8_t>(reg.name.length());

        *ptr++ = nameSize + 3;
        *ptr++ = reg.id;
        *ptr++ = reg.size;

        ptr = copyStringToBuffer( ptr, nameSize, (uint8_t*)reg.name.c_str() );
    }

    sendResponse(bodySize, Type::REGISTERS_AVAILABLE, Error::OK, command.requestId, body);

    delete[] body;
}

auto BinaryMonitor::sendRegisters(Command& command) -> void {
    if (command.length < 1) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    uint8_t space = command.body[0];

    sendRegisters(command.requestId, space);
}

auto BinaryMonitor::sendRegisters(unsigned requestId, uint8_t space) -> void {
    auto* regList = getRegList( space );

    if (!regList) {
        sendError( Error::INVALID_MEMSPACE, requestId );
        return;
    }

    unsigned bodySize = 2 + regList->size() * 4;

    auto* body = new uint8_t[bodySize];
    uint8_t* ptr = body;
    uint16_t value;

    ptr = copyIntToBuffer<uint16_t>(ptr, regList->size());

    LIBC64::DebuggerSnapshot::Drive* snapD = nullptr;
    if (regList == &regs6502)
        snapD = &snapshot->drives[space - 1];

    for (auto& reg : *regList) {
        *ptr++ = 3;
        *ptr++ = reg.id;

        switch (reg.id) {
            case rA: value = snapD ? snapD->regA : snapshot->regA; break;
            case rX: value = snapD ? snapD->regX : snapshot->regX; break;
            case rY: value = snapD ? snapD->regY : snapshot->regY; break;
            case rPC: value = snapD ? snapD->pc : snapshot->pc; break;
            case rSP: value = snapD ? snapD->regS : snapshot->regS; break;
            case rFlags: value = snapD ? snapD->flags : snapshot->flags; break;
            case rD: value = snapshot->regD; break;
            case rPBR: value = snapshot->pbr; break;
            case rDBR: value = snapshot->dbr; break;
            case rModeE: value = snapshot->modeE; break;
            case rRaster: value = snapshot->vPos; break;
            case rCycle: value = snapshot->hPos; break;
            case rZero: value = snapshot->por; break;
            case rOne: value = snapshot->ddr; break;
            default: value = 0; break;
        }

        ptr = copyIntToBuffer<uint16_t>(ptr, value);
    }

    sendResponse(bodySize, Type::REGISTER_INFO, Error::OK, requestId, body);

    delete[] body;
}

auto BinaryMonitor::getRegList(uint8_t space) -> std::vector<Reg>* {
    switch (space) {
        case 0: // main CPU
            return MiscHelper::hasSuperCpuActive() ? &regs65816 : &regs6510;
        case 1: // drive 8
        case 2: // drive 9
        case 3: // drive 10
        case 4: // drive 11
            return &regs6502;
        default:
            return nullptr;
    }
}

auto BinaryMonitor::sendError(Error error, uint32_t requestId) -> void {
    sendResponse(0, Type::INVALID, error, requestId, nullptr);
}

auto BinaryMonitor::destroy(Socket*& socket) -> void {
    if (socket) {
        delete socket;
        socket = nullptr;
    }
}

// -initbreak 0x$(DebugStartAddressHex)