
#pragma once

#include "../tools/socket.h"
#include "../debugger/watcherHelper.h"
#include <cstdint>
#include <vector>

struct BinaryMonitor {

    BinaryMonitor();

    ~BinaryMonitor();

    static constexpr unsigned ASC_STX = 0x02;

    static constexpr unsigned API_VERSION = 0x02;

    static constexpr unsigned MON_EVENT = ~0;

    enum class Type {
        INVALID = 0x00,
        REGISTER_INFO = 0x31,
        STOPPED = 0x62,
        RESUMED = 0x63,
        Ping = 0x81,
        REGISTERS_AVAILABLE = 0x83,
        QUIT = 0xbb,
        RESET = 0xcc,
        AUTOSTART = 0xdd,
    };

    enum class Error {
        OK = 0x00,
        OBJECT_MISSING = 0x01,
        INVALID_MEMSPACE = 0x02,
        CMD_INVALID_LENGTH = 0x80,
        INVALID_PARAMETER = 0x81,
        CMD_INVALID_API_VERSION = 0x82,
        CMD_INVALID_TYPE = 0x83,
        CMD_FAILURE = 0x8f,
    };

    struct Command {
        Type type;
        uint32_t requestId;
        uint32_t length;
        uint8_t* body;
    };

    enum {
        rA = 0, rX = 1, rY = 2, rPC = 3, rSP = 4, rFlags = 5,
        rD = 0x23, rPBR = 0x24, rDBR = 0x25, rModeE = 0x29,
        rRaster = 0x35, rCycle = 0x36, rZero = 0x37, rOne = 0x38,
    };

    struct Reg {
        std::string name;
        uint8_t id;
        uint8_t size;
    };
    std::vector<Reg> regs6510;
    std::vector<Reg> regs6502;
    std::vector<Reg> regs65816;

    Socket* server = nullptr;

    Socket* client = nullptr;

    std::vector<DbgWatcher> watchers;
    LIBC64::DebuggerSnapshot* snapshot = nullptr;

    auto setServer(const std::string& uri) -> void;

    auto update() -> void;

    auto waitForClientToAccept() -> void;

    auto destroy(Socket*& socket) -> void;

    auto fetchCommands() -> bool;

    auto handleCommand(uint8_t* buffer) -> void;

    auto sendResponse(uint32_t length, Type type, Error errorCode, uint32_t requestId, const uint8_t* body) -> void;

    auto clientConnected() const -> bool { return client && client->connected(); }

    auto responseStopped(LIBC64::DebuggerSnapshot* snap) -> void;

    auto initRegs() -> void;

    auto sendRegistersAvailable(Command& command) -> void;
    auto sendRegisters(Command& command) -> void;
    auto sendRegisters(unsigned requestId, uint8_t space) -> void;

    auto getRegList(uint8_t space) -> std::vector<Reg>*;

    auto sendError(Error error, uint32_t requestId) -> void;

    auto performReset(Command& command) -> void;

    auto performQuit(Command& command) -> void;

    auto performAutostart(Command& command) -> void;

    auto sendResume() -> void;
};
