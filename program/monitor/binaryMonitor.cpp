
#include "binaryMonitor.h"

#include "../../emulation/libc64/system/debuggerSnapshot.h"
#include "../../guikit/api.h"
#include "../helper/miscHelper.h"
#include "../tools/buffer.h"
#include "../thread/emuThread.h"
#include "../program.h"
#include "../debugger/cpuDebugger.h"
#include "../media/autoloader.h"
#include "../view/view.h"
#include "../tools/error.h"
#include <cstring>

#define LOG_BINARY

#ifdef LOG_BINARY
    #define _log(msg, ...) { _inform(msg, ##__VA_ARGS__); fflush( stdout ); }
#else
    #define _log(msg, var)
#endif

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

    _log("Socket: server online at %s", uri.c_str())
    waitForClientToAccept();
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
        _log("Socket: client %i accepted", client->handle)

        delayedJobs = false;
        activeEmulator = program->getEmulator("C64");
        initDebugger(DebuggerTheme::CPU);
        activeEmulator->debuggerAdd( DebuggerTheme::Unspecified, DebuggerAction::UIRequestedStop, 0 );
        break;
    }
}

auto BinaryMonitor::initDebugger(DebuggerTheme theme) -> Debugger* {
    Debugger* debugger = program->getDebugger( activeEmulator, theme );

    if (!debugger) {
        debugger = program->createDebugger( activeEmulator, theme );
        if (debugger)
            debugger->initTheme();
    }

    return debugger;
}

auto BinaryMonitor::update() -> void {
    bool error = false;
    if (client == nullptr)
        return;

    if (delayedJobs) {
        delayedJobs = false;
        std::vector<CheckPoint*> toDelete;
        for (auto& cp : checkPoints) {
            if (cp.hit && cp.temporary)
                toDelete.push_back(&cp);
        }
        for (auto& cp : toDelete) {
            _log("Socket: temporary checkpoint ID %i removed", cp->num );
            deleteCheckpoint( cp );
        }
    }

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

    handleCommand( reinterpret_cast<uint8_t*>(&buffer[0]) );

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
            _log("Socket: unknown command type: %02x", command.type)
            break;
        case Type::PING:
            sendResponse(0, Type::PING, Error::OK, command.requestId, nullptr);
            _log("Socket: ping")
            break;
        case Type::MEM_GET:
            getMem(command);
            break;
        case Type::REGISTERS_AVAILABLE:
            sendRegistersAvailable(command);
            break;
        case Type::BANKS_AVAILABLE:
            sendBanksAvailable(command);
            break;
        case Type::REGISTER_INFO:
            getRegisters( command );
            break;
        case Type::REGISTERS_SET:
            setRegisters( command );
            break;
        case Type::RESET:
            performReset( command );
            break;
        case Type::AUTOSTART:
            performAutostart( command );
            break;
        case Type::CHECKPOINT_INFO:
            getCheckpoint( command );
            break;
        case Type::CHECKPOINT_SET:
            setCheckpoint( command );
            break;
        case Type::CHECKPOINT_DELETE:
            deleteCheckpoint( command );
            break;
        case Type::CHECKPOINT_LIST:
            listCheckpoints( command );
            break;
        case Type::CHECKPOINT_TOGGLE:
            toggleCheckpoints( command );
            break;
        case Type::ADVANCE_INSTRUCTIONS:
            advanceInstruction( command );
            break;
        case Type::EXECUTE_UNTIL_RETURN:
            stepOut( command );
            break;
        case Type::CONDITION_SET:
            setCondition( command );
            break;
        case Type::EXIT:
            performExit( command );
            break;
        case Type::QUIT:
            performQuit( command );
            break;
    }
}

auto BinaryMonitor::listCheckpoints(Command& command) -> void {
    uint8_t response[sizeof(uint32_t)];

    for (auto& cp : checkPoints) {
        sendCheckpointInfo(command.requestId, cp, false);
    }

    copyIntToBuffer<uint32_t>(response, checkPoints.size() );

    sendResponse(sizeof(response), Type::CHECKPOINT_LIST, Error::OK, command.requestId, response);

    _log("Socket: list %i checkpoints", static_cast<int>(checkPoints.size()) )
}

