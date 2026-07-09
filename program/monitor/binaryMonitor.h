
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
        MEM_GET = 0x01,
        CHECKPOINT_INFO = 0x11,
        CHECKPOINT_SET = 0x12,
        CHECKPOINT_DELETE = 0x13,
        CHECKPOINT_LIST = 0x14,
        CHECKPOINT_TOGGLE = 0x15,
        CONDITION_SET = 0x22,
        REGISTER_INFO = 0x31,
        REGISTERS_SET = 0x32,
        STOPPED = 0x62,
        RESUMED = 0x63,
        ADVANCE_INSTRUCTIONS = 0x71,
        EXECUTE_UNTIL_RETURN = 0x73,
        PING = 0x81,
        BANKS_AVAILABLE = 0x82,
        REGISTERS_AVAILABLE = 0x83,
        EXIT = 0xaa,
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

    struct CheckPoint {
        unsigned num;
        uint16_t address;
        uint16_t endAddress;
        uint8_t op;
        uint8_t temporary;
        uint8_t enable;
        uint8_t stop;
        bool condition;
        uint8_t space;
        DebuggerTheme theme;
    };

    std::vector<CheckPoint> checkPoints;

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
    auto sendBanksAvailable(Command& command) -> void;
    auto getRegisters(Command& command) -> void;
    auto sendRegisters(unsigned requestId, uint8_t space) -> void;
    auto setRegisters(Command& command) -> void;

    auto getRegList(uint8_t space) -> std::vector<Reg>*;

    auto sendError(Error error, uint32_t requestId) -> void;

    auto performReset(Command& command) -> void;

    auto performExit(Command& command) -> void;
    auto performQuit(Command& command) -> void;

    auto performAutostart(Command& command) -> void;

    auto findCheckpoint(unsigned num) -> CheckPoint*;

    auto hasCheckpoint(uint16_t address, DebuggerTheme theme, uint8_t op) -> bool;
    auto hasEnabledCheckpoint(uint16_t address, DebuggerTheme theme, uint8_t op) -> bool;

    auto setCheckpoint(Command& command) -> void;

    auto getCheckpoint(Command& command) -> void;

    auto deleteCheckpoint(Command& command) -> void;
    auto deleteCheckpoint(CheckPoint* cp) -> bool;

    auto listCheckpoints(Command& command) -> void;

    auto toggleCheckpoints(Command& command) -> void;

    auto advanceInstruction(Command& command) -> void;

    auto stepOut(Command& command) -> void;

    auto setCondition(Command& command) -> void;

    auto sendCheckpointInfo(uint32_t requestId, CheckPoint& cp, bool hit) -> void;

    auto sendResume() -> void;

    auto getMem(Command& command) -> void;

    auto getTheme(uint8_t space) -> DebuggerTheme;

    auto initDebugger(DebuggerTheme theme) -> Debugger*;
};