auto BinaryMonitor::toggleCheckpoints(Command& command) -> void {
    if (command.length < 5) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    emuThread->lock();

    bool _enable = !!command.body[4];

    unsigned num = copyBufferToInt<uint32_t>(&command.body[0]);

    if (num == ~0) {
        for (auto& cp : checkPoints) {
            Debugger* debugger = program->getDebugger( activeEmulator, cp.theme );
            if (!debugger) {
                sendError( Error::CMD_FAILURE, command.requestId );
                emuThread->unlock();
                return;
            }

            auto* cpuDebugger = dynamic_cast<CpuDebugger*>(debugger);
            cp.enable = _enable;

            if (cp.op & 1) {
                cpuDebugger->enableEntry( cp.address, DebuggerAction::Watchpoint, _enable);
            } if (cp.op & 2) {
                cpuDebugger->enableEntry( cp.address, DebuggerAction::WatchpointWrite, _enable);
            } if (cp.op & 4) {
                cpuDebugger->enableEntry( cp.address, DebuggerAction::Breakpoint, _enable);
            }
        }
    } else {
        CheckPoint* cp = findCheckpoint( num );

        if (!cp) {
            sendError( Error::OBJECT_MISSING, command.requestId );
            emuThread->unlock();
            return;
        }

        cp->enable = _enable;

        Debugger* debugger = program->getDebugger( activeEmulator, cp->theme );
        if (!debugger) {
            sendError( Error::CMD_FAILURE, command.requestId );
            emuThread->unlock();
            return;
        }

        auto* cpuDebugger = dynamic_cast<CpuDebugger*>(debugger);

        if (cp->op & 1) {
            bool enable = hasEnabledCheckpoint( cp->address, cp->theme, 1 );
            cpuDebugger->enableEntry( cp->address, DebuggerAction::Watchpoint, enable);
        } if (cp->op & 2) {
            bool enable = hasEnabledCheckpoint( cp->address, cp->theme, 2 );
            cpuDebugger->enableEntry( cp->address, DebuggerAction::WatchpointWrite, enable);
        } if (cp->op & 4) {
            bool enable = hasEnabledCheckpoint( cp->address, cp->theme, 4 );
            cpuDebugger->enableEntry( cp->address, DebuggerAction::Breakpoint, enable);
        }
    }

    sendResponse(0, Type::CHECKPOINT_TOGGLE, Error::OK, command.requestId, nullptr);

    emuThread->unlock();

    _log("Socket: toggle checkpoint ID %i", num )
}

auto BinaryMonitor::getCheckpoint(Command& command) -> void {
    if (command.length < sizeof(unsigned)) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    unsigned num = copyBufferToInt<uint32_t>(&command.body[0]);

    CheckPoint* cp = findCheckpoint( num );

    if (!cp) {
        sendError( Error::OBJECT_MISSING, command.requestId );
        return;
    }

    sendCheckpointInfo(command.requestId, *cp, false);

    _log("Socket: get checkpoint ID %i", num )
}

auto BinaryMonitor::deleteCheckpoint(Command& command) -> void {
    if (command.length < sizeof(unsigned)) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    auto num = copyBufferToInt<uint32_t>(&command.body[0]);

    emuThread->lock(  );
    if (num == ~0) {
        for (auto& cp : checkPoints) {
            Debugger* debugger = program->getDebugger( activeEmulator, cp.theme );
            if (!debugger) {
                sendError( Error::CMD_FAILURE, command.requestId );
                emuThread->unlock();
                return;
            }

            auto* cpuDebugger = dynamic_cast<CpuDebugger*>(debugger);

            if (cp.op & 1) {
                cpuDebugger->deleteEntry( cp.address, DebuggerAction::Watchpoint);
            } if (cp.op & 2) {
                cpuDebugger->deleteEntry( cp.address, DebuggerAction::WatchpointWrite);
            } if (cp.op & 4) {
                cpuDebugger->deleteEntry( cp.address, DebuggerAction::Breakpoint);
            }
        }

        checkPoints.clear();
    } else {
        CheckPoint* cp = findCheckpoint( num );

        if (!cp) {
            sendError( Error::OBJECT_MISSING, command.requestId );
            emuThread->unlock();
            return;
        }

        if (!deleteCheckpoint(cp)) {
            sendError( Error::CMD_FAILURE, command.requestId );
            emuThread->unlock();
            return;
        }
    }

    sendResponse(0, Type::CHECKPOINT_DELETE, Error::OK, command.requestId, nullptr);
    emuThread->unlock(  );
    _log("Socket: delete checkpoint ID %i", num )
}

auto BinaryMonitor::deleteCheckpoint(CheckPoint* cp) -> bool {
    DebuggerTheme theme = cp->theme;
    uint8_t op = cp->op;
    uint16_t address = cp->address;

    auto iter = std::remove_if(checkPoints.begin(), checkPoints.end(),
    [cp](const CheckPoint& _cp) {
        return _cp.num == cp->num;
    });

    checkPoints.erase(iter, checkPoints.end());

    Debugger* debugger = program->getDebugger( activeEmulator, theme );
    if (!debugger) {
        return false;
    }

    auto* cpuDebugger = dynamic_cast<CpuDebugger*>(debugger);

    if (op & 1) {
        if (!hasCheckpoint( address, theme, 1 ))
            cpuDebugger->deleteEntry( address, DebuggerAction::Watchpoint);
        else {
            bool enable = hasEnabledCheckpoint( address, theme, 1 );
            cpuDebugger->enableEntry( address, DebuggerAction::Watchpoint, enable);
        }

    } if (op & 2) {
        if (!hasCheckpoint( address, theme, 2 ))
            cpuDebugger->deleteEntry( address, DebuggerAction::WatchpointWrite);
        else {
            bool enable = hasEnabledCheckpoint( address, theme, 2 );
            cpuDebugger->enableEntry( address, DebuggerAction::WatchpointWrite, enable);
        }
    } if (op & 4) {
        if (!hasCheckpoint( address, theme, 4 ))
            cpuDebugger->deleteEntry( address, DebuggerAction::Breakpoint);
        else {
            bool enable = hasEnabledCheckpoint( address, theme, 4 );
            cpuDebugger->enableEntry( address, DebuggerAction::Breakpoint, enable);
        }
    }

    return true;
}

auto BinaryMonitor::findCheckpoint(unsigned num) -> CheckPoint* {
    for (auto& cp : checkPoints) {
        if (cp.num == num)
            return &cp;
    }
    return nullptr;
}

auto BinaryMonitor::hasEnabledCheckpoint(uint16_t address, DebuggerTheme theme, uint8_t op) -> bool {
    for (auto& cp : checkPoints) {
        if (cp.enable && cp.theme == theme && cp.address == address && (cp.op & op))
            return true;
    }
    return false;
}

auto BinaryMonitor::hasCheckpoint(uint16_t address, DebuggerTheme theme, uint8_t op) -> bool {
    for (auto& cp : checkPoints) {
        if (cp.theme == theme && cp.address == address && (cp.op & op))
            return true;
    }
    return false;
}

auto BinaryMonitor::setCheckpoint(Command& command) -> void {
    DebuggerTheme theme = DebuggerTheme::CPU;
    if (command.length < 8) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    if (command.length >= 9) {
        theme = getTheme( command.body[8] );

        if (theme == DebuggerTheme::Unspecified) {
            sendError( Error::INVALID_MEMSPACE, command.requestId );
            return;
        }
    }

    emuThread->lock();
    Debugger* debugger = initDebugger(theme);

    if (!debugger) {
        sendError( Error::CMD_FAILURE, command.requestId );
        return;
    }

    auto* cpuDebugger = dynamic_cast<CpuDebugger*>(debugger);
    static unsigned num = 1;

    CheckPoint cp;
    cp.num = num++;
    cp.theme = theme;
    cp.space = command.body[8];
    cp.op = command.body[6];
    cp.condition = false;
    cp.address = copyBufferToInt<uint16_t>(&command.body[0]);
    cp.endAddress = copyBufferToInt<uint16_t>(&command.body[2]);
    cp.temporary = command.body[7];
    cp.hit = false;
    cp.stop = command.body[4];
    cp.enable = command.body[5] >= 1;
    checkPoints.push_back( cp );

    if (cp.op & 1) {
        cpuDebugger->addEntry( cp.address, DebuggerAction::Watchpoint);
        bool enable = hasEnabledCheckpoint( cp.address, cp.theme, 1 );
        cpuDebugger->enableEntry( cp.address, DebuggerAction::Watchpoint, enable);
    } if (cp.op & 2) {
        cpuDebugger->addEntry( cp.address, DebuggerAction::WatchpointWrite);
        bool enable = hasEnabledCheckpoint( cp.address, cp.theme, 2 );
        cpuDebugger->enableEntry( cp.address, DebuggerAction::WatchpointWrite, enable);
    } if (cp.op & 4) {
        cpuDebugger->addEntry( cp.address, DebuggerAction::Breakpoint);
        bool enable = hasEnabledCheckpoint( cp.address, cp.theme, 4 );
        cpuDebugger->enableEntry( cp.address, DebuggerAction::Breakpoint, enable);
    }

    sendCheckpointInfo(command.requestId, cp, false);
    emuThread->unlock();

    _log("Socket: set checkpoint ID %i", cp.num )
}

auto BinaryMonitor::advanceInstruction(Command& command) -> void {
    uint8_t stepOver = command.body[0];
    bool debugging = emuThread->debugging;

    if (!debugging)
        emuThread->lock();
    if (stepOver)
        activeEmulator->debuggerStepOver( DebuggerTheme::CPU, stepOver == 1 );
    else
        activeEmulator->debuggerStepInto( DebuggerTheme::CPU );

    sendResponse(0, Type::ADVANCE_INSTRUCTIONS, Error::OK, command.requestId, nullptr);
    if (!debugging)
        emuThread->unlock();
    else
        emuThread->unlockDebugger();

    if (stepOver)
        _log("Socket: step over")
    else
        _log("Socket: step into")
}

auto BinaryMonitor::stepOut(Command& command) -> void {
    bool debugging = emuThread->debugging;

    if (!debugging)
        emuThread->lock();

    activeEmulator->debuggerStepOut( DebuggerTheme::CPU );

    sendResponse(0, Type::EXECUTE_UNTIL_RETURN, Error::OK, command.requestId, nullptr);

    if (!debugging)
        emuThread->unlock();
    else
        emuThread->unlockDebugger();

    _log("Socket: step out" )
}

auto BinaryMonitor::setCondition(Command& command) -> void {
    auto num = copyBufferToInt<uint32_t>(&command.body[0]);
    uint8_t length = command.body[4];

    if(command.length < 5 + length) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    CheckPoint* cp = findCheckpoint( num );

    if (!cp || !cp->op) {
        sendError( Error::OBJECT_MISSING, command.requestId );
        return;
    }

    std::string condStr(reinterpret_cast<char*>(command.body + 5), length);

    Debugger* debugger = program->getDebugger( activeEmulator, DebuggerTheme::CPU );

    if (!debugger) {
        sendError( Error::CMD_FAILURE, command.requestId );
        return;
    }

    emuThread->lock();
    auto* cpuDebugger = dynamic_cast<CpuDebugger*>(debugger);
    bool result = false;

    if (cp->op & 1)
        result |= cpuDebugger->addCondition( cp->address, DebuggerAction::Watchpoint, condStr );
    if (cp->op & 2)
        result |= cpuDebugger->addCondition( cp->address, DebuggerAction::WatchpointWrite, condStr );
    if (cp->op & 4)
        result |= cpuDebugger->addCondition( cp->address, DebuggerAction::Breakpoint, condStr );

    emuThread->unlock();
    if (!result) {
        sendError( Error::CMD_FAILURE, command.requestId );
        return;
    }

    sendResponse(0, Type::CONDITION_SET, Error::OK, command.requestId, nullptr);

    _log("Socket: set condition for ID %i", cp->num )
}

auto BinaryMonitor::sendCheckpointInfo(uint32_t requestId, CheckPoint& cp, bool hit) -> void {
    uint8_t response[23];
    copyIntToBuffer<uint32_t>( &response[0], cp.num);
    response[4] = hit;
    copyIntToBuffer<uint16_t>( &response[5], cp.address);
    copyIntToBuffer<uint16_t>( &response[7], cp.endAddress);
    response[9] = cp.stop;
    response[10] = cp.enable;
    response[11] = cp.op;
    response[12] = cp.temporary;
    copyIntToBuffer<uint32_t>( &response[13], hit ? 1 : 0); // hit count
    copyIntToBuffer<uint32_t>( &response[17], 0); // ignore count
    response[21] = !!cp.condition;
    response[22] = cp.space;
    cp.hit |= hit;

    if (hit && cp.temporary)
        delayedJobs = true;

    sendResponse(sizeof(response), Type::CHECKPOINT_INFO, Error::OK, requestId, response);
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
    _log("Socket: reset with type %i", resetType );
}

auto BinaryMonitor::performExit(Command& command) -> void {
    emuThread->unlockDebugger();

    sendResponse(0, Type::EXIT, Error::OK, command.requestId, nullptr);

    _log("Socket: resume" )
}

auto BinaryMonitor::performQuit(Command& command) -> void {

    view->onClose();

    sendResponse(0, Type::QUIT, Error::OK, command.requestId, nullptr);

    _log("Socket: quit" )
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

    emuThread->lock(true);
    autoloader->init( {fn}, run ? Autoloader::Mode::AutoStart : Autoloader::Mode::Open, index );

    autoloader->setErrorLevel(1);

    autoloader->loadFiles();

    if (!autoloader->hasLoaded()) {
        emuThread->unlock();
        sendError( Error::CMD_FAILURE, command.requestId );
        return;
    }

    emuThread->unlock();

    sendResponse(0, Type::AUTOSTART, Error::OK, command.requestId, nullptr);

    _log("Socket: autostart mode %i", run )
}

auto BinaryMonitor::getMem(Command& command) -> void {
    if (command.length < 8) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    bool hasSCPUActive = MiscHelper::hasSuperCpuActive();
    uint8_t bank = command.body[0];

    unsigned startAddress = copyBufferToInt<uint16_t>(&command.body[1]);
    unsigned endAddress = copyBufferToInt<uint16_t>(&command.body[3]);

    if (startAddress > endAddress) {
        sendError( Error::INVALID_PARAMETER, command.requestId );
        return;
    }

    auto bankType = copyBufferToInt<uint16_t>(&command.body[6]);

    unsigned length = (endAddress + 1) - startAddress;

    DebuggerTheme theme;
    DebuggerAction action = DebuggerAction::None;

    switch (command.body[5]) {
        case 0: theme = DebuggerTheme::CPU; break;
        case 1: theme = DebuggerTheme::Drive8Memory; break;
        case 2: theme = DebuggerTheme::Drive9Memory; break;
        case 3: theme = DebuggerTheme::Drive10Memory; break;
        case 4: theme = DebuggerTheme::Drive11Memory; break;
        default:
            sendError( Error::INVALID_MEMSPACE, command.requestId );
            return;
    }

    if (theme == DebuggerTheme::CPU) {
        unsigned elements = std::size(LIBC64::DebuggerSnapshot::memAccesses);
        if (bankType >= elements) {
            sendError( Error::INVALID_PARAMETER, command.requestId );
            return;
        }
        if (bankType > 0) {
            theme = DebuggerTheme::Memory;
            if (bankType == 2)
                action = DebuggerAction::MemROM;
            else if (bankType == 3)
                action = DebuggerAction::MemIO;
            else if (bankType == 4)
                action = DebuggerAction::MemCART;
        } else if (hasSCPUActive) {
            startAddress |= bank << 16;
            endAddress |= bank << 16;
        }
    }

    unsigned responseSize = 2 + length;
    auto* response = new uint8_t[ responseSize ];
    copyIntToBuffer<uint16_t>(response, length );

    emuThread->lock();
    activeEmulator->getMemory( theme, action, startAddress, endAddress, response + 2 );
    emuThread->unlock();

    sendResponse(responseSize, Type::MEM_GET, Error::OK, command.requestId, response);

    _log("Socket: get mem %x - %x", startAddress, endAddress)

    delete[] response;
}

auto BinaryMonitor::sendResume() -> void {
    uint8_t body[2];

    copyIntToBuffer<uint16_t>(&body[0], snapshot->pcEdge);

    sendResponse(2, Type::RESUMED, Error::OK, MON_EVENT, body);
}

auto BinaryMonitor::responseStopped(LIBC64::DebuggerSnapshot* snap) -> void {
    snapshot = snap;

    for (auto& cp : checkPoints) {
        if (cp.address == snapshot->callbackAddress && cp.theme == snapshot->callbackTheme) {
            if ((cp.op & 1) && snapshot->callbackAction == DebuggerAction::Watchpoint) {
                _log("Socket: hit watchpoint ID %i", cp.num)
                sendCheckpointInfo(~0, cp, true);
            } if ((cp.op & 2) && snapshot->callbackAction == DebuggerAction::WatchpointWrite) {
                _log("Socket: hit store watchpoint ID %i", cp.num)
                sendCheckpointInfo(~0, cp, true);
            } if ((cp.op & 4) && snapshot->callbackAction == DebuggerAction::Breakpoint) {
                _log("Socket: hit breakpoint ID %i", cp.num)
                sendCheckpointInfo(~0, cp, true);
            }
        }
    }

    sendRegisters( MON_EVENT, 0 );

    uint8_t body[2];
    copyIntToBuffer<uint16_t>(&body[0], snap->pcEdge);

    _log("Socket: stopped")
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

    client->sendData( reinterpret_cast<const char*>(response), sizeof(response) );

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

    _log("Socket: %i regs available", static_cast<int>(regList->size()) )

    delete[] body;
}

auto BinaryMonitor::sendBanksAvailable(Command& command) -> void {
    auto& memAccesses = LIBC64::DebuggerSnapshot::memAccesses;

    int count = 0;
    unsigned bodySize = 2;
    for (auto& memAccess : memAccesses) {
        bodySize += std::strlen(memAccess.ident) + 4;
        count++;
    }

    auto* body = new uint8_t[bodySize];
    uint8_t* ptr = body;
    ptr = copyIntToBuffer<uint16_t>(ptr, count);

    for (auto& memAccess : memAccesses) {
        unsigned _s = std::strlen(memAccess.ident);
        *ptr++ = _s + 3;
        ptr = copyIntToBuffer<uint16_t>(ptr, memAccess.vector);
        ptr = copyStringToBuffer( ptr, _s, (uint8_t*)memAccess.ident );
    }

    sendResponse(bodySize, Type::BANKS_AVAILABLE, Error::OK, command.requestId, body);

    _log("Socket: %i banks available", static_cast<int>(std::size(memAccesses)) )

    delete[] body;
}

auto BinaryMonitor::getRegisters(Command& command) -> void {
    if (command.length < 1) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    uint8_t space = command.body[0];

    emuThread->lock();
    initDebugger( getTheme( space ) );
    sendRegisters(command.requestId, space);
    emuThread->unlock();
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
            case rPC: value = snapD ? snapD->pcEdge : snapshot->pcEdge; break;
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

    _log("Socket: register info count %i", static_cast<int>(regList->size()) )

    delete[] body;
}

auto BinaryMonitor::setRegisters(Command& command) -> void {
    if (command.length < 3) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    uint8_t space = command.body[0];
    auto count = copyBufferToInt<uint16_t>( &command.body[1] );

    if (command.length < (3 + count * 4)) {
        sendError( Error::CMD_INVALID_LENGTH, command.requestId );
        return;
    }

    auto* regList = getRegList( space );

    if (!regList) {
        sendError( Error::INVALID_MEMSPACE, command.requestId );
        return;
    }

    auto theme = getTheme( space );

    if (theme == DebuggerTheme::Unspecified) {
        sendError( Error::INVALID_MEMSPACE, command.requestId );
        return;
    }

    uint8_t* body = command.body + 3;

    LIBC64::DebuggerSnapshot::Drive* snapD = nullptr;
    if (regList == &regs6502)
        snapD = &snapshot->drives[space - 1];

    emuThread->lock();
    initDebugger( theme );
    Reg reg;
    for (int i = 0; i < count; i++) {
        reg.size = body[0];
        reg.id = body[1];

        if (reg.size < 3) {
            sendError( Error::CMD_INVALID_LENGTH, command.requestId );
            emuThread->unlock();
            return;
        }

        auto value = copyBufferToInt<uint16_t>( body + 2 );

        switch (reg.id) {
            case rA: snapD ? snapD->regA = value : snapshot->regA = value; break;
            case rX: snapD ? snapD->regX = value : snapshot->regX = value; break;
            case rY: snapD ? snapD->regY = value : snapshot->regY = value; break;
            case rPC: snapD ? snapD->pc = value : snapshot->pc = value; break;
            case rSP: snapD ? snapD->regS = value : snapshot->regS; break;
            case rFlags: snapD ? snapD->flags = value : snapshot->flags = value; break;

            case rD: snapshot->regD = value; break;
            case rPBR: snapshot->pbr = value; break;
            case rDBR: snapshot->dbr = value; break;
            case rModeE: snapshot->modeE = value; break;
            default: break;
        }

        body += reg.size + 1;
    }

    if (snapD)
        snapD->updateFromExtern = true;
    else
        snapshot->updateFromExtern = true;

    sendRegisters(command.requestId, space);

    emuThread->unlock();
    _log("Socket: set register count %i", count )
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

auto BinaryMonitor::getTheme(uint8_t space) -> DebuggerTheme {
    switch (space) {
        case 0: return DebuggerTheme::CPU;
        case 1: return DebuggerTheme::Drive8CPU;
        case 2: return DebuggerTheme::Drive9CPU;
        case 3: return DebuggerTheme::Drive10CPU;
        case 4: return DebuggerTheme::Drive11CPU;
        default:
            return DebuggerTheme::Unspecified;
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
